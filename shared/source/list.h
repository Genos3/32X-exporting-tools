void init_list(void *_list, int type_size);
void free_list(void *_list);
void write_list_int(void *_list, u32 offset, int input);
void write_list_pnt(void *_list, u32 offset, void *input);
void list_malloc_inc(void *_list);
void list_malloc_size_inc(void *_list, int size);
void list_malloc_size(void *_list, int size);
void list_push_int(void *_list, int input);
void list_push_pnt(void *_list, void *input);
void list_push_val_type(void *_list, void *input, int type_size);
void list_alloc_mem(list_def *list, int type_size);

#define LIST_ALLOC_SIZE 64