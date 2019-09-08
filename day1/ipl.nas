; hello-os
; TAB=4

    ORG 0x7c00 ; このプログラムがどこに読み込まれるか

; 以下は標準的なFAT12フォーマットフロッピーディスクのための記述

    JMP SHORT   entry
    DB  0x90
    DB  "HELLOIPL" ; ブートセクタの名前（8バイト）
    DW  512 ; 1セクタの大きさ（512固定）
    DB  1 ; クラスタの大きさ（1セクタ固定）
    DW  1 ; FATがどこから始まるか（普通は1セクタ目）
    DB  2 ; FATの個数（2固定）
    DW  224 ; ルートディレクトリ領域の大きさ（普通や224エントリ）
    DW  2280 ; このドライブの大きさ
    DB  0xf0 ; メディアのタイプ（0xf0固定）
    DW  9 ; FAT領域の長さ（9セクタ固定）
    DW  18 ; 1トラックにいくつのセクタがあるか（18固定）
    DW  2 ; ヘッドの数（2固定）
    DD  0 ; パーティションを使っていないので0固定
    DD  2880 ; このドライブの大きさ（13行目で書いたのと同じ値）
    DB  0,0,0x29 ; よくわからないけどこの値にしておくといいらしい
    DD  0xffffffff ; 多分ボリュームシリアル番号
    DB  "HELLO-OS   " ; ディスクの名前（11バイト）
    DB  "FAT12   " ; フォーマットの名前（8バイト）
    RESB    18 ; とりあえず18バイトあけておく

; プログラム本体

entry:
    MOV AX,0 ; レジスタ初期化
    MOV SS,AX
    MOV SP,0x7c00
    MOV DS,AX
    MOV ES,AX

    MOV AH,0x00
    MOV AL,0x13
    INT 0x10
    
    MOV SI,msg
putloop:
    MOV AL,[SI]
    ADD SI,1 ; SIに1を足す
    CMP AL,0
    JE  fin
    MOV AH,0x0e ; 位置文字表示ファンクション
    MOV BX,4 ; カラーコード
    INT 0x10 ; ビデオBIOS呼び出し

    CMP AL,0x0a
    JE  putloop
    MOV AL,0x0d ; CR
    MOV AH,0x0e
    MOV BX,15
    INT 0x10

    MOV AL,0x0a ; LF
    MOV AH,0x0e
    MOV BX,15
    INT 0x10

    JMP SHORT putloop
fin:
    HLT ; 何かあるまでCPUを停止させる
    JMP SHORT fin ; 無限ループ

msg:
    DB  0x0a, 0x0a
    DB  "hello, world!!"
    DB  0x0a
    DB  0

    RESB    0x1fe-($-$$) ; 0x001feまでを0x00で埋める命令

    DB  0x55, 0xaa
