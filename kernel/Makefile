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

# 内核模块名
obj-m += kcode.o
#obj-m += stub.o

# 当前内核头目录
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# 构建模块
all:
	make -C $(KDIR) M=$(PWD) modules

# 清理生成文件
clean:
	-mv compile_commands.json /tmp/ 2>/dev/null
	make -C $(KDIR) M=$(PWD) clean
	-mv /tmp/compile_commands.json . 2>/dev/null

# 安装模块
install:
	sudo insmod kcode.ko

# 卸载模块
uninstall:
	-@sudo rmmod kcode || true

# 测试模块：卸载旧模块 -> 安装模块 -> 看日志 -> 卸载 -> 看日志 -> 清理
test: all
	-@sudo rmmod kcode 2>/dev/null || true   # 卸载旧模块
	sudo insmod kcode.ko                     # 加载模块
	@echo "\n\n=== dmesg logs after insmod ==="
	@sudo dmesg -T | tail -n 20
	sudo rmmod kcode                          # 卸载模块
	@echo "\n\n=== dmesg logs after rmmod ==="
	@sudo dmesg -T | tail -n 20
	make clean
