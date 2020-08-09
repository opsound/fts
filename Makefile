CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -g -MMD
CFLAGS += -fsanitize=undefined -fsanitize=address
CFLAGS += -O3
LDFLAGS += -lprofiler
LDFLAGS += -ltcmalloc
PROFILE_INPUT = enwiki-latest-abstract1.xml
PROFILE_OUTPUT = /tmp/prof.out

.PHONY: all
all: parse tokenize

parse: parse.c Makefile | format
	$(CC) $(CFLAGS) parse.c shared.c -o $@ $(LDFLAGS)

tokenize: tokenize.c Makefile | format
	$(CC) $(CFLAGS) tokenize.c shared.c -o $@ $(LDFLAGS)

-include parse.d tokenize.d shared.d

.PHONY: profile
profile: tokenize $(PROFILE_INPUT)
	HEAPPROFILE=$(PROFILE_OUTPUT) CPUPROFILE=$(PROFILE_OUTPUT) ./tokenize bin $(PROFILE_INPUT) > dump
	~/go/bin/pprof --pdf ./tokenize $(PROFILE_OUTPUT)

.PHONY: clean
clean:
	rm -rf tokenize parse *.d *.dSYM

.PHONY: format
format:
	fd -e c -e h | xargs clang-format -i
