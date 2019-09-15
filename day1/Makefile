# デフォルト動作

default :
	make img

# ファイル生成規則

build/ipl10.bin : ipl10.nas Makefile
	nasm -l build/ipl10.lst -o build/ipl10.bin ipl10.nas

build/haribote.sys : haribote.nas Makefile
	nasm -l build/haribote.lst -o build/haribote.sys haribote.nas

build/hariboteos.img : build/ipl10.bin build/haribote.sys Makefile
	mformat -f 1440 -C -B build/ipl10.bin -i build/hariboteos.img ::
	mcopy -i build/hariboteos.img build/haribote.sys ::

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
