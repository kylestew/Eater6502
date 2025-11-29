// Pin Configuration Constants
const int ADDR[]    = {22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52}; // A0-A15 (16 pins)
const int DATA[]    = {39, 41, 43, 45, 47, 49, 51, 53};                                 // D0-D7 (8 pins)
const int CLK_PIN   = 2;                                                                // Clock pin (output)
const int RESET_PIN = 4;                                                                // Reset pin (output)
const int RW_PIN    = 3;                                                                // Read/Write pin (input)

// Clock timing constants
const int CLK_DELAY_US = 100; // Clock pulse HIGH duration in microseconds

// State variables
unsigned long cycleCount = 0;
bool continuousMode      = false;
uint8_t lastWriteData    = 0;     // Store the data written during write cycles
uint16_t lastWriteAddr   = 0;     // Store the address written to
bool lastWasWrite        = false; // Track if last cycle was a write

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
    Serial.println("  Any key = Step clock");
    Serial.println("  'r' = Reset target");
    Serial.println("  'c' = Toggle continuous mode");
    Serial.println("  's' = Single step");
    Serial.println("  'h' = Show this help");
    Serial.println();
}

void stepClock() {
    // Read address and R/W BEFORE doing anything else
    uint16_t addr = readAddressBus();
    bool rw       = readRW();

    // Pulse the clock pin HIGH then LOW
    digitalWrite(CLK_PIN, HIGH);
    digitalWrite(LED_BUILTIN, HIGH); // LED on when clock is high

    // During clock HIGH, if it's a write cycle, capture the data the 6502 is driving
    if (!rw) {
        // Write cycle - read what the 6502 is putting on the data bus
        // Give the 6502 time to fully drive the bus after clock goes HIGH
        delayMicroseconds(20); // Delay to ensure 6502 has driven the bus

        // Read the data bus
        uint8_t writeData = 0;
        for (int n = 0; n < 8; n++) {
            if (digitalRead(DATA[n]) == HIGH) {
                writeData |= (1UL << n);
            }
        }

        // Verify by reading again
        delayMicroseconds(5);
        uint8_t verifyData = 0;
        for (int n = 0; n < 8; n++) {
            if (digitalRead(DATA[n]) == HIGH) {
                verifyData |= (1UL << n);
            }
        }

        // Use the verified data
        lastWriteData = verifyData;
        lastWriteAddr = addr;
        lastWasWrite  = true;

        // Remaining clock high time (100us total - 25us already spent)
        delayMicroseconds(CLK_DELAY_US - 25);
    } else {
        // Read cycle - just hold clock HIGH for specified duration
        delayMicroseconds(CLK_DELAY_US);
        lastWasWrite = false;
    }

    digitalWrite(CLK_PIN, LOW);
    digitalWrite(LED_BUILTIN, LOW); // LED off when clock is low

    // Small delay after clock goes LOW for stabilization
    delayMicroseconds(10);
}

uint16_t readAddressBus() {
    uint16_t address = 0;
    for (int n = 0; n < 16; n++) {
        if (digitalRead(ADDR[n]) == HIGH) {
            address |= (1UL << n);
        }
    }
    return address;
}

uint8_t readDataBus() {
    uint16_t addr = readAddressBus();
    bool rw       = readRW();

    // If the last cycle was a write and address matches, return the captured write data
    if (lastWasWrite && addr == lastWriteAddr) {
        return lastWriteData;
    }

    // Otherwise read from pins
    uint8_t data = 0;
    for (int n = 0; n < 8; n++) {
        if (digitalRead(DATA[n]) == HIGH) {
            data |= (1UL << n);
        }
    }
    return data;
}

bool readRW() { return digitalRead(RW_PIN) == HIGH; }

void printState() {
    uint16_t addr = readAddressBus();
    uint8_t data  = readDataBus();
    bool rw       = readRW();

    // Print cycle count (left padded to 4 digits)
    Serial.print("Cycle: ");
    if (cycleCount < 1000)
        Serial.print("0");
    if (cycleCount < 100)
        Serial.print("0");
    if (cycleCount < 10)
        Serial.print("0");
    Serial.print(cycleCount);
    Serial.print(" | ");

    // Print address (hex and binary)
    Serial.print("Addr: 0x");
    if (addr < 0x1000)
        Serial.print("0");
    if (addr < 0x100)
        Serial.print("0");
    if (addr < 0x10)
        Serial.print("0");
    Serial.print(addr, HEX);
    Serial.print(" (");
    for (int n = 15; n >= 0; n--) {
        Serial.print((addr >> n) & 1);
    }
    Serial.print(") | ");

    // Print data (hex and binary)
    Serial.print("Data: 0x");
    if (data < 0x10)
        Serial.print("0");
    Serial.print(data, HEX);
    Serial.print(" (");
    for (int n = 7; n >= 0; n--) {
        Serial.print((data >> n) & 1);
    }
    Serial.print(") | ");

    // Print R/W state
    Serial.print("R/W: ");
    Serial.print(rw ? "R" : "W");

    Serial.println();
}

void resetTarget() {
    // Ensure clock is LOW during reset
    digitalWrite(CLK_PIN, LOW);
    digitalWrite(LED_BUILTIN, LOW);

    // Pulse reset pin LOW then HIGH
    // 6502 requires reset to be held LOW for at least 2 clock cycles minimum
    // Using 100ms provides plenty of margin
    digitalWrite(RESET_PIN, LOW);
    delay(100); // Hold reset LOW for 100ms

    digitalWrite(RESET_PIN, HIGH);
    delay(10); // Small delay after releasing reset before first clock cycle

    // Reset state variables
    cycleCount    = 0;
    lastWriteData = 0;
    lastWriteAddr = 0;
    lastWasWrite  = false;
    Serial.println("Target reset!");
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
            Serial.print("Continuous mode: ");
            Serial.println(continuousMode ? "ON" : "OFF");
            break;

        case 's':
        case 'S':
            // Single step (explicit)
            stepClock();
            cycleCount++;
            printState();
            break;

        case 'h':
        case 'H':
            Serial.println("Commands:");
            Serial.println("  Any key = Step clock");
            Serial.println("  'r' = Reset target");
            Serial.println("  'c' = Toggle continuous mode");
            Serial.println("  's' = Single step");
            Serial.println("  'h' = Show this help");
            break;

        default:
            // Any other key = step clock
            stepClock();
            cycleCount++;
            printState();
            break;
        }

        // Clear any remaining characters in buffer
        while (Serial.available() > 0) {
            Serial.read();
        }
    }

    // Continuous mode: step automatically
    if (continuousMode) {
        stepClock();
        cycleCount++;
        printState();
        delay(100); // Delay between steps in continuous mode
    }
}
