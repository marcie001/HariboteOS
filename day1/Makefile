# デフォルト動作

default :
	make img

# ファイル生成規則

build/asmhead.bin : asmhead.asm
	nasm -l build/asmhead.lst -o build/asmhead.bin asmhead.asm

build/bootpack.o : bootpack.c
	gcc -c -m32 -fno-pic -o build/bootpack.o bootpack.c

build/nasmfunc.o: nasmfunc.asm
	nasm -f elf32 -o build/nasmfunc.o nasmfunc.asm

build/bootpack.bin: build/bootpack.o build/nasmfunc.o
	ld -m elf_i386 -e HariMain -o build/bootpack.bin -Tos.ls build/bootpack.o build/nasmfunc.o

build/os.sys: build/asmhead.bin build/bootpack.bin
	cat build/asmhead.bin build/bootpack.bin > build/os.sys

build/ipl10.bin : ipl10.asm Makefile
	nasm -l build/ipl10.lst -o build/ipl10.bin ipl10.asm

build/haribote.sys : haribote.asm Makefile
	nasm -l build/haribote.lst -o build/haribote.sys haribote.asm

build/hariboteos.img : build/ipl10.bin build/os.sys Makefile
	mformat -f 1440 -C -B build/ipl10.bin -i build/hariboteos.img ::
	mcopy -i build/hariboteos.img build/os.sys ::

# コマンド
asm :
	make -r build/ipl10.bin

img :
	make -r build/hariboteos.img

run :
	make img
	qemu-system-i386 -drive file=build/hariboteos.img,index=0,if=floppy,format=raw

clean :
	-rm build/*.bin build/*.lst build/*.sys

src_only :
	-rm -r build
	mkdir build
