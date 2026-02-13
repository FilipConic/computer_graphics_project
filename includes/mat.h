#ifndef __MAT_H__
#define __MAT_H__

typedef struct {
	float x, y, z, w;
} Vec4;

Vec4 vec4(float x, float y, float z);
float vec4_magn(Vec4 vec);
float vec4_magn_self(Vec4* vec);
Vec4 vec4_sub(Vec4 vec1, Vec4 vec2);
Vec4 vec4_norm(Vec4 vec);
Vec4* vec4_norm_self(Vec4* vec);
float vec4_dot(Vec4 vec1, Vec4 vec2);
Vec4 vec4_cross(Vec4 vec1, Vec4 vec2);
Vec4* vec4_cross_self(Vec4* self, Vec4 other);
Vec4 vec4_add(Vec4 vec1, Vec4 vec2);
Vec4* vec4_add_self(Vec4* self, Vec4 other);
Vec4* vec4_sub_self(Vec4* self, Vec4 other);
Vec4 vec4_mult_s(Vec4 vec, float x);

void vec4_print(Vec4 vec);

typedef struct {
	float data[16];
} Mat4;

Mat4 mat4_unit();
Mat4* mat4_unit_self(Mat4* self);
Mat4 mat4_diag(float x1, float x2, float x3, float x4);
Mat4* mat4_diag_self(Mat4* self, float x1, float x2, float x3, float x4);
Mat4 mat4_add(Mat4 mat1, Mat4 mat2);
Mat4* mat4_add_self(Mat4* self, Mat4 other);
Mat4 mat4_sub(Mat4 mat1, Mat4 mat2);
Mat4* mat4_sub_self(Mat4* self, Mat4 other);
Mat4 mat4_mult_s(Mat4 mat, float scalar);
Mat4* mat4_mult_s_self(Mat4* self, float scalar);
Mat4 mat4_mult(Mat4 mat1, Mat4 mat2);
Mat4* mat4_mult_self(Mat4* self, Mat4 other);
Mat4 mat4_div_s(Mat4 mat, float scalar);
Mat4* mat4_div_s_self(Mat4* self, float scalar);
Vec4 mat4_mult_vec4(Mat4 mat, Vec4 vec);
Vec4* mat4_mult_vec4_self(Mat4 mat, Vec4* self);

Mat4 mat4_transpose(Mat4 mat);
Mat4* mat4_transpose_self(Mat4* self);

Mat4 mat4_ortho(float left, float right, float top, float bottom, float near, float far);
Mat4* mat4_ortho_self(Mat4* self, float left, float right, float top, float bottom, float near, float far);
Mat4 mat4_perspective(float fov, float aspect, float near, float far);
Mat4* mat4_perspective_self(Mat4* self, float fov, float aspect, float near, float far);
Mat4 mat4_rotation_x(float roll);
Mat4* mat4_rotation_x_self(Mat4* self, float roll);
Mat4 mat4_rotation_y(float pitch);
Mat4* mat4_rotation_y_self(Mat4* self, float pitch);
Mat4 mat4_rotation_z(float yaw);
Mat4* mat4_rotation_z_self(Mat4* self, float yaw);
Mat4 mat4_look_at(Vec4 pos, Vec4 target, Vec4 up);
Mat4* mat4_look_at_self(Mat4* self, Vec4 pos, Vec4 target, Vec4 up);

Mat4 mat4_rotation(float angle, Vec4 axis);
Mat4* mat4_rotation_self(Mat4* self, float angle, Vec4 axis);
Mat4 mat4_translation(Vec4 vec);
Mat4* mat4_translation_self(Mat4* self, Vec4 vec);
Mat4 mat4_scale(Vec4 scale);
Mat4* mat4_scale_self(Mat4* self, Vec4 scale);

Mat4 mat4_model(Vec4 translation, float angle, Vec4 axis, Vec4 scale);
Mat4* mat4_model_self(Mat4* self, Vec4 translation, float angle, Vec4 axis, Vec4 scale);

void mat4_print(Mat4 mat);

#endif // __MAT_H__
