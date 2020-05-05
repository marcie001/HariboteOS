BITS 32

; io_cli を far-CALL しようとしている
    CALL 2*8:0xc52
    MOV EDX,4
    INT 0x40
