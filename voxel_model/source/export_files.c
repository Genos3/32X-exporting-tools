#include <shlwapi.h>
#include "common.h"

void export_model(char *export_path, vx_model_t *model);

void export_file(int argc, char *file_path, char *export_path, vx_model_t *model) {
  if (argc < 3) {
    export_path = strdup(file_path);
  } else {
    // try to create the directory if it doesn't exist
    if (!PathFileExists(export_path) && !CreateDirectory(export_path, NULL)) {
      puts("can't create the export directory");
      return;
    }
    
    PathStripPath(file_path);
    PathAppend(export_path, file_path);
  }
  
  export_model(export_path, model);
  
  if (argc < 3) {
    free(export_path);
  }
}

void export_model(char *export_path, vx_model_t *model) {
  FILE *file;
  char path[MAX_PATH];
  strncpy(path, export_path, MAX_PATH - 1);
  
  PathRenameExtension(path, ".c");
  
  file = fopen(path, "w");
  
  if (file == NULL) {
    puts("can't make the model file");
    return;
  }
  
  fprintf(file, 
    "#include \"../../shared/source/defines.h\"\n"
    "#include \"../../shared/source/structs.h\"\n\n"
  );
  
  fprintf(file, "static const size3i_t size_i = {%d, %d, %d};\n", (int)model->size_i.w, (int)model->size_i.d, (int)model->size_i.h);
  // fprintf(file, "static const int grid_size = %d;\n", model->rle_grid_size);
  fprintf(file, "static const int pal_size = %d;\n", model->pal_size);
  fprintf(file, "static const int num_vertices = 8;\n");
  fprintf(file, "static const fixed voxel_radius = %d;\n", fix(model->voxel_radius));
  fprintf(file, "static const fixed model_radius = %d;\n", fix(model->model_radius));
  fprintf(file, "static const size3_t size = {%d, %d, %d};\n\n", fix(model->size.w), fix(model->size.d), fix(model->size.h));
  
  fprintf(file, "static const vec3_t vertices[] = {\n");
  
  for (int i = 0; i < 8; i++) {
    fprintf(file, "{%d,%d,%d}", fix(model->vertices[i].x), fix(model->vertices[i].y), fix(model->vertices[i].z));
    
    if (i < 7) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, "};\n\n");
  
  fprintf(file, "static const rle_grid_t rle_grid[] = {\n");
  
  for (int i = 0; i < model->rle_grid_size; i++) {
    fprintf(file, "{%d,%d}", model->rle_grid[i].pnt, model->rle_grid[i].length);
    
    if (i < model->rle_grid_size - 1) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, "};\n\n");
  
  fprintf(file, "static const rle_column_t rle_columns[] = {\n");
  
  for (int i = 0; i < model->rle_columns.size; i++) {
    fprintf(file, "{%d,%d,%d}", model->rle_columns.data[i].color_index, model->rle_columns.data[i].vis, model->rle_columns.data[i].length);
    
    if (i < model->rle_columns.size - 1) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, "};\n\n");
  
  fprintf(file, "static const u16 palette[] = {\n");
  
  for (int i = 0; i < model->pal_size; i++) {
    fprintf(file, "0x%04x", model->palette[i]);
    
    if (i < model->pal_size - 1) {
      fprintf(file, ",");
      
      if ((i & 15) == 15) {
        fprintf(file, "\n");
      }
    }
  }
  fprintf(file, "};\n\n");
  
  fprintf(file,
    "const vx_model_t model_0 = {\n"
    "  .size_i = size_i,\n"
    // "  .grid_size = grid_size,\n"
    "  .pal_size = pal_size,\n"
    "  .num_vertices = num_vertices,\n"
    "  .voxel_radius = voxel_radius,\n"
    "  .model_radius = model_radius,\n"
    "  .size = size,\n"
    "  .vertices = vertices,\n"
    "  .rle_grid = rle_grid,\n"
    "  .rle_columns = rle_columns,\n"
    "  .palette = palette,\n"
    "};"
    );
  
  fclose(file);
}