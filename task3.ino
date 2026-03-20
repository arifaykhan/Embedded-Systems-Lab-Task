#include <avr/io.h>
#include <avr/interrupt.h>

#define TARGET_VALUE 7
#define WIN_WINDOW_MS 100

volatile uint8_t game_state = 0;
volatile uint8_t counter = 0;
volatile uint16_t ms_ticks = 0;

uint8_t segment_map[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };

void i2c_init() {
    TWBR = 72; // Set 100kHz SCL frequency at 16MHz Clock
}

void rtc_enable_sqw() {
    // Start -> Address + W -> Reg 0x07 -> Data 0x10 (1Hz Enable) -> Stop
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // Start
    while (!(TWCR & (1 << TWINT)));
    
    TWDR = 0xD0; // DS1307 Write Address
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    
    TWDR = 0x07; // Control Register
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    
    TWDR = 0x10; // Enable SQWE, Set Rate to 1Hz
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); // Stop
}

void setup() {
  // 1. Data Direction
  DDRD |= 0xF0;  // PD4-PD7 (Segments a-d)
  DDRB |= 0x3F;  // PB0-PB2 (e-g), PB3-PB4 (Digits), PB5 (Buzzer)

  // 2. Button with Pull-up
  DDRD &= ~(1 << DDD2);
  PORTD |= (1 << PORTD2); 

  i2c_init();
  rtc_enable_sqw();

  // 3. Interrupts - We use Pin 2 (Button) and Pin 3 (Clock)
  EICRA |= (1 << ISC01) | (1 << ISC11); // Falling edge
  EIMSK |= (1 << INT0) | (1 << INT1);

  // 4. Timer0: Display Multiplexing (This MUST run for the display to light up)
  TCCR0A = (1 << WGM01); // CTC Mode
  OCR0A = 249;
  TIMSK0 |= (1 << OCIE0A);
  TCCR0B = (1 << CS01) | (1 << CS00); // 64 Prescaler

  // 5. Timer1: Millisecond counter for the "Success Window"
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); 
  OCR1A = 249;
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

// Fixed Segment Writer: Ensures we don't overwrite the Button/Clock pins
void write_segments(uint8_t pattern) {
  PORTD = (PORTD & 0x0F) | (pattern << 4);
  PORTB = (PORTB & 0xF8) | (pattern & 0x70) >> 4; // Map e,f,g to PB0,1,2
}

ISR(TIMER0_COMPA_vect) {
  static uint8_t current_digit = 0;
  
  // Turn off digits (Common Cathode)
  PORTB &= ~((1 << PB3) | (1 << PB4));

  if (current_digit == 0) {
    write_segments(segment_map[counter % 10]);
    PORTB |= (1 << PB3);
    current_digit = 1;
  } else {
    write_segments(segment_map[(counter / 10) % 10]);
    PORTB |= (1 << PB4);
    current_digit = 0;
  }
}

ISR(TIMER1_COMPA_vect) { ms_ticks++; }

ISR(INT1_vect) { // RTC 1Hz Pulse
  if (game_state == 1) {
    counter++;
    ms_ticks = 0;
    if (counter > 9) game_state = 0;
  }
}

ISR(INT0_vect) { // Button Press
  if (game_state == 0) {
    counter = 0;
    game_state = 1;
  } else if (game_state == 1) {
    if ((counter == TARGET_VALUE && ms_ticks <= WIN_WINDOW_MS) || 
        (counter == TARGET_VALUE - 1 && ms_ticks >= (1000 - WIN_WINDOW_MS))) {
      game_state = 2; 
    } else {
      game_state = 0;
    }
  }
}

int main() {
  setup();
  while (1) {
    if (game_state == 2) {
      PORTB |= (1 << PB5); // Buzzer
      for (volatile long i = 0; i < 200000; i++);
      PORTB &= ~(1 << PB5);
      game_state = 0;
      counter = 0;
    }
  }
}
