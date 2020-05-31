OBJS_BOOTPACK = build/bootpack.o build/hankaku.o build/mysprintf.o build/mystring.o build/nasmfunc.o build/dsctbl.o build/graphic.o build/int.o build/fifo.o build/keyboard.o build/mouse.o build/memory.o build/sheet.o build/timer.o build/mtask.o build/window.o build/console.o build/file.o
# デフォルト動作

default :
	make run

# ファイル生成規則

build/asmhead.bin : asmhead.asm Makefile
	nasm -l build/asmhead.lst -o build/asmhead.bin asmhead.asm

build/bootpack.bin: $(OBJS_BOOTPACK) Makefile
	ld -m elf_i386 -e HariMain -o build/bootpack.bin -Tos.ls -Map=build/bootpack.map $(OBJS_BOOTPACK)

build/hello.hrb: build/hello.o Makefile api.ls
	ld -m elf_i386 -e HariMain -o build/hello.hrb -Tapi.ls -Map=build/hello.map build/hello.o

build/hello2.hrb: build/hello2.o Makefile api.ls
	ld -m elf_i386 -e HariMain -o build/hello2.hrb -Tapi.ls -Map=build/hello2.map build/hello2.o

build/a.hrb: build/a.o build/a_nasm.o Makefile api.ls
	ld -m elf_i386 -e HariMain -o build/a.hrb -Tapi.ls -Map=build/a.map build/a.o build/a_nasm.o

build/hello3.hrb: build/hello3.o build/a_nasm.o Makefile api.ls
	ld -m elf_i386 -e HariMain -o build/hello3.hrb -Tapi.ls -Map=build/hello3.map build/hello3.o build/a_nasm.o

build/hello4.hrb: build/hello4.o build/a_nasm.o Makefile api.ls
	ld -m elf_i386 -e HariMain -o build/hello4.hrb -Tapi.ls -Map=build/hello4.map build/hello4.o build/a_nasm.o

build/winhelo.hrb: build/winhelo.o build/a_nasm.o Makefile api.ls
	ld -m elf_i386 -e HariMain -o build/winhelo.hrb -Tapi.ls -Map=build/winhelo.map build/winhelo.o build/a_nasm.o

build/star1.hrb: build/star1.o build/a_nasm.o Makefile api.ls
	ld -m elf_i386 -e HariMain -o build/star1.hrb -Tapi.ls -Map=build/star1.map build/star1.o build/a_nasm.o

build/lines.hrb: build/lines.o build/a_nasm.o Makefile api.ls
	ld -m elf_i386 -e HariMain -o build/lines.hrb -Tapi.ls -Map=build/lines.map build/lines.o build/a_nasm.o

build/crack1.hrb: build/crack1.o build/a_nasm.o Makefile api.ls
	ld -m elf_i386 -e HariMain -o build/crack1.hrb -Tapi.ls -Map=build/crack1.map build/crack1.o build/a_nasm.o

build/crack2.hrb: crack2.asm Makefile
	nasm -l build/crack2.lst -o build/crack2.hrb crack2.asm

build/crack3.hrb: crack3.asm Makefile
	nasm -l build/crack3.lst -o build/crack3.hrb crack3.asm

build/crack4.hrb: crack4.asm Makefile
	nasm -l build/crack4.lst -o build/crack4.hrb crack4.asm

build/crack5.hrb: crack5.asm Makefile
	nasm -l build/crack5.lst -o build/crack5.hrb crack5.asm

build/bug1.hrb: build/bug1.o build/a_nasm.o Makefile api.ls
	ld -m elf_i386 -e HariMain -o build/bug1.hrb -Tapi.ls -Map=build/bug1.map build/bug1.o build/a_nasm.o

build/bug2.hrb: build/bug2.o build/a_nasm.o Makefile api.ls
	ld -m elf_i386 -e HariMain -o build/bug2.hrb -Tapi.ls -Map=build/bug2.map build/bug2.o build/a_nasm.o

build/os.sys: build/asmhead.bin build/bootpack.bin Makefile
	cat build/asmhead.bin build/bootpack.bin > build/os.sys

build/ipl10.bin : ipl10.asm Makefile
	nasm -l build/ipl10.lst -o build/ipl10.bin ipl10.asm

build/haribote.sys : haribote.asm Makefile
	nasm -l build/haribote.lst -o build/haribote.sys haribote.asm

build/hariboteos.img : build/ipl10.bin build/os.sys Makefile testfile build/hello.hrb build/hello2.hrb build/a.hrb build/hello3.hrb build/hello4.hrb build/winhelo.hrb build/star1.hrb build/lines.hrb build/crack1.hrb build/crack2.hrb build/crack3.hrb build/crack4.hrb build/crack5.hrb build/bug1.hrb build/bug2.hrb
	mformat -f 1440 -C -B build/ipl10.bin -i build/hariboteos.img ::
	mcopy -i build/hariboteos.img build/os.sys ::
	mcopy -i build/hariboteos.img Makefile ::
	mcopy -i build/hariboteos.img testfile ::
	mcopy -i build/hariboteos.img build/hello.hrb ::
	mcopy -i build/hariboteos.img build/hello2.hrb ::
	mcopy -i build/hariboteos.img build/a.hrb ::
	mcopy -i build/hariboteos.img build/hello3.hrb ::
	mcopy -i build/hariboteos.img build/hello4.hrb ::
	mcopy -i build/hariboteos.img build/winhelo.hrb ::
	mcopy -i build/hariboteos.img build/star1.hrb ::
	mcopy -i build/hariboteos.img build/lines.hrb ::
	mcopy -i build/hariboteos.img build/crack1.hrb ::
	mcopy -i build/hariboteos.img build/crack2.hrb ::
	mcopy -i build/hariboteos.img build/crack3.hrb ::
	mcopy -i build/hariboteos.img build/crack4.hrb ::
	mcopy -i build/hariboteos.img build/crack5.hrb ::
	mcopy -i build/hariboteos.img build/bug1.hrb ::
	mcopy -i build/hariboteos.img build/bug2.hrb ::

# 一般規則

build/%.o: %.c Makefile
	gcc -O0 -c -m32 -fno-pic -nostdlib -o build/$*.o $*.c

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
