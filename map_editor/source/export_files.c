#include <shlwapi.h>
#include "common.h"

void get_map_size(size3_t *map_size);
void compress_rle_map(size3_t map_size);

void export_map_file(char *file_path) {
  size3_t map_size;
  get_map_size(&map_size);
  compress_rle_map(map_size);
  
  FILE *file;
  char *path = strdup(file_path);
  // char path[MAX_PATH];
  // strncpy(path, file_path, MAX_PATH - 1);
  
  PathRenameExtension(path, ".map");
  
  int str_len = strlen(path) + 2; // strlen doesn't return the null terminator
  char *path_bkp = malloc(str_len);
  strncpy(path_bkp, path, str_len);
  PathRenameExtension(path_bkp, ".map1");
  
  if (!CopyFile(path, path_bkp, FALSE)) {
    printf("can't make the backup map file\n");
  }
  free(path_bkp);
  
  file = fopen(path, "w");
  free(path);
  
  if (file == NULL) {
    printf("can't make the map file\n");
    return;
  }
  
  fprintf(file, "map_size = %d, %d, %d;\n", map_size.w, map_size.d, map_size.h);
  // fprintf(file, "map_max_size = %d, %d, %d;\n", MAX_MAP_WIDTH, MAX_MAP_HEIGHT, MAX_MAP_HEIGHT);
  fprintf(file, "tile_size = %.6f, %.6f, %.6f;\n\n", fixtoft(TILE_SIZE), fixtoft(TILE_SIZE), fixtoft(TILE_HEIGHT));
  
  fprintf(file, "rle_map =\n");
  
  for (int i = 0; i < rle_map.size; i++) {
    fprintf(file, "%d,%d", rle_map.data[i].tile_id, rle_map.data[i].length);
    
    if (i < rle_map.size - 1) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, ";");
  
  fclose(file);
  printf("map exported\n");
}

void get_map_size(size3_t *map_size) {
  size3_t max_size;
  max_size.w = 0;
  max_size.h = 0;
  max_size.d = 0;
  
  for (int i = 0; i < MAX_MAP_HEIGHT; i++) {
    for (int j = 0; j < MAX_MAP_DEPTH; j++) {
      for (int k = 0; k < MAX_MAP_WIDTH; k++) {
        if (map[i][j][k].y || map[i][j][k].x || map[i][j][k].z) {
          if (k > max_size.w - 1) max_size.w = k + 1;
          if (i > max_size.h - 1) max_size.h = i + 1;
          if (j > max_size.d - 1) max_size.d = j + 1;
        }
      }
    }
  }
  
  *map_size = max_size;
}

void compress_rle_map(size3_t map_size) {
  rle_segment_t rle_segment;
  int curr_tile = map[0][0][0].y;
  int num_repeated_elements = 0;
  rle_map.size = 0;
  
  #if 1
    for (int i = 0; i < map_size.h; i++) {
      for (int j = 0; j < map_size.d; j++) {
        for (int k = 0; k < map_size.w; k++) {
          for (int l = 0; l < 3; l++) {
            if (*((u8*)&map[i][j][k] + l) != curr_tile) {
              rle_segment.tile_id = curr_tile;
              rle_segment.length = num_repeated_elements;
              
              list_push_pnt(&rle_map, &rle_segment);
              
              curr_tile = *((u8*)&map[i][j][k] + l);
              num_repeated_elements = 1;
            } else {
              num_repeated_elements++;
            }
          }
        }
      }
    }
  #else
    u8 *_map = (u8 *)map;
    int map_length = MAX_MAP_HEIGHT * MAX_MAP_DEPTH * MAX_MAP_WIDTH;
    
    for (int i = 0; i < map_length; i++) {
      if (_map[i] != curr_tile) {
        rle_segment.tile_id = curr_tile;
        rle_segment.length = num_repeated_elements;
        
        list_push_pnt(&rle_map, &rle_segment);
        
        curr_tile = _map[i];
        num_repeated_elements = 1;
      } else {
        num_repeated_elements++;
      }
    }
  #endif
  
  rle_segment.tile_id = curr_tile;
  rle_segment.length = num_repeated_elements;
  list_push_pnt(&rle_map, &rle_segment);
}