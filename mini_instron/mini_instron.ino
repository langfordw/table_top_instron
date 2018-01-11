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
const uint8_t STOPPING = 6;

const uint8_t STANDARD = 1;
const uint8_t CONFIGURATION = 2;

volatile uint8_t SERIAL_STATE = STANDARD;
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
//float base_speed = 0.0001;
//float slew_speed = 0.25;
//float acceleration = 0.001;
float accel_dist, initial_step_delay, slew_step_delay, multiplier, step_delay;
const unsigned long timer_freq = 256;
const int epsilon = 1;
bool motor_enabled = false;

const bool DEBUG = true;

String inputString = "";         // a string to hold incoming data
volatile boolean stringComplete = false;  // whether the string is complete

unsigned long last_time;

float travel = 0.5;
long positions[2] = {0, 0};
int index = 0;

int dir = 1;

struct param {
  float val; 
  char id; 
  String desc;
};

param travelParam = {0.5, 't', "Travel Distance"}; 
param baseSpeedParam = {0.0001, 'b', "Base Speed"}; 
param slewSpeedParam = {0.25, 's', "Slew Speed"}; 
param accelerationParam = {0.001, 'a', "Acceleration"}; 

const int nParams = 4;
struct param *params[] = {&travelParam, &baseSpeedParam, &slewSpeedParam, &accelerationParam};
//*params[0] = travelParam;// = [travelParam, baseSpeedParam, slewSpeedParam, accelerationParam];
//params[0]= {travelParam, baseSpeedParam, slewSpeedParam, accelerationParam};


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

void debug(char *fmt, ...) {
    char tmp[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(tmp, 128, fmt, args);
    va_end(args);
    Serial.print(tmp);
}

void calculateMovementParameters() {
  accel_dist = (slewSpeedParam.val*slewSpeedParam.val - baseSpeedParam.val*baseSpeedParam.val)/(2*accelerationParam.val);
  initial_step_delay = timer_freq/sqrt(baseSpeedParam.val*baseSpeedParam.val+2*accelerationParam.val);
  slew_step_delay = timer_freq/slewSpeedParam.val;
  multiplier = accelerationParam.val/(timer_freq*timer_freq);
  step_delay = initial_step_delay;
  positions[0] = long(travelParam.val*5039.3700787);
//  Serial.println(positions[0]);
}

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

void printControls() {
  Serial.println("#User Controls:");
  Serial.println("#x --> stop");
  Serial.println("#g --> start (go)");
  Serial.println("#r --> resume");
  Serial.println("#z --> go to zero");
  Serial.println("#e --> enable motor");
  Serial.println("#d --> disable motor");
  Serial.println("#s --> stream load cell data");
  Serial.println("#t --> stop load cell data");
  Serial.println("#! --> enter setup mode");
  Serial.println("#q --> exit setup mode");
//  Serial.println("#l --> zero load cell");
}
  
void setup() {
  Serial.begin(115200);

  constructJSONObject();
  calculateMovementParameters();

  pinMode(step_pin, OUTPUT);
  pinMode(dir_pin, OUTPUT);
  pinMode(enable_pin, OUTPUT);
  
  digitalWrite(enable_pin, LOW);
  motor_enabled = true;
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
  UCSR1B = (1 << RXEN1) | (1 << TXEN1);   // Turn on the reception circuitry
  UCSR1C = (1 << UCSZ10) | (1 << UCSZ11); // Use 8-bit character sizes

  UBRR1H = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register
  UBRR1L = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register

//  UCSR1B |= (1 << RXCIE1); // Enable the USART Recieve Complete interrupt (USART_RXC)

  interrupts();             // enable all interrupts

  last_time = millis();
  inputString.reserve(200);
  STEPPER_STATE = STOPPING;
  current_position = 0;

  while (!Serial) {
    ;
  }

  printControls();
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

  switch(SERIAL_STATE) {
    case(STANDARD):
      // user controls
      // x = stop
      // r = resume
      // z = go to zero
      // g = go again
      // e = enable motor
      // d = disable motor
      if (Serial.available()) {
        char inByte = Serial.read();
        if (inByte == 'x') {
          LAST_STEPPER_STATE = STEPPER_STATE;
          STEPPER_STATE = STOPPING;
          if(DEBUG) { 
             Serial.println("#user interrupt --> done (stop!)"); 
             Serial.println("#EOE"); 
          }
        } else if (inByte == 'z') {
          if(DEBUG) { Serial.println("#user interrupt --> stopped (go to zero)"); }
          if (!motor_enabled) {
            if(DEBUG) { Serial.println("#motor was disabled, re-enabling"); }
            digitalWrite(enable_pin, LOW);
            motor_enabled = true;
          }
          STEPPER_STATE = STOPPED;
          index = 1;
        } else if (inByte == 'r') {
          if(DEBUG) { Serial.println("#user interrupt --> resume"); }
          if (!motor_enabled) {
            if(DEBUG) { Serial.println("#motor was disabled, re-enabling"); }
            digitalWrite(enable_pin, LOW);
            motor_enabled = true;
          }
          STEPPER_STATE = LAST_STEPPER_STATE;
          UCSR1B |= (1 << RXCIE1); // Enable the USART Recieve Complete interrupt (USART_RXC)
        } else if (inByte == 'g') {
          if(DEBUG) { Serial.println("#user interrupt --> stopped (go again)"); }
          
          if (STEPPER_STATE == DONE) {
            if (!motor_enabled) {
              if(DEBUG) { Serial.println("#motor was disabled, re-enabling"); }
              digitalWrite(enable_pin, LOW);
              motor_enabled = true;
            }
            STEPPER_STATE = STOPPED;
            index = 0;
            UCSR1B |= (1 << RXCIE1); // Enable the USART Recieve Complete interrupt (USART_RXC)
          }
        } else if (inByte == 'l') {
          if(DEBUG) { Serial.println("#user interrupt --> zeroing load cell"); }
          Serial.write('z');
    //      while ((UCSR1A & (1 << UDRE1)) == 0) {}; // Do nothing until UDR is ready for more data to be written to it
    //      UDR1 = 'z'; // Echo back the received byte back to the computer
    //      while ((UCSR1A & (1 << RXC1)) == 0) {}; // Do nothing until data have been received and is ready to be read from UDR
    //      Serial.println(UDR1); // Fetch the received byte value into the variable "ByteReceived"
        } else if (inByte == 'e') {
          if(DEBUG) { Serial.println("#user interrupt --> enable motor"); }
          digitalWrite(enable_pin, LOW);
          motor_enabled = true;
        } else if (inByte == 'd') {
          if(DEBUG) { Serial.println("#user interrupt --> disable motor"); }
          digitalWrite(enable_pin, HIGH);
          motor_enabled = false;
        } else if (inByte == 's') {
          if (DEBUG) { Serial.println("#user interrupt --> stream load cell data"); }
          UCSR1B |= (1 << RXCIE1); // Enable the USART Recieve Complete interrupt (USART_RXC)
        } else if (inByte == 't') {
          if (DEBUG) { Serial.println("#user interrupt --> stop load cell data"); }
          UCSR1B &= ~(1 << RXCIE1); // disable the USART Recieve Complete interrupt (USART_RXC)
        } else if (inByte == '?') {
          sendJSONObject();
        } else if (inByte == '!') {
          if (DEBUG) { Serial.println("#user interrupt --> enter setup mode"); }
          // display configurable parameters
          Serial.println("#Configurable parameters:");
          for (int i=0; i < nParams; i++) {
              debug("#%c: %s = ",(*params[i]).id,(*params[i]).desc.c_str());
              Serial.println((*params[i]).val,4);  // converting float to string is nontrivial
          }
          Serial.println("#use syntax: 'b=0.001' to change base speed, for example");
          SERIAL_STATE = CONFIGURATION; 
        }
      }
      break;
    case(CONFIGURATION):
      if (Serial.available()) {
        char inByte = Serial.read();
        String inString;
        while(inByte != '\n') {
          inString += inByte;
          inByte = Serial.read();
        }
        if (inString[0] == 'q') {
          Serial.println("#leaving setup mode");
          printControls();
          calculateMovementParameters();
          SERIAL_STATE = STANDARD;
        } else {
          // print parameters
          for (int i=0; i < nParams; i++) {
            if (inString[0] == (*params[i]).id) {
              params[i]->val = inString.substring(2).toFloat();
              debug("#%s = ",(*params[i]).desc.c_str());
              Serial.println((*params[i]).val,4); // converting float to string is nontrivial
            }
          }
        }
      }
      
      break;
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
        STEPPER_STATE = STOPPING;
        if(DEBUG) { 
          Serial.println("#stopped --> done"); 
          Serial.println("#EOE"); 
        }
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
    case STOPPING:
      UCSR1B &= ~(1 << RXCIE1); // disable the USART Recieve Complete interrupt (USART_RXC)
      STEPPER_STATE = DONE;
      break;
    case DONE:
      // do nothing
      break;
  }
}


