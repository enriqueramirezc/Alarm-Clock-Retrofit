
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

// Buzzer pin
#define BUZZER A4

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
const unsigned long snoozeDuration = 5UL * 60UL * 1000UL; // 5 minutes in ms

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
    // pinMode(ONOFF, INPUT_PULLDOWN);
    
    // Buzzer
    pinMode(BUZZER, OUTPUT);
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

bool isValidDigit(int digit, int position) {

    // Position 0: first hour digit (0 or 1)
    if (position == 0) {
        return (digit == 0 || digit == 1);
    }

    // Position 1: second hour digit
    if (position == 1) {
        if (inputDigits[0] == 0) {
            return (digit >= 1 && digit <= 9); // 01-09
        } else {
            return (digit >= 0 && digit <= 2); // 10-12
        }
    }

    // Position 2: first minute digit (0-5)
    if (position == 2) {
        return (digit >= 0 && digit <= 5);
    }

    // Position 3: second minute digit (0-9)
    if (position == 3) {
        return (digit >= 0 && digit <= 9);
    }
    return false;
}

void handleKey(char key) {
    if (key == 0) return;
    
    static char lastKey = 0;
    static unsigned long lastKeyTime = 0;
    
    // Debounce
    if (key == lastKey && millis() - lastKeyTime < 300) return;
    lastKey = key;
    lastKeyTime = millis();
    
    // SNOOZE: snooze alarm for 5 minutes
    if (key == 'S') {
        if (alarmTriggered) {
            alarmSnoozed = true;
            snoozeStart = millis();
            alarmTriggered = false;
            noTone(BUZZER);
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
    
    // AL: enter alarm setting mode
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
    
    // PM: toggle PM in setting mode
    if (key == 'P') {
        if (mode == 1 || mode == 2) {
            inputPM = !inputPM;
        }
        return;
    }
    
    // ENT: confirm setting
    if (key == 'E') {
        if (mode == 1 || mode == 2) {
            // Only save if at least one digit was entered
            if (inputIndex > 0) {
                int newHours = inputDigits[0] * 10 + inputDigits[1];
                int newMinutes = inputDigits[2] * 10 + inputDigits[3];
                
                // Handle special case: if only 1 digit entered for hours, treat as single digit hour
                if (inputIndex == 1) {
                    newHours = inputDigits[0];
                    newMinutes = 0;
                } else if (inputIndex == 2) {
                    newHours = inputDigits[0] * 10 + inputDigits[1];
                    newMinutes = 0;
                } else if (inputIndex == 3) {
                    newHours = inputDigits[0] * 10 + inputDigits[1];
                    newMinutes = inputDigits[2] * 10;
                }
                
                // Validate hours (1-12)
                if (newHours >= 1 && newHours <= 12 && newMinutes >= 0 && newMinutes <= 59) {
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
                        lastMillis = millis();
                    } else {
                        alarmHours = newHours;
                        alarmMinutes = newMinutes;
                        alarmEnabled = true;
                    }
                }
            }
            mode = 0;
        }
        return;
    }
    
    // Number keys
    if (key >= '0' && key <= '9') {
        if (mode == 1 || mode == 2) {
            if (inputIndex < 4) {
                int digit = key - '0';
                if (isValidDigit(digit, inputIndex)) {
                    inputDigits[inputIndex] = digit;
                    inputIndex++;
                }
            }
        }
        return;
    }
}

void playAlarm() {
    static unsigned long lastTone = 0;
    static int toneState = 0;
    
    unsigned long now = millis();
    
    switch (toneState) {
        case 0:
            tone(BUZZER, 700);
            if (now - lastTone > 100) {
                lastTone = now;
                toneState = 1;
            }
            break;
        case 1:
            tone(BUZZER, 1000);
            if (now - lastTone > 100) {
                lastTone = now;
                toneState = 2;
            }
            break;
        case 2:
            tone(BUZZER, 2000);
            if (now - lastTone > 100) {
                lastTone = now;
                toneState = 3;
            }
            break;
        case 3:
            noTone(BUZZER);
            lastTone = now;
            toneState = 0;
            break;
    }
}

void displayTime() {
    int displayHour, displayMinute;
    bool isPM;
    bool showColon = true;
    bool showL5 = false;
    
    if (mode == 0) {
        // Normal mode: show current time
        displayHour = hours;
        isPM = (hours >= 12);
        if (displayHour == 0) displayHour = 12;
        if (displayHour > 12) displayHour -= 12;
        displayMinute = minutes;
        
        // Colon always on in normal mode
        showColon = true;
        
        // Alarm indicator
        showL5 = alarmEnabled;
        
        // Blink L5 when alarm triggered
        if (alarmTriggered) {
            showL5 = (millis() / 250) % 2;
        }
    } else {
        // Setting mode: show input
        displayHour = inputDigits[0] * 10 + inputDigits[1];
        displayMinute = inputDigits[2] * 10 + inputDigits[3];
        isPM = inputPM;
        showL5 = (mode == 2); // Show L5 when setting alarm
        
        // Blink colon in setting mode
        showColon = (millis() / 500) % 2;
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
    // ON/OFF switch turns off alarm
    // if (digitalRead(ONOFF) == LOW) {
    if (digitalRead(ONOFF) == HIGH) {
        if (alarmTriggered || alarmSnoozed) {
            alarmTriggered = false;
            alarmSnoozed = false;
            alarmEnabled = false;
            noTone(BUZZER);
        }
        return;
    }
    
    if (!alarmEnabled) return;
    
    // after 5 minutes, alarm gets triggered again
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
    
    // Play alarm sound continuously once triggered
    if (alarmTriggered) {
        playAlarm();
    }
}

void loop() {
    updateTime();
    checkAlarm();
    
    char key = scanKey();
    handleKey(key);    
    displayTime();
}
