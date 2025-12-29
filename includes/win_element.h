#ifndef __WIN_ELEMENT_H__
#define __WIN_ELEMENT_H__

#include <shapes.h>
#include <font.h>

#define WIDTH 1600
#define HEIGHT 900

typedef struct {
	enum {
		EVENT_CLICK_EMTPY,
		EVENT_ELEMENT_CLICK,
		EVENT_SCENE_CHANGE,
	} type;
	int value;
} CustomEvent;
#define event_create(t, v) ((CustomEvent){ .type = (t), .value = (v) })

typedef enum {
	SCENE_ACTIVE_ALWAYS,
	SCENE_FONT_CHANGE,
} Scene;

typedef enum {
	TEXT_INPUT_ELEMENT,
	TEXT_ELEMENT,
	BUTTON_ELEMENT,
} ElementType;


typedef struct Element Element;
typedef void (*ClickCallback)(Element* el, void* out);

typedef struct {
	ElementType type;
	Scene scene_needed;
	ClickCallback click_callback;
	
	uint16_t id;
	int x, y, z;
	union {
		struct {
			FontTTF* font;
			uint32_t program;

			int border_width;
			int w, h;
			Color color;
			Color border_color;
		} text_input;
		struct {
			FontTTF* font;
			const char* str;
		} text;
		struct {
			int w, h;
			const char* str;
		} button;
	} as;
} ElementDescription;

struct Element {
	ElementType type;
	Scene scene_needed;
	ClickCallback click_callback;
	uint16_t id;
	int z;
	
	union {
		struct {
			StaticRectangle rect;
			StaticRectangle cursor;
			Text main;
			int size;
		} text_input;
		Text text;
		struct {
			int empty;
		} button;
	} as;
};

Element element_create(const ElementDescription *desc);
void element_show(Element* el, Text* curr_text_input, Scene curr_scene);
void element_fill_click_space(Element* el, uint16_t* ids);
void element_free(Element* el);

#endif // __WIN_ELEMENT_H__
