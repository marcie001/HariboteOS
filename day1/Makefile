OBJS_BOOTPACK = build/bootpack.o build/hankaku.o build/mysprintf.o build/mystring.o build/nasmfunc.o build/dsctbl.o build/graphic.o build/int.o build/fifo.o build/keyboard.o build/mouse.o build/memory.o build/sheet.o build/timer.o build/mtask.o build/window.o build/console.o build/file.o
# デフォルト動作

default :
	make run

# ファイル生成規則

build/asmhead.bin : asmhead.asm Makefile
	nasm -l build/asmhead.lst -o build/asmhead.bin asmhead.asm

build/bootpack.bin: $(OBJS_BOOTPACK) Makefile
	ld -m elf_i386 -e HariMain -o build/bootpack.bin -Tos.ls -Map=build/bootpack.map $(OBJS_BOOTPACK)

build/hello.hrb: hello.asm Makefile
	nasm -l build/hello.lst -o build/hello.hrb hello.asm

build/hello2.hrb: hello2.asm Makefile
	nasm -l build/hello2.lst -o build/hello2.hrb hello2.asm

build/a.hrb: build/a.o build/a_nasm.o Makefile
	ld -m elf_i386 -e HariMain -o build/a.hrb -Tos.ls -Map=build/a.map build/a.o build/a_nasm.o

build/hello3.hrb: build/hello3.o build/a_nasm.o Makefile
	ld -m elf_i386 -e HariMain -o build/hello3.hrb -Tos.ls -Map=build/hello3.map build/hello3.o build/a_nasm.o

build/crack1.hrb: build/crack1.o build/a_nasm.o Makefile
	ld -m elf_i386 -e HariMain -o build/crack1.hrb -Tos.ls -Map=build/crack1.map build/crack1.o build/a_nasm.o

build/crack2.hrb: crack2.asm Makefile
	nasm -l build/crack2.lst -o build/crack2.hrb crack2.asm

build/os.sys: build/asmhead.bin build/bootpack.bin Makefile
	cat build/asmhead.bin build/bootpack.bin > build/os.sys

build/ipl10.bin : ipl10.asm Makefile
	nasm -l build/ipl10.lst -o build/ipl10.bin ipl10.asm

build/haribote.sys : haribote.asm Makefile
	nasm -l build/haribote.lst -o build/haribote.sys haribote.asm

build/hariboteos.img : build/ipl10.bin build/os.sys Makefile testfile build/hello.hrb build/hello2.hrb build/a.hrb build/hello3.hrb build/crack1.hrb build/crack2.hrb
	mformat -f 1440 -C -B build/ipl10.bin -i build/hariboteos.img ::
	mcopy -i build/hariboteos.img build/os.sys ::
	mcopy -i build/hariboteos.img Makefile ::
	mcopy -i build/hariboteos.img testfile ::
	mcopy -i build/hariboteos.img build/hello.hrb ::
	mcopy -i build/hariboteos.img build/hello2.hrb ::
	mcopy -i build/hariboteos.img build/a.hrb ::
	mcopy -i build/hariboteos.img build/hello3.hrb ::
	mcopy -i build/hariboteos.img build/crack1.hrb ::
	mcopy -i build/hariboteos.img build/crack2.hrb ::

# 一般規則

build/%.o: %.c Makefile
	gcc -Os -c -m32 -fno-pic -nostdlib -o build/$*.o $*.c

build/%.o: %.asm Makefile
	nasm -f elf32 -o build/$*.o $*.asm

# コマンド
asm :
	make -r build/ipl10.bin

img :
	make -r build/hariboteos.img

run :
	make img
	qemu-system-i386 -drive file=build/hariboteos.img,index=0,if=floppy,format=raw -m 32M

run-vb :
	make img
	virtualbox --startvm HariboteOS

debug :
	make img
	qemu-system-i386 -m 32 -localtime -vga std -drive file=build/hariboteos.img,index=0,if=floppy,format=raw -gdb tcp::10000 -S -monitor stdio

clean :
	-rm build/*.bin build/*.lst build/*.sys

src_only :
	-rm -r build
	mkdir build
