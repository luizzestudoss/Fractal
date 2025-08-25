#include "shaders.h"
#include <stdio.h>
#include <stdlib.h>

static char* readFile(const char* path){
    FILE* file = fopen(path,"rb");
    if(!file){ fprintf(stderr,"Erro ao abrir shader: %s\n", path); exit(1);}
    fseek(file,0,SEEK_END);
    long len = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(len+1);
    fread(buffer,1,len,file);
    buffer[len] = '\0';
    fclose(file);
    return buffer;
}

GLuint loadShaders(const char* vertexPath,const char* fragmentPath){
    char* vertexCode = readFile(vertexPath);
    char* fragmentCode = readFile(fragmentPath);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader,1,(const GLchar* const*)&vertexCode,NULL);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader,GL_COMPILE_STATUS,&success);
    if(!success){
        char infoLog[512];
        glGetShaderInfoLog(vertexShader,512,NULL,infoLog);
        fprintf(stderr,"Erro no Vertex Shader: %s\n",infoLog);
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader,1,(const GLchar* const*)&fragmentCode,NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader,GL_COMPILE_STATUS,&success);
    if(!success){
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader,512,NULL,infoLog);
        fprintf(stderr,"Erro no Fragment Shader: %s\n",infoLog);
    }

    GLuint program = glCreateProgram(); 
    glAttachShader(program,vertexShader);
    glAttachShader(program,fragmentShader);
    glBindAttribLocation(program,0,"position");
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    free(vertexCode);
    free(fragmentCode);

    return program;
}
