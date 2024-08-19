Image: setup bootsect jump.bin ../tools/build/build ../tools/link/link
	../tools/build/build bootsect setup jump.bin > $@
	dd if=$@ of=../floppy.img conv=notrunc

.PHONY: Image


../tools/build/build:
	make -C ../tools/build

../tools/link/link:
	make -C ../tools/link

setup: setup.s
	as86 -0 -a -o setup.o $^
	ld86 -0 -s -o $@ setup.o

bootsect: bootsect.s
	as86 -0 -a -o bootsect.o $^
	ld86 -0 -s -o $@ bootsect.o

jump.bin: jump.o
	ld -m elf_i386 -Ttext 0 -e _start -o jump.elf $^
	../tools/link/link jump.json

jump.o: jump.s
	as -o $@ --32 $^


clean:
	@rm bootsect.o build Image jump.elf jump.o setup.o setup bootsect
