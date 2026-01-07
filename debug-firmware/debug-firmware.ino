// Pin Configuration Constants
const int ADDR[]    = {22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52}; // A0-A15 (16 pins)
const int DATA[]    = {53, 51, 49, 47, 45, 43, 41, 39};                                 // D0-D7 (8 pins)
const int CLK_PIN   = 2;                                                                // Clock pin (output)
const int RW_PIN    = 3;                                                                // Read/Write pin (input)
const int RESET_PIN = 4;                                                                // Reset pin (output)

// Clock timing constants
const int CLK_DELAY_US = 100; // Clock pulse HIGH duration in microseconds

// State variables
bool continuousMode = false;
bool freeRunMode    = false;

// Previous state for change detection
uint16_t prevAddr = 0;
uint8_t prevData  = 0;
bool prevRW       = true;

// 6502 Opcode lookup table (stored in flash to save RAM)
// Reference: http://www.6502.org/tutorials/6502opcodes.html
const char OPCODE_NAMES[][4] PROGMEM = {
    // 0x00-0x0F
    "BRK", "ORA", "???", "???", "???", "ORA", "ASL", "???", "PHP", "ORA", "ASL", "???", "???", "ORA", "ASL", "???",
    // 0x10-0x1F
    "BPL", "ORA", "???", "???", "???", "ORA", "ASL", "???", "CLC", "ORA", "???", "???", "???", "ORA", "ASL", "???",
    // 0x20-0x2F
    "JSR", "AND", "???", "???", "BIT", "AND", "ROL", "???", "PLP", "AND", "ROL", "???", "BIT", "AND", "ROL", "???",
    // 0x30-0x3F
    "BMI", "AND", "???", "???", "???", "AND", "ROL", "???", "SEC", "AND", "???", "???", "???", "AND", "ROL", "???",
    // 0x40-0x4F
    "RTI", "EOR", "???", "???", "???", "EOR", "LSR", "???", "PHA", "EOR", "LSR", "???", "JMP", "EOR", "LSR", "???",
    // 0x50-0x5F
    "BVC", "EOR", "???", "???", "???", "EOR", "LSR", "???", "CLI", "EOR", "???", "???", "???", "EOR", "LSR", "???",
    // 0x60-0x6F
    "RTS", "ADC", "???", "???", "???", "ADC", "ROR", "???", "PLA", "ADC", "ROR", "???", "JMP", "ADC", "ROR", "???",
    // 0x70-0x7F
    "BVS", "ADC", "???", "???", "???", "ADC", "ROR", "???", "SEI", "ADC", "???", "???", "???", "ADC", "ROR", "???",
    // 0x80-0x8F
    "???", "STA", "???", "???", "STY", "STA", "STX", "???", "DEY", "???", "TXA", "???", "STY", "STA", "STX", "???",
    // 0x90-0x9F
    "BCC", "STA", "???", "???", "STY", "STA", "STX", "???", "TYA", "STA", "TXS", "???", "???", "STA", "???", "???",
    // 0xA0-0xAF
    "LDY", "LDA", "LDX", "???", "LDY", "LDA", "LDX", "???", "TAY", "LDA", "TAX", "???", "LDY", "LDA", "LDX", "???",
    // 0xB0-0xBF
    "BCS", "LDA", "???", "???", "LDY", "LDA", "LDX", "???", "CLV", "LDA", "TSX", "???", "LDY", "LDA", "LDX", "???",
    // 0xC0-0xCF
    "CPY", "CMP", "???", "???", "CPY", "CMP", "DEC", "???", "INY", "CMP", "DEX", "???", "CPY", "CMP", "DEC", "???",
    // 0xD0-0xDF
    "BNE", "CMP", "???", "???", "???", "CMP", "DEC", "???", "CLD", "CMP", "???", "???", "???", "CMP", "DEC", "???",
    // 0xE0-0xEF
    "CPX", "SBC", "???", "???", "CPX", "SBC", "INC", "???", "INX", "SBC", "NOP", "???", "CPX", "SBC", "INC", "???",
    // 0xF0-0xFF
    "BEQ", "SBC", "???", "???", "???", "SBC", "INC", "???", "SED", "SBC", "???", "???", "???", "SBC", "INC", "???"};

void setup() {
    // Initialize serial communication
    Serial.begin(9600);

    // Wait for serial port to connect (useful for some boards)
    while (!Serial) {
        ; // wait for serial port to connect
    }

    // Initialize address bus pins as INPUT
    for (int n = 0; n < 16; n++) {
        pinMode(ADDR[n], INPUT);
    }

    // Initialize data bus pins as INPUT
    for (int n = 0; n < 8; n++) {
        pinMode(DATA[n], INPUT);
    }

    // Initialize clock pin as OUTPUT (initially HIGH - idle state)
    pinMode(CLK_PIN, OUTPUT);
    digitalWrite(CLK_PIN, HIGH);

    // Initialize reset pin as OUTPUT (initially HIGH - not reset)
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, HIGH);

    // Initialize R/W pin as INPUT
    pinMode(RW_PIN, INPUT);

    // Initialize built-in LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("6502 Target Debugger Ready!");
    Serial.println("Commands:");
    Serial.println("  'r' = Reset target");
    Serial.println("  'c' = Toggle continuous mode (slow, ~2 Hz)");
    Serial.println("  's' = Single clock step");
    Serial.println("  'p' = Read bus (no clock step)");
    Serial.println("  'f' = Free run (reset + fast clock)");
    Serial.println("  'h' = Show this help");
    Serial.println();
    Serial.println("Monitoring bus for changes...");

    // Initialize previous state
    prevAddr = readAddressBus();
    prevData = readDataBus();
    prevRW   = readRW();
}

uint16_t readAddressBus() {
    uint16_t addr = 0;
    for (int n = 0; n < 16; n++) {
        if (digitalRead(ADDR[n])) {
            addr |= (1UL << n);
        }
    }
    return addr;
}

uint8_t readDataBus() {
    uint8_t data = 0;
    for (int n = 0; n < 8; n++) {
        if (digitalRead(DATA[n])) {
            data |= (1UL << n);
        }
    }
    return data;
}

bool readRW() { return digitalRead(RW_PIN); }

void getOpcodeName(uint8_t opcode, char *buf) { strcpy_P(buf, OPCODE_NAMES[opcode]); }

void printCurrentState(uint16_t addr, uint8_t data, bool rw) {
    char buf[64];
    sprintf(buf, "Addr: 0x%04X (", addr);
    Serial.print(buf);
    for (int n = 15; n >= 0; n--) {
        Serial.print((addr >> n) & 1);
    }
    sprintf(buf, ") | Data: 0x%02X (", data);
    Serial.print(buf);
    for (int n = 7; n >= 0; n--) {
        Serial.print((data >> n) & 1);
    }
    Serial.print(") | R/W: ");
    Serial.print(rw ? "R" : "W");

    // Decode opcode on reads
    if (rw) {
        char opname[4];
        getOpcodeName(data, opname);
        Serial.print(" | Op: ");
        Serial.print(opname);
    }
    Serial.println();
}

/*
 The Reset (RESB) input is used to initialize the microprocessor and start program execution.
 The RESB signal must be held low for at least two clock cycles after VDD reaches operating
 voltage.
*/
void resetTarget() {
    // Clock 4 cycles while RESET is held high
    for (int i = 0; i < 4; i++) {
        digitalWrite(CLK_PIN, LOW);
        delayMicroseconds(CLK_DELAY_US);
        digitalWrite(CLK_PIN, HIGH);
        delayMicroseconds(CLK_DELAY_US);
    }

    digitalWrite(RESET_PIN, LOW);
    digitalWrite(LED_BUILTIN, LOW);

    // Clock 4 cycles while RESET is held low
    for (int i = 0; i < 20; i++) {
        digitalWrite(CLK_PIN, LOW);
        delayMicroseconds(CLK_DELAY_US);
        digitalWrite(CLK_PIN, HIGH);
        delayMicroseconds(CLK_DELAY_US);
    }

    digitalWrite(RESET_PIN, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Target reset!");

    // Clock 4 cycles while RESET is held low
    for (int i = 0; i < 20; i++) {
        digitalWrite(CLK_PIN, LOW);
        delayMicroseconds(CLK_DELAY_US);
        digitalWrite(CLK_PIN, HIGH);
        delayMicroseconds(CLK_DELAY_US);
    }
}

/*
 The CPU drives the address bus and reads/writes the data bus only during PHI2 HIGH.
 Clock idles HIGH between cycles. Step: LOW -> HIGH, read during HIGH.
*/
void stepClock() {
    digitalWrite(CLK_PIN, LOW);
    digitalWrite(LED_BUILTIN, LOW);
    delayMicroseconds(CLK_DELAY_US);
    delayMicroseconds(CLK_DELAY_US);

    digitalWrite(CLK_PIN, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
    delayMicroseconds(CLK_DELAY_US);
    delayMicroseconds(CLK_DELAY_US);

    // Read bus while PHI2 is HIGH (when data is valid)
    uint16_t addr = readAddressBus();
    uint8_t data  = readDataBus();
    bool rw       = readRW();

    // Print captured state and update previous values
    printCurrentState(addr, data, rw);
    prevAddr = addr;
    prevData = data;
    prevRW   = rw;
}

// Fast clock step for free run mode - no printing, minimal delay
void stepClockFast() {
    digitalWrite(CLK_PIN, LOW);
    delayMicroseconds(CLK_DELAY_US);
    digitalWrite(CLK_PIN, HIGH);
    delayMicroseconds(CLK_DELAY_US);
}

void loop() {
    // Check for serial input
    if (Serial.available() > 0) {
        char command = Serial.read();

        // Process command
        switch (command) {
        case 'r':
        case 'R':
            resetTarget();
            break;

        case 'c':
        case 'C':
            continuousMode = !continuousMode;
            freeRunMode    = false; // Disable free run when toggling continuous
            Serial.print("Continuous clock mode: ");
            Serial.println(continuousMode ? "ON" : "OFF");
            break;

        case 's':
        case 'S':
            stepClock();
            break;

        case 'p':
        case 'P': {
            // Read and print current bus state without stepping clock
            uint16_t addr = readAddressBus();
            uint8_t data  = readDataBus();
            bool rw       = readRW();
            printCurrentState(addr, data, rw);
            break;
        }

        case 'f':
        case 'F':
            resetTarget();
            freeRunMode    = true;
            continuousMode = false; // Disable continuous when starting free run
            Serial.println("Free run mode started!");
            break;

        case 'h':
        case 'H':
            Serial.println("Commands:");
            Serial.println("  'r' = Reset target");
            Serial.println("  'c' = Toggle continuous mode (slow, ~2 Hz)");
            Serial.println("  's' = Single clock step");
            Serial.println("  'p' = Read bus (no clock step)");
            Serial.println("  'f' = Free run (reset + fast clock)");
            Serial.println("  'h' = Show this help");
            break;

        case '\n':
        case '\r':
        case ' ':
            // Ignore whitespace/newline characters
            break;

        default:
            // Unknown command - ignore (don't step clock on garbage)
            break;
        }

        // DROP any remaining characters in buffer
        while (Serial.available() > 0) {
            Serial.read();
        }
    }

    if (freeRunMode) {
        stepClockFast();
    } else if (continuousMode) {
        stepClock();
        delay(500); // ~2 instructions per second
    }
}
