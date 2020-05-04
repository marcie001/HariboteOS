BITS 32

    MOV AL,'A'
    CALL    2*8:0xd5e
fin:
    HLT
    JMP fin
