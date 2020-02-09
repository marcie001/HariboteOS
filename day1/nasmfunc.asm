; nasmfunc
; TAB=4

BITS 32

GLOBAL  io_hlt, io_cli, io_sti, io_stihlt
GLOBAL  io_in8, io_in16, io_in32
GLOBAL  io_out8, io_out16, io_out32
GLOBAL  io_load_eflags, io_store_eflags
GLOBAL  load_gdtr, load_idtr
GLOBAL  asm_inthandler21, asm_inthandler2c
EXTERN  inthandler21, inthandler2c

SECTION .text

io_hlt:
    HLT
    RET

io_cli:
    CLI
    RET

io_sti:
    STI
    RET

io_stihlt:
    STI
    HLT
    RET

io_in8: ; int io_in8(int port);
    MOV EDX,[ESP+4] ; port
    MOV EAX,0
    IN  AL,DX
    RET

io_in16: ; void io_in16(int port);
    MOV EDX,[ESP+4] ; port
    MOV EAX,0
    IN  AX,DX
    RET

io_in32: ; void io_in16(int port);
    MOV EDX,[ESP+4] ; port
    IN  EAX,DX
    RET

io_out8: ; void io_out8(int port, int data);
    MOV EDX,[ESP+4] ; port
    MOV AL,[ESP+8] ; data
    OUT DX,AL
    RET

io_out16: ; void io_out16(int port, int data);
    MOV EDX,[ESP+4] ; port
    MOV AX,[ESP+8] ; data
    OUT DX,AX
    RET

io_out32: ; void io_out32(int port, int data);
    MOV EDX,[ESP+4] ; port
    MOV EAX,[ESP+8] ; data
    OUT DX,EAX
    RET

io_load_eflags:
    PUSHFD ; push flags double-word: フラグをダブルワードでスタックに積む
    POP EAX
    RET

io_store_eflags:
    MOV EAX,[ESP+4]
    PUSH    EAX
    POPFD ; pop flags double-word: フラグをダブルワードでスタックから取り出す
    RET

load_gdtr:  ; void load_gdtr(int limit, int addr);
    MOV AX,[ESP+4]  ; limit
    MOV [ESP+6],AX
    LGDT    [ESP+6]
    RET

load_idtr:  ; void load_idtr(int limit, int addr);
    MOV AX,[ESP+4]  ; limit
    MOV [ESP+6],AX
    LIDT    [ESP+6]
    RET

asm_inthandler21:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV EAX,ESP
    PUSH    EAX
    MOV AX,SS
    MOV DS,AX
    MOV ES,AX
    CALL inthandler21
    POP EAX
    POPAD
    POP DS
    POP ES
    IRETD

asm_inthandler2c:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV EAX,ESP
    PUSH    EAX
    MOV AX,SS
    MOV DS,AX
    MOV ES,AX
    CALL inthandler2c
    POP EAX
    POPAD
    POP DS
    POP ES
    IRETD