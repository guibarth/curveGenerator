#version 330 core

layout(location=0) in vec3 vp;
layout(location=1) in vec3 vc;
uniform mat4 matrix;
out vec3 color;

void main () {
 color = vc;
 gl_Position = matrix * vec4 (vp, 1.0);
}