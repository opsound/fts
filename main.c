#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOK_TAG_BEGIN (1 << 0)
#define TOK_TAG_END (1 << 1)
#define TOK_TEXT (1 << 2)

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

struct tokbin {
	int64_t fpos;
	int32_t tok;
	int32_t len;
};

static void escape(void *p) { __asm__ volatile("" : : "g"(p) : "memory"); }

static void clobber(void) { __asm__ volatile("" : : : "memory"); }

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

void write_token(int64_t fpos, int token, const char *begin, const char *end,
		 void *arg)
{
	struct tokbin tokbin = {
	    .fpos = fpos,
	    .tok = token,
	    .len = end - begin,
	};
	fwrite(&tokbin, sizeof(tokbin), 1, (FILE *)arg);
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

const char *strchr2(const char *begin, const char *end, char ch)
{
	for (; begin < end && *begin != ch; begin++) {
	}
	return begin;
}

typedef void (*emit_t)(int64_t fpos, int token, const char *begin,
		       const char *end, void *arg);

// Scan the [begin, end) range for tokens, `emit`ing them as they are found.
// Returns a pointer to the beginning of un-tokenized input.
const char *tokenize(int64_t fpos, const char *begin, const char *end,
		     emit_t emit, void *arg)
{
	int64_t tpos;
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

		if (tbegin > ptr) {
			tpos = fpos + ptr - begin;
			emit(tpos, TOK_TEXT, ptr, tbegin, arg);
		}

		ptr = tbegin;

		tend = strchr2(tbegin, end, '>');
		if (tend == end)
			break;
		assert(tend > (tbegin + 1) && "Illegal tag: <>");

		if (tbegin[1] == '/') {
			tpos = fpos + &tbegin[2] - begin;
			emit(tpos, TOK_TAG_END, &tbegin[2], tend, arg);
		} else {
			tpos = fpos + &tbegin[1] - begin;
			space = strchr2(&tbegin[1], tend, ' ');
			if (space < tend) {
				slash = strchr2(space, tend, '/');
				if (slash < tend)
					emit(tpos, TOK_TAG_BEGIN | TOK_TAG_END,
					     &tbegin[1], space, arg);
				else
					emit(tpos, TOK_TAG_BEGIN, &tbegin[1],
					     space, arg);
			} else {
				emit(tpos, TOK_TAG_BEGIN, &tbegin[1], tend,
				     arg);
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
	int64_t fpos;
	struct arr buf;
	char *dst;
	const char *ptr;

	// Set toread to the initial buffer size
	toread = 8;
	ainit(&buf, toread, sizeof(char));

	fpos = 0;
	dst = apushn(&buf, toread);
	nread = fread(dst, 1, toread, fp);
	apopn(&buf, toread - nread);

	while (nread > 0) {
		ptr = tokenize(fpos, abegin(&buf), aend(&buf), emit, arg);
		tokenized = ptr - (char *)abegin(&buf);
		untokenized = (char *)aend(&buf) - ptr;
		memmove(abegin(&buf), ptr, untokenized);
		apopn(&buf, tokenized);
		fpos += tokenized;

		if (tokenized == 0)
			toread = 2 * acount(&buf);
		else
			toread = tokenized;

		dst = apushn(&buf, toread);
		nread = fread(dst, 1, toread, fp);
		apopn(&buf, toread - nread);
	}
}

void donothing(int64_t fpos, int token, const char *begin, const char *end,
	       void *arg)
{
	escape(&fpos);
	escape(&token);
	escape((void *)begin);
	escape((void *)end);
	escape(arg);
	clobber();
}

int main(int argc, char **argv)
{
	if (argc != 3)
		fprintf(stderr, "Usage: fts <dom,bin,0> <doc.xml>\n");

	char *mode = argv[1];

	FILE *fp = fopen(argv[2], "r");
	assert(fp);

	if (strcmp(mode, "dom") == 0) {
		struct parse_state ps;
		memset(&ps, 0, sizeof(ps));

		ainit(&ps.nodestk, 8, sizeof(int64_t));
		ainit(&ps.nodes, 8, sizeof(struct node));

		// Push a root node so that we can maintain the invariant that
		// all nodes that we parse have a parent.
		int64_t *inode = apushn(&ps.nodestk, 1);
		*inode = aidx(&ps.nodes, nodepush(&ps.nodes));

		process_file(fp, build_dom, &ps);
		// Start at the first real node (not the root)
		dfs(&ps.nodes, 1, 0);
	} else if (strcmp(mode, "bin") == 0) {
		freopen(NULL, "wb", stdout);
		process_file(fp, write_token, stdout);
	} else if (strcmp(mode, "0") == 0) {
		process_file(fp, donothing, NULL);
	}

	return 0;
}
