// Shift register pins
#define DATA_PIN 4
#define CLOCK_PIN 6
#define LATCH_PIN 7

// Cathode pins
#define CATHODE_2 2
#define CATHODE_1 5

// Keypad pins
#define ROW1 8
#define ROW2 9
#define ROW3 10
#define ROW4 11
#define COL5 A0
#define COL6 A1
#define COL7 A2
#define COL8 A3
#define ONOFF 12

// Time variables
int hours = 12;
int minutes = 0;
int seconds = 0;
unsigned long lastMillis = 0;

// Alarm variables
int alarmHours = 12;
int alarmMinutes = 0;
bool alarmEnabled = false;
bool alarmTriggered = false;
bool alarmSnoozed = false;
unsigned long snoozeStart = 0;
const int snoozeDuration = 5 * 60 * 1000; // 5 minutes in ms

// Mode: 0=normal, 1=set time, 2=set alarm
int mode = 0;
int inputDigits[4] = {0, 0, 0, 0};
int inputIndex = 0;
bool inputPM = false;

// Segment patterns: A=0, B=1, C=2, D=3, E=4, F=5, G=6
const bool seg[10][7] = {
    {1,1,1,1,1,1,0}, // 0
    {0,1,1,0,0,0,0}, // 1
    {1,1,0,1,1,0,1}, // 2
    {1,1,1,1,0,0,1}, // 3
    {0,1,1,0,0,1,1}, // 4
    {1,0,1,1,0,1,1}, // 5
    {1,0,1,1,1,1,1}, // 6
    {1,1,1,0,0,0,0}, // 7
    {1,1,1,1,1,1,1}, // 8
    {1,1,1,1,0,1,1} // 9
};

void setup() {

    // Shift register pins
    pinMode(DATA_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(CATHODE_1, OUTPUT);
    pinMode(CATHODE_2, OUTPUT);
    digitalWrite(CATHODE_1, LOW);
    digitalWrite(CATHODE_2, LOW);

    // Keypad rows as outputs
    pinMode(ROW1, OUTPUT);
    pinMode(ROW2, OUTPUT);
    pinMode(ROW3, OUTPUT);
    pinMode(ROW4, OUTPUT);
    digitalWrite(ROW1, HIGH);
    digitalWrite(ROW2, HIGH);
    digitalWrite(ROW3, HIGH);
    digitalWrite(ROW4, HIGH);

    // Keypad columns as inputs with pullup
    pinMode(COL5, INPUT_PULLUP);
    pinMode(COL6, INPUT_PULLUP);
    pinMode(COL7, INPUT_PULLUP);
    pinMode(COL8, INPUT_PULLUP);

    // ON/OFF switch
    pinMode(ONOFF, INPUT_PULLUP);
}

void shiftOut16(uint16_t data) {
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, (data >> 8));
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, data & 0xFF);
    digitalWrite(LATCH_PIN, HIGH);
}

char scanKey() {
    char key = 0;

    // Row 1: PM, 2, 1, 0
    digitalWrite(ROW1, LOW);
    digitalWrite(ROW2, HIGH);
    digitalWrite(ROW3, HIGH);
    digitalWrite(ROW4, HIGH);
    delayMicroseconds(10);
    if (digitalRead(COL5) == LOW) key = 'P';
    else if (digitalRead(COL6) == LOW) key = '2';
    else if (digitalRead(COL7) == LOW) key = '1';
    else if (digitalRead(COL8) == LOW) key = '0';

    // Row 2: ENT, 3, 5, 4
    digitalWrite(ROW1, HIGH);
    digitalWrite(ROW2, LOW);
    delayMicroseconds(10);
    if (digitalRead(COL5) == LOW) key = 'E';
    else if (digitalRead(COL6) == LOW) key = '3';
    else if (digitalRead(COL7) == LOW) key = '5';
    else if (digitalRead(COL8) == LOW) key = '4';

    // Row 3: 7, 6, 9, 8
    digitalWrite(ROW2, HIGH);
    digitalWrite(ROW3, LOW);
    delayMicroseconds(10);
    if (digitalRead(COL5) == LOW) key = '7';
    else if (digitalRead(COL6) == LOW) key = '6';
    else if (digitalRead(COL7) == LOW) key = '9';
    else if (digitalRead(COL8) == LOW) key = '8';

    // Row 4: SNOOZE, -, AL, TM
    digitalWrite(ROW3, HIGH);
    digitalWrite(ROW4, LOW);
    delayMicroseconds(10);
    if (digitalRead(COL5) == LOW) key = 'S';
    else if (digitalRead(COL7) == LOW) key = 'A';
    else if (digitalRead(COL8) == LOW) key = 'T';
    digitalWrite(ROW4, HIGH);
    return key;
}

void handleKey(char key) {
    if (key == 0) return;
    static char lastKey = 0;
    static unsigned long lastKeyTime = 0;

    // Debounce
    if (key == lastKey && millis() - lastKeyTime < 300) return;
    lastKey = key;
    lastKeyTime = millis();

    // Stop alarm or snooze
    if (key == 'S') {
        if (alarmTriggered) {
            alarmSnoozed = true;
            snoozeStart = millis();
            alarmTriggered = false;
        }
        return;
    }
 
    // TM: enter time setting mode
    if (key == 'T') {
        mode = 1;
        inputIndex = 0;
        inputPM = false;
        inputDigits[0] = 0;
        inputDigits[1] = 0;
        inputDigits[2] = 0;
        inputDigits[3] = 0;
        return;
    }
 
    // AL: nter alarm setting mode or toggle alarm
    if (key == 'A') {
        if (mode == 0) {
            mode = 2;
            inputIndex = 0;
            inputPM = false;
            inputDigits[0] = 0;
            inputDigits[1] = 0;
            inputDigits[2] = 0;
            inputDigits[3] = 0;
        }
        return;
    }
 
    // PM:toggle PM in setting mode
    if (key == 'P') {
        if (mode == 1 || mode == 2) {
            inputPM = !inputPM;
        }
        return;
    }

    // ENT:confirm setting
    if (key == 'E') {
        if (mode == 1 || mode == 2) {
            int newHours = inputDigits[0] * 10 + inputDigits[1];
            int newMinutes = inputDigits[2] * 10 + inputDigits[3];

            // Convert 12-hour to 24-hour
            if (newHours == 12) {
                newHours = inputPM ? 12 : 0;
            } else if (inputPM) {
                newHours = newHours + 12;
            }

            if (mode == 1) {
                hours = newHours;
                minutes = newMinutes;
                seconds = 0;
                lastMillis = millis(); // Reset time counter
            } else {
                alarmHours = newHours;
                alarmMinutes = newMinutes;
                alarmEnabled = true;
            }
            mode = 0;
        }
        return;
    }

    // Number keys
    if (key >= '0' && key <= '9') {
        if (mode == 1 || mode == 2) {
            if (inputIndex < 4) {
                inputDigits[inputIndex] = key - '0';
                inputIndex++;
            }
        }
        return;
    }
}

void displayTime() {
    int displayHour, displayMinute;
    bool isPM;
    bool showColon = true;
    bool showL5 = false;

    if (mode == 0) {

        // Normal mode:show current time
        displayHour = hours;
        isPM = (hours >= 12);
        if (displayHour == 0) displayHour = 12;
        if (displayHour > 12) displayHour -= 12;
        displayMinute = minutes;


        // // Blink colon every second

        // showColon = (millis() / 500) % 2;


        // Indica si la alarma esta puesta
        showL5 = alarmEnabled;


        // Blink L5 when alarm triggered
        if (alarmTriggered) {
            showL5 = (millis() / 250) % 2;
        }
    } else {

        // Setting mode:show input
        displayHour = inputDigits[0] * 10 + inputDigits[1];
        displayMinute = inputDigits[2] * 10 + inputDigits[3];
        isPM = inputPM;
        showL5 = (mode == 2); // Show L5 when setting alarm
    }

    int d1 = displayHour / 10;
    int d2 = displayHour % 10;
    int d3 = displayMinute / 10;
    int d4 = displayMinute % 10;

    uint16_t data;


    // CATHODE 2 PHASE
    digitalWrite(CATHODE_1, LOW);
    digitalWrite(CATHODE_2, LOW);

    data = 0;

    if (d1 == 1) data |= (1 << 0);
    if (seg[d2][0]) data |= (1 << 1);
    if (seg[d2][1]) data |= (1 << 2);
    if (seg[d2][2]) data |= (1 << 3);
    if (seg[d3][3]) data |= (1 << 6);
    if (seg[d3][4]) data |= (1 << 7);
    if (seg[d3][5]) data |= (1 << 4);
    if (seg[d3][6]) data |= (1 << 5);
    if (seg[d4][0]) data |= (1 << 10);
    if (seg[d4][1]) data |= (1 << 8);
    if (seg[d4][2]) data |= (1 << 9);
    if (!isPM) data |= (1 << 13); // AM
    if (showL5) data |= (1 << 14); // L5 alarm indicator

    shiftOut16(data);
    digitalWrite(CATHODE_2, HIGH);
    delay(5);


    // CATHODE 1 PHASE
    digitalWrite(CATHODE_1, LOW);
    digitalWrite(CATHODE_2, LOW);

    data = 0;

    if (seg[d2][3]) data |= (1 << 3);
    if (seg[d2][4]) data |= (1 << 0);
    if (seg[d2][5]) data |= (1 << 1);
    if (seg[d2][6]) data |= (1 << 2);
    if (seg[d3][0]) data |= (1 << 4);
    if (seg[d3][1]) data |= (1 << 5);
    if (seg[d3][2]) data |= (1 << 6);
    if (seg[d4][3]) data |= (1 << 9);
    if (seg[d4][4]) data |= (1 << 7);
    if (seg[d4][5]) data |= (1 << 10);
    if (seg[d4][6]) data |= (1 << 8);
    if (isPM) data |= (1 << 12); // PM
    if (showColon) data |= (1 << 11); // Colon

    shiftOut16(data);
    digitalWrite(CATHODE_1, HIGH);
    delay(5);
}

void updateTime() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastMillis >= 1000) {
        lastMillis = currentMillis;
        seconds++;
        if (seconds >= 60) {
            seconds = 0;
            minutes++;
            if (minutes >= 60) {
                minutes = 0;
                hours++;
                if (hours >= 24) {
                    hours = 0;
                }
            }
        }
    }
}

void checkAlarm() {
    if (!alarmEnabled) return;
 
    // Check snooze
    if (alarmSnoozed) {
        if (millis() - snoozeStart >= snoozeDuration) {
            alarmSnoozed = false;
            alarmTriggered = true;
        }
        return;
    }
 
    // Check alarm time
    if (hours == alarmHours && minutes == alarmMinutes && seconds == 0) {
        alarmTriggered = true;
    }
 
    // ON/OFF switch turns off alarm
    if (digitalRead(ONOFF) == LOW && alarmTriggered) {
        alarmTriggered = false;
        alarmEnabled = false;
    }
}

void loop() {
    updateTime();
    checkAlarm();

    char key = scanKey();
    handleKey(key);

    displayTime();
}
