CC = gcc
CFLAGS = -Wall
INCLUDES = -I../../lib/include -I../../kernel
LDFLAGS = -L../../lib -lkcode -lpthread -Wl,-rpath,../../lib

TARGET = test_rbtree

all: $(TARGET)

$(TARGET): test_rbtree.c
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LDFLAGS) -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all test clean
