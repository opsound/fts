CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -g
# CFLAGS += -fsanitize=undefined -fsanitize=address
CFLAGS += -O3
LDFLAGS += -lprofiler
SRCS = main.c

fts: $(SRCS) Makefile
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f fts
