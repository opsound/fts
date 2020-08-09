CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -g
CFLAGS += -fsanitize=undefined -fsanitize=address
CFLAGS += -O3
LDFLAGS += -lprofiler
LDFLAGS += -ltcmalloc
SRCS = main.c
PROFILE_INPUT = enwiki-latest-abstract1.xml
PROFILE_OUTPUT = /tmp/prof.out

fts: $(SRCS) Makefile | format
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

.PHONY: profile
profile: fts $(PROFILE_INPUT)
	HEAPPROFILE=$(PROFILE_OUTPUT) CPUPROFILE=$(PROFILE_OUTPUT) ./fts bin $(PROFILE_INPUT) > dump
	~/go/bin/pprof --pdf ./fts $(PROFILE_OUTPUT)

.PHONY: clean
clean:
	rm -rf fts *.dSYM

.PHONY: format
format:
	fd -e c -e h | xargs clang-format -i
