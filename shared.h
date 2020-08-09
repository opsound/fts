#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct tokbin {
	int64_t fpos;
	int32_t tok;
#define TOK_TAG_BEGIN (1 << 0)
#define TOK_TAG_END (1 << 1)
#define TOK_TEXT (1 << 2)
	int32_t len;
};

struct arr {
	uint8_t *items;
	int32_t itemsz;
	int64_t i;
	int64_t n;
};

static inline void aresize(struct arr *arr, int64_t n)
{
	if (arr->n >= n)
		return;
	arr->items = realloc(arr->items, n * arr->itemsz);
	arr->n = n;
	assert(arr->items);
}

static inline void ainit(struct arr *arr, int64_t n, int32_t itemsz)
{
	memset(arr, 0, sizeof(*arr));
	arr->itemsz = itemsz;
	aresize(arr, n);
}

static inline void *aat(struct arr *arr, int64_t idx)
{
	return &arr->items[idx * arr->itemsz];
}

static inline void *abegin(struct arr *arr) { return aat(arr, 0); }

static inline void *aend(struct arr *arr) { return aat(arr, arr->i); }

static inline void *alast(struct arr *arr) { return aat(arr, arr->i - 1); }

static inline int64_t acount(struct arr *arr) { return arr->i; }

static inline int64_t aidx(struct arr *arr, void *item)
{
	return ((uint8_t *)item - (uint8_t *)abegin(arr)) / arr->itemsz;
}

static inline void *apushn(struct arr *arr, int64_t n)
{
	while ((arr->i + n) >= arr->n)
		aresize(arr, 2 * arr->n);
	void *rv = aend(arr);
	arr->i += n;
	memset(rv, 0, n * arr->itemsz);
	return rv;
}

static inline void *apopn(struct arr *arr, int64_t n)
{
	arr->i -= n;
	return aend(arr);
}
