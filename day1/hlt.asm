BITS 32

    MOV AL,'A'
    CALL    0xd5e
fin:
    HLT
    JMP fin
