    .org $8000

reset:
    ; init VIA
    lda #$ff
    sta $6002

    ; output pattern
    lda #$50
    sta $6000

loop:
    ; rotate value right and push to adapter
    ror
    sta $6000

    ; repeat
    jmp loop

    ; reset vector
    .org $fffc
    .word reset
    .word $0000
