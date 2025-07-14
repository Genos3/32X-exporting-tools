typedef signed char s8;
typedef unsigned char byte, u8;
typedef short s16;
typedef unsigned short u16;
typedef int fixed, s32;
typedef unsigned int fixed_u, uint, u32;
typedef long long s64, fixed64;
typedef unsigned long long u64;

#define FP 16
#define fix(x) (int)((float)(x) * (1 << FP))
#define fix8(x) (int)((float)(x) * (1 << 8))
#define fixtoft(x) ((float)(x) / (1 << FP))
#define PI_FP fix(3.14159265)
// 512 element sin lut
#define lu_sin(x) sin_lut[((x) >> 7) & 0x1FF]
#define lu_cos(x) sin_lut[(((x) >> 7) + 128) & 0x1FF]
// in: .8, out: .FP
#define tan_c(x) ((lu_sin(x) << FP) / lu_cos(x))

#define fp_mul64(a, b) (((s64)(a) * (b)) >> FP)
#define fp_mul(a, b) (int)(((s64)(a) * (b)) >> FP)
#define fp_mul32(a, b) (((a) * (b)) >> FP)
#define fp_div(a, b) (int)(((s64)(a) << FP) / (b))
#define fp_round(x) (((x) + (1 << (FP - 1))) & ~((1 << FP) - 1))
#define fp_trunc(x) ((x) & ~((1 << FP) - 1))
#define fp_ceil(x) (((x) + (1 << FP) - 1) & ~((1 << FP) - 1))
#define fp_ceil_i(x) (((x) + (1 << FP) - 1) >> FP) // ceil to int
#define abs_c(x) ((x) < 0 ? -(x) : (x))
#define min_c(a, b) ((a) < (b) ? (a) : (b))
#define max_c(a, b) ((a) > (b) ? (a) : (b))
// turns one fixed point value into another one by shifting the bits, ex: from .8 to .12
#define shift_fp(a, b, c) ((b) > (c) ? (a) >> ((b) - (c)) : (a) << ((c) - (b)))
#define dup8(x) ((x) | ((x) << 8))
#define dup16(x) ((x) | ((x) << 16))
#define quad8(x) dup16(dup8(x))
#define dp2(ax, ay, bx, by) (((s64)(ax) * (ay) + (s64)(bx) * (by)) >> FP)
#define dp3(ax, ay, bx, by, cx, cy) (((s64)(ax) * (ay) + (s64)(bx) * (by) + (s64)(cx) * (cy)) >> FP)
#define dp3_32(ax, ay, bx, by, cx, cy) (((ax) * (ay) + (bx) * (by) + (cx) * (cy)) >> FP)

#define DEFINE_VEC2_TYPE(type, name, a1, a2) \
typedef struct { \
  type a1, a2; \
} name;

#define DEFINE_VEC3_TYPE(type, name, a1, a2, a3) \
typedef struct { \
  type a1, a2, a3; \
} name;

#define DEFINE_VEC4_TYPE(type, name, a1, a2, a3, a4) \
typedef struct { \
  type a1, a2, a3, a4; \
} name;

#define DEFINE_VEC5_TYPE(type, name, a1, a2, a3, a4, a5) \
typedef struct { \
  type a1, a2, a3, a4, a5; \
} name;

DEFINE_VEC2_TYPE(fixed, vec2_t, x, y)
DEFINE_VEC3_TYPE(fixed, vec3_t, x, y, z)

DEFINE_VEC2_TYPE(int, vec2i_t, x, y)
DEFINE_VEC3_TYPE(int, vec3i_t, x, y, z)

DEFINE_VEC3_TYPE(u16, vec3_u16_t, x, y, z)

DEFINE_VEC2_TYPE(fixed, vec2_tx_t, u, v)
DEFINE_VEC5_TYPE(fixed, vec5_t, x, y, z, u, v)

DEFINE_VEC4_TYPE(fixed, vec4_nm_t, x, y, z, d)

DEFINE_VEC3_TYPE(fixed, size3_t, w, d, h)
DEFINE_VEC2_TYPE(int, size2i_t, w, h)
DEFINE_VEC3_TYPE(int, size3i_t, w, d, h)

DEFINE_VEC3_TYPE(u8, rgb_t, r, g, b)

typedef struct {
  char *data;
  int alloc_mem;
  int size;
  int type_size;
} list_def;

#define list(T) typedef struct {  \
  T* data;                        \
  int alloc_mem;                  \
  int size;                       \
  int type_size;                  \
} list_ ## T

#define dlist(T) typedef struct {  \
  T** data;                        \
  int alloc_mem;                   \
  int size;                        \
  int type_size;                   \
} dlist_ ## T

// list types
list(int);
list(u8);
list(s8);
list(u16);
list(s16);
list(u32);
list(vec3_t);
list(vec2_tx_t);
list(size2i_t);
list(size3_t);