#include <math.h>
#include "defines.h"
#include "math_c.h"

void calc_normal(vec3_t *v0, vec3_t *v1, vec3_t *v2, vec3_t *nm) {
  vec3_t d0, d1;
  d0.x = v2->x - v0->x;
  d0.y = v2->y - v0->y;
  d0.z = v2->z - v0->z;
  d1.x = v1->x - v0->x;
  d1.y = v1->y - v0->y;
  d1.z = v1->z - v0->z;
  
  cross(&d0, &d1, nm);
}

float calc_dp(vec3_t *v0, vec3_t *v1, vec3_t *v2) {
  vec3_t d0, d1;
  d0.x = v2->x - v0->x;
  d0.y = v2->y - v0->y;
  d0.z = v2->z - v0->z;
  d1.x = v1->x - v0->x;
  d1.y = v1->y - v0->y;
  d1.z = v1->z - v0->z;
  
  return dot(&d0, &d1);
}

float dot(vec3_t *v0, vec3_t *v1) {
  return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

void cross(vec3_t *v0, vec3_t *v1, vec3_t *nm) {
  nm->x = v0->y * v1->z - v0->z * v1->y;
  nm->y = v0->z * v1->x - v0->x * v1->z;
  nm->z = v0->x * v1->y - v0->y * v1->x;
}

float calc_length_vt(vec3_t *v0, vec3_t *v1) {
  vec3_t dv;
  
  dv.x = v0->x - v1->x;
  dv.y = v0->y - v1->y;
  dv.z = v0->z - v1->z;
  
  return sqrt(dv.x * dv.x + dv.y * dv.y + dv.z * dv.z);
}

float calc_length(vec3_t *v) {
  return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

void normalize(vec3_t *v, vec3_t *u) {
  float length = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
  
  if (length) {
    length = 1 / length;
  }
  
  u->x = v->x * length;
  u->y = v->y * length;
  u->z = v->z * length;
}