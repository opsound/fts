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

struct node {
	const char *val;
	int64_t ichildren;
	int64_t ilastchild;
	int64_t inext;
	int toktype;
};

struct parse_state {
	struct arr nodestk;
	struct arr nodes;
};

void aresize(struct arr *arr, int64_t n);
void ainit(struct arr *arr, int64_t n, int32_t itemsz);
void *aat(struct arr *arr, int64_t idx);
void *abegin(struct arr *arr);
void *aend(struct arr *arr);
void *alast(struct arr *arr);
int64_t acount(struct arr *arr);
int64_t aidx(struct arr *arr, void *item);
void *apushn(struct arr *arr, int64_t n);
void *apopn(struct arr *arr, int64_t n);

struct node *nodepush(struct arr *arr);

void build_dom(int64_t fpos, int token, const char *begin, const char *end,
	       void *arg);
void dfs(struct arr *arr, int64_t idx, int32_t indent);

static inline void escape(void *p)
{
	__asm__ volatile("" : : "g"(p) : "memory");
}
static inline void clobber(void) { __asm__ volatile("" : : : "memory"); }
