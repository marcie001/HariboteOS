# デフォルト動作

default :
	make build/haribote.img

# ファイル生成規則

build/ :
	mkdir build

build/hariboteos.img : build/ haribote/build/ipl10.bin haribote/build/os.sys \
		Makefile utf8file sjisfile eucfile nihongo/nihongo.fnt \
		a/build/a.hrb hello3/build/hello3.hrb hello4/build/hello4.hrb winhelo/build/winhelo.hrb \
		star1/build/star1.hrb lines/build/lines.hrb walk/build/walk.hrb noodle/build/noodle.hrb \
		beepdown/build/beepdown.hrb color/build/color.hrb sosu/build/sosu.hrb cat/build/cat.hrb \
        head/build/head.hrb hexdump/build/hexdump.hrb iroha/build/iroha.hrb chklang/build/chklang.hrb \
        notrec/build/notrec.hrb bball/build/bball.hrb invader/build/invader.hrb

	mformat -f 1440 -C -B haribote/build/ipl10.bin -i build/hariboteos.img ::
	mcopy -i build/hariboteos.img haribote/build/os.sys ::
	mcopy -i build/hariboteos.img Makefile ::
	mcopy -i build/hariboteos.img utf8file ::
	mcopy -i build/hariboteos.img sjisfile ::
	mcopy -i build/hariboteos.img eucfile ::
	mcopy -i build/hariboteos.img a/build/a.hrb ::
	mcopy -i build/hariboteos.img hello3/build/hello3.hrb ::
	mcopy -i build/hariboteos.img hello4/build/hello4.hrb ::
	mcopy -i build/hariboteos.img winhelo/build/winhelo.hrb ::
	mcopy -i build/hariboteos.img star1/build/star1.hrb ::
	mcopy -i build/hariboteos.img lines/build/lines.hrb ::
	mcopy -i build/hariboteos.img walk/build/walk.hrb ::
	mcopy -i build/hariboteos.img noodle/build/noodle.hrb ::
	mcopy -i build/hariboteos.img beepdown/build/beepdown.hrb ::
	mcopy -i build/hariboteos.img color/build/color.hrb ::
	mcopy -i build/hariboteos.img sosu/build/sosu.hrb ::
	mcopy -i build/hariboteos.img cat/build/cat.hrb ::
	mcopy -i build/hariboteos.img head/build/head.hrb ::
	mcopy -i build/hariboteos.img iroha/build/iroha.hrb ::
	mcopy -i build/hariboteos.img nihongo/nihongo.fnt ::
	mcopy -i build/hariboteos.img hexdump/build/hexdump.hrb ::
	mcopy -i build/hariboteos.img chklang/build/chklang.hrb ::
	mcopy -i build/hariboteos.img notrec/build/notrec.hrb ::
	mcopy -i build/hariboteos.img bball/build/bball.hrb ::
	mcopy -i build/hariboteos.img invader/build/invader.hrb ::

# コマンド
run :
	make build/hariboteos.img
	qemu-system-i386 -drive file=build/hariboteos.img,index=0,if=floppy,format=raw -m 32M

debug :
	make build/hariboteos.img
	qemu-system-i386 -m 32 -localtime -vga std -drive file=build/hariboteos.img,index=0,if=floppy,format=raw -gdb tcp::10000 -S -monitor stdio

full :
	make -C libcommon
	make -C haribote
	make -C libapi
	make -C a
	make -C hello3
	make -C hello4
	make -C winhelo
	make -C star1
	make -C lines
	make -C walk
	make -C noodle
	make -C beepdown
	make -C color
	make -C sosu
	make -C cat
	make -C head
	make -C hexdump
	make -C iroha
	make -C chklang
	make -C notrec
	make -C bball
	make -C invader
	make build/hariboteos.img

run_full :
	make full
	make run

run_full_vb :
	make full
	virtualbox --startvm HariboteOS

clean :
# do nothing

src_only :
	-rm -r build
	mkdir build

clean_full :
	make -C libcommon clean
	make -C haribote clean
	make -C libapi clean
	make -C a clean
	make -C hello3 clean
	make -C hello4 clean
	make -C winhelo clean
	make -C star1 clean
	make -C lines clean
	make -C walk clean
	make -C noodle clean
	make -C beepdown clean
	make -C color clean
	make -C sosu clean
	make -C cat clean
	make -C head clean
	make -C hexdump clean
	make -C iroha clean
	make -C chklang clean
	make -C notrec clean
	make -C bball clean
	make -C invader clean

src_only_full :
	make -C libcommon src_only
	make -C haribote src_only
	make -C libapi src_only
	make -C a src_only
	make -C hello3 src_only
	make -C hello4 src_only
	make -C winhelo src_only
	make -C star1 src_only
	make -C lines src_only
	make -C walk src_only
	make -C noodle src_only
	make -C beepdown src_only
	make -C color src_only
	make -C sosu src_only
	make -C cat src_only
	make -C head src_only
	make -C hexdump src_only
	make -C iroha src_only
	make -C chklang src_only
	make -C notrec src_only
	make -C bball src_only
	make -C invader src_only
