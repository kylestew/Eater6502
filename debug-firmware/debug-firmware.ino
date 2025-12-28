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

// Previous state for change detection
uint16_t prevAddr = 0;
uint8_t prevData  = 0;
bool prevRW       = true;

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
    Serial.println(rw ? "R" : "W");
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

        case 'h':
        case 'H':
            Serial.println("Commands:");
            Serial.println("  'r' = Reset target");
            Serial.println("  'c' = Toggle continuous clock mode");
            Serial.println("  's' = Single clock step");
            Serial.println("  'h' = Show this help");
            break;

        default:
            stepClock();
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
