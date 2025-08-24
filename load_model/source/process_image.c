#include <png.h>
#include "common.h"
#include "libpng.h"
#include "oct_quant.h"

#define LOAD_PNG 1

void load_png_image(image img);
void loadbmp(char *filename, int **data);
void make_alpha_mask();
void average_colors();
int count_colors(image img);
void load_image(image img);
void set_palette(textures_t *textures);
void index_image(textures_t *textures);
void output_image_data();
void output_palette(textures_t *textures);
//void makefile_txt();

static int image_width, image_height, image_size, alpha_color;
byte tx_has_alpha, image_is_rgba;
static int *image_data;
//static byte textures_repeated[num_textures];
static byte *buffer, *alpha_mask;

void process_image(char *str, textures_t *textures) {
  image img;
  tx_has_alpha = 0;
  
  #if !LOAD_PNG
    loadbmp(str, &image_data);
  #else
    Read_PNG(str, &buffer, &image_width, &image_height);
    img = img_new(image_width, image_height);
    load_png_image(img);
  #endif
  
  if (image_is_rgba || ini.has_alpha_cr) {
    make_alpha_mask();
  }
  
  #if EXPORT_AVERAGE_PAL
    average_colors();
  #endif
  
  free(buffer);
  
  // quantize the image
  
  if (ini.export_texture_data && ini.quantize_tx_colors && count_colors(img)) {
    color_quant(img, ini.quant_tx_pal_size, ini.quant_dithering);
  }
  
  load_image(img);
  free(img);
  
  if (ini.export_texture_data) {
    set_palette(textures);
    index_image(textures);
  }
  
  //output_text();
  //makefile_txt();
  free(image_data);
  if (image_is_rgba || ini.has_alpha_cr) {
    free(alpha_mask);
  }
}

void load_png_image(image img) {
  image_size = image_width * image_height;
  //image_data = malloc(image_width * image_height * sizeof(int));
  
  for (int i = 0; i < image_size; i++) {
    img->pix[i * 3] = buffer[i * 4];
    img->pix[i * 3 + 1] = buffer[i * 4 + 1];
    img->pix[i * 3 + 2] = buffer[i * 4 + 2];
    //image_data[i * image_width + j] = row_pointers[i][j * 3 + 2] << 16 | row_pointers[i][j * 3 + 1] << 8 | row_pointers[i][j * 3];
  }
  
  img->w = image_width;
  img->h = image_height;
  //image_width = image_width;
  //image_height = image_height;
}

void loadbmp(char *filename, int **data) {
  byte *imgt;
  FILE *file;
  uint imgsize;
  int width, h;
  
  file = fopen(filename, "rb");
  if (file == NULL)
    return;
  
  fseek(file, 18, SEEK_SET);
  fread(&width, 4, 1, file);
  fseek(file, 22, SEEK_SET);
  fread(&h, 4, 1, file);
  imgsize = width * h * 3;
  imgt = malloc(imgsize);
  *data = malloc(width * h * sizeof(int));
  fseek(file, 54, SEEK_SET);
  fread(imgt, imgsize, 1, file);
  fclose(file);

  for (int i = 0; i < h; i++) {
    for (int j = 0; j < width; j++) {
      int a = ((h - i - 1) * width + j) * 3;
      (*data)[i * width + j] = imgt[a + 2] << 16 | imgt[a + 1] << 8 | imgt[a];
    }
  }
  
  //image_width = width;
  //image_height = h;
  //image_size = image_width * image_height;
  free(imgt);
}

void make_alpha_mask() {
  alpha_mask = malloc(image_size);
  
  if (image_is_rgba) {
    for (int i = 0; i < image_size; i++) {
      if (buffer[i * 4 + 3] == 0) {
        if (!tx_has_alpha) {
          tx_has_alpha = 1;
        }
        alpha_mask[i] = 1;
      } else {
        alpha_mask[i] = 0;
      }
    }
  }
  else if (ini.has_alpha_cr) {
    for (int i = 0; i < image_size; i++) {
      if (buffer[i * 4] == ini.alpha_cr_r &&
          buffer[i * 4 + 1] == ini.alpha_cr_g &&
          buffer[i * 4 + 2] == ini.alpha_cr_b) {
        if (!tx_has_alpha) {
          tx_has_alpha = 1;
        }
        alpha_mask[i] = 1;
      } else {
        alpha_mask[i] = 0;
      }
    }
  }
}

// create the palette with the average colors

void average_colors(textures_t *textures) {
  int avg_r = 0;
  int avg_g = 0;
  int avg_b = 0;
  int visible_size = 0;
  
  for (int i = 0; i < image_size; i++) {
    if (tx_has_alpha && alpha_mask[i]) continue;
    avg_r += buffer[i * 4];
    avg_g += buffer[i * 4 + 1];
    avg_b += buffer[i * 4 + 2];
    visible_size++;
  }
  //printf("%d,%d,%d\n",avg_r,avg_g,avg_b);
  
  if (visible_size) {
    avg_r = (int)(avg_r / visible_size);
    avg_g = (int)(avg_g / visible_size);
    avg_b = (int)(avg_b / visible_size);
  }
  
  textures->cr_palette.data[textures->num_textures] = ((avg_b >> 3) << 10) | ((avg_g >> 3) << 5) | (avg_r >> 3);
  textures->pal_size++;
}

// if the image has more colors than quant_tx_pal_size return 1 else return 0

int count_colors(image img) {
  //int cr_buffer[PAL_SIZE];
  int *cr_buffer = malloc((ini.quant_tx_pal_size + 1) * sizeof(int));
  int num_colors = 0;
  
  for (int i = 0; i < image_size * 3; i += 3) {
    int color_repeated = 0;
    int color = img->pix[i + 2] << 16 | img->pix[i + 1] << 8 | img->pix[i];
    
    for (int j = 0; j < num_colors; j++) {
      if (cr_buffer[j] == color) {
        color_repeated = 1;
        break;
      }
    }
    
    if (!color_repeated) {
      if (num_colors == ini.quant_tx_pal_size) {
        free(cr_buffer);
        return 1;
      }
      
      cr_buffer[num_colors] = color;
      num_colors++;
    }
  }
  
  free(cr_buffer);
  return 0;
}

void load_image(image img) {
  image_data = malloc(image_size * sizeof(int));
  
  for (int i = 0; i < image_height; i++) {
    for (int j = 0; j < image_width; j++) {
      int img_pnt = (i * image_width + j) * 3;
      //image_data[i * image_width + j] = img->pix[pt + 2] << 16 | img->pix[pt + 1] << 8 | img->pix[pt];
      // reduce 24-bit image to 15-bit
      if (tx_has_alpha && alpha_mask[i * image_width + j]) {
        image_data[i * image_width + j] = alpha_color;
      } else {
        image_data[i * image_width + j] = (img->pix[img_pnt + 2] >> 3) << 10 | (img->pix[img_pnt + 1] >> 3) << 5 | (img->pix[img_pnt] >> 3);
      }
    }
  }
}

// create the palette for the texture

void set_palette(textures_t *textures) {
  //textures->pal_size_tx = 0;
  for (int i = 0; i < image_size; i++) {
    int color_repeated = 0;
    
    for (int j = 0; j < textures->pal_size_tx; j++) {
      if (textures->cr_palette_tx.data[j] == image_data[i]) {
        color_repeated = 1;
        break;
      }
    }
    
    if (!color_repeated) {
      list_push_int(&textures->cr_palette_tx, image_data[i]);
      
      for (int j = 0; j < textures->lightmap_levels; j++) {
        list_malloc_inc(&textures->cr_palette_tx_idx);
      }
      
      textures->pal_size_tx++;
    }
  }
}

// create a palette indexed texture and store the metadata for it

void index_image(textures_t *textures) {
  int full_image_width = 1 << (count_bits(image_width - 1));
  int full_image_size = full_image_width * image_height;
  
  list_malloc_size_inc(&textures->texture_data, full_image_size);
  
  for (int i = 0; i < image_height; i++) {
    for (int j = 0; j < full_image_width; j++) {
      int tx_pnt = textures->texture_data_total_size + i * full_image_width + j;
      textures->texture_data.data[tx_pnt] = 0;
      
      if (j < image_width) {
        int img_pnt = i * image_width + j;
        
        for (int k = 0; k < textures->pal_size_tx; k++) {
          if (image_data[img_pnt] == textures->cr_palette_tx.data[k]) {
            textures->texture_data.data[tx_pnt] = k;
            break;
          }
        }
      }
    }
  }
  
  /* for (int i = 0; i < image_size; i++) {
    int tx_pnt = textures->texture_data_total_size + i;
    textures->texture_data.data[tx_pnt] = 0;
    
    for (int j = 0; j < textures->pal_size_tx; j++) {
      if (image_data[i] == textures->cr_palette_tx.data[j]) {
        textures->texture_data.data[tx_pnt] = j;
        break;
      }
    }
  } */
  
  /* textures->texture_data.data[textures->num_textures] = malloc(image_size * sizeof(u16));
  for (int i = 0; i < image_size; i++) {
    textures->texture_data.data[textures->num_textures][i] = 0;
    for (int j = 0; j < textures->pal_size_tx; j++) {
      if (image_data[i] == textures->cr_palette_tx.data[j]) {
        textures->texture_data.data[textures->num_textures][i] = j;
        break;
      }
    }
  } */
  tx_group_t *texture = &textures->tx_group.data[textures->num_textures];
  
  texture->size.w = image_width;
  texture->size.h = image_height;
  texture->size_padded.w = full_image_width - 1;
  texture->size_padded.h = (1 << (count_bits(image_height - 1))) - 1;
  texture->width_bits = count_bits(image_width - 1);
  //textures_repeated[textures->num_textures] = !(image_width & (image_width - 1) || image_height & (image_height - 1));
  //textures->image_width_bit[textures->num_textures] = log2_c(image_width);
  texture->total_size = full_image_size;
  texture->tx_index = textures->texture_data_total_size;
  textures->texture_data_total_size += full_image_size;
}

// start of functions called from outside

// reduce the number of colors of the palette

void quantize_palette(list_u16 *palette, int n_colors, textures_t *textures) {
  image img = img_new(palette->size, 1);
  
  if (ini.create_lightmap) {
    n_colors /= textures->lightmap_levels;
  }
  
  for (int i = 0; i < palette->size; i++) {
    img->pix[i * 3] = (palette->data[i] & 31) << 3;
    img->pix[i * 3 + 1] = ((palette->data[i] >> 5) & 31) << 3;
    img->pix[i * 3 + 2] = (palette->data[i] >> 10) << 3;
  }
  
  color_quant(img, n_colors, 0);
  
  for (int i = 0; i < palette->size; i++) {
    palette->data[i] = ((img->pix[i * 3 + 2] >> 3) << 10) | ((img->pix[i * 3 + 1] >> 3) << 5) | (img->pix[i * 3] >> 3);
  }
  
  free(img);
}

// remove the repeated colors from the palette and reindex the colors

void index_palette(list_u16 *palette, u8 textured, textures_t *textures) {
  int *pal_index = malloc(palette->size * sizeof(int));
  int pal_map_size = 0;
  
  // remove the repeated colors from the palette
  
  for (int i = 0; i < palette->size; i++) {
    int color_repeated = 0;
    
    for (int j = 0; j < pal_map_size; j++) {
      if (palette->data[i] == palette->data[j]) {
        color_repeated = 1;
        break;
      }
    }
    
    pal_index[i] = pal_map_size;
    
    if (!color_repeated) {
      if (i != pal_map_size) {
        palette->data[pal_map_size] = palette->data[i];
      }
      
      pal_map_size++;
    }
  }
  
  palette->size = pal_map_size;
  
  // reindex the colors
  
  if (!textured) {
    textures->pal_size = pal_map_size;
    
    for (int i = 0; i < model.num_materials; i++) {
      u16 mtl_color = model.materials.data[i].colors;
      model.materials.data[i].colors = pal_index[mtl_color];
    }
  } else {
    textures->pal_size_tx = pal_map_size;
    
    for (int i = 0; i < textures->num_textures; i++) {
      int tx_index = textures->tx_group.data[i].tx_index;
      
      if (ini.create_lightmap) {
        for (int j = 0; j < textures->tx_group.data[i].total_size; j++) {
          int tx_pnt = tx_index + j;
          u16 tx_color = textures->texture_data.data[tx_pnt];
          textures->texture_data.data[tx_pnt] = pal_index[tx_color] * textures->lightmap_levels;
        }
      } else {
        for (int j = 0; j < textures->tx_group.data[i].total_size; j++) {
          int tx_pnt = tx_index + j;
          u16 tx_color = textures->texture_data.data[tx_pnt];
          textures->texture_data.data[tx_pnt] = pal_index[tx_color];
        }
      }
    }
    
    for (int i = 0; i < model.num_materials; i++) {
      u16 tx_color = model.materials.data[i].colors_tx;
      model.materials.data[i].colors_tx = pal_index[tx_color];
    }
  }
  
  free(pal_index);
}

void set_alpha_color(textures_t *textures) {
  alpha_color = (((ini.alpha_cr_b) >> 3) << 10) | (((ini.alpha_cr_g) >> 3) << 5) | ((ini.alpha_cr_r) >> 3);
  list_push_int(&textures->cr_palette_tx, alpha_color);
  
  for (int i = 0; i < textures->lightmap_levels; i++) {
    list_malloc_inc(&textures->cr_palette_tx_idx);
  }
  
  // memset(textures->cr_palette_tx_idx.data, 0, textures->cr_palette_tx_idx.type_size);
  textures->pal_size_tx++;
}

// create an index to access the textures

void set_tx_index(textures_t *textures) {
  int n = 0;
  
  for (int i = 0; i < textures->num_textures; i++) {
    textures->tx_group.data[i].tx_index = n;
    n += textures->tx_group.data[i].size.w * textures->tx_group.data[i].size.h;
    // n += textures->texture_total_sizes.data[i];
  }
}

// set the list of untextured colors for each material
// count the number of colors and select the one with the higher count

void set_untextured_colors(textures_t *textures) {
  u16 *indexed_color = malloc(ini.quant_tx_pal_size * sizeof(u16));
  int *indexed_color_count = malloc(ini.quant_tx_pal_size * sizeof(int));
  
  for (int i = 0; i < textures->num_textures; i++) {
    memset(indexed_color, 0, ini.quant_tx_pal_size * sizeof(u16));
    memset(indexed_color_count, 0, ini.quant_tx_pal_size * sizeof(int));
    // int tx_idx = textures->tx_index.data[i];
    int num_image_colors = 0;
    int tx_index = textures->tx_group.data[i].tx_index;
    
    for (int j = 0; j < textures->tx_group.data[i].total_size; j++) {
      int color_repeated = 0;
      u16 tx_color_index = textures->texture_data.data[tx_index + j];
      
      for (int k = 0; k < num_image_colors; k++) {
        if (tx_color_index == indexed_color[k]) {
          indexed_color_count[k]++;
          color_repeated = 1;
          break;
        }
      }
      
      if (!color_repeated) {
        indexed_color[num_image_colors] = tx_color_index;
        num_image_colors++;
      }
    }
    
    int max_index = 0;
    int max_count = indexed_color_count[0];
    
    for (int j = 1; j < num_image_colors; j++) {
      if (indexed_color_count[j] > max_count) {
        max_index = j;
        max_count = indexed_color_count[j];
      }
    }
    
    model.materials.data[i].colors_tx = indexed_color[max_index];
  }
  
  free(indexed_color);
  free(indexed_color_count);
}

// create the light gradient for the average and the texture palette

void set_palette_gradients(textures_t *textures) {
  float lr = (float)ini.light_color_r / 31;
  float lg = (float)ini.light_color_g / 31;
  float lb = (float)ini.light_color_b / 31;
  
  #if EXPORT_AVERAGE_PAL
    for (int i = 0; i < textures->num_textures; i++) {
      float b = lb * (textures->cr_palette.data[i] >> 10);
      float g = lg * ((textures->cr_palette.data[i] >> 5) & 31);
      float r = lr * (textures->cr_palette.data[i] & 31);
      float tr = r;
      float tg = g;
      float tb = b;
      float sr = r / textures->lightmap_levels;
      float sg = g / textures->lightmap_levels;
      float sb = b / textures->lightmap_levels;
      
      for (int j = textures->lightmap_levels - 1; j >= 0; j--) {
        // textures->cr_palette_idx.data[i * LIGHT_GRD_T + j] = ((int)tb << 10) | ((int)tg << 5) | (int)tr;
        textures->cr_palette_idx.data[i * textures->lightmap_levels + j] = ((int)tb << 10) | ((int)tg << 5) | (int)tr;
        tr -= sr;
        tg -= sg;
        tb -= sb;
      }
      
      if (SPECULAR_LIGHT) {
        tr = r;
        tg = g;
        tb = b;
        sr = (ini.light_color_r - r) / textures->lightmap_levels;
        sg = (ini.light_color_g - g) / textures->lightmap_levels;
        sb = (ini.light_color_b - b) / textures->lightmap_levels;
        
        for (int j = 0; j < textures->lightmap_levels; j++) {
          tr += sr;
          tg += sg;
          tb += sb;
          
          if (tr > 31) tr = 31;
          if (tg > 31) tg = 31;
          if (tb > 31) tb = 31;
          
          textures->cr_palette_idx.data[i * textures->lightmap_levels * 2 + j + textures->lightmap_levels] = ((int)tb << 10) | ((int)tg << 5) | (int)tr;
        }
      }
    }
    
    textures->pal_num_colors = textures->pal_size;
    textures->pal_size *= textures->lightmap_levels;
  #endif
  
  if (ini.export_texture_data) {
    for (int i = 0; i < textures->pal_size_tx; i++) {
      float b = lb * (textures->cr_palette_tx.data[i] >> 10);
      float g = lg * ((textures->cr_palette_tx.data[i] >> 5) & 31);
      float r = lr * (textures->cr_palette_tx.data[i] & 31);
      float tr = r;
      float tg = g;
      float tb = b;
      float sr = r / textures->lightmap_levels;
      float sg = g / textures->lightmap_levels;
      float sb = b / textures->lightmap_levels;
      
      for (int j = textures->lightmap_levels - 1; j >= 0; j--) {
        // textures->cr_palette_tx_idx.data[i * LIGHT_GRD_T + j] = ((int)tb << 10) | ((int)tg << 5) | (int)tr;
        textures->cr_palette_tx_idx.data[i * textures->lightmap_levels + j] = ((int)tb << 10) | ((int)tg << 5) | (int)tr;
        tr -= sr;
        tg -= sg;
        tb -= sb;
      }
      
      if (SPECULAR_LIGHT) {
        tr = r;
        tg = g;
        tb = b;
        sr = (ini.light_color_r - r) / textures->lightmap_levels;
        sg = (ini.light_color_g - g) / textures->lightmap_levels;
        sb = (ini.light_color_b - b) / textures->lightmap_levels;
        
        for (int j = 0; j < textures->lightmap_levels; j++) {
          tr += sr;
          tg += sg;
          tb += sb;
          
          if (tr > 31) tr = 31;
          if (tg > 31) tg = 31;
          if (tb > 31) tb = 31;
          
          textures->cr_palette_tx_idx.data[i * textures->lightmap_levels * 2 + j + textures->lightmap_levels] = ((int)tb << 10) | ((int)tg << 5) | (int)tr;
        }
      }
    }
    
    textures->pal_tx_num_colors = textures->pal_size_tx;
    textures->pal_size_tx *= textures->lightmap_levels;
  }
}

// duplicates the color indices inside the textures

void dup_textures_color_index(textures_t *textures) {
  for (int i = 0; i < textures->texture_data_total_size; i++) {
    u16 color_index = textures->texture_data.data[i];
    textures->texture_data.data[i] = dup8(color_index);
  }
}

void output_image_data() {
  for (int i = 0; i < image_size * 3; i++) {
    printf("%d ", buffer[i]);
  }
}

void output_palette(textures_t *textures) {
  for (int i = 0; i < textures->pal_size_tx; i++) {
    printf("0x%06x", textures->cr_palette_tx.data[i]);
    
    if (i < textures->pal_size_tx - 1) {
      printf(",");
    }
  }
}

/* void makefile_txt(textures_t *textures) {
  FILE *file;
  file = fopen("image.txt", "wb");
  for (int i = 0; i < textures->pal_size_tx; i++) {
    fprintf(file, "0x%06x", textures->cr_palette_tx.data[i]);
    if (i < textures->pal_size_tx - 1) {
      fprintf(file, ",");
    }
  }
  fclose(file);
} */