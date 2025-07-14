void merge_shared_faces(int face0_id, int face1_id, int shared_vt0_id, d_lnk_ls_t *grid_list, grid_t *grid, pl_list_t *pl_list, model_t *model);

typedef struct {
  vec3_t vertices[8];
  vec2_tx_t txcoords[8];
  float angles[8];
  int num_vertices;
} face_t;