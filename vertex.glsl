#version 330 core
layout(location = 0) in vec2 inPos;
out vec2 vPos;
void main(){
    vPos = inPos;
    gl_Position = vec4(inPos, 0.0, 1.0);
}
