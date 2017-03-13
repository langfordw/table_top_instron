#include <avr/delay.h>
#include <avr/interrupt.h>

#define USART_BAUDRATE 115200
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

String inputString = "";         // a string to hold incoming data
volatile boolean stringComplete = false;  // whether the string is complete

ISR(USART1_RX_vect)
{ 
    char inChar = UDR1;
    inputString += inChar;
    if (inChar == '\n') { 
      stringComplete = true; 
    }   
}

void setup() {

  Serial.begin(115200); // virtual serial link to computer
  
  noInterrupts();           // disable all interrupts
  // default arduino is 8 databits, no parity, 1 stopbit
  UCSR1B = (1 << RXEN1);   // Turn on the reception circuitry
  UCSR1C = (1 << UCSZ10) | (1 << UCSZ11); // Use 8-bit character sizes

  UBRR1H = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register
  UBRR1L = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register

  UCSR1B |= (1 << RXCIE1); // Enable the USART Recieve Complete interrupt (USART_RXC)
  interrupts();             // enable all interrupts

}

void loop() {
  if (stringComplete) {
    Serial.print(inputString);
    inputString = "";
    stringComplete = false;
  }
}
