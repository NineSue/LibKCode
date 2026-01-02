CC = gcc
CFLAGS = -Wall -fPIC -O2
INCLUDES = -Iinclude -Iinternal -I../kernel

SOURCES = src/kcode_init.c src/kcode_rbtree.c src/kcode_sort.c
TARGET = libkcode.so

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -shared $(INCLUDES) $^ -lpthread -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean
