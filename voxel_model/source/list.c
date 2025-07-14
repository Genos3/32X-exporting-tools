#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"
#include "structs.h"
#include "globals.h"
#include "list.h"

int g_ls_malloc_count = 0;

// init the list

void init_list(void *_list, int type_size) {
  list_def *list = _list;
  
  list->data = NULL;
  list->alloc_mem = 0;
  list->size = 0;
  list->type_size = type_size;
}

// reset and free the list memory

void free_list(void *_list) {
  list_def *list = _list;
  
  if (list->data) {
    list->alloc_mem = 0;
    list->size = 0;
    free(list->data);
    list->data = NULL;
    g_ls_malloc_count--;
  }
}

// write individual values to an already allocated list

void write_list_int(void *_list, u32 offset, int input) {
  list_def *list = _list;
  
  if (offset < list->size) {
    memcpy(&list->data[list->size * list->type_size], &input, list->type_size);
  }
}

// write individual values to an already allocated list

void write_list_pnt(void *_list, u32 offset, void *input) {
  list_def *list = _list;
  
  if (offset < list->size) {
    memcpy(&list->data[offset], input, list->type_size);
  }
}

// write a struct to an already allocated list

void list_malloc_inc(void *_list) {
  list_def *list = _list;
  
  list_alloc_mem(list, 1);
  
  list->size++;
}

// malloc a specific size_mem of memory to the list

void list_malloc_size(void *_list, int size) {
  list_def *list = _list;
  
  list_alloc_mem(list, size);
  
  list->size += size;
}

// push an integer to the top of the list

void list_push_int(void *_list, int input) {
  list_def *list = _list;
  
  list_alloc_mem(list, 1);
  
  memcpy(&list->data[list->size * list->type_size], &input, list->type_size);
  
  list->size++;
}

// push a struct to the top of the list

void list_push_pnt(void *_list, void *input) {
  list_def *list = _list;
  
  list_alloc_mem(list, 1);
  
  memcpy(&list->data[list->size * list->type_size], input, list->type_size);
  
  list->size++;
}

// push a specific type size or a float to the top of the list

void list_push_val_type(void *_list, void *input, int type_size) {
  list_def *list = _list;
  
  list_alloc_mem(list, 1);
  
  memcpy(&list->data[list->size * list->type_size], input, type_size);
  
  list->size++;
}

// allocates the initial memory of the list or uses realloc to extend it

inline void list_alloc_mem(list_def *list, int size) {
  if (!(list->alloc_mem)) {
    list->alloc_mem = max_c(LIST_ALLOC_SIZE, size);
    list->data = malloc(list->alloc_mem * list->type_size);
    g_ls_malloc_count++;
  }
  else if (list->size + size > list->alloc_mem) {
    do {
      list->alloc_mem <<= 1;
    } while (list->size + size > list->alloc_mem);
    
    list->data = realloc(list->data, list->alloc_mem * list->type_size);
  }
}