CC = gcc
CFLAGS = -Wall
INCLUDES = -I../../lib/include -I../../kernel
LDFLAGS = -L../../lib -lkcode -lpthread -Wl,-rpath,../../lib

TARGETS = test_rbtree test_sort

all: $(TARGETS)

test_rbtree: test_rbtree.c
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LDFLAGS) -o $@

test_sort: test_sort.c
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LDFLAGS) -o $@

test: $(TARGETS)
	./test_rbtree
	./test_sort

clean:
	rm -f $(TARGETS)

.PHONY: all test clean
