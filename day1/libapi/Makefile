OBJS_API = build/api001.o build/api001.o build/api002.o build/api003.o build/api004.o build/api005.o build/api006.o build/api007.o build/api008.o build/api009.o build/api010.o build/api011.o build/api012.o build/api013.o build/api014.o build/api015.o build/api016.o build/api017.o build/api018.o build/api019.o build/api020.o

default :
	make build/libapi.a

build/ :
	mkdir build

build/%.o: %.asm Makefile build/
	nasm -f elf32 -o build/$*.o $*.asm

build/libapi.a : Makefile build/ $(OBJS_API)
	ar rcs build/libapi.a $(OBJS_API)

clean :
	-rm *.o

src_only :
	-rm -r build
