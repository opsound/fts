CFLAGS += -std=c11 -Wall -Wextra 
# CFLAGS += -fsanitize=undefined -fsanitize=address
CFLAGS += -O3

fts: main.c
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -f fts
