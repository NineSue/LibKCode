CC = gcc
CFLAGS = -Wall -g -I../../kernel

test_jit_demo: test_jit_demo.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f test_jit_demo

.PHONY: clean
