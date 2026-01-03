# Copyright (C) 2026 E Zuan, Liu Jiayou, Xia Yefei (Team Xinjian Wenjianjia)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

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
