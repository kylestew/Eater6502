VASM = ../vasm/vasm6502_oldstyle
VASM_FLAGS = -Fbin -dotdir

SRC = prog.s
OUT = rom.bin

all: $(OUT)

$(OUT): $(SRC)
	$(VASM) $(VASM_FLAGS) -o $@ $<

flash: $(OUT)
	minipro -p AT28C256 -w $(OUT)

clean:
	rm -f $(OUT)

dump: $(OUT)
	hexdump -C $(OUT)

serial:
	picocom /dev/tty.usbmodem1101

help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all     Build rom.bin (default)"
	@echo "  flash   Build and flash to AT28C256 EEPROM"
	@echo "  clean   Remove rom.bin"
	@echo "  dump    Hex dump of rom.bin"
	@echo "  serial  Open UART connection with picocom"
	@echo "  help    Show this help message"

.PHONY: all flash clean dump serial help

