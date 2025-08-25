#version 330 core
out vec4 FragColor;
in vec2 vPos;

uniform vec2 resolution;
uniform float zoom;
uniform vec2 center;
uniform vec2 localOffset;
uniform int fractalType;
uniform int isAnimating;

vec3 paletteMandelbrot(float t){
    t = clamp(t,0.0,1.0);
    return vec3(0.3 + 0.5*t, 0.1 + 0.2*t*t, 0.4*t);
}

vec3 paletteJulia(float t){
    t = clamp(t,0.0,1.0);
    return vec3(0.0, 0.3 + 0.4*t, 0.5 + 0.3*t);
}

float mandelbrot(vec2 c, int maxIter){
    float x=0.0,y=0.0;
    int i=0;
    for(i=0;i<maxIter;i++){
        float x2=x*x, y2=y*y;
        if(x2 + y2 > 4.0) break;
        y = 2.0*x*y + c.y;
        x = x2 - y2 + c.x;
    }
    return float(i)/float(maxIter);
}

float julia(vec2 z, vec2 c, int maxIter){
    float x=z.x,y=z.y;
    int i=0;
    for(i=0;i<maxIter;i++){
        float x2=x*x, y2=y*y;
        if(x2 + y2 > 4.0) break;
        y = 2.0*x*y + c.y;
        x = x2 - y2 + c.x;
    }
    return float(i)/float(maxIter);
}

void main(){
    vec2 frag = gl_FragCoord.xy;
    float aspect = resolution.x / max(resolution.y,1.0);
    vec2 ndc = (frag / resolution) * 2.0 - 1.0;
    ndc.x *= aspect;
    vec2 coord = ndc / max(zoom,1e-12) + center + localOffset;

    int maxIter = int(clamp(2000.0 + log2(max(zoom,1.0))*500.0, 32.0, 4000.0));

    float t = (fractalType==0) ? mandelbrot(coord,maxIter)
                                : julia(coord, vec2(-0.7,0.27015), maxIter);

    vec3 col = (fractalType==0) ? paletteMandelbrot(t)
                                 : paletteJulia(t);

    FragColor = vec4(col,1.0);
}
