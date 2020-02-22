; haribote-os
; TAB=4

BOTPAK  EQU 0x00280000 ; bootpackのロード先
DSKCAC  EQU 0x00100000 ; ディスクキャッシュの場所
DSKCAC0 EQU 0x00008000 ; ディスクキャッシュの場所（リアルモード）

; BOOT_INFO 関係
CYLS    EQU 0x0ff0
LEDS    EQU 0x0ff1
VMODE   EQU 0x0ff2 ; 色数に関する情報。何ビットカラーか。
SCRNX   EQU 0x0ff4 ; 解像度のX
SCRNY   EQU 0x0ff6 ; 解像度のX
VRAM    EQU 0x0ff8 ; グラフィックバッファの開始番地

    ORG 0xc200 ; このプログラムがどこに読み込まれるか

    MOV AL,0x13
    MOV AH,0x00
    INT 0x10
    MOV BYTE [VMODE],8 ; 画面モードをメモする
    MOV WORD [SCRNX],320
    MOV WORD [SCRNY],200
    MOV DWORD [VRAM],0x000a0000

; キーボードの LED 状態を BIOS に教えてもらう
    MOV AH,0x02
    INT 0x16 ; keyboard BIOS
    MOV [LEDS],AL

; PIC が一切の割り込みを受け付けないようにする
; AT 互換機の仕様では、 PIC の初期化をするなら、
; これを CLI 前にやっておかないと、たまにハングアップする
; PIC の初期化はあとでやる
    MOV AL,0xff
    OUT 0x21,AL
    NOP ; OUT 命令を連続させるとうまくいかない機種があるらしいので
    OUT 0xa1,AL

    CLI ; さらに CPU レベルでも割り込みを禁止

; CPU から 1MB 以上のメモリにアクセスできるように A20GATE を設定
    CALL    waitkbdout
    MOV AL,0xd1
    OUT 0x64,AL
    CALL    waitkbdout
    MOV AL,0xdf ; enable A20
    OUT 0x60,AL
    CALL    waitkbdout

; プロテクトモード移行
    LGDT    [GDTR0] ; 暫定 GDT を設定
    MOV EAX,CR0
    AND EAX,0x7fffffff ; bit31を0にする（ページング禁止のため）
    OR  EAX,0x00000001 ; bit0を1にする（プロテクトモード移行のため）
    MOV CR0,EAX
    JMP pipelineflush ; プロテクトモードに切り替えた場合はすぐにJMP命令を実行する必要がある。これは、パイプラインによりすでに実行されている次の命令をやり直してもらうため。
pipelineflush:
    MOV AX,1*8 ; 読み書き可能セグメント32bit。0x0008はgdt+1のセグメントに相当。
    MOV DS,AX ; 以下CS以外のセグメントレジスタの値を変更
    MOV ES,AX
    MOV FS,AX
    MOV GS,AX
    MOV SS,AX

; bootpackの転送
    MOV ESI,bootpack ; 転送元
    MOV EDI,BOTPAK ; 転送先
    MOV ECX,512*1024/4
    CALL    memcpy

; ついでにディスクデータも本来の位置へ転送
; まずはブートセクタから
    MOV ESI,0x7c00  ; 転送元
    MOV EDI,DSKCAC  ; 転送先
    MOV ECX,512/4
    CALL    memcpy

; 残り全部
    MOV ESI,DSKCAC0+512 ; 転送元
    MOV EDI,DSKCAC+512 ; 転送先
    MOV ECX,0
    MOV CL,BYTE [CYLS]
    IMUL    ECX,512*18*2/4 ; シリンダ数からバイト数/4に変換
    SUB ECX,512/4 ; IPLの分だけ差し引く
    CALL    memcpy

; asmhead でしなければいけないことは全部終わったので、あとは bootpack に任せる

; bootpack の起動
    MOV EBX,BOTPAK
    MOV ECX,[EBX+16]
    ADD ECX,3
    SHR ECX,2 ; ECX /= 4. SHR は右シフト命令
    JZ  skip ; 転送するべきものがない。JZはjump if zero.直前の計算結果が0の場合jump.この場合ECXが0かどうか。
    MOV ESI,[EBX+20] ; 転送元
    ADD ESI,EBX
    MOV EDI,[EBX+12] ; 転送先
    CALL    memcpy
skip:
    MOV ESP,[EBX+12] ; スタック初期値
    JMP DWORD   2*8:0x0000001b

waitkbdout:
    IN  AL,0x64
    AND AL,0x02
    IN  AL,0x60 ; キーボードに溜まっていたキーコードやマスすのデータがあれば空読み。受信バッファが悪さをしないように。
    JNZ waitkbdout ; ANDの結果が 0 でなければ waitkbdout へ
    RET

memcpy:
    MOV EAX,[ESI]
    ADD ESI,4
    MOV [EDI],EAX
    ADD EDI,4
    SUB ECX,1
    JNZ memcpy  ; 引き算した結果が0でなければmemcpyへ
    RET

    ALIGNB  16
GDT0:
    RESB    8 ; ヌルセレクタ
    DW  0xffff,0x0000,0x9200,0x00cf ; 読み書き可能セグメント32bit
    DW  0xffff,0x0000,0x9a28,0x0047 ; 実行可能セグメント32bit（bootpack用）

    DW  0
GDTR0:
    DW  8*3-1
    DD  GDT0

    ALIGNB  16
bootpack:
