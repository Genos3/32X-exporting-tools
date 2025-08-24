#include "common.h"

void init_model_struct(model_t *model) {
  init_list(&model->materials, sizeof(*model->materials.data));
  init_list(&model->objects, sizeof(*model->objects.data));
  init_list(&model->objects_id, sizeof(*model->objects_id.data));
}

void init_object_struct(object_t *object) {
  init_list(&object->vertices, sizeof(*object->vertices.data));
  init_list(&object->txcoords, sizeof(*object->txcoords.data));
  init_list(&object->sprite_vertices, sizeof(*object->sprite_vertices.data));
  init_list(&object->faces, sizeof(*object->faces.data));
}

void init_textures_struct(textures_t *textures) {
  init_list(&textures->cr_palette_idx, sizeof(*textures->cr_palette_idx.data));
  init_list(&textures->cr_palette, sizeof(*textures->cr_palette.data));
  init_list(&textures->cr_palette_tx_idx, sizeof(*textures->cr_palette_tx_idx.data));
  init_list(&textures->cr_palette_tx, sizeof(*textures->cr_palette_tx.data));
  init_list(&textures->tx_group, sizeof(*textures->tx_group.data));
  init_list(&textures->texture_data, sizeof(*textures->texture_data.data));
}

void free_model_memory(model_t *model) {
  free_list(&model->materials);
  free_list(&model->objects);
  free_list(&model->objects_id);
}

void free_object_memory(object_t *object) {
  free_list(&object->vertices);
  free_list(&object->txcoords);
  free_list(&object->sprite_vertices);
  free_list(&object->faces);
}

void free_textures_memory(textures_t *textures) {
  free_list(&textures->cr_palette_idx);
  free_list(&textures->cr_palette);
  free_list(&textures->cr_palette_tx_idx);
  free_list(&textures->cr_palette_tx);
  free_list(&textures->tx_group);
  free_list(&textures->texture_data);
}