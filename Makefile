obj-m += hash_table.o

KERNEL_DIR := /usr/src/kernels/4.8.6-300.fc25.x86_64/

PWD :=$(shell pwd)

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

