include ../rules.mk

objs := _exit.o close.o ctype.o dup.o errno.o execve.o \
		malloc.o open.o setsid.o string.o wait.o write.o

lib.o: $(objs)
	$(LD) -r -o lib.o $(objs)

clean:
	rm $(objs) lib.o
