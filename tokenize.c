#include "shared.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void write_token(int64_t fpos, int token, const char *begin, const char *end,
		 void *arg)
{
	FILE *fp = arg;
	struct tokbin tokbin = {
	    .fpos = fpos,
	    .tok = token,
	    .len = end - begin,
	};

	fwrite(&tokbin, sizeof(tokbin), 1, fp);
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
			emit(fpos + ptr - begin, TOK_TEXT, ptr, tbegin, arg);

		ptr = tbegin;

		tend = strchr2(tbegin, end, '>');
		if (tend == end)
			break;
		assert(tend > (tbegin + 1) && "Illegal tag: <>");

		if (tbegin[1] == '/') {
			emit(fpos + &tbegin[2] - begin, TOK_TAG_END, &tbegin[2],
			     tend, arg);
		} else {
			space = strchr2(&tbegin[1], tend, ' ');
			if (space < tend) {
				slash = strchr2(space, tend, '/');
				if (slash < tend)
					emit(fpos + &tbegin[1] - begin,
					     TOK_TAG_BEGIN | TOK_TAG_END,
					     &tbegin[1], space, arg);
				else
					emit(fpos + &tbegin[1] - begin,
					     TOK_TAG_BEGIN, &tbegin[1], space,
					     arg);
			} else {
				emit(fpos + &tbegin[1] - begin, TOK_TAG_BEGIN,
				     &tbegin[1], tend, arg);
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

	fpos = ftell(fp);
	dst = apushn(&buf, toread);
	nread = fread(dst, 1, toread, fp);
	apopn(&buf, toread - nread);

	while (nread > 0) {
		ptr = tokenize(fpos, abegin(&buf), aend(&buf), emit, arg);
		tokenized = ptr - (char *)abegin(&buf);
		untokenized = (char *)aend(&buf) - ptr;
		memmove(abegin(&buf), ptr, untokenized);
		apopn(&buf, tokenized);

		if (tokenized == 0)
			toread = 2 * acount(&buf);
		else
			toread = tokenized;

		fpos = ftell(fp);
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
		fprintf(stderr, "Usage: fts <dom,bin,nothing> <doc.xml>\n");

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
	} else if (strcmp(mode, "nothing") == 0) {
		process_file(fp, donothing, NULL);
	}

	return 0;
}
