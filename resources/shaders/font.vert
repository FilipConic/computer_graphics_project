#version 330 core
layout (location = 0) in vec3 pos;

uniform vec2 screen_pos;
uniform mat4 projection;

void main() {
	vec4 p = vec4(pos.x + screen_pos.x, -pos.y + screen_pos.y, 0.0f, 1.0f);
	gl_Position = projection * p;
}
