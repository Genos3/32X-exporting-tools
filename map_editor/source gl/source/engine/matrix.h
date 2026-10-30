void set_matrix(s32_fix dst[], const s32_fix src[]);
void set_matrix_sp(int a0, int a1, int a2, int a3, s32_fix dst[], const s32_fix src[]);
void translate_matrix(s32_fix xpos, s32_fix ypos, s32_fix zpos, s32_fix matrix[]);
void scale_matrix(s32_fix xscale, s32_fix yscale, s32_fix zscale, s32_fix matrix[]);
void rotate_matrix(u16 xrot, u16 yrot, u16 zrot, s32_fix matrix[], int view);
void rotate_matrix_vertex(s32_fix cxr, s32_fix cyr, s32_fix czr, s32_fix sxr, s32_fix syr, s32_fix szr, s32_fix *xr, s32_fix *yr, s32_fix *zr);
void rotate_matrix_vertex_view(s32_fix cxr, s32_fix cyr, s32_fix czr, s32_fix sxr, s32_fix syr, s32_fix szr, s32_fix *xr, s32_fix *yr, s32_fix *zr);