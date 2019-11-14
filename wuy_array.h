#ifndef WUY_ARRAY_H
#define WUY_ARRAY_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	int	item_size;
	int	items;
	int	max;
	char	*data;
} wuy_array_t;

static inline void wuy_array_init(wuy_array_t *array, int item_size)
{
	array->items = 0;
	array->item_size = item_size;
	array->max = 16;
	array->data = malloc(item_size * array->max);
}

static inline bool wuy_array_yet_init(wuy_array_t *array)
{
	return array->data != NULL;
}

static inline wuy_array_t *wuy_array_new(int item_size)
{
	wuy_array_t *array = malloc(sizeof(wuy_array_t));
	if (array == NULL) {
		return NULL;
	}
	wuy_array_init(array, item_size);
	return array;
}

static inline void *wuy_array_push(wuy_array_t *array)
{
	if (array->items == array->max) {
		array->max *= 2;
		array->data = realloc(array->data, array->max * array->item_size);
	}

	return array->data + array->item_size * array->items++;
}

static inline void wuy_array_append(wuy_array_t *array, void *p)
{
	void **pp = wuy_array_push(array);
	*pp = p;
}

static inline int wuy_array_count(wuy_array_t *array)
{
	return array->items;
}

#define wuy_array_iter(array, p) \
	for (p = (void *)((array)->data); (char *)p < (array)->data + (array)->item_size * (array)->items; p++)

#endif
