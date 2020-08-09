#include "shared.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: parse <doc.xml>\n");
		return -EINVAL;
	}

	FILE *fp = fopen(argv[1], "r");
	size_t nread;
	struct tokbin tokbin;
	struct arr buf;

	ainit(&buf, 8, sizeof(char));
	freopen(NULL, "rb", stdin);

	nread = fread(&tokbin, sizeof(tokbin), 1, stdin);

	while (nread > 0) {
		fseek(fp, tokbin.fpos, SEEK_SET);
		apushn(&buf, tokbin.len + 1);
		nread = fread(abegin(&buf), 1, tokbin.len, fp);
		if (nread != (size_t)tokbin.len) {
			fprintf(stderr,
				"Error: nread=%zu from token: fpos=%lld tok=%d "
				"len=%d\n",
				nread, tokbin.fpos, tokbin.tok, tokbin.len);
			break;
		}
		*(char *)aat(&buf, tokbin.len) = '\0';
		printf("%s\n", (char *)abegin(&buf));
		apopn(&buf, tokbin.len + 1);

		nread = fread(&tokbin, sizeof(tokbin), 1, stdin);
	}

	return 0;
}
