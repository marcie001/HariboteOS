BITS 32

SECTION .text

GLOBAL HariMain

HariMain:
    MOV EDX,2
    MOV EBX,msg
    INT 0x40
    MOV EDX,4
    INT 0x40

SECTION .data

msg:
    DB  "hello",0
