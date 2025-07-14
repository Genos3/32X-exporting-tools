#include <stdio.h>
#include <string.h>
#include <math.h>
#include "defines.h"
#include "pc.h"

u16 pc_palette[256];
u16 key_curr, key_prev;
u16 *screen;

inline u32 key_is_down(u32 key) {
  return key_curr &key;
}

inline u32 key_hit(u32 key) {
  return (key_curr & ~key_prev) &key;
}

// in: .14, out: .8

fixed ArcTan(fixed x) {
  return (int)((double)atan((double)x / (1 << 14)) * 180 / PI_FL * 256 / 360 * (1 << 8));
}

float atan2_c(float y, float x) {
  if (x > 0) {
    return atan(y / x);
  } else if (x < 0 && y >= 0) {
    return atan(y / x) + PI_FL;
  } else if (x < 0 && y < 0) {
    return atan(y / x) - PI_FL;
  } else if (x == 0 && y > 0) {
    return PI_FL / 2;
  } else if (x == 0 && y < 0) {
    return -PI_FL / 2;
  }
  
  return 0; // (0,0) case: Undefined, but returning 0
}