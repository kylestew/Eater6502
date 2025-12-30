// Pin Configuration Constants
const int ADDR[]    = {22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52}; // A0-A15 (16 pins)
const int DATA[]    = {53, 51, 49, 47, 45, 43, 41, 39};                                 // D0-D7 (8 pins)
const int CLK_PIN   = 2;                                                                // Clock pin (output)
const int RW_PIN    = 3;                                                                // Read/Write pin (input)
const int RESET_PIN = 4;                                                                // Reset pin (output)

// Clock timing constants
const int CLK_DELAY_US = 100; // Clock pulse HIGH duration in microseconds

// State variables
bool continuousMode       = false;
bool constantDataMode     = false;
uint8_t constantDataValue = 0xEA; // Default to NOP opcode

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

    // Initialize clock pin as OUTPUT (initially LOW)
    pinMode(CLK_PIN, OUTPUT);
    digitalWrite(CLK_PIN, LOW);

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
    Serial.println("  'c' = Toggle continuous clock mode");
    Serial.println("  's' = Single clock step");
    Serial.println("  'd' = Toggle constant data mode");
    Serial.println("  'd XX' = Set constant data value (hex)");
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

void setDataBusOutput() {
    for (int n = 0; n < 8; n++) {
        pinMode(DATA[n], OUTPUT);
        digitalWrite(DATA[n], (constantDataValue >> n) & 1);
    }
}

void setDataBusInput() {
    for (int n = 0; n < 8; n++) {
        pinMode(DATA[n], INPUT);
    }
}

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
    digitalWrite(CLK_PIN, LOW);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(RESET_PIN, LOW);
    delay(100);
    digitalWrite(RESET_PIN, HIGH);
    Serial.println("Target reset!");
}

/*
 The CPU drives the address bus and reads/writes the data bus only during PHI2 HIGH.
*/
void stepClock() {
    digitalWrite(CLK_PIN, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
    delayMicroseconds(CLK_DELAY_US);
    digitalWrite(CLK_PIN, LOW);
    digitalWrite(LED_BUILTIN, LOW);
    delayMicroseconds(CLK_DELAY_US);
}

void loop() {
    // Continuously poll the bus for changes
    uint16_t addr = readAddressBus();
    uint8_t data  = readDataBus();
    bool rw       = readRW();

    // Check for changes
    if (addr != prevAddr || data != prevData || rw != prevRW) {
        printCurrentState(addr, data, rw);
        prevAddr = addr;
        prevData = data;
        prevRW   = rw;
    }

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
            Serial.print("Continuous clock mode: ");
            Serial.println(continuousMode ? "ON" : "OFF");
            break;

        case 's':
        case 'S':
            stepClock();
            break;

        case 'd':
        case 'D': {
            // Check if there's a hex value following
            delay(10); // Allow time for serial buffer to fill
            if (Serial.available() > 0) {
                char next = Serial.peek();
                if (next == ' ' || (next >= '0' && next <= '9') || (next >= 'a' && next <= 'f') ||
                    (next >= 'A' && next <= 'F')) {
                    // Skip space if present
                    if (next == ' ')
                        Serial.read();
                    // Parse hex value
                    char hexStr[3] = {0};
                    int idx        = 0;
                    while (Serial.available() > 0 && idx < 2) {
                        char c = Serial.peek();
                        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                            hexStr[idx++] = Serial.read();
                        } else {
                            break;
                        }
                    }
                    if (idx > 0) {
                        constantDataValue = (uint8_t) strtol(hexStr, NULL, 16);
                        Serial.print("Constant data value set to: 0x");
                        Serial.println(constantDataValue, HEX);
                        if (constantDataMode) {
                            setDataBusOutput(); // Update output if already enabled
                        }
                        break;
                    }
                }
            }
            // Toggle mode if no hex value provided
            constantDataMode = !constantDataMode;
            if (constantDataMode) {
                setDataBusOutput();
            } else {
                setDataBusInput();
            }
            Serial.print("Constant data mode: ");
            Serial.println(constantDataMode ? "ON" : "OFF");
            break;
        }

        case 'h':
        case 'H':
            Serial.println("Commands:");
            Serial.println("  'r' = Reset target");
            Serial.println("  'c' = Toggle continuous clock mode");
            Serial.println("  's' = Single clock step");
            Serial.println("  'd' = Toggle constant data mode");
            Serial.println("  'd XX' = Set constant data value (hex)");
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

    if (continuousMode) {
        stepClock();
        delay(CLK_DELAY_US);
    }
}
