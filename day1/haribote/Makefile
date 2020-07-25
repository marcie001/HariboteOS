OBJS_BOOTPACK = build/bootpack.o build/hankaku.o build/nasmfunc.o build/dsctbl.o build/graphic.o build/int.o build/fifo.o build/keyboard.o build/mouse.o build/memory.o build/sheet.o build/timer.o build/mtask.o build/window.o build/console.o build/file.o build/tek.o
LIBCOMMON_PATH = ../libcommon/
LIBCOMMON_BUILD_PATH = $(LIBCOMMON_PATH)build/
# デフォルト動作

default :
	make build/ipl10.bin
	make build/os.sys

# ファイル生成規則

build/ :
	mkdir build

build/asmhead.bin : asmhead.asm Makefile build/
	nasm -l build/asmhead.lst -o build/asmhead.bin asmhead.asm

build/bootpack.bin: $(OBJS_BOOTPACK) Makefile build/ $(LIBCOMMON_BUILD_PATH)libcommon.a
	ld -m elf_i386 -e HariMain -o build/bootpack.bin -Tos.ls -Map=build/bootpack.map $(OBJS_BOOTPACK) $(LIBCOMMON_BUILD_PATH)libcommon.a

build/os.sys: build/asmhead.bin build/bootpack.bin Makefile
	cat build/asmhead.bin build/bootpack.bin > build/os.sys

build/ipl10.bin : ipl10.asm Makefile build/
	nasm -l build/ipl10.lst -o build/ipl10.bin ipl10.asm

build/hariboteos.img : build/ipl10.bin build/os.sys Makefile
	mformat -f 1440 -C -B build/ipl10.bin -i build/hariboteos.img ::
	mcopy -i build/hariboteos.img build/os.sys ::
	mcopy -i build/hariboteos.img Makefile ::

# 一般規則

build/%.o: %.c bootpack.h ../libcommon.h Makefile build/
	gcc -O0 -c -m32 -fno-pic -nostdlib -iquote .. -o build/$*.o $*.c

build/%.o: %.asm Makefile build/
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
