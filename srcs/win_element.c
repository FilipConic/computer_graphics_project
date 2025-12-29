#include <stdint.h>
#include <win_element.h>

#include <cylibx.h>

#define CURSOR_PADDING(size) vec2i((int)(size * 0.15), (int)(size * 0.3))
#define CLAMP(val, addon, lower, upper) \
	if ((val + addon) <= lower) (val) = (lower - addon); \
	if ((val + addon) >= upper) (val) = (upper - addon);

Element element_create(const ElementDescription *desc) {
	Element new_element = { 
		.type = desc->type,
		.scene_needed = desc->scene_needed,
		.click_callback = desc->click_callback,
		.id = desc->id,
		.z = desc->z,
	};
	switch (desc->type) {
		case TEXT_ELEMENT: {
			new_element.as.text = text_create(desc->as.text.font, WIDTH, HEIGHT,
				.x = desc->x, .y = desc->y, .is_static = 1, .text = desc->as.text.str);
		} break;
		case TEXT_INPUT_ELEMENT: {
			new_element.as.text_input.main = text_create(desc->as.text_input.font, WIDTH, HEIGHT,
				.width = desc->as.text_input.w, .height = desc->as.text_input.h,
				.word_wrapping = 1,
				.x = desc->x + desc->as.text_input.border_width,
				.y = desc->y + desc->as.text_input.border_width);
			new_element.as.text_input.rect = s_rect_create(desc->as.text_input.program, WIDTH, HEIGHT,
				desc->x, desc->y,
				desc->as.text_input.w,
				desc->as.text_input.h,
				desc->as.text_input.color,
				.border_color = desc->as.text_input.border_color,
				.border_width = desc->as.text_input.border_width);
			new_element.as.text_input.cursor = s_rect_create(desc->as.text_input.program, WIDTH, HEIGHT,
				desc->x, desc->y,
				(int)(desc->as.text_input.font->size * 0.8),
				(int)(desc->as.text_input.font->size * 2.3),
				desc->as.text_input.border_color);
			new_element.as.text_input.size = desc->as.text_input.font->size;
		} break;
		case BUTTON_ELEMENT: { } break;
		default: assert(0 && "UNREACHABLE");
	}
	return new_element;
}
void element_show(Element* el, Text* curr_text_input, Scene curr_scene) {
	if (el->scene_needed == curr_scene || el->scene_needed == SCENE_ACTIVE_ALWAYS) {
		switch (el->type) {
			case TEXT_ELEMENT: text_show(&el->as.text); break;
			case TEXT_INPUT_ELEMENT: {
				if (curr_text_input == &el->as.text_input.main) {
					Vec2i new_cursor_pos = text_cursor_pos(&el->as.text_input.main, CURSOR_PADDING(el->as.text_input.size));
					if (!vec2i_eq(el->as.text_input.cursor.pos, new_cursor_pos)) {
						s_rect_set_pos(&el->as.text_input.cursor, new_cursor_pos);
					}
					s_rect_show(&el->as.text_input.cursor);
				}
				s_rect_show(&el->as.text_input.rect);
				text_show(&el->as.text_input.main);
			 } break;
			case BUTTON_ELEMENT:
			default: assert(0 && "UNREACHABLE");
		}
	}
}
void element_fill_click_space(Element* el, uint16_t* ids) {
	if (!el->click_callback) { return; }
	int x = 0, y = 0, w = 0, h = 0;
	if (el->type == TEXT_ELEMENT) {
		return;
	} else if (el->type == TEXT_INPUT_ELEMENT) {
		x = el->as.text_input.rect.pos.x;
		y = el->as.text_input.rect.pos.y;
		w = el->as.text_input.rect.wh.x;
		h = el->as.text_input.rect.wh.y;
	} else if (el->type == BUTTON_ELEMENT) {
		assert(0 && "UNREACHABLE");
	}

	CLAMP(w, x, 0, WIDTH);
	CLAMP(h, y, 0, HEIGHT);

	for (int j = 0; j < h; ++j) {
		int yjw = (y + j) * WIDTH;
		for (int i = 0; i < w; ++i) {
			ids[x + i + yjw] = el->id;
		}
	}
}
void element_free(Element* el) {
	switch (el->type) {
		case TEXT_ELEMENT:
			text_free(&el->as.text);
			break;
		case TEXT_INPUT_ELEMENT:
			text_free(&el->as.text_input.main);
			s_rect_free(&el->as.text_input.rect);
			s_rect_free(&el->as.text_input.cursor);
			break;
		case BUTTON_ELEMENT:
		default: assert(0 && "UNREACHABLE");
	}
}

