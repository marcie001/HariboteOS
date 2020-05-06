BITS 32

SECTION .text

GLOBAL HariMain

HariMain:
    MOV ECX,msg
    MOV EDX,1
putloop:
    MOV AL,[CS:ECX]
    CMP AL,0
    JE  fin
    INT 0x40
    ADD ECX,1
    JMP putloop
fin:
    ; 終了 API を呼び出す
    MOV EDX,4
    INT 0x40

SECTION .data

msg:
    DB "hello",0
