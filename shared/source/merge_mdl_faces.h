void merge_mdl_faces(pl_list_t *pl_list, model_t *model);
void set_map_list(int face_id, aabb_t *aabb, d_lnk_ls_t *list, grid_t *grid);
void remove_extra_vertices(model_t *model);
void remove_extra_txcoords(model_t *model);

extern u8 *removed_faces;