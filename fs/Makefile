include ../rules.mk

objs := buffer.o super.o inode.o bitmap.o namei.o \
		fcntl.o truncate.o file_table.o open.o read_write.o \
		char_dev.o pipe.o block_dev.o file_dev.o exec.o \
		stat.o ioctl.o select.o


all: fs.o

fs.o: $(objs)
	$(LD) -r -o $@ $^

clean:
	@rm $(objs) fs.o

