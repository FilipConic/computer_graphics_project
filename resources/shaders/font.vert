#version 330 core
layout (location = 0) in vec3 pos;

uniform mat4 u_model;
uniform mat4 u_proj;

void main() {
	gl_Position = u_proj * u_model * vec4(pos, 1);
}
