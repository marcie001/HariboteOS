# デフォルト動作

default :
	make img

# ファイル生成規則

build/ipl.bin : ipl.nas Makefile
	nasm -l build/ipl.lst -o build/ipl.bin ipl.nas

build/tail.bin : tail.nas Makefile
	nasm -o build/tail.bin tail.nas

build/helloos.img : build/ipl.bin build/tail.bin Makefile
	cat build/ipl.bin build/tail.bin > build/helloos.img

# コマンド
asm :
	make -r build/ipl.bin

img :
	make -r build/helloos.img

run :
	make img
	qemu-system-i386 build/helloos.img

clean :
	-rm build/ipl.bin build/ipl.lst build/tail.bin

src_only :
	make clean
	-rm build/helloos.img
