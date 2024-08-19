include ../rules.mk

objs := memory.o swap.o page.o


all: mm.o

mm.o: $(objs)
	$(LD) -r -o $@ $^


clean:
	@rm $(objs) mm.o
