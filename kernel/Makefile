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
