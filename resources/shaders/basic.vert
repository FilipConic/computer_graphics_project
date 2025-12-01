#version 330 core
layout (location = 0) in vec3 pos;

uniform vec2 screenPos;

void main() {
	gl_Position = vec4(screenPos.x + pos.x, screenPos.y + pos.y, pos.z, 1.0);
}
