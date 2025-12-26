#version 330 core
layout (location = 0) in vec2 a_pos;

uniform vec2 u_pos;

out vec2 frag_pos;

void main() {
	frag_pos = a_pos + u_pos;
	gl_Position = vec4(a_pos + u_pos, 0, 1.0);
}

