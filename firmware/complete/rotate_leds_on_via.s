; VIA is wired to address range starting at 0x6000
; PORTB = $6000
; PORTA = $6001

    .org $8000

reset:
    ; init VIA
    ; set data direction reg DDRB to output 
    lda #$ff    ; load immediate value to reg A
    sta $6002   ; store reg A to DDRB on VIA (0x6002)

loop:
    ; output pattern
    ; write 8-bits of data to ORB on VIA
    lda #$55    ; store bit pattern 01010101 in reg A
    sta $6000   ; store reg A to ORB on VIA (0x6000)

    ; repeat
    jmp loop

    ; reset vector
    .org $fffc
    .word reset
    .word $0000
