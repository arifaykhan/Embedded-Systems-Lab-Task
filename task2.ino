#include <avr/io.h>

#define THRESHOLD_LOW  400
#define THRESHOLD_HIGH 600
#define DEADZONE_LOW   480
#define DEADZONE_HIGH  544

void setup() {
  // 1. Configure ADC
  ADMUX = (1 << REFS0); // Reference AVcc (5V)
  ADCSRA = (1 << ADEN) | (1 << ADPS2); // Enable ADC, Prescaler 16 (1MHz ADC clock)

  // 2. Configure LEDs (PB0-PB3 as outputs)
  DDRB |= 0x0F; 

  // 3. Configure UART (9600 baud at 16MHz)
  UBRR0H = 0;
  UBRR0L = 103;
  UCSR0B = (1 << TXEN0); 
}

uint16_t readADC(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    ADCSRA |= (1 << ADSC);          // Start conversion
    while (ADCSRA & (1 << ADSC));   // Wait for conversion
    return ADC;                     // Return 10-bit value
}

void sendStr(const char* s) {
    while (*s) {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = *s++;
    }
}

void loop() {
  uint16_t x = readADC(0);
  uint16_t y = readADC(1);
  
  PORTB &= ~0x0F; // Turn off all LEDs

  if (y < THRESHOLD_LOW) { 
    PORTB |= (1 << PB0); sendStr("UP\n"); 
  } else if (y > THRESHOLD_HIGH) { 
    PORTB |= (1 << PB1); sendStr("DOWN\n"); 
  } else if (x > THRESHOLD_HIGH) { 
    PORTB |= (1 << PB2); sendStr("RIGHT\n"); 
  } else if (x < THRESHOLD_LOW) { 
    PORTB |= (1 << PB3); sendStr("LEFT\n"); 
  }
}
