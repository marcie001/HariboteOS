; haribote-ipl
; TAB=4

CYLS    EQU 10 ; どこまで読み込むか

    ORG 0x7c00 ; このプログラムがどこに読み込まれるか

; 以下は標準的なFAT12フォーマットフロッピーディスクのための記述

    JMP SHORT   entry
    DB  0x90
    DB  "HARIBOTE" ; ブートセクタの名前（8バイト）
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
    DB  "HARIBOTEOS " ; ディスクの名前（11バイト）
    DB  "FAT12   " ; フォーマットの名前（8バイト）
    RESB    18 ; とりあえず18バイトあけておく

; プログラム本体

entry:
    MOV AX,0 ; レジスタ初期化
    MOV SS,AX
    MOV SP,0x7c00
    MOV DS,AX

; ディスクを読む

    MOV AX,0x0820
    MOV ES,AX
    MOV CH,0 ; シリンダ0
    MOV DH,0 ; ヘッド0
    MOV CL,2 ; セクタ2
readloop:
    MOV SI,0 ; 失敗回数を数えるレジスタ
retry:
    MOV AH,0x02 ; ディスク読み込み
    MOV AL,1 ; 1セクタ
    MOV BX,0
    MOV DL,0x00 ; Aドライブ
    INT 0x13 ; ディスクBIOS呼び出し
    JNC next ; エラーが起きなければnextへ. JNC: jamp if not carry
    ADD SI,1 ; SIに1を足す
    CMP SI,5 ; SIと5を比較
    JAE error ; SI >= 5 だったらerrorへ. JAE: jamp if  above or equal
    MOV AH,0x00
    MOV DL,0x00
    INT 0x13 ; ドライブのリセット
    JMP retry
next:
    MOV AX,ES ; アドレスを0x200に進める
    ADD AX,0x0020
    MOV ES,AX ; ADD ES,0x020という命令がないので、AXに代入し加算し再度代入する方法をとっている
    ADD CL,1
    CMP CL,18
    JBE readloop ; CL <= 18だったらreadloopへ. JBE: jump if below or equal
    MOV CL,1
    ADD DH,1
    CMP DH,2
    JB  readloop ; DH < 2 だったらreadloopへ
    MOV DH,0
    ADD CH,1
    CMP CH,CYLS
    JB  readloop ; CH < CYLS だったらreadloopへ

; 読み終わったので haribote.sys を実行

    MOV [0x0ff0], CH
    JMP 0xc200

error:
    MOV AH,0x00
    MOV AL,0x13
    INT 0x10 ; ビデオBIOS呼び出し
    MOV SI,msg
putloop:
    MOV AL,[SI]
    ADD SI,1 ; SIに1を足す
    CMP AL,0
    JE  fin
    MOV AH,0x0e ; 位置文字表示ファンクション
    MOV BX,4 ; カラーコード
    INT 0x10 ; ビデオBIOS呼び出し
    JMP putloop
fin:
    HLT
    JMP fin
msg:
    DB  0x0a, 0x0a
    DB  "load error"
    DB  0x0a
    DB  0

    RESB    0x1fe-($-$$) ; 0x001feまでを0x00で埋める命令

    DB  0x55, 0xaa
