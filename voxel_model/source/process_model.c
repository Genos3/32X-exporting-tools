#include "common.h"

void set_face_vis(vx_model_t *model);
void create_rle_grid(vx_model_t *model);
void reduce_model_palette(vx_model_t *model);
void set_model_size(vx_model_t *model);

void process_model(vx_model_t *model) {
  set_face_vis(model);
  
  if (model->pal_size) {
    reduce_model_palette(model);
  }
  
  create_rle_grid(model);
  set_model_size(model);
}

void set_face_vis(vx_model_t *model) {
  int height_size = model->size_i.d * model->size_i.w;
  
  for (int i = 0; i < model->size_i.h; i++) {
    for (int j = 0; j < model->size_i.d; j++) {
      for (int k = 0; k < model->size_i.w; k++) {
        int vx_pnt = i * height_size + j * model->size_i.w + k;
        
        if (!model->voxels[vx_pnt].color_index) continue;
        
        u8 face_vis = 0;
        
        if (!k || !model->voxels[vx_pnt - 1].color_index) {
          face_vis |= 1;
        }
        
        if (k == model->size_i.w - 1 || !model->voxels[vx_pnt + 1].color_index) {
          face_vis |= 2;
        }
        
        if (!j || !model->voxels[vx_pnt - model->size_i.w].color_index) {
          face_vis |= 4;
        }
        
        if (j == model->size_i.d - 1 || !model->voxels[vx_pnt + model->size_i.w].color_index) {
          face_vis |= 8;
        }
        
        if (!i || !model->voxels[vx_pnt - (model->size_i.d * model->size_i.w)].color_index) {
          face_vis |= 16;
        }
        
        if (i == model->size_i.h - 1 || !model->voxels[vx_pnt + (model->size_i.d * model->size_i.w)].color_index) {
          face_vis |= 32;
        }
        
        model->voxels[vx_pnt].vis = face_vis;
      }
    }
  }
}

void create_rle_grid(vx_model_t *model) {
  int height_size = model->size_i.d * model->size_i.w;
  
  for (int i = 0; i < model->size_i.d; i++) {
    for (int j = 0; j < model->size_i.w; j++) {
      int grid_pnt = i * model->size_i.w + j;
      int vx_pnt = grid_pnt;
      
      voxel_vis_t prev_voxel = model->voxels[vx_pnt];
      
      int num_repeated_elements = 1;
      int rle_column_size = 1;
      
      for (int k = 1; k < model->size_i.h; k++) {
        vx_pnt += height_size;
        
        voxel_vis_t curr_voxel = model->voxels[vx_pnt];
        
        // if the voxel is different to the previous one
        if (curr_voxel.color_index != prev_voxel.color_index || curr_voxel.vis != prev_voxel.vis) {
          if (model->rle_grid[grid_pnt].pnt < 0) {
            model->rle_grid[grid_pnt].pnt = model->rle_columns.size;
          }
          
          rle_columns_t rle_segment;
          rle_segment.color_index = prev_voxel.color_index;
          rle_segment.vis = prev_voxel.vis;
          rle_segment.length = num_repeated_elements;
          
          list_push_pnt(&model->rle_columns, &rle_segment);
          
          prev_voxel = curr_voxel;
          
          rle_column_size++;
          num_repeated_elements = 1;
        } else {
          num_repeated_elements++;
        }
      }
      
      // if the whole column is empty
      if (!prev_voxel.color_index && num_repeated_elements == model->size_i.h) continue;
      
      rle_columns_t rle_segment;
      rle_segment.color_index = prev_voxel.color_index;
      rle_segment.vis = prev_voxel.vis;
      rle_segment.length = num_repeated_elements;
      
      list_push_pnt(&model->rle_columns, &rle_segment);
      
      model->rle_grid[grid_pnt].length = rle_column_size;
    }
  }
}

void reduce_model_palette(vx_model_t *model) {
  u16 palette_temp[PALETTE_SIZE];
  u8 palette_index[PALETTE_SIZE];
  palette_index[0] = BG_COLOR;
  int pal_size = 1;
  
  // creates the new palette
  
  for (int i = 0; i < model->length; i++) {
    if (!model->voxels[i].color_index) continue; // voxel is transparent
    
    int color_repeated = 0;
    u16 pal_color = model->palette[model->voxels[i].color_index];
    
    for (int j = 1; j < pal_size; j++) {
      if (palette_temp[j] == pal_color) {
        color_repeated = 1;
        break;
      }
    }
    
    if (!color_repeated) {
      palette_temp[pal_size] = pal_color;
      palette_index[pal_size] = model->voxels[i].color_index;
      pal_size++;
    }
  }
  
  // reindex the colors
  
  for (int i = 0; i < model->length; i++) {
    if (!model->voxels[i].color_index) continue; // voxel is transparent
    
    for (int j = 1; j < pal_size; j++) {
      if (model->voxels[i].color_index == palette_index[j]) {
        model->voxels[i].color_index = j;
        break;
      }
    }
  }
  
  model->pal_size = pal_size;
  
  // replaces the original palette with the new one
  
  memcpy(&model->palette, &palette_temp, model->pal_size * sizeof(u16));
}

void set_model_size(vx_model_t *model) {
  model->size.w = model->size_i.w * VOXEL_SIZE;
  model->size.d = model->size_i.d * VOXEL_SIZE;
  model->size.h = model->size_i.h * VOXEL_SIZE;
  
  model->voxel_radius = (VOXEL_SIZE / 2) / sin(45 * PI_fl / 180);
  
  model->model_radius = max_c(max_c(model->size.w, model->size.d), model->size.h) / 2;
  model->model_radius /= sin(45 * PI_fl / 180);
  
  model->vertices[0].x = -model->size.w / 2;
  model->vertices[0].z = -model->size.d / 2;
  model->vertices[0].y = model->size.h;
  
  model->vertices[1].x = model->size.w / 2;
  model->vertices[1].z = -model->size.d / 2;
  model->vertices[1].y = model->size.h;
  
  model->vertices[2].x = model->size.w / 2;
  model->vertices[2].z = model->size.d / 2;
  model->vertices[2].y = model->size.h;
  
  model->vertices[3].x = -model->size.w / 2;
  model->vertices[3].z = model->size.d / 2;
  model->vertices[3].y = model->size.h;
  
  model->vertices[4].x = -model->size.w / 2;
  model->vertices[4].z = -model->size.d / 2;
  model->vertices[4].y = 0;
  
  model->vertices[5].x = model->size.w / 2;
  model->vertices[5].z = -model->size.d / 2;
  model->vertices[5].y = 0;
  
  model->vertices[6].x = model->size.w / 2;
  model->vertices[6].z = model->size.d / 2;
  model->vertices[6].y = 0;
  
  model->vertices[7].x = -model->size.w / 2;
  model->vertices[7].z = model->size.d / 2;
  model->vertices[7].y = 0;
}