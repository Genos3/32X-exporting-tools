u32 key_is_down(u32 key);
u32 key_hit(u32 key);
fixed ArcTan(fixed x);
float atan2_c(float y, float x);

#define PI_FL 3.14159265
#define sin_deg(x) sin((x) * PI_FL / 180)
#define cos_deg(x) cos((x) * PI_FL / 180)
#define tan_deg(x) tan((x) * PI_FL / 180)
#define atan_deg(x) (atan(x) / PI_FL * 180)
#define atan2_deg(x, y) (atan2_c((x), (y)) / PI_FL * 180)

// #define vid_page screen
#define hw_palette pc_palette
// #define Sqrt(x) (int)sqrt(x)
#define memset32_asm memset32

// #define PC_SCREEN_WIDTH 320
// #define PC_SCREEN_HEIGHT 240
// #define PC_SCREEN_SCALE_SIZE 2

extern u16 pc_palette[256];
extern u16 key_curr, key_prev;
extern u16 *screen;