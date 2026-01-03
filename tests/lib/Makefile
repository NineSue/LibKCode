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
CXX = g++
CFLAGS = -Wall -O2
CXXFLAGS = -Wall -O3
INCLUDES = -I../../lib/include -I../../lib/internal -I../../kernel
LDFLAGS = -L../../lib -lkcode -lpthread -Wl,-rpath,../../lib

TARGETS = test_rbtree test_sort test_rbtree_stress test_rbtree_advanced
BENCH = benchmark

all: $(TARGETS)

test_rbtree: test_rbtree.c
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LDFLAGS) -o $@

test_sort: test_sort.c
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LDFLAGS) -o $@

test_rbtree_stress: test_rbtree_stress.c rbtree_checker.h
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LDFLAGS) -o $@

test_rbtree_advanced: test_rbtree_advanced.c rbtree_checker.h
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LDFLAGS) -o $@

# 性能对标测试 (C++)
$(BENCH): benchmark.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LDFLAGS) -o $@

bench: $(BENCH)
	@echo "=== Performance Benchmark ==="
	./$(BENCH) 1000000

test: $(TARGETS)
	@echo "=== Running basic tests ==="
	./test_rbtree
	./test_sort
	@echo ""
	@echo "=== Running advanced rbtree tests ==="
	./test_rbtree_advanced
	@echo ""
	@echo "=== Running stress test ==="
	./test_rbtree_stress

clean:
	rm -f $(TARGETS) $(BENCH)

.PHONY: all test bench clean
