include ../../rules.mk

objs := math_emulate.o error.o convert.o ea.o get_put.o \
	add.o mul.o div.o compare.o

math.o: $(objs)
	$(LD) -r -o $@ $^

clean:
	@rm $(objs) math.o math.o
