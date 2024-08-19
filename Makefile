SRC_ROOT := $(shell pwd)

export SRC_ROOT

include ./rules.mk

deps := mm/mm.o lib/lib.o init/main.o init/head.o kernel/kernel.o init/patch.o kernel/chr_drv/chr_drv.o \
		kernel/blk_drv/blk_drv.o fs/fs.o kernel/math/math.o


all: tools/system.bin

.PHONY: tools/system.bin qemu

QEMU := qemu-system-i386
QEMU_FLAGS = -cpu 'SandyBridge' -echr 0x14 \
		-fda floppy.img -boot a \
		-device loader,file=tools/system.bin,addr=0x100000,force-raw=on \
		-device loader,file=disk.img,addr=0x800000,force-raw=on \
		-m 64 -display curses


qemu: tools/system.bin
	$(QEMU) $(QEMU_FLAGS)

qemu-gdb: tools/system.bin
	$(QEMU) $(QEMU_FLAGS) -S -s

tools/system.bin:
	make -C tools/link
	make -C boot
	make -C kernel
	make -C mm
	make -C lib
	make -C init
	make -C kernel/chr_drv
	make -C kernel/blk_drv
	make -C kernel/math
	make -C fs

	$(LD) -T kernel.lds -o tools/system.elf $(deps)
	tools/link/link kernel.json	


clean:
	make -C kernel clean
	make -C mm clean
	make -C lib clean
	make -C init clean
	make -C kernel/chr_drv clean
	make -C kernel/blk_drv clean
	make -C kernel/math clean
	make -C fs clean
