// Shift register pins
#define DATA_PIN 4
#define CLOCK_PIN 6
#define LATCH_PIN 7

// Catodos
#define CATHODE_2 2 // controla: 1bc, 2abc, 3defg, 4abc, AM(L2)
#define CATHODE_1 5 // controla: 2defg, 3abc, 4defg, PM(L1), colon

// Variables de tiempo
int hours = 12;
int minutes = 0;
int seconds = 0;
unsigned long lastMillis = 0;

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

    pinMode(DATA_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(CATHODE_1, OUTPUT);
    pinMode(CATHODE_2, OUTPUT);

    // cathodes initially off
    digitalWrite(CATHODE_1, LOW);
    digitalWrite(CATHODE_2, LOW);
}

void shiftOut16(uint16_t data) {

    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, (data >> 8));
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, data & 0xFF);
    digitalWrite(LATCH_PIN, HIGH);
}



void displayTime() {

    int displayHour = hours;
    bool isPM = (hours >= 12);
    if (displayHour == 0) displayHour = 12;
    if (displayHour > 12) displayHour -= 12;

    int d1 = displayHour / 10;
    int d2 = displayHour % 10;
    int d3 = minutes / 10;
    int d4 = minutes % 10;

    uint16_t data;

    // CATHODE 2 PHASE
    // Bit: 0=1bc, 1=2a, 2=2b, 3=2c, 4=3f, 5=3g, 6=3d, 7=3e, 8=4b, 9=4c, 10=4a, 13=AM

    // primero apago los catodos

    digitalWrite(CATHODE_1, LOW);
    digitalWrite(CATHODE_2, LOW);
    data = 0;

    // Digit 1: segments B, C
    if (d1 == 1) {
        data |= (1 << 0);
    }

    // Digit 2: segments A, B, C
    if (seg[d2][0]) data |= (1 << 1); // 2a
    if (seg[d2][1]) data |= (1 << 2); // 2b
    if (seg[d2][2]) data |= (1 << 3); // 2c

    // Digit 3: segments D, E, F, G
    if (seg[d3][3]) data |= (1 << 6); // 3d
    if (seg[d3][4]) data |= (1 << 7); // 3e
    if (seg[d3][5]) data |= (1 << 4); // 3f
    if (seg[d3][6]) data |= (1 << 5); // 3g

    // Digit 4: segments A, B, C
    if (seg[d4][0]) data |= (1 << 10); // 4a
    if (seg[d4][1]) data |= (1 << 8); // 4b
    if (seg[d4][2]) data |= (1 << 9); // 4c

    // AM light (L2)
    if (!isPM) {
        data |= (1 << 13);
    }

    // Shift data, then turn on cathode

    shiftOut16(data);
    digitalWrite(CATHODE_2, HIGH);
    delay(5);


    // CATHODE 1 PHASE
    // Bit: 0=2e, 1=2f, 2=2g, 3=2d, 4=3a, 5=3b, 6=3c, 7=4e, 8=4g, 9=4d, 10=4f, 11=colon, 12=PM

    // apagar catodos
    digitalWrite(CATHODE_1, LOW);
    digitalWrite(CATHODE_2, LOW);
    data = 0;

    // Digit 2: segments D, E, F, G
    if (seg[d2][3]) data |= (1 << 3); // 2d
    if (seg[d2][4]) data |= (1 << 0); // 2e
    if (seg[d2][5]) data |= (1 << 1); // 2f
    if (seg[d2][6]) data |= (1 << 2); // 2g

    // Digit 3: segments A, B, C
    if (seg[d3][0]) data |= (1 << 4); // 3a
    if (seg[d3][1]) data |= (1 << 5); // 3b
    if (seg[d3][2]) data |= (1 << 6); // 3c

    // Digit 4: segments D, E, F, G
    if (seg[d4][3]) data |= (1 << 9); // 4d
    if (seg[d4][4]) data |= (1 << 7); // 4e
    if (seg[d4][5]) data |= (1 << 10); // 4f
    if (seg[d4][6]) data |= (1 << 8); // 4g

    // PM light (L1)
    if (isPM) {
        data |= (1 << 12);
    }

    // Colon always on
    data |= (1 << 11);

    // Shift data, then turn on cathode
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



void loop() {
    updateTime();
    displayTime();
}

