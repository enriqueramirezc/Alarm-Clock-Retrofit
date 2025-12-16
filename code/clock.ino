#define DATA_PIN 4
#define CLOCK_PIN 7
#define LATCH_PIN 6
#define CATHODE_2 2
#define CATHODE_1 5

void setup() {

 pinMode(DATA_PIN, OUTPUT);
 pinMode(CLOCK_PIN, OUTPUT);
 pinMode(LATCH_PIN, OUTPUT);
 pinMode(CATHODE_1, OUTPUT);
 pinMode(CATHODE_2, OUTPUT);
}

void shiftOut16(uint16_t data) {

 digitalWrite(LATCH_PIN, LOW);
 shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, (data >> 8)); // Chip 2
 shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, data & 0xFF); // Chip 1
 digitalWrite(LATCH_PIN, HIGH);
}

void loop() {

 // Turn on cathode 2, all segments on chip 1
 digitalWrite(CATHODE_2, HIGH);
 digitalWrite(CATHODE_1, LOW);
 shiftOut16(0x00FF); // All chip 1 outputs HIGH
 delay(5);

 // Turn on cathode 1, all segments on chip 1
 digitalWrite(CATHODE_2, LOW);
 digitalWrite(CATHODE_1, HIGH);
 shiftOut16(0x00FF); // All chip 1 outputs HIGH
 delay(5);
}
