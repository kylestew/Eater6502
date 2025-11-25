// Pin Configuration Constants
const int ADDR[]    = {22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52}; // A0-A15 (16 pins)
const int DATA[]    = {39, 41, 43, 45, 47, 49, 51, 53};                                 // D0-D7 (8 pins)
const int CLK_PIN   = 2;                                                                // Clock pin (output)
const int RESET_PIN = 3;                                                                // Reset pin (output)
const int RW_PIN    = 4;                                                                // Read/Write pin (input)

// Clock timing constants
const int CLK_DELAY_US = 100; // Clock pulse delay in microseconds

// State variables
unsigned long cycleCount = 0;
bool continuousMode      = false;

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
    // Pulse the clock pin HIGH then LOW
    digitalWrite(CLK_PIN, HIGH);
    digitalWrite(LED_BUILTIN, HIGH); // LED on when clock is high
    delayMicroseconds(CLK_DELAY_US); // Clock high time
    digitalWrite(CLK_PIN, LOW);
    digitalWrite(LED_BUILTIN, LOW);  // LED off when clock is low
    delayMicroseconds(CLK_DELAY_US); // Stabilization delay
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
    // Pulse reset pin LOW then HIGH
    // 6502 requires reset to be held LOW for sufficient time
    digitalWrite(RESET_PIN, LOW);
    delay(100); // Hold reset LOW for 100ms (typical 6502 requirement)
    digitalWrite(RESET_PIN, HIGH);
    delay(10); // Small delay after releasing reset
    cycleCount = 0;
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
