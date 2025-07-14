void calc_normal(vec3_t *v0, vec3_t *v1, vec3_t *v2, vec3_t *nm);
float calc_dp(vec3_t *v0, vec3_t *v1, vec3_t *v2);
float dot(vec3_t *v0, vec3_t *v1);
void cross(vec3_t *v0, vec3_t *v1, vec3_t *nm);
float calc_length_vt(vec3_t *v0, vec3_t *v1);
float calc_length(vec3_t *v);
void normalize(vec3_t *v, vec3_t *u);