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
	int id;
};

struct arr {
	char *items;
	int itemsz;
	int i;
	int n;
};

void aresize(struct arr *arr, int n)
{
	if (arr->n >= n)
		return;
	arr->items = realloc(arr->items, n * arr->itemsz);
	arr->n = n;
	assert(arr->items);
}

void ainit(struct arr *arr, int n, int itemsz)
{
	memset(arr, 0, sizeof(*arr));
	arr->itemsz = itemsz;
	aresize(arr, n);
}

void *aat(struct arr *arr, int idx)
{
	if (idx < 0 || idx >= arr->i)
		return NULL;
	return &arr->items[idx * arr->itemsz];
}

int aidx(struct arr *arr, void *item)
{
	return ((char *)item - &arr->items[0]) / arr->itemsz;
}

void *abegin(struct arr *arr) { return aat(arr, 0); }

void *aend(struct arr *arr) { return aat(arr, arr->i - 1); }

void *apush(struct arr *arr)
{
	if (arr->i >= arr->n)
		aresize(arr, 2 * arr->n);
	arr->i++;
	void *rv = aend(arr);
	memset(rv, 0, arr->itemsz);
	return rv;
}

void *apop(struct arr *arr)
{
	void *rv = aend(arr);
	if (rv)
		arr->i--;
	return rv;
}

struct tag {
	const char *tag;
	const char *arg;
	const char *val;

	int ichildren;
	int ilastchild;
	int inext;
};

struct tag *tagpush(struct arr *arr)
{
	struct tag *tag = apush(arr);
	tag->ichildren = -1;
	tag->ilastchild = -1;
	tag->inext = -1;
	return tag;
}

void addchild(struct tag *parent, struct tag *child, struct arr *arr)
{
	int ichild = aidx(arr, child);

	// Add the only child
	if (parent->ichildren < 0) {
		assert(parent->ilastchild < 0);
		parent->ichildren = ichild;
		parent->ilastchild = ichild;
		return;
	}

	// Append to sibling list
	struct tag *sib = aat(arr, parent->ilastchild);
	assert(sib);
	parent->ilastchild = ichild;

	assert(sib->inext < 0);
	sib->inext = ichild;
}

void dfs(struct arr *arr, int idx, int indent)
{
	struct tag *tag = aat(arr, idx);
	if (!tag)
		return;

	dfs(arr, tag->ichildren, indent + 2);
	dfs(arr, tag->inext, indent);
}

void parsedom(char *data, long fsize, struct arr *tagstk, struct arr *tags)
{
	struct tag *tag;
	int *itag;
	char *ptr = &data[0];
	char *end = &data[fsize];
	char *tbegin;
	char *tend;
	char *space;
	char *skippush;

	while (ptr < end) {
		// Replace angle brackets surrounding tags with nulls to
		// create C-strings
		ptr = strchr(ptr, '<');
		if (!ptr)
			break;

		*ptr = '\0';
		tbegin = ptr + 1;
		assert((tbegin < end) && *tbegin);

		tend = strchr(tbegin, '>');
		assert(((tend - tbegin) >= 1) && (tend < end) && *tend);

		*tend = '\0';
		ptr = tend + 1;

		if (*tbegin == '/') {
			tbegin++;
			assert((tbegin < end) && *tbegin);

			itag = apop(tagstk);
			assert(itag);
			tag = aat(tags, *itag);

			if (strcmp(tag->tag, tbegin) != 0) {
				fprintf(stderr, "xml malformed\n");
				fprintf(stderr, "%s %s %s\n", tag->tag, tbegin,
					tag->val);
				exit(-EINVAL);
			}
		} else {
			// New tag we are adding to the array
			tag = tagpush(tags);
			assert(tag);

			tag->tag = tbegin;
			if (ptr < end)
				tag->val = ptr;

			// detects <tagname /> (a non-paired tag)
			skippush = strchr(tbegin, '/');

			// detect tag params <tagname params=42>
			space = strchr(tbegin, ' ');
			if (space) {
				*space = '\0';
				tag->arg = space + 1;
			}

			// Parent of new tag
			itag = aend(tagstk);
			assert(itag);

			addchild(aat(tags, *itag), tag, tags);

			if (!skippush) {
				itag = apush(tagstk);
				assert(itag);
				*itag = aidx(tags, tag);
			}
		}
	}
}

int main(int argc, char **argv)
{
	long rv, fsize;
	size_t nread;
	char *data;
	FILE *fp;

	if (argc != 2)
		fprintf(stderr, "Usage: fts <doc.xml>\n");

	// Read .xml into memory

	fp = fopen(argv[1], "r");
	assert(fp);

	rv = fseek(fp, 0, SEEK_END);
	assert(rv >= 0);

	fsize = ftell(fp);
	assert(fsize >= 0);

	rv = fseek(fp, 0, SEEK_SET);
	assert(rv >= 0);

	// Null terminate our file to make it a C-string
	fsize += 1;
	data = malloc(fsize);
	assert(data);
	data[fsize - 1] = '\0';

	nread = fread(data, 1, fsize - 1, fp);
	assert(nread == (size_t)(fsize - 1));

	// Parse the xml

	struct tag *tag;
	int *itag;

	struct arr tagstk;
	ainit(&tagstk, 8, sizeof(int));

	struct arr tags;
	ainit(&tags, 8, sizeof(struct tag));

	// Push a root tag so that we can maintain the invariant that all
	// tags that we parse have a parent.
	tag = tagpush(&tags);
	assert(tag);
	itag = apush(&tagstk);
	assert(itag);
	*itag = aidx(&tags, tag);

	parsedom(data, fsize, &tagstk, &tags);

	// Start at the first real tag (not the root)
	dfs(&tags, 1, 0);

	return 0;
}
