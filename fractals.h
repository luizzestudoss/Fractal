#ifndef FRACTALS_H
#define FRACTALS_H

#include <GL/glew.h>

typedef enum {
    MANDELBROT = 0,
    JULIA = 1
} FractalType;

typedef struct {
    double zoom;
    double centerX;
    double centerY;
    FractalType type;
} FractalParams;

void resetFractal(FractalParams* params);
void setFractalUniforms(GLuint program, FractalParams params);

#endif
