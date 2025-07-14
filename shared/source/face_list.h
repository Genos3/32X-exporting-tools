void init_pl_list(pl_list_t *pl_list);
void free_pl_list(pl_list_t *pl_list);
void set_pl_list_from_model(pl_list_t *pl_list, model_t *model);
void pl_list_set_poly(int face_id, poly_t *poly, pl_list_t *pl_list);
void pl_list_add_face(int face_id, u8 add_new, poly_t *poly, pl_list_t *pl_list, model_t *model);
void pl_list_remove_marked_faces(pl_list_t *pl_list, model_t *model);
void set_model_from_pl_list(pl_list_t *pl_list, model_t *model);