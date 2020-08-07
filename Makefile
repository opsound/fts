CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -g
CFLAGS += -fsanitize=undefined -fsanitize=address
CFLAGS += -O3
# LDFLAGS += -lprofiler
LDFLAGS += -ltcmalloc
SRCS = main.c
PROFILE_INPUT = enwiki-20200720-abstract.xml

fts: $(SRCS) Makefile
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

.PHONY: profile
profile: fts $(PROFILE_INPUT)
	CPUPROFILE=/tmp/prof.out ./fts $(PROFILE_INPUT)
	~/go/bin/pprof --pdf ./fts /tmp/prof.out

.PHONY: clean
clean:
	rm -f fts
