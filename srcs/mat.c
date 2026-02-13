#include <mat.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

inline Vec4 vec4(float x, float y, float z) {
	return (Vec4){ .x = x, .y = y, .z = z, .w = 1 };
}
inline float vec4_magn(Vec4 vec) {
	return sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}
float vec4_magn_self(Vec4* vec) {
	return sqrtf(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);
}
inline Vec4 vec4_sub(Vec4 vec1, Vec4 vec2) {
	return (Vec4){ .x = vec1.x - vec2.x, .y = vec1.y - vec2.y, .z = vec1.z - vec2.z, .w = 1.f };
}
inline Vec4 vec4_norm(Vec4 vec) {
	float len = vec4_magn(vec);
	return (Vec4){ .x = vec.x / len, .y = vec.y / len, .z = vec.z / len, .w = 1.f };
}
Vec4* vec4_norm_self(Vec4* self) {
	float len = vec4_magn_self(self);
	self->x /= len;
	self->y /= len;
	self->z /= len;
	self->w = 1.f;
	return self;
}
inline float vec4_dot(Vec4 vec1, Vec4 vec2) {
	return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
}
inline Vec4 vec4_cross(Vec4 vec1, Vec4 vec2) {
	return (Vec4){
		.x = vec1.y * vec2.z - vec1.z * vec2.y,
		.y = vec1.z * vec2.x - vec1.x * vec2.z, 
		.z = vec1.x * vec2.y - vec1.y * vec2.x,
		.w = 1.f,
	};
}
Vec4* vec4_cross_self(Vec4* self, Vec4 other) {
	Vec4 temp = {
		.x = self->y * other.z - self->z * other.y,
		.y = self->z * other.x - self->x * other.z, 
		.z = self->x * other.y - self->y * other.x,
		.w = 1.f,
	};
	memcpy(self, &temp, sizeof(*self));
	return self;
}
inline Vec4 vec4_add(Vec4 vec1, Vec4 vec2) {
	return (Vec4) {
		.x = vec1.x + vec2.x,
		.y = vec1.y + vec2.y, 
		.z = vec1.z + vec2.z,
		.w = 1.f,
	};
}
Vec4* vec4_add_self(Vec4* self, Vec4 other) {
	self->x += other.x;
	self->y += other.y;
	self->z += other.z;
	return self;
}
Vec4* vec4_sub_self(Vec4* self, Vec4 other) {
	self->x -= other.x;
	self->y -= other.y;
	self->z -= other.z;
	return self;
}
Vec4 vec4_mult_s(Vec4 vec, float x) {
	return (Vec4){ .x = vec.x * x, .y = vec.y * x, .z = vec.z * x, .w = 1 };
}

void vec4_print(Vec4 vec) {
	printf("{ %.2f, %.2f, %.2f, %.2f }\n", vec.x, vec.y, vec.z, vec.w);
}

inline Mat4 mat4_unit() {
	return (Mat4){ .data = {
		[0] = 1.f,
		[5] = 1.f,
		[10] = 1.f,
		[15] = 1.f,
	}};
}
Mat4* mat4_unit_self(Mat4* self) {
	memset(self, 0, sizeof(*self));

	self->data[0] = 1.f;
	self->data[5] = 1.f;
	self->data[10] = 1.f;
	self->data[15] = 1.f;

	return self;
}
inline Mat4 mat4_diag(float x1, float x2, float x3, float x4) {
	return (Mat4){ .data = {
		[0] = x1,
		[5] = x2,
		[10] = x3,
		[15] = x4,
	}};
}
Mat4* mat4_diag_self(Mat4* self, float x1, float x2, float x3, float x4) {
	memset(self, 0, sizeof(*self));
	self->data[0] = x1;
	self->data[5] = x2;
	self->data[10] = x3;
	self->data[15] = x4;
	return self;
}
inline Mat4 mat4_add(Mat4 mat1, Mat4 mat2) {
	Mat4 ret = { 0 };

	for (size_t i = 0; i < 16; ++i) {
		ret.data[i] = mat1.data[i] + mat2.data[i];
	}

	return ret;
}
Mat4* mat4_add_self(Mat4* self, Mat4 other) {
	for (size_t i = 0; i < 16; ++i) {
		self->data[i] += other.data[i];
	}

	return self;
}
inline Mat4 mat4_sub(Mat4 mat1, Mat4 mat2) {
	Mat4 ret = { 0 };

	for (size_t i = 0; i < 16; ++i) {
		ret.data[i] = mat1.data[i] - mat2.data[i];
	}

	return ret;
}
Mat4* mat4_sub_self(Mat4* self, Mat4 other) {
	for (size_t i = 0; i < 16; ++i) {
		self->data[i] -= other.data[i];
	}

	return self;
}
inline Mat4 mat4_mult_s(Mat4 mat, float scalar) {
	Mat4 ret = { 0 };

	for (size_t i = 0; i < 16; ++i) {
		ret.data[i] = mat.data[i] * scalar;
	}

	return ret;
}
Mat4* mat4_mult_s_self(Mat4* self, float scalar) {
	for (size_t i = 0; i < 16; ++i) {
		self->data[i] *= scalar;
	}

	return self;
}
inline Mat4 mat4_mult(Mat4 mat1, Mat4 mat2) {
	Mat4 ret = { 0 };

	for (size_t j = 0; j < 4; ++j) {
		for (size_t i = 0; i < 4; ++i) {
			float sum = 0;
			for (size_t k = 0; k < 4; ++k) {
				sum += mat1.data[j + k * 4] * mat2.data[k + i * 4];
			}
			ret.data[j + i * 4] = sum;
		}
	}

	return ret;
}
Mat4* mat4_mult_self(Mat4* self, Mat4 other) {
	Mat4 saved = { 0 };
	
	for (size_t j = 0; j < 4; ++j) {
		for (size_t i = 0; i < 4; ++i) {
			float sum = 0;
			for (size_t k = 0; k < 4; ++k) {
				sum += self->data[j + k * 4] * other.data[k + i * 4];
			}
			saved.data[j + i * 4] = sum;
		}
	}

	memcpy(self, &saved, sizeof(*self));
	return self;
}
inline Mat4 mat4_div_s(Mat4 mat, float scalar) {
	Mat4 ret = { 0 };

	if (fabsf(scalar) < 1e-5) {
		fprintf(stderr, "ERROR:\tDivision by zero in mat4_div_s!\n");
		return ret;
	}
	for (size_t i = 0; i < 16; ++i) {
		ret.data[i] = mat.data[i] / scalar;
	}
	
	return ret;
}
Mat4* mat4_div_s_self(Mat4* self, float scalar) {
	if (fabsf(scalar) < 1e-5) {
		fprintf(stderr, "ERROR:\tDivision by zero in mat4_div_s_self!\n");
		return self;
	}

	for (size_t i = 0; i < 16; ++i) {
		self->data[i] /= scalar;
	}
	
	return self;
}

inline Vec4 mat4_mult_vec4(Mat4 mat, Vec4 vec) {
	return (Vec4){ 
		.x = mat.data[0] * vec.x + mat.data[4] * vec.y + mat.data[8] * vec.z + mat.data[12] * vec.w,
		.y = mat.data[1] * vec.x + mat.data[5] * vec.y + mat.data[9] * vec.z + mat.data[13] * vec.w,
		.z = mat.data[2] * vec.x + mat.data[6] * vec.y + mat.data[10] * vec.z + mat.data[14] * vec.w,
		.w = mat.data[3] * vec.x + mat.data[7] * vec.y + mat.data[11] * vec.z + mat.data[15] * vec.w,
	};
}
Vec4* mat4_mult_vec4_self(Mat4 mat, Vec4* self) {
	Vec4 temp = { 0 };
	temp.x = mat.data[0] * self->x + mat.data[4] * self->y + mat.data[8] * self->z + mat.data[12] * self->w;
	temp.y = mat.data[1] * self->x + mat.data[5] * self->y + mat.data[9] * self->z + mat.data[13] * self->w;
	temp.z = mat.data[2] * self->x + mat.data[6] * self->y + mat.data[10] * self->z + mat.data[14] * self->w;
	temp.w = mat.data[3] * self->x + mat.data[7] * self->y + mat.data[11] * self->z + mat.data[15] * self->w;
	memcpy(self, &temp, sizeof(*self));
	return self;
}

inline Mat4 mat4_transpose(Mat4 mat) {
	Mat4 ret = { 0 };

	for (size_t j = 0; j < 4; ++j) {
		for (size_t i = 0; i < 4; ++i) {
			ret.data[j + i * 4] = mat.data[i + j * 4];
		}
	}

	return ret;
}
Mat4* mat4_transpose_self(Mat4* self) {
	for (size_t j = 0; j < 4; ++j) {
		for (size_t i = 0; i < 4; ++i) {
			if (i == j) { continue; }
			float temp = self->data[j + i * 4];
			self->data[j + i * 4] = self->data[i + j * 4];
			self->data[i + j * 4] = temp;
		}
	}

	return self;
}

/*
 *  0  1  2  3
 *  4  5  6  7
 *  8  9 10 11
 * 12 13 14 15
 */
inline Mat4 mat4_ortho(float left, float right, float top, float bottom, float near, float far) {
	return (Mat4){ .data = {
		[0] = 2.0 / (right - left),
		[12] = -(right + left) / (right - left),
		[5] = 2.0 / (top - bottom),
		[13] = -(top + bottom) / (top - bottom),
		[10] = -2.0 / (far - near),
		[14] = -(far + near) / (far - near),
		[15] = 1.0,
	}};
}
Mat4* mat4_ortho_self(Mat4* self, float left, float right, float top, float bottom, float near, float far) {
	memset(self, 0, sizeof(*self));
	self->data[0] = 2.0 / (right - left);
	self->data[12] = -(right + left) / (right - left);
	self->data[5] = 2.0 / (top - bottom);
	self->data[13] = -(top + bottom) / (top - bottom);
	self->data[10] = -2.0 / (far - near);
	self->data[14] = -(far + near) / (far - near);
	self->data[15] = 1.0;
	return self;
}
#define TO_RAD(angle) (M_PI * (angle) / 180.f)
inline Mat4 mat4_perspective(float fov, float aspect, float near, float far) {
	float tangent = tanf(TO_RAD(fov) / 2.f);
	float top = near * tangent;
	float right = top * aspect;

	return (Mat4){ .data = {
		[0] = near / right,
		[5] = near / top,
		[10] = -(far + near) / (far - near),
		[11] = -2.f * far * near / (far - near),
		[14] = -1.f,
	}};
}
Mat4* mat4_perspective_self(Mat4* self, float fov, float aspect, float near, float far) {
	float tangent = tanf(TO_RAD(fov) / 2.f);
	float top = near * tangent;
	float right = top * aspect;

	self->data[0] = near / right;
	self->data[5] = near / top;
	self->data[10] = -(far + near) / (far - near);
	self->data[11] = -2.f * far * near / (far - near);
	self->data[14] = -1.f;

	return self;
}
inline Mat4 mat4_rotation_x(float pitch) {
	return (Mat4){ .data = {
		[0] = 1.f,
		[5] = cosf(pitch),
		[9] = -sinf(pitch),
		[6] = sinf(pitch),
		[10] = cosf(pitch),
		[15] = 1.f,
	}};
}
Mat4* mat4_rotation_x_self(Mat4* self, float pitch) {
	self->data[0] = 1.f;
	self->data[5] = cosf(pitch);
	self->data[9] = -sinf(pitch);
	self->data[6] = sinf(pitch);
	self->data[10] = cosf(pitch);
	self->data[15] = 1.f;
	return self;
}
inline Mat4 mat4_rotation_y(float yaw) {
	return (Mat4) { .data = {
		[0] = cosf(yaw),
		[8] = sinf(yaw),
		[5] = 1.f,
		[2] = -sinf(yaw),
		[10] = cosf(yaw),
		[15] = 1.f,
	}};
}
Mat4* mat4_rotation_y_self(Mat4* self, float yaw) {
	self->data[0] = cosf(yaw);
	self->data[8] = sinf(yaw);
	self->data[5] = 1.f;
	self->data[2] = -sinf(yaw);
	self->data[10] = cosf(yaw);
	self->data[15] = 1.f;
	return self;
}
inline Mat4 mat4_rotation_z(float roll) {
	return (Mat4) { .data = {
		[0] = cosf(roll),
		[4] = -sinf(roll),
		[1] = sinf(roll),
		[5] = cosf(roll),
		[10] = 1.f,
		[15] = 1.f,
	}};
}
Mat4* mat4_rotation_z_self(Mat4* self, float roll) {
	self->data[0] = cosf(roll);
	self->data[4] = -sinf(roll);
	self->data[1] = sinf(roll);
	self->data[5] = cosf(roll);
	self->data[10] = 1.f;
	self->data[15] = 1.f;
	return self;
}
inline Mat4 mat4_look_at(Vec4 eye, Vec4 target, Vec4 up) {
	Vec4 f = vec4_norm(vec4_sub(target, eye));
	Vec4 s = vec4_norm(vec4_cross(f, up));
	Vec4 u = vec4_cross(s, f);
	return (Mat4) { .data = {
		[0] = s.x,
		[4] = s.y,
		[8] = s.z,
		[1] = u.x,
		[5] = u.y,
		[9] = u.z,
		[2] = -f.x,
		[6] = -f.y,
		[10] = -f.z,
		[12] = -vec4_dot(s, eye),
		[13] = -vec4_dot(u, eye),
		[14] = vec4_dot(f, eye),
		[15] = 1.f,
	}};
}
Mat4* mat4_look_at_self(Mat4* self, Vec4 eye, Vec4 target, Vec4 up) {
	Vec4 f = vec4_norm(vec4_sub(target, eye));
	Vec4 s = vec4_norm(vec4_cross(f, up));
	Vec4 u = vec4_cross(s, f);
	self->data[0] = s.x;
	self->data[4] = s.y;
	self->data[8] = s.z;
	self->data[1] = u.x;
	self->data[5] = u.y;
	self->data[9] = u.z;
	self->data[2] = -f.x;
	self->data[6] = -f.y;
	self->data[10] = -f.z;
	self->data[12] = -vec4_dot(s, eye);
	self->data[13] = -vec4_dot(u, eye);
	self->data[14] = vec4_dot(f, eye);
	self->data[15] = 1.f;
	return self;
}

inline Mat4 mat4_translation(Vec4 vec) {
	return (Mat4) { .data = {
		[0] = 1.f,
		[12] = vec.x,
		[5] = 1.f,
		[13] = vec.y,
		[10] = 1.f,
		[14] = vec.z,
		[15] = 1.f,
	}};
}
Mat4* mat4_translation_self(Mat4* self, Vec4 vec) {
	self->data[0] = 1.f;
	self->data[12] = vec.x;
	self->data[5] = 1.f;
	self->data[13] = vec.y;
	self->data[10] = 1.f;
	self->data[14] = vec.z;
	self->data[15] = 1.f;
	return self;
}
inline Mat4 mat4_scale(Vec4 scale) {
	return (Mat4){ .data = {
		[0] = scale.x,
		[5] = scale.y,
		[10] = scale.z,
		[15] = 1.f,
	}};
}
Mat4* mat4_scale_self(Mat4* self, Vec4 scale) {
	self->data[0] = scale.x;
	self->data[5] = scale.y;
	self->data[10] = scale.z;
	self->data[15] = 1.f;
	return self;
}
inline Mat4 mat4_rotation(float angle, Vec4 axis) {
	float c = cosf(TO_RAD(angle));
	float c_op = 1.f - c;
	float s = sinf(TO_RAD(angle));
	return (Mat4){ .data = {
		[0]  = c + axis.x * axis.x * c_op,
		[4]  = axis.x * axis.y * c_op - axis.z * s,
		[8]  = axis.x * axis.z * c_op + axis.y * s,
		[1]  = axis.x * axis.y * c_op + axis.z * s,
		[5]  = c + axis.y * axis.y * c_op,
		[9]  = axis.y * axis.z * c_op - axis.x * s,
		[2]  = axis.x * axis.z * c_op - axis.y * s,
		[6]  = axis.y * axis.z * c_op + axis.x * s,
		[10] = c + axis.z * axis.z * c_op,
		[15] = 1.f,
	}};
}
Mat4* mat4_rotation_self(Mat4* self, float angle, Vec4 axis) {
	float c = cosf(TO_RAD(angle));
	float c_op = 1.f - c;
	float s = sinf(TO_RAD(angle));
	self->data[0]  = c + axis.x * axis.x * c_op;
	self->data[4]  = axis.x * axis.y * c_op - axis.z * s;
	self->data[8]  = axis.x * axis.z * c_op + axis.y * s;
	self->data[1]  = axis.x * axis.y * c_op + axis.z * s;
	self->data[5]  = c + axis.y * axis.y * c_op;
	self->data[9]  = axis.y * axis.z * c_op - axis.x * s;
	self->data[2]  = axis.x * axis.z * c_op - axis.y * s;
	self->data[6]  = axis.y * axis.z * c_op + axis.x * s;
	self->data[10] = c + axis.z * axis.z * c_op;
	self->data[15] = 1.f;
	return self;
}

inline Mat4 mat4_model(Vec4 translation, float angle, Vec4 axis, Vec4 scale) {
	return mat4_mult(mat4_translation(translation), mat4_mult(mat4_rotation(angle, axis), mat4_scale(scale)));
}
Mat4* mat4_model_self(Mat4* self, Vec4 translation, float angle, Vec4 axis, Vec4 scale) {
	return mat4_mult_self(mat4_translation_self(self, translation), mat4_mult(mat4_rotation(angle, axis), mat4_scale(scale)));
}

void mat4_print(Mat4 mat) {
	printf("{\n");
	for (size_t i = 0; i < 4; ++i) {
		printf("\t");
		for (size_t j = 0; j < 4; ++j) {
			if (j) printf(", ");
			printf("%.2f", mat.data[i + j * 4]);
		}
		printf("\n");
	}
	printf("}\n");
}

