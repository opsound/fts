#include "shared.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void aresize(struct arr *arr, int64_t n)
{
	if (arr->n >= n)
		return;
	arr->items = realloc(arr->items, n * arr->itemsz);
	arr->n = n;
	assert(arr->items);
}

void ainit(struct arr *arr, int64_t n, int32_t itemsz)
{
	memset(arr, 0, sizeof(*arr));
	arr->itemsz = itemsz;
	aresize(arr, n);
}

void *aat(struct arr *arr, int64_t idx)
{
	return &arr->items[idx * arr->itemsz];
}

void *abegin(struct arr *arr) { return aat(arr, 0); }

void *aend(struct arr *arr) { return aat(arr, arr->i); }

void *alast(struct arr *arr) { return aat(arr, arr->i - 1); }

int64_t acount(struct arr *arr) { return arr->i; }

int64_t aidx(struct arr *arr, void *item)
{
	return ((uint8_t *)item - (uint8_t *)abegin(arr)) / arr->itemsz;
}

void *apushn(struct arr *arr, int64_t n)
{
	while ((arr->i + n) >= arr->n)
		aresize(arr, 2 * arr->n);
	void *rv = aend(arr);
	arr->i += n;
	memset(rv, 0, n * arr->itemsz);
	return rv;
}

void *apopn(struct arr *arr, int64_t n)
{
	arr->i -= n;
	return aend(arr);
}

struct node *nodepush(struct arr *arr)
{
	struct node *node = apushn(arr, 1);
	node->ichildren = -1;
	node->ilastchild = -1;
	node->inext = -1;
	return node;
}

void addchild(struct node *parent, struct node *child, struct arr *arr)
{
	int64_t ichild = aidx(arr, child);

	// Add the only child
	if (parent->ichildren < 0) {
		assert(parent->ilastchild < 0);
		parent->ichildren = ichild;
		parent->ilastchild = ichild;
		return;
	}

	// Append to sibling list
	struct node *sib = aat(arr, parent->ilastchild);
	parent->ilastchild = ichild;

	assert(sib->inext < 0);
	sib->inext = ichild;
}

void build_dom(int64_t fpos, int token, const char *begin, const char *end,
	       void *arg)
{
	(void)fpos;
	char *str;
	struct parse_state *ps = arg;
	struct node *node;
	struct node *parent;
	int64_t *inode;
	assert(begin <= end);

	switch (token) {
	case TOK_TEXT:
	case TOK_TAG_BEGIN:
	case TOK_TAG_BEGIN | TOK_TAG_END:
		node = nodepush(&ps->nodes);
		node->val = strndup(begin, end - begin);
		node->toktype = token;
		inode = alast(&ps->nodestk);
		parent = aat(&ps->nodes, *inode);
		addchild(parent, node, &ps->nodes);
		if (token == TOK_TAG_BEGIN) {
			inode = apushn(&ps->nodestk, 1);
			*inode = aidx(&ps->nodes, node);
		}
		break;

	case TOK_TAG_END:
		inode = apopn(&ps->nodestk, 1);
		node = aat(&ps->nodes, *inode);
		str = strndup(begin, end - begin);
		if (node->toktype != TOK_TAG_BEGIN || strcmp(node->val, str)) {
			fprintf(stderr,
				"Error: malformed xml. Opening tag %s does not "
				"match closing tag %s\n",
				node->val, str);
			exit(-EINVAL);
		}
		free(str);
		break;

	default:
		assert(0 && "Bad token");
	}
}

void dfs(struct arr *arr, int64_t idx, int32_t indent)
{
	if (idx < 0)
		return;

	struct node *node = aat(arr, idx);

	for (int32_t i = 0; i < indent; i++)
		putchar(' ');
	if (node->toktype & TOK_TAG_BEGIN)
		printf("<%s>\n", node->val);
	else
		printf("%s\n", node->val);

	dfs(arr, node->ichildren, indent + 2);
	dfs(arr, node->inext, indent);
}
