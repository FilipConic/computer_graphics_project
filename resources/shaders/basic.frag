#version 330 core
out vec4 FragColor;

in vec2 v_pos;

uniform vec4 u_color;
uniform vec4 u_border_color;
uniform vec2 u_border;

void main() {
	FragColor = (!(u_border.x > 0) || !(u_border.y > 0) ||
			(v_pos.x > -1.0f + u_border.x &&
			 v_pos.x <  1.0f - u_border.x &&
			 v_pos.y > -1.0f + u_border.y &&
			 v_pos.y <  1.0f - u_border.y)) ?
		u_color : u_border_color;
}
