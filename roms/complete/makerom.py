code = bytearray([
    0xA9, 0x42,         # LDA #$42 (load value 42 into A register)
    0x8D, 0x00, 0x60,   # STA $6000 (store A in memory at address 0x6000)
    # there is nothing there right now
])
rom = code + bytearray([0xEA] * 32768 - len(code))

# RESET ADDRESS
rom[0x7ffc] = 0x00
rom[0x7ffd] = 0x80

with open("rom.bin", "wb") as out_file:
    out_file.write(rom);

