CFLAGS := -DCONFIG_PATCH -I$(SRC_ROOT)/include -fno-builtin -fno-stack-protector \
		  -fomit-frame-pointer \
		  -ffreestanding -nostdinc \
		  -fno-asynchronous-unwind-tables \
		  -fno-pic -fno-dwarf2-cfi-asm -Wall -Werror -m32 \
		  -Wno-unused-function -Wno-unused-variable -Wno-parentheses \
		  -Wno-unused-but-set-variable -g

LD := ld -m elf_i386


%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

%.o: %.s
	$(AS) -o $@ --32 $^

%.s: %.c
	$(CC) $(CFLAGS) -o $@ -S $^

