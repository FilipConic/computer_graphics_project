#version 330 core
layout (location = 0) in vec2 a_pos;

uniform mat4 u_proj;
uniform mat4 u_model;

out vec2 v_pos;

void main() {
	v_pos = a_pos;
	gl_Position = u_proj * u_model * vec4(a_pos, 0, 1);
}
