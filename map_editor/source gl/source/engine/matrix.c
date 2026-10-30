#include "common.h"

void set_matrix(s32_fix dst[restrict], const s32_fix src[restrict]) {
    memcpy32(dst, (void *)src, 12);
    /* for (int i=0; i<12; i++){
        dst[i]=src[i];
    } */
}

void set_matrix_sp(int a0, int a1, int a2, int a3, s32_fix dst[restrict], const s32_fix src[restrict]) {
    if (a0) {
        memcpy32(dst, (void *)src, 3);
    }
    if (a1) {
        memcpy32(dst + 3, (void *)(src + 3), 3);
    }
    if (a2) {
        memcpy32(dst + 6, (void *)(src + 6), 3);
    }
    if (a3) {
        memcpy32(dst + 9, (void *)(src + 9), 3);
    }
}

void translate_matrix(s32_fix xpos, s32_fix ypos, s32_fix zpos, s32_fix matrix[restrict]) {
    matrix[0] += xpos;
    matrix[4] += ypos;
    matrix[8] += zpos;
}

void scale_matrix(s32_fix xscale, s32_fix yscale, s32_fix zscale, s32_fix matrix[restrict]) {
    for (int i = 0; i <= 3; i++) {
        if (xscale) {
            matrix[i] = fp_mul(matrix[i], xscale);
        }
        if (yscale) {
            matrix[i + 4] = fp_mul(matrix[i + 4], yscale);
        }
        if (zscale) {
            matrix[i + 8] = fp_mul(matrix[i + 8], zscale);
        }
    }
}

RAM_CODE void rotate_matrix(u16 xrot, u16 yrot, u16 zrot, s32_fix matrix[restrict], int view) {
    s32_fix cxr = lu_cos(xrot); // .16
    s32_fix sxr = lu_sin(xrot);
    s32_fix cyr = lu_cos(yrot);
    s32_fix syr = lu_sin(yrot);
    s32_fix czr = lu_cos(zrot);
    s32_fix szr = lu_sin(zrot);
    
    for (int i = 0; i <= 3; i++) {
        s32_fix *xr = &matrix[i];
        s32_fix *yr = &matrix[i + 4];
        s32_fix *zr = &matrix[i + 8];
        
        if (view) {
            rotate_matrix_vertex_view(cxr, cyr, czr, sxr, syr, szr, xr, yr, zr);
        } else {
            rotate_matrix_vertex(cxr, cyr, czr, sxr, syr, szr, xr, yr, zr);
        }
    }
}

// rotate the matrix in local space

void rotate_matrix_vertex(s32_fix cxr, s32_fix cyr, s32_fix czr, s32_fix sxr, s32_fix syr, s32_fix szr, s32_fix *xr, s32_fix *yr, s32_fix *zr) {
    s32_fix xt, yt, zt;
    s32_fix xs = *xr;
    s32_fix ys = *yr;
    s32_fix zs = *zr;
    yt = fp_mul(ys, cxr) - fp_mul(zs, sxr); // x
    zt = fp_mul(ys, sxr) + fp_mul(zs, cxr);
    ys = yt;
    zs = zt;
    zt = fp_mul(zs, cyr) - fp_mul(xs, syr); // y
    xt = fp_mul(zs, syr) + fp_mul(xs, cyr);
    zs = zt;
    xs = xt;
    xt = fp_mul(xs, czr) - fp_mul(ys, szr); // z
    yt = fp_mul(xs, szr) + fp_mul(ys, czr);
    xs = xt;
    ys = yt;
    *xr = xs;
    *yr = ys;
    *zr = zs;
}

// rotate the matrix against the camera

RAM_CODE void rotate_matrix_vertex_view(s32_fix cxr, s32_fix cyr, s32_fix czr, s32_fix sxr, s32_fix syr, s32_fix szr, s32_fix *xr, s32_fix *yr, s32_fix *zr) {
    s32_fix xt, yt, zt;
    s32_fix xs = *xr;
    s32_fix ys = *yr;
    s32_fix zs = *zr;
    zt = fp_mul(zs, cyr) - fp_mul(xs, syr); // x
    xt = fp_mul(zs, syr) + fp_mul(xs, cyr);
    zs = zt;
    xs = xt;
    yt = fp_mul(ys, cxr) - fp_mul(zs, sxr); // y
    zt = fp_mul(ys, sxr) + fp_mul(zs, cxr);
    ys = yt;
    zs = zt;
    xt = fp_mul(xs, czr) - fp_mul(ys, szr); // z
    yt = fp_mul(xs, szr) + fp_mul(ys, czr);
    xs = xt;
    ys = yt;
    *xr = xs;
    *yr = ys;
    *zr = zs;
}