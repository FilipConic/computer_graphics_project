#ifndef __OBJ_PARSE_H__
#define __OBJ_PARSE_H__

#include <stdint.h>

int obj_parse(const char* file_path, uint32_t** indices, float** vertices);

#endif // __OBJ_PARSE_H__
