include ../rules.mk

objs := string.o ctype.o open.o errno.o _exit.o wait.o

lib.o: $(objs)
	$(LD) -r -o lib.o $(objs)

clean:
	rm $(objs) lib.o
