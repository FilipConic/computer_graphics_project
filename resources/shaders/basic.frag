#version 330 core
out vec4 FragColor;

uniform vec4 u_color;
uniform vec4 u_border_color;
uniform vec4 u_rect_info;

uniform vec2 u_border;

in vec2 frag_pos;

void main() {
	if (frag_pos.x <= u_rect_info.x + u_border.x ||
		frag_pos.y >= u_rect_info.y - u_border.y ||
		frag_pos.x >= u_rect_info.z - u_border.x ||
		frag_pos.y <= u_rect_info.w + u_border.y) {
		FragColor = u_border_color;
	} else {
		FragColor = u_color;
	}
}
