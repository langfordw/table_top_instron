#include <avr/delay.h>
#include <avr/interrupt.h>

//#define F_CPU 16000000
#define USART_BAUDRATE 115200
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

const uint8_t STEADY = 1;
const uint8_t ACCEL = 2;
const uint8_t DECCEL = 3;
const uint8_t STOPPED = 4;
const uint8_t DONE = 5;
volatile uint8_t STEPPER_STATE = STOPPED;
uint8_t LAST_STEPPER_STATE = STEPPER_STATE;

volatile long current_position = 0;
long desired_position = 0;
volatile long n_steps_since_last_move = 0;
bool current_dir = 1; //1 is CW, 0 is CCW

uint8_t step_pin = 18;
uint8_t dir_pin = 19;
uint8_t enable_pin = 10;

//const float base_speed = 0.1;
//const float slew_speed = 50.0;
//const float acceleration = 0.25;
const float base_speed = 0.0001;
const float slew_speed = 0.5;
const float acceleration = 0.001;
const unsigned long timer_freq = 256;
const float accel_dist = (slew_speed*slew_speed - base_speed*base_speed)/(2*acceleration);
const float initial_step_delay = timer_freq/sqrt(base_speed*base_speed+2*acceleration);
const float slew_step_delay = timer_freq/slew_speed;
const float multiplier = acceleration/(timer_freq*timer_freq);
float step_delay = initial_step_delay;
const int epsilon = 1;

const bool DEBUG = false;

String inputString = "";         // a string to hold incoming data
volatile boolean stringComplete = false;  // whether the string is complete

unsigned long last_time;

long positions[] = {16000, 0};
int index = 0;

int dir = 1;

//float readLoadCell() {
//  if (Serial1.available()) {
//    while (Serial1.available()) {
//      char inChar = (char)Serial1.read();
//      inputString += inChar;
//      if (inChar == '\n') {
////        Serial.print(millis());
////        Serial.print(",");
//        Serial.print(current_position);
//        Serial.print(",");
//        Serial.print(inputString);
//        inputString = "";
//      }
//    }
//  }
//}

void setDir(long vector) {
  if (vector > 0) {
    digitalWrite(dir_pin, HIGH);
    dir = 1;
  } else {
    digitalWrite(dir_pin, LOW);
    dir = -1;
  }
}

void toggleStep() {
//  int dt = micros()-last_time;
//  Serial.print(dt);                   // this is on the order of 50us (with loadCellRead + passthrough)
//  Serial.print(", ");
//  Serial.println(step_delay);         // this is on the order of 500us (with 2.0 slew speed)
  if (micros()-last_time > step_delay) {
      PORTF ^= (1 << PF7); //pin 18
      PORTB ^= (1 << PB4); //pin 8 for debugging
      PORTF ^= (1 << PF7); //pin 18
      PORTB ^= (1 << PB4); //pin 8 for debugging
      current_position += dir;
      n_steps_since_last_move++; 
      last_time = micros();
  }
}
  
void setup() {
  Serial.begin(115200);

  pinMode(step_pin, OUTPUT);
  pinMode(dir_pin, OUTPUT);
  pinMode(enable_pin, OUTPUT);
  
  digitalWrite(enable_pin, LOW);
  digitalWrite(dir_pin, HIGH);

  if (DEBUG) {
    Serial.print("#accel_dist: ");
    Serial.println(accel_dist);
    Serial.print("#initial_step_delay: ");
    Serial.println(initial_step_delay);
    Serial.print("#slew_step_delay: ");
    Serial.println(slew_step_delay);
    Serial.print("#multiplier: ");
    Serial.println(multiplier,6);
  }

  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 31250;            // compare match register 16MHz --> 256Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS10);    // no prescaler
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt

  // default arduino is 8 databits, no parity, 1 stopbit
  UCSR1B = (1 << RXEN1);   // Turn on the reception circuitry
  UCSR1C = (1 << UCSZ10) | (1 << UCSZ11); // Use 8-bit character sizes

  UBRR1H = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register
  UBRR1L = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register

  UCSR1B |= (1 << RXCIE1); // Enable the USART Recieve Complete interrupt (USART_RXC)

  interrupts();             // enable all interrupts

  last_time = millis();
  inputString.reserve(200);
  STEPPER_STATE = STOPPED;
  current_position = 0;

  while (!Serial) {
    ;
  }
}

ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
  if (STEPPER_STATE == ACCEL) {
    step_delay = step_delay*(1-multiplier*step_delay*step_delay);
  } else if (STEPPER_STATE == DECCEL) {
    step_delay = step_delay*(1+multiplier*step_delay*step_delay);
  } else if (STEPPER_STATE == STEADY) {
    //step_delay = step_delay;
  }
  if (step_delay < slew_step_delay) {
    step_delay = slew_step_delay;
  } else if (step_delay > initial_step_delay) {
    step_delay = initial_step_delay;
  }
}

ISR(USART1_RX_vect)
{ 
    char inChar = UDR1;
    inputString += inChar;
    if (inChar == '\n') { 
      stringComplete = true; 
    }   
}

void loop() {
//  PORTE ^= (1 << PE6);

  if (stringComplete) {
    Serial.print(current_position);
    Serial.print(",");
    Serial.print(inputString);
    inputString = "";
    stringComplete = false;
  }

  // user controls
  // x = stop
  // r = resume
  // z = go to zero
  // g = go again
  if (Serial.available()) {
    char inByte = Serial.read();
    if (inByte == 'x') {
      LAST_STEPPER_STATE = STEPPER_STATE;
      STEPPER_STATE = DONE;
      if(DEBUG) { Serial.println("#user interrupt --> done (stop!)"); }
    } else if (inByte == 'z') {
      if(DEBUG) { Serial.println("#user interrupt --> stopped (go to zero)"); }
      STEPPER_STATE = STOPPED;
      index = 1;
    } else if (inByte == 'r') {
      if(DEBUG) { Serial.println("#user interrupt --> resume"); }
      STEPPER_STATE = LAST_STEPPER_STATE;
      UCSR1B |= (1 << RXCIE1); // Enable the USART Recieve Complete interrupt (USART_RXC)
    } else if (inByte == 'g') {
      if(DEBUG) { Serial.println("#user interrupt --> stopped (go again)"); }
      if (STEPPER_STATE == DONE) {
        STEPPER_STATE = STOPPED;
        index = 0;
        UCSR1B |= (1 << RXCIE1); // Enable the USART Recieve Complete interrupt (USART_RXC)
      }
    }
  }
  
  switch(STEPPER_STATE) {
    case STEADY:
      toggleStep();
      if (abs(desired_position-current_position)<accel_dist) {
        STEPPER_STATE = DECCEL;
        if(DEBUG) { Serial.println("#steady --> deccel"); }
      } else if (abs(desired_position-current_position) < epsilon) {
        STEPPER_STATE = STOPPED;
        if(DEBUG) { Serial.println("#steady --> stopped"); }
      }
      break;
    case STOPPED:
      if (index <= 1) {
        desired_position = positions[index];
        index++;
        setDir(desired_position-current_position);
        n_steps_since_last_move = 0;
        if(DEBUG) { Serial.println("#stopped --> new position"); }
        STEPPER_STATE = ACCEL;
      } else {
        STEPPER_STATE = DONE;
        if(DEBUG) { Serial.println("#stopped --> done"); }
      }
      break;
    case ACCEL:
      toggleStep();
      if (n_steps_since_last_move > accel_dist) {
        STEPPER_STATE = STEADY;
        if(DEBUG) { Serial.println("#accel --> steady"); }
      } else if (abs(desired_position-current_position)<accel_dist) {
        STEPPER_STATE = DECCEL;
        if(DEBUG) { Serial.println("#accel --> deccel"); }
      } else if (abs(desired_position-current_position) < epsilon) {
        STEPPER_STATE = STOPPED;
        if(DEBUG) { Serial.println("#accel --> stopped"); }
      }
      break;
    case DECCEL:
      toggleStep();
      if (abs(desired_position-current_position) < epsilon) {
        STEPPER_STATE = STOPPED;
        if(DEBUG) { Serial.println("#deccel --> stopped"); }
      }
      break;
    case DONE:
      // do nothing
      UCSR1B &= ~(1 << RXCIE1); // disable the USART Recieve Complete interrupt (USART_RXC)
      break;
  }
}

