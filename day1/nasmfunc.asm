; nasmfunc
; TAB=4

BITS 32

GLOBAL io_hlt
GLOBAL write_mem8

SECTION .text

io_hlt:
    HLT
    RET

write_mem8: ; void write_mem8(int addr, int data);
    ; C と連携するときに自由に使える（代入してもよい）レジスタは EAX, ECX, EDX のみ
    MOV ECX,[ESP+4] ; [ESP+4] にaddrが入っているのでそれを ECX に読み込む
    MOV AL,[ESP+8] ; [ESP+8] にdataが入っているのでそれを AL に読み込む
    MOV [ECX],AL
    RET
