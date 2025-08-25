#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "fractals.h"

static int width = 1280, height = 720;
static FractalParams fractal = { 1.0, 0.0, 0.0, MANDELBROT };

static int zooming = 0, needsRedraw = 1, selecting = 0;
static double zoomTarget = 1.0, centerXTarget = 0.0, centerYTarget = 0.0;
static const float zoomLerp = 0.25f;

static float mouseX = 0.0f, mouseY = 0.0f;
static double mouseFractalX = 0.0, mouseFractalY = 0.0;

static GLuint prog = 0, vao = 0, vbo = 0, ebo = 0;
static GLuint uiProg = 0, uiVao = 0, uiVbo = 0;

static GLint uResolution = -1, uZoom = -1, uCenter = -1, uLocalOffset = -1, uType = -1, uAnimating = -1;

static int selX0 = 0, selY0 = 0, selX1 = 0, selY1 = 0;

static char* readFile(const char* path){
    FILE* f = fopen(path, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(sz + 1);
    fread(buf,1,sz,f);
    buf[sz]=0;
    fclose(f);
    return buf;
}

static GLuint compileShader(GLenum t,const char*src){
    GLuint s=glCreateShader(t);
    glShaderSource(s,1,&src,NULL);
    glCompileShader(s);
    GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){ GLint len=0; glGetShaderiv(s,GL_INFO_LOG_LENGTH,&len); char*log=(char*)malloc(len); glGetShaderInfoLog(s,len,NULL,log); fprintf(stderr,"%s\n",log); free(log); glDeleteShader(s); return 0; }
    return s;
}

static GLuint linkProgram(GLuint vs,GLuint fs){
    GLuint p=glCreateProgram();
    glAttachShader(p,vs); glAttachShader(p,fs);
    glBindAttribLocation(p,0,"inPos");
    glLinkProgram(p);
    GLint ok=0; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){ GLint len=0; glGetProgramiv(p,GL_INFO_LOG_LENGTH,&len); char*log=(char*)malloc(len); glGetProgramInfoLog(p,len,NULL,log); fprintf(stderr,"%s\n",log); free(log); glDeleteProgram(p); return 0; }
    return p;
}

static void initQuad(void){
    const GLfloat vertices[] = { -1,-1,  1,-1,  1,1, -1,1 };
    const GLuint indices[]  = { 0,1,2, 2,3,0 };
    glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);
    glGenBuffers(1,&vbo);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    glGenBuffers(1,&ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(GLfloat),(void*)0);
    glBindVertexArray(0);
}

static int initShaders(const char* vsPath,const char* fsPath){
    char* vsrc = readFile(vsPath);
    char* fsrc = readFile(fsPath);
    if(!vsrc||!fsrc) return 0;
    GLuint vs = compileShader(GL_VERTEX_SHADER,vsrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER,fsrc);
    free(vsrc); free(fsrc);
    if(!vs||!fs) return 0;
    prog = linkProgram(vs,fs);
    glDeleteShader(vs); glDeleteShader(fs);
    if(!prog) return 0;
    glUseProgram(prog);
    uResolution  = glGetUniformLocation(prog,"resolution");
    uZoom        = glGetUniformLocation(prog,"zoom");
    uCenter      = glGetUniformLocation(prog,"center");
    uLocalOffset = glGetUniformLocation(prog,"localOffset");
    uType        = glGetUniformLocation(prog,"fractalType");
    uAnimating   = glGetUniformLocation(prog,"isAnimating");
    if(uResolution>=0) glUniform2f(uResolution,(float)width,(float)height);
    double bx=floor(fractal.centerX), by=floor(fractal.centerY);
    float cx=(float)bx, cy=(float)by;
    float ox=(float)(fractal.centerX-bx), oy=(float)(fractal.centerY-by);
    if(uZoom>=0) glUniform1f(uZoom,(float)fractal.zoom);
    if(uCenter>=0) glUniform2f(uCenter,cx,cy);
    if(uLocalOffset>=0) glUniform2f(uLocalOffset,ox,oy);
    if(uType>=0) glUniform1i(uType,(int)fractal.type);
    if(uAnimating>=0) glUniform1i(uAnimating,0);
    const char* ui_vs = "#version 330 core\nlayout(location=0) in vec2 inPos;void main(){gl_Position = vec4(inPos,0.0,1.0);}"; 
    const char* ui_fs = "#version 330 core\nout vec4 FragColor;uniform vec3 color;void main(){FragColor = vec4(color,1.0);}"; 
    GLuint uvs = compileShader(GL_VERTEX_SHADER,ui_vs);
    GLuint ufs = compileShader(GL_FRAGMENT_SHADER,ui_fs);
    uiProg = linkProgram(uvs,ufs);
    glDeleteShader(uvs); glDeleteShader(ufs);
    glGenVertexArrays(1,&uiVao);
    glGenBuffers(1,&uiVbo);
    glBindVertexArray(uiVao);
    glBindBuffer(GL_ARRAY_BUFFER,uiVbo);
    glBufferData(GL_ARRAY_BUFFER,4*sizeof(float)*2,NULL,GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void*)0);
    glBindVertexArray(0);
    return 1;
}

static void pushUniforms(int anim){
    glUseProgram(prog);
    double bx=floor(fractal.centerX), by=floor(fractal.centerY);
    float cx=(float)bx, cy=(float)by;
    float ox=(float)(fractal.centerX-bx), oy=(float)(fractal.centerY-by);
    if(uZoom>=0) glUniform1f(uZoom,(float)fractal.zoom);
    if(uCenter>=0) glUniform2f(uCenter,cx,cy);
    if(uLocalOffset>=0) glUniform2f(uLocalOffset,ox,oy);
    if(uType>=0) glUniform1i(uType,(int)fractal.type);
    if(uAnimating>=0) glUniform1i(uAnimating,anim);
}

static void updateMouseFractal(){
    double aspect = (double)width / (double)height;
    mouseFractalX = fractal.centerX + (mouseX * aspect) / fractal.zoom;
    mouseFractalY = fractal.centerY + mouseY / fractal.zoom;
}

static void drawSelectionRect(){
    if(!selecting) return;
    float nx0 = (float)selX0 / (float)width * 2.0f - 1.0f;
    float ny0 = (float)(height - selY0) / (float)height * 2.0f - 1.0f;
    float nx1 = (float)selX1 / (float)width * 2.0f - 1.0f;
    float ny1 = (float)(height - selY1) / (float)height * 2.0f - 1.0f;
    float verts[8] = { nx0, ny0, nx1, ny0, nx1, ny1, nx0, ny1 };
    glUseProgram(uiProg);
    glBindVertexArray(uiVao);
    glBindBuffer(GL_ARRAY_BUFFER,uiVbo);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(verts),verts);
    GLint colLoc = glGetUniformLocation(uiProg,"color");
    glUniform3f(colLoc,1.0f,1.0f,1.0f);
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    glDrawArrays(GL_LINE_LOOP,0,4);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glBindVertexArray(0);
}

static void display(void){
    int anim = 0;
    if(zooming){
        anim = 1;
        double dz = zoomTarget - fractal.zoom;
        double dx = centerXTarget - fractal.centerX;
        double dy = centerYTarget - fractal.centerY;
        fractal.zoom    += dz * zoomLerp;
        fractal.centerX += dx * zoomLerp;
        fractal.centerY += dy * zoomLerp;
        if(fabs(dz) < 1e-6 && fabs(dx) < 1e-6 && fabs(dy) < 1e-6){
            zooming = 0;
            fractal.zoom = zoomTarget;
            fractal.centerX = centerXTarget;
            fractal.centerY = centerYTarget;
        }
        needsRedraw = 1;
    }
    if(!needsRedraw) return;
    needsRedraw = 0;
    glViewport(0,0,width,height);
    glClear(GL_COLOR_BUFFER_BIT);
    pushUniforms(anim);
    glBindVertexArray(vao);
    glUseProgram(prog);
    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
    glBindVertexArray(0);
    drawSelectionRect();
    glutSwapBuffers();
}

static void idle(void){
    if(zooming) glutPostRedisplay();
}

static void reshape(int w,int h){
    width = (w>0?w:1);
    height = (h>0?h:1);
    glUseProgram(prog);
    if(uResolution>=0) glUniform2f(uResolution,(float)width,(float)height);
    needsRedraw = 1;
    glutPostRedisplay();
}

static void mouseMove(int x,int y){
    mouseX = (float)x / (float)width * 2.0f - 1.0f;
    mouseY = (float)(height - y) / (float)height * 2.0f - 1.0f;
    updateMouseFractal();
    if(selecting){
        selX1 = x; selY1 = y;
        needsRedraw = 1;
    }
}

static double pixelToFractalX(int px){
    double ndcX = (double)px / (double)width * 2.0 - 1.0;
    double aspect = (double)width / (double)height;
    return fractal.centerX + (ndcX * aspect) / fractal.zoom;
}

static double pixelToFractalY(int py){
    double ndcY = (double)(height - py) / (double)height * 2.0 - 1.0;
    return fractal.centerY + ndcY / fractal.zoom;
}

static void applySelectionZoom(){
    int dx = abs(selX1 - selX0);
    int dy = abs(selY1 - selY0);
    if(dx < 2 || dy < 2) return;
    int x0 = selX0 < selX1 ? selX0 : selX1;
    int x1 = selX0 > selX1 ? selX0 : selX1;
    int y0 = selY0 < selY1 ? selY0 : selY1;
    int y1 = selY0 > selY1 ? selY0 : selY1;
    double fx0 = pixelToFractalX(x0);
    double fx1 = pixelToFractalX(x1);
    double fy0 = pixelToFractalY(y0);
    double fy1 = pixelToFractalY(y1);
    double cx = (fx0 + fx1) * 0.5;
    double cy = (fy0 + fy1) * 0.5;
    double selw = (double)(x1 - x0);
    double selh = (double)(y1 - y0);
    double scale = fmax((double)width / selw, (double)height / selh);
    zoomTarget = fractal.zoom * scale;
    centerXTarget = cx;
    centerYTarget = cy;
    zooming = 1;
    needsRedraw = 1;
}

static void mouseButton(int button,int state,int x,int y){
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
        selecting = 1;
        selX0 = selX1 = x;
        selY0 = selY1 = y;
    } else if(button == GLUT_LEFT_BUTTON && state == GLUT_UP){
        selecting = 0;
        selX1 = x; selY1 = y;
        applySelectionZoom();
    }
}

static void zoomToMouse(double factor){
    updateMouseFractal();
    double aspect = (double)width / (double)height;
    double uvx = (double)mouseX * aspect;
    double uvy = (double)mouseY;
    zoomTarget = fmax(1.0, fractal.zoom * factor);
    centerXTarget = mouseFractalX - uvx / zoomTarget;
    centerYTarget = mouseFractalY - uvy / zoomTarget;
    zooming = 1;
    needsRedraw = 1;
}

static void keyboard(unsigned char key,int x,int y){
    (void)x; (void)y;
    switch(key){
        case '1': fractal.type = MANDELBROT; needsRedraw = 1; break;
        case '2': fractal.type = JULIA; needsRedraw = 1; break;
        case 'r': case 'R': fractal.zoom = 1.0; fractal.centerX = 0.0; fractal.centerY = 0.0; zooming = 0; needsRedraw = 1; break;
        case 'z': case 'Z': zoomToMouse(2.0); break;
        case 'x': case 'X': zoomToMouse(0.5); break;
        case 27: exit(0); break;
    }
    if(!zooming){ needsRedraw = 1; glutPostRedisplay(); }
}

int main(int argc,char** argv){
    glutInit(&argc,argv);
    glutInitContextVersion(3,3);
    glutInitContextFlags(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(width,height);
    glutCreateWindow("Fractal Explorer");
    if(glewInit() != GLEW_OK) return -1;
    glClearColor(0,0,0,1);
    if(!initShaders("vertex.glsl","fragment.glsl")) return -1;
    initQuad();
    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutPassiveMotionFunc(mouseMove);
    glutMotionFunc(mouseMove);
    glutMouseFunc(mouseButton);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);
    needsRedraw = 1;
    glutPostRedisplay();
    glutMainLoop();
    return 0;
}
