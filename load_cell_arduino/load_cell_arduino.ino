#include <EEPROM.h>

const uint8_t READ_SENSOR = 1;
const uint8_t ZERO = 2;
const uint8_t WAITING = 3;
uint8_t STATE = READ_SENSOR;

uint8_t data_pin = 11; // --> PB7
uint8_t clock_pin = 10; // --> PB6
uint8_t speed_pin = 0; // --> PD2
uint8_t pdwn_pin = A1; // --> PF6

uint8_t n_bits = 24;

unsigned long filtered = 0;
float alpha = 0.8;

uint8_t oversample_num = 16;

unsigned long zero_balance = 0;

uint8_t n_calibration = 25;

// calibration:
const long twentyg = 3096666;
const long teng = 1548142;
const long dx = twentyg-teng;

const bool DEBUG = true;

const int eeAddress = 0;   //Location we want the data to be put.

unsigned long readADS1231() {
  unsigned long data = 0;
  for (uint8_t i=0; i < n_bits; i++) { // bitshift data out
    // this is shifting MSB first
    PORTB ^= (1 << PB6);
    PORTB ^= (1 << PB6); // toggle twice
    data = data<<1;
    if (PINB & (1 << PB7)) { data++; }
  }
  
  PORTB ^= (1 << PB6);
  PORTB ^= (1 << PB6); // toggle twice
  return data;
}

void zeroADS1231() {
  unsigned long zero;
  for (uint8_t k = 0; k < n_calibration; k++) {
    unsigned long raw_val = 0;
    for (char j = 0; j < oversample_num; j++) {
      while(PINB & (1 << PB7)) { } // wait for data_pin to go low
      raw_val += readADS1231();
    }
    raw_val = raw_val >> 2; // decimate based on oversampling
    zero += raw_val; // accumulate
  }
  zero_balance = zero/n_calibration;
  filtered = zero_balance;
  if (DEBUG) {
    Serial.print("#zero balance: ");
    Serial.println(zero_balance);
    Serial1.print("#zero balance: ");
    Serial1.print(zero_balance);
    Serial1.print('\n');
  }
 
  EEPROM.put(eeAddress, zero_balance);
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200); //virtual serial to computer
  Serial1.begin(115200);  //hwardware usart to main board
  
  pinMode(speed_pin, OUTPUT);
  digitalWrite(speed_pin, HIGH);  // 80 samples per second (low = 10 samples per second)
  pinMode(pdwn_pin, OUTPUT);
  digitalWrite(pdwn_pin, HIGH);
  pinMode(clock_pin, OUTPUT);
  digitalWrite(clock_pin, LOW);
  pinMode(data_pin, INPUT);

  // get zero
//  zero_balance = 19121422; //on table
//  zero_balance = 18862371; //at 90 degrees
  EEPROM.get(eeAddress, zero_balance);
  filtered = zero_balance;
  if (DEBUG) {
    Serial.print("#zero balance: ");
    Serial.println(zero_balance);
  }
}

void loop() {

  if (Serial.available()) {
    char inByte = Serial.read();
    if (inByte == 'z') {
      STATE = ZERO;
      if (DEBUG) { Serial.println("#zeroing"); }
    } else if (inByte == 'g') {
      if (STATE == WAITING) {
        STATE = READ_SENSOR;
      }
    }
  }

  if (Serial1.available()) {
    char inByte = Serial1.read();
    if (inByte == 'z') {
      STATE = ZERO;
      if (DEBUG) { 
        Serial1.print("#zeroing");
        Serial1.print('\n'); 
      }
    } else if (inByte == 'g') {
      if (STATE == WAITING) {
        STATE = READ_SENSOR;
      }
    }
  }

  if (STATE == READ_SENSOR) {
    unsigned long raw_val = 0;
    for (char j = 0; j < oversample_num; j++) {
      while(PINB & (1 << PB7)) { } // wait for data_pin to go low
      raw_val += readADS1231();
    }
    raw_val = raw_val >> 2; // decimate based on oversampling
    filtered = alpha*raw_val + (1-alpha)*filtered; // smooth data
    
    long val = filtered-zero_balance;
    long raw = raw_val-zero_balance;
    float weight = float(val/float(dx))*10;
  
    if (DEBUG) { 
      Serial.print(weight,4); 
      Serial.print('\n');
    }
    Serial1.print(weight,4);
    Serial1.print('\n');
  } else if (STATE == ZERO) {
    zeroADS1231();
//    STATE = WAITING;
    STATE = READ_SENSOR;
  } else if (STATE == WAITING) {
    // do nothing
  }
}
