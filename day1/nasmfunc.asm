; nasmfunc
; TAB=4

BITS 32

GLOBAL  io_hlt, io_cli, io_sti, io_stihlt
GLOBAL  io_in8, io_in16, io_in32
GLOBAL  io_out8, io_out16, io_out32
GLOBAL  io_load_eflags, io_store_eflags
GLOBAL  load_gdtr, load_idtr
GLOBAL  load_cr0, store_cr0
GLOBAL  asm_inthandler0d, asm_inthandler20, asm_inthandler21, asm_inthandler2c
GLOBAL  memtest_sub
GLOBAL  load_tr, farjmp, farcall
GLOBAL  start_app, asm_hrb_api
EXTERN  inthandler0d, inthandler20, inthandler21, inthandler2c
EXTERN  hrb_api

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

asm_inthandler0d:
    STI
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV EAX,ESP
    MOV AX,SS
    MOV DS,AX
    MOV ES,AX
    CALL    inthandler0d
    CMP EAX,0
    JNE end_app
    POP EAX
    POPAD
    POP DS
    POP ES
    ADD ESP,4   ; INT 0x0d ではこの行が必要
    IRETD

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

farcall: ; void farcall(int eip, int cs);
    CALL FAR [ESP+4]
    RET

start_app:  ; void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
    PUSHAD  ; 32 ビットレジスタを全部保存
    MOV EAX,[ESP+36]    ; アプリ用の EIP
    MOV ECX,[ESP+40]    ; アプリ用の CS
    MOV EDX,[ESP+44]    ; アプリ用の ESP
    MOV EBX,[ESP+48]    ; アプリ用の DS/SS
    MOV EBP,[ESP+52]    ; tss.esp0 の番地
    MOV [EBP],ESP     ; OS 用の ESP を保存
    MOV [EBP+4],SS     ; OS 用の SS を保存
    MOV ES,BX
    MOV DS,BX
    MOV FS,BX
    MOV GS,BX
    ; 以下は RETF でアプリにジャンプさせるためのスタック調整
    OR  ECX,3   ; アプリ用のセグメント番号に 3 を OR する
    OR  EBX,3   ; アプリ用のセグメント番号に 3 を OR する
    PUSH    EBX ; アプリの SS
    PUSH    EDX ; アプリの ESP
    PUSH    ECX ; アプリの CS
    PUSH    EAX ; アプリの EIP
    RETF
    ; アプリが終了してもここには戻ってこない

asm_hrb_api:
    STI
    PUSH    DS
    PUSH    ES
    PUSHAD  ; 保存のための PUSH
    PUSHAD  ; hrb_api に渡すための PUSH
    MOV     AX,SS
    MOV     DS,AX       ; OS 用のセグメントを DS と ES にも入れる
    MOV     ES,AX
    CALL    hrb_api
    CMP     EAX,0       ; EAX が 0 でなければアプリ終了処理
    JNE     end_app
    ADD     ESP,32
    POPAD
    POP     ES
    POP     DS
    IRETD
end_app:
    ; EAX は tss.esp0 の番地
    MOV ESP,[EAX]
    POPAD
    RET   ; cmd_app へ戻る。この命令が自動で STI してくれる
