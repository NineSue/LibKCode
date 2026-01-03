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

# LibKCode 项目 Makefile
# 用法:
#   make            - 编译所有（内核模块 + 用户库 + 测试）
#   make lib        - 只编译用户库
#   make kernel     - 只编译内核模块
#   make tests      - 编译所有测试
#   make test       - 编译并运行所有测试
#   make bench      - 运行性能对标测试 (vs C++ STL)
#   make clean      - 清理所有
#   make load       - 加载内核模块 (需要 sudo)
#   make unload     - 卸载内核模块 (需要 sudo)

.PHONY: all lib kernel tests test bench clean load unload

all: kernel lib tests

# 编译用户库
lib:
	$(MAKE) -C lib

# 编译内核模块
kernel:
	$(MAKE) -C kernel

# 编译所有测试（内核测试 + 用户库测试）
tests: lib
	$(MAKE) -C tests/lib
	gcc -Wall -I kernel tests/kernel/test_kcode.c -o tests/kernel/test_kcode

# 运行所有测试
test: tests
	@echo ""
	@echo "=== 内核模块测试 ==="
	tests/kernel/test_kcode
	@echo ""
	@echo "=== 用户库测试 ==="
	$(MAKE) -C tests/lib test

# 性能对标测试
bench: lib
	$(MAKE) -C tests/lib bench

# 清理
clean:
	$(MAKE) -C lib clean
	$(MAKE) -C kernel clean
	$(MAKE) -C tests/lib clean
	rm -f tests/kernel/test_kcode

# 加载内核模块
load:
	sudo insmod kernel/kcode.ko
	@dmesg | grep kcode | tail -15

# 卸载内核模块
unload:
	sudo rmmod kcode
	@dmesg | grep kcode | tail -3
