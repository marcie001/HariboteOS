; nasmfunc
; TAB=4

BITS 32

GLOBAL  io_hlt, io_cli, io_sti, io_stihlt
GLOBAL  io_in8, io_in16, io_in32
GLOBAL  io_out8, io_out16, io_out32
GLOBAL  io_load_eflags, io_store_eflags
GLOBAL  load_gdtr, load_idtr
GLOBAL  load_cr0, store_cr0
GLOBAL  asm_inthandler20, asm_inthandler21, asm_inthandler2c
GLOBAL  memtest_sub
GLOBAL  load_tr, farjmp
EXTERN  inthandler20, inthandler21, inthandler2c

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

load_cr0: ; int load_cr0(void);
    MOV EAX,CR0
    RET

store_cr0: ; void store_cr0(int cr0);
    MOV EAX,[ESP+4]
    MOV CR0,EAX
    RET

asm_inthandler20:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV EAX,ESP
    PUSH    EAX
    MOV AX,SS
    MOV DS,AX
    MOV ES,AX
    CALL inthandler20
    POP EAX
    POPAD
    POP DS
    POP ES
    IRETD

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

memtest_sub: ; unsigned int memtest_sub(unsigned int start, unsigned int end);
    PUSH    EDI ; EBX, ESI, EDI も使いたいので
    PUSH    ESI
    PUSH    EBX
    MOV ESI,0xaa55aa55          ; pat0 = 0xaa55aa55;
    MOV EDI,0x55aa55aa          ; pat1 = 0x55aa55aa;
    MOV EAX,[ESP+12+4]          ; i = start;
mts_loop:
    MOV EBX,EAX
    ADD EBX,0xffffc             ; p = i + 0xffffc;
    MOV EDX,[EBX]               ; old = *p;
    MOV [EBX],ESI               ; *p = pat0;
    XOR DWORD [EBX],0xffffffff  ; *p ^= 0xffffffff;
    CMP EDI,[EBX]               ; if (*p != pat1) goto fin;
    JNE mts_fin
    XOR DWORD [EBX],0xffffffff  ; *p ^= 0xffffffff;
    CMP ESI,[EBX]               ; if (*p != pat0) goto fin;
    JNE mts_fin
    MOV [EBX],EDX               ; *p = old;
    ADD EAX,0x100000            ; i+= 0x100000;
    CMP EAX,[ESP+12+8]          ; if (i <= end) goto mts_loop;
    JBE mts_loop
    POP EBX
    POP ESI
    POP EDI
    RET
mts_fin:
    MOV [EBX],EDX               ; *p = old
    POP EBX
    POP ESI
    POP EDI
    RET

load_tr:    ; void load_tr(int tr);
    LTR [ESP+4] ; tr
    RET

farjmp: ; void farjmp(int eip, int cs);
    JMP FAR [ESP+4] ; eip, cs
    RET

; JMP がタスクスイッチ（ far-JMP ）の場合、
; タスクスイッチ後にまたこのタスクに戻ってきたときにここから処理が再開されるので、
; 次行の RET 命令が必要。
; また、 far-JMP の場合、セグメント（:より前の部分）が TSS を指す。番地（:より後ろの部分）は無視される。番地は普通 0 にしておく。
; taskswitch4:    ; void taskswitch4(void);
;     JMP 4*8:0
;     RET
