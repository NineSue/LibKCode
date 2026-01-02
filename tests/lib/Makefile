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
