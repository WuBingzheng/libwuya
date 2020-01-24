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

#define wuy_array_get_ppval(a, i) *(void **)wuy_array_getp(a, i)
#define wuy_array_get_type(a, i, type) *(type *)wuy_array_getp(a, i)
static inline void *wuy_array_getp(wuy_array_t *array, int i)
{
	return array->data + array->item_size * i;
}

static inline void *wuy_array_push(wuy_array_t *array)
{
	if (array->items == array->max) {
		array->max *= 2;
		array->data = realloc(array->data, array->max * array->item_size);
	}

	return wuy_array_getp(array, array->items++);
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

#define wuy_array_iter_type(array, node, type) \
	for (int _wai = 0, node = wuy_array_get_type(array, _wai, type); \
			_wai < wuy_array_count(array); \
			_wai++, node = wuy_array_get_type(array, _wai, type))

#define wuy_array_iter_ppval(array, node) \
	for (int _wai = 0; _wai < wuy_array_count(array) && (node = wuy_array_get_ppval(array, _wai), 1); _wai++)

#endif
