void set_face_vertices_from_object(object_t *object);
void set_object_vertices_from_face(object_t *object);
void set_poly_from_obj_face(int face_id, poly_t *poly, object_t *object);
void set_obj_face_from_poly(int face_id, poly_t *poly, object_t *object);
void add_obj_face_from_poly(int face_id, poly_t *poly, object_t *object);
void remove_object_marked_faces(object_t *object);
void set_merged_object_vertices_from_face(object_t *object);
void set_merged_object_txcoords_from_face(object_t *object);