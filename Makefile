CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -g
# CFLAGS += -fsanitize=undefined -fsanitize=address
CFLAGS += -O3
LDFLAGS += -lprofiler
SRCS = main.c

fts: $(SRCS) Makefile
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

.PHONY: profile
profile: fts
	CPUPROFILE=/tmp/prof.out ./fts enwiki-latest-abstract1.xml
	~/go/bin/pprof --pdf ./fts /tmp/prof.out

.PHONY: clean
clean:
	rm -f fts
