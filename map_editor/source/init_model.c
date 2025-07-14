#include "common.h"

void init_model_struct(list_model_t *model) {
  init_list(&model->vertices, sizeof(*model->vertices.data));
  init_list(&model->normals, sizeof(*model->normals.data));
  init_list(&model->sprite_vertices, sizeof(*model->sprite_vertices.data));
  init_list(&model->txcoords, sizeof(*model->txcoords.data));
  init_list(&model->faces, sizeof(*model->faces.data));
  init_list(&model->tx_faces, sizeof(*model->tx_faces.data));
  init_list(&model->face_num_vertices, sizeof(*model->face_num_vertices.data));
  init_list(&model->face_materials, sizeof(*model->face_materials.data));
  init_list(&model->face_types, sizeof(*model->face_types.data));
  init_list(&model->face_index, sizeof(*model->face_index.data));
  init_list(&model->tx_face_index, sizeof(*model->tx_face_index.data));
  init_list(&model->sprite_faces, sizeof(*model->sprite_faces.data));
  init_list(&model->sprite_face_index, sizeof(*model->sprite_face_index.data));
  init_list(&model->object_face_index, sizeof(*model->object_face_index.data));
  init_list(&model->object_num_faces, sizeof(*model->object_num_faces.data));
  init_list(&model->objects_size, sizeof(*model->objects_size.data));
  init_list(&model->mtl_textures, sizeof(*model->mtl_textures.data));
}

void init_textures_struct(list_textures_t *textures) {
  init_list(&textures->material_colors, sizeof(*textures->material_colors.data));
  init_list(&textures->cr_palette_idx, sizeof(*textures->cr_palette_idx.data));
  init_list(&textures->material_colors_tx, sizeof(*textures->material_colors_tx.data));
  init_list(&textures->cr_palette_tx_idx, sizeof(*textures->cr_palette_tx_idx.data));
  init_list(&textures->texture_sizes_padded, sizeof(*textures->texture_sizes_padded.data));
  init_list(&textures->texture_width_bits, sizeof(*textures->texture_width_bits.data));
  init_list(&textures->tx_animation_id, sizeof(*textures->tx_animation_id.data));
  init_list(&textures->tx_index, sizeof(*textures->tx_index.data));
  init_list(&textures->texture_data, sizeof(*textures->texture_data.data));
}

void free_model_memory(list_model_t *model) {
  free_list(&model->vertices);
  free_list(&model->normals);
  free_list(&model->sprite_vertices);
  free_list(&model->txcoords);
  free_list(&model->faces);
  free_list(&model->tx_faces);
  free_list(&model->face_num_vertices);
  free_list(&model->face_materials);
  free_list(&model->face_types);
  free_list(&model->face_index);
  free_list(&model->tx_face_index);
  free_list(&model->sprite_faces);
  free_list(&model->sprite_face_index);
  free_list(&model->object_face_index);
  free_list(&model->object_num_faces);
  free_list(&model->objects_size);
  free_list(&model->mtl_textures);
}

void free_textures_memory(list_textures_t *textures) {
  free_list(&textures->material_colors);
  free_list(&textures->cr_palette_idx);
  free_list(&textures->material_colors_tx);
  free_list(&textures->cr_palette_tx_idx);
  free_list(&textures->texture_sizes_padded);
  free_list(&textures->texture_width_bits);
  free_list(&textures->tx_animation_id);
  free_list(&textures->tx_index);
  free_list(&textures->texture_data);
}