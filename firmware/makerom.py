rom = bytearray([0xea] * 32768)

# load value 42 into A register
rom[0] = 0xA9
rom[1] = 0x42

# store A in memory
rom[2] = 0x8D
rom[3] = 0x00 # address 6000
rom[4] = 0x60

# jump to start address
rom[0x7ffc] = 0x00
rom[0x7ffd] = 0x80

with open("rom.bin", "wb") as out_file:
    out_file.write(rom);

