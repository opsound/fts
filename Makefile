CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -g
CFLAGS += -fsanitize=undefined -fsanitize=address
CFLAGS += -O3
LDFLAGS += -lprofiler
LDFLAGS += -ltcmalloc
PROFILE_INPUT = enwiki-latest-abstract1.xml
PROFILE_OUTPUT = /tmp/prof.out

.PHONY: all
all: parse tokenize

parse: parse.c Makefile | format
	$(CC) $(CFLAGS) parse.c -o $@ $(LDFLAGS)

tokenize: tokenize.c Makefile | format
	$(CC) $(CFLAGS) tokenize.c -o $@ $(LDFLAGS)

.PHONY: profile
profile: tokenize $(PROFILE_INPUT)
	HEAPPROFILE=$(PROFILE_OUTPUT) CPUPROFILE=$(PROFILE_OUTPUT) ./tokenize bin $(PROFILE_INPUT) > dump
	~/go/bin/pprof --pdf ./tokenize $(PROFILE_OUTPUT)

.PHONY: clean
clean:
	rm -f tokenize

.PHONY: format
format:
	fd -e c -e h | xargs clang-format -i
