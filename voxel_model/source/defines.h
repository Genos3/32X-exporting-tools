typedef signed char s8;
typedef unsigned char byte, u8;
typedef short s16;
typedef unsigned short u16;
typedef int fixed, s32;
typedef unsigned int fixed_u, uint, u32;
typedef long long s64;
typedef unsigned long long u64;

#define FP 16
#define PI_fl 3.14159265
#define fix(x) (int)((float)(x) * (1 << FP))
#define round(x) (int)((x) + 0.5)
#define floor(x) (int)(x)
#define ceil(x) ((x) > (int)(x) ? (int)(x + 1) : (int)(x))
#define abs_c(x) ((x) < 0 ? -(x) : (x))
#define min_c(a, b) ((a) < (b) ? (a) : (b))
#define max_c(a, b) ((a) > (b) ? (a) : (b))
#define PI 3.14159265
#define cosd(a) cos((a) * PI / 180)
#define sind(a) sin((a) * PI / 180)

#define UNTEXTURED 1
#define BACKFACE 2
#define BILLBOARD_N 4
#define BILLBOARD_V 8
#define SPRITE (4 | 8)
#define ANIMATED 16
#define HAS_ALPHA 32

// palette

#define RGB15(a, b, c) (((c) << 10) | ((b) << 5) | (a))

#define BG_COLOR RGB15(0, 0, 0)

#define VOXEL_SIZE (1.0 / 16)

#define STR_SIZE 256

#define DEFINE_VEC3_TYPE(type, name, a1, a2, a3) \
typedef struct { \
  type a1, a2, a3; \
} name;

#define DEFINE_VEC2_TYPE(type, name, a1, a2) \
typedef struct { \
  type a1, a2; \
} name;

#define DEFINE_VEC4_TYPE(type, name, a1, a2, a3, a4) \
typedef struct { \
  type a1, a2, a3, a4; \
} name;

#define DEFINE_VEC5_TYPE(type, name, a1, a2, a3, a4, a5) \
typedef struct { \
  type a1, a2, a3, a4, a5; \
} name;

DEFINE_VEC2_TYPE(float, vec2_t, x, y)
DEFINE_VEC3_TYPE(float, vec3_t, x, y, z)

DEFINE_VEC2_TYPE(int, vec2i_t, x, y)
DEFINE_VEC3_TYPE(int, vec3i_t, x, y, z)

DEFINE_VEC2_TYPE(float, vec2_tx_t, u, v)
DEFINE_VEC4_TYPE(float, vec4_t, x, y, u, v)
DEFINE_VEC5_TYPE(float, vec5_t, x, y, z, u, v)

DEFINE_VEC2_TYPE(float, size2_t, w, h)
DEFINE_VEC3_TYPE(float, size3_t, w, d, h)
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