#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct doc {
	const char *title;
	const char *url;
	const char *text;
	int64_t id;
};

struct arr {
	uint8_t *items;
	int32_t itemsz;
	int64_t i;
	int64_t n;
};

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

#define TOK_TAG_BEGIN (1 << 0)
#define TOK_TAG_END (1 << 1)
#define TOK_TEXT (1 << 2)

struct node {
	const char *val;

	int64_t ichildren;
	int64_t ilastchild;
	int64_t inext;

	int toktype;
};

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

struct parse_state {
	struct arr nodestk;
	struct arr nodes;
};

void emit(int token, const char *begin, const char *end, void *arg)
{
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

const char *strchr2(const char *begin, const char *end, char ch)
{
	for (; begin < end && *begin != ch; begin++) {
	}
	return begin;
}

typedef void (*emit_t)(int token, const char *begin, const char *end,
		       void *arg);

// Scan the [begin, end) range for tokens, `emit`ing them as they are found.
// Returns a pointer to the beginning of un-tokenized input.
const char *tokenize(const char *begin, const char *end, emit_t emit, void *arg)
{
	const char *tbegin;
	const char *tend;
	const char *slash;
	const char *space;

	const char *ptr = begin;

	// Advance ptr when we emit a token
	while (ptr < end) {
		tbegin = strchr2(ptr, end, '<');
		if (tbegin == end)
			break;

		if (tbegin > ptr)
			emit(TOK_TEXT, ptr, tbegin, arg);

		ptr = tbegin;

		tend = strchr2(tbegin, end, '>');
		if (tend == end)
			break;
		assert(tend > (tbegin + 1) && "Illegal tag: <>");

		if (tbegin[1] == '/') {
			emit(TOK_TAG_END, &tbegin[2], tend, arg);
		} else {
			space = strchr2(&tbegin[1], tend, ' ');
			if (space < tend) {
				slash = strchr2(space, tend, '/');
				if (slash < tend)
					emit(TOK_TAG_BEGIN | TOK_TAG_END,
					     &tbegin[1], space, arg);
				else
					emit(TOK_TAG_BEGIN, &tbegin[1], space,
					     arg);
			} else {
				emit(TOK_TAG_BEGIN, &tbegin[1], tend, arg);
			}
		}

		ptr = tend + 1;
	}

	return ptr;
}

void process_file(FILE *fp, emit_t emit, void *arg)
{
	size_t nread;
	int64_t toread;
	int64_t untokenized;
	int64_t tokenized;
	struct arr buf;
	char *dst;
	const char *ptr;

	// Set toread to the initial buffer size
	toread = 8;
	ainit(&buf, toread, sizeof(char));

	dst = apushn(&buf, toread);
	nread = fread(dst, 1, toread, fp);
	apopn(&buf, toread - nread);

	while (nread > 0) {
		ptr = tokenize(abegin(&buf), aend(&buf), emit, arg);
		tokenized = ptr - (char *)abegin(&buf);
		untokenized = (char *)aend(&buf) - ptr;
		memmove(abegin(&buf), ptr, untokenized);
		apopn(&buf, tokenized);

		if (tokenized == 0)
			toread = 2 * acount(&buf);
		else
			toread = tokenized;

		dst = apushn(&buf, toread);
		nread = fread(dst, 1, toread, fp);
		apopn(&buf, toread - nread);
	}
}

int64_t hash(const char *str)
{
	int32_t p = 31;
	int32_t m = 1e9 + 9;
	int64_t hash_value = 0;
	int64_t p_pow = 1;
	while (*str) {
		hash_value = (hash_value + (*str - 'a' + 1) * p_pow) % m;
		p_pow = (p_pow * p) % m;
		str++;
	}
	return hash_value;
}

static void escape(void *p) { __asm__ volatile("" : : "g"(p) : "memory"); }

static void clobber() { __asm__ volatile("" : : : "memory"); }

void donothing(int token, const char *begin, const char *end, void *arg)
{
	escape(&token);
	escape((void *)begin);
	escape((void *)end);
	escape(arg);
	clobber();
}

int main(int argc, char **argv)
{
	if (argc != 2)
		fprintf(stderr, "Usage: fts <doc.xml>\n");

	FILE *fp = fopen(argv[1], "r");
	assert(fp);

	struct parse_state ps;
	memset(&ps, 0, sizeof(ps));

	ainit(&ps.nodestk, 8, sizeof(int64_t));
	ainit(&ps.nodes, 8, sizeof(struct node));

	// Push a root node so that we can maintain the invariant that all
	// nodes that we parse have a parent.
	int64_t *inode = apushn(&ps.nodestk, 1);
	*inode = aidx(&ps.nodes, nodepush(&ps.nodes));

	process_file(fp, emit, &ps);
	/* process_file(fp, donothing, NULL); */

	// Start at the first real node (not the root)
	dfs(&ps.nodes, 1, 0);

	return 0;
}
