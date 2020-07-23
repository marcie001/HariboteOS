OBJS = build/mystring.o build/mysprintf.o

default :
	make build/libcommon.a

build/ :
	mkdir build

build/%.o: %.c Makefile build/
	gcc -O0 -c -m32 -fno-pic -nostdlib -o build/$*.o $*.c

build/libcommon.a : Makefile $(OBJS) build/
	ar rcs build/libcommon.a $(OBJS)

clean :
	-rm *.o

src_only :
	-rm -r build
