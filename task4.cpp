#include <Arduino.h>
#include <avr/io.h>

#define THRESHOLD_LOW  400
#define THRESHOLD_HIGH 600
#define DEADZONE_LOW   480
#define DEADZONE_HIGH  544

void setup() {
  // 1. Configure ADC
  ADMUX = (1 << REFS0); 
  ADCSRA = (1 << ADEN) | (1 << ADPS2); 

  // 2. Configure LEDs (PB0-PB3)
  DDRB |= 0x0F; 

  // 3. Configure Button (PD2)
  DDRD &= ~(1 << DDD2);   // Set PD2 as Input
  PORTD |= (1 << PORTD2); // Enable Internal Pull-up resistor

  // 4. Configure UART (9600 baud)
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
  uint16_t y_raw = readADC(1);
  uint16_t y = 1023 - y_raw; // Invert the 10-bit value
  
  // Read PD2. Because of the pull-up, 0 = Pressed, 1 = Released.
  // We flip it with '!' so that 1 = Pressed for the Python UI.
  uint8_t btn = !(PIND & (1 << PIND2)); 

  // Send formatted data to app.py: X,Y, Button
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%u,%u,%u\n", x, y, btn);
  sendStr(buffer);
  
  PORTB &= ~0x0F;
  if (y > THRESHOLD_HIGH) PORTB |= (1 << PB0);
  else if (y < THRESHOLD_LOW) PORTB |= (1 << PB1);
  if (x > THRESHOLD_HIGH)      PORTB |= (1 << PB2); // RIGHT
  else if (x < THRESHOLD_LOW)  PORTB |= (1 << PB3); // LEFT

  _delay_ms(20); 
}
