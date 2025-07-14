#include "common.h"

void init_memory(vx_model_t *model);
void free_memory(vx_model_t *model);

int main(int argc, char *argv[]) {
  if (argc < 2) return 1;
  
  char *file_path = argv[1];
  char *export_path = NULL;
  
  if (argc == 3){
    export_path = argv[2];
  }
  
  load_model(file_path, &model);
  
  init_memory(&model);
  
  process_model(&model);
  
  export_file(argc, file_path, export_path, &model);
  
  free_memory(&model);
  
  return 0;
}

void init_memory(vx_model_t *model) {
  model->rle_grid_size = model->size_i.w * model->size_i.d;
  model->rle_grid = malloc(model->rle_grid_size * sizeof(*model->rle_grid));
  
  for (int i = 0; i < model->rle_grid_size; i++) {
    model->rle_grid[i].pnt = -1;
    model->rle_grid[i].length = 0;
  }
  
  init_list(&model->rle_columns, sizeof(*model->rle_columns.data));
}

void free_memory(vx_model_t *model) {
  free(model->voxels);
  free(model->rle_grid);
  free_list(&model->rle_columns);
}