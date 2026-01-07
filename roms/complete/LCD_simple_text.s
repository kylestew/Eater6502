; VIA is at address range 0x6000-....
PORTB = $6000
PORTA = $6001
DDRB = $6002
DDRA = $6003

E =  %10000000
RW = %01000000
RS = %00100000

    .org $8000

reset:
    ; init VIA
    lda #%11111111 ; set all pins on port B to output
    sta DDRB       

    lda #%11100000 ; set top 3 pins on port A to output
    sta DDRA

    ; 1: Function Set
    lda #%00111000 ; Set 8-bit mode, 2-line display, 5x8 font
    sta PORTB      ; VIA port B connected to LCD data pins

    lda #0         ; Clear RS/RW/E bits on LCD
    sta PORTA      ; port A top 3 pins connected to LCD RS/RW/E

    lda #E         ; set (E) enable bit to send instruction
    sta PORTA

    lda #0
    sta PORTA

    ; 2: Display On
    lda #%00001110 ; Display on, cursor on, blink off
    sta PORTB      

    lda #0         ; Clear RS/RW/E bits on LCD
    sta PORTA      ; port A top 3 pins connected to LCD RS/RW/E

    lda #E         ; set (E) enable bit to send instruction
    sta PORTA

    lda #0
    sta PORTA

    ; 3: Entry Mod
    lda #%00000110 ; Incrementing and shift cursor, don't shift display
    sta PORTB      

    lda #0         ; Clear RS/RW/E bits on LCD
    sta PORTA      ; port A top 3 pins connected to LCD RS/RW/E

    lda #E         ; set (E) enable bit to send instruction
    sta PORTA

    lda #0
    sta PORTA

    ; 4: Write Data
    lda #"H"
    sta PORTB      

    lda #RS        ; set (RS) high
    sta PORTA      

    lda #(RS | E)  ; set (E) enable bit and (RS) reg select
    sta PORTA

    lda #RS        ; clear (E) enable bit
    sta PORTA

    lda #"I"
    sta PORTB      

    lda #RS        ; set (RS) high
    sta PORTA      

    lda #(RS | E)  ; set (E) enable bit and (RS) reg select
    sta PORTA

    lda #RS        ; clear (E) enable bit
    sta PORTA

    lda #"!"
    sta PORTB      

    lda #RS        ; set (RS) high
    sta PORTA      

    lda #(RS | E)  ; set (E) enable bit and (RS) reg select
    sta PORTA

    lda #RS        ; clear (E) enable bit
    sta PORTA

loop:
    jmp loop

    .org $fffc
    .word reset
    .word $0000
