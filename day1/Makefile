# デフォルト動作

default :
	make img

# ファイル生成規則

build/ipl.bin : ipl.nas Makefile
	nasm -l build/ipl.lst -o build/ipl.bin ipl.nas

build/helloos.img : build/ipl.bin Makefile
	@if [ ! -f build/helloos.img ]; then\
		fallocate -l 1474560 build/helloos.img;\
		printf "000200: f0ffff" | xxd -r - build/helloos.img;\
		printf "001400: f0ffff" | xxd -r - build/helloos.img;\
	fi
	dd if=build/ipl.bin of=build/helloos.img bs=512 seek=0 count=512 conv=notrunc

# コマンド
asm :
	make -r build/ipl.bin

img :
	make -r build/helloos.img

run :
	make img
	qemu-system-i386 build/helloos.img

clean :
	rm -f build/ipl.bin build/ipl.lst

src_only :
	make clean
	rm -f build/helloos.img
