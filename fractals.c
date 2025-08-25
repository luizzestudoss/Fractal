#include "fractals.h"
#include <math.h>

void resetFractal(FractalParams* params) {
    params->zoom = 1.0;
    params->centerX = 0.0;
    params->centerY = 0.0;
    params->type = MANDELBROT;
}

void setFractalUniforms(GLuint program, FractalParams params) {
    GLint zoomLoc    = glGetUniformLocation(program, "zoom");
    GLint centerLoc  = glGetUniformLocation(program, "center");
    GLint offsetLoc  = glGetUniformLocation(program, "localOffset");
    GLint typeLoc    = glGetUniformLocation(program, "fractalType");

    float localOffsetX = (float)fmod(params.centerX, 1.0);
    float localOffsetY = (float)fmod(params.centerY, 1.0);

    glUniform1f(zoomLoc, (float)params.zoom);
    glUniform2f(centerLoc, (float)params.centerX, (float)params.centerY);
    glUniform2f(offsetLoc, localOffsetX, localOffsetY);
    glUniform1i(typeLoc, params.type);
}
