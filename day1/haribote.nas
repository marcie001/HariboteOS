; haribote-os
; TAB=4

    ORG 0xc200 ; このプログラムがどこに読み込まれるか

    MOV AL,0x13
    MOV AH,0x00
    INT 0x10
fin:
    HLT
    JMP fin
