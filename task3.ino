#include <avr/io.h>
#include <avr/interrupt.h>

#define TARGET_VALUE 7
#define WIN_WINDOW_MS 500

volatile uint8_t game_state = 0; 
volatile uint8_t counter = 0;
volatile uint16_t ms_ticks = 0;

// Segment Map (Standard ABCDEFG -> Bits 0-6)
// 0x3F = 0b00111111 (Segments a,b,c,d,e,f on)
uint8_t segment_map[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

void setup() {
    // 1. Data Direction Registers
    DDRD |= 0xF0;           // PD4-PD7 as Output (Segments a-d)
    DDRB |= 0x3F;           // PB0-PB2 (e-g), PB3-PB4 (Digits), PB5 (Buzzer)
    
    // 2. External Interrupts (Button on PD2, SQW on PD3)
    EICRA |= (1 << ISC01) | (1 << ISC11); // Falling edge for both
    EIMSK |= (1 << INT0) | (1 << INT1);

    // 3. Timer0: Display Multiplexing (~1ms)
    TCCR0A |= (1 << WGM01); // CTC Mode
    OCR0A = 249;            
    TIMSK0 |= (1 << OCIE0A);
    TCCR0B |= (1 << CS01) | (1 << CS00); // 64 Prescaler

    // 4. Timer1: Precise MS Counter
    TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC, 64 Prescaler
    OCR1A = 249; 
    TIMSK1 |= (1 << OCIE1A);

    sei();
}

// Function to handle the split-port segment writing
void write_segments(uint8_t pattern) {
    // Port D: Segments a,b,c,d (bits 0,1,2,3 of pattern) go to PD4,5,6,7
    PORTD = (PORTD & 0x0F) | (pattern << 4);
    
    // Port B: Segments e,f,g (bits 4,5,6 of pattern) go to PB0,1,2
    PORTB = (PORTB & 0xF8) | (pattern >> 4);
}

// Multiplexing ISR
ISR(TIMER0_COMPA_vect) {
    static uint8_t current_digit = 0;
    
    // Turn off both digits first (Common Cathode assumed)
    PORTB &= ~((1 << PB3) | (1 << PB4));

    if (current_digit == 0) {
        write_segments(segment_map[counter % 10]); // Ones place
        PORTB |= (1 << PB3); 
        current_digit = 1;
    } else {
        write_segments(segment_map[(counter / 10) % 10]); // Tens place
        PORTB |= (1 << PB4);
        current_digit = 0;
    }
}

ISR(TIMER1_COMPA_vect) { ms_ticks++; }

// DS1307 1Hz Heartbeat
ISR(INT1_vect) {
    if (game_state == 1) {
        counter++;
        ms_ticks = 0;
        if (counter > 10) game_state = 0;
    }
}

// User Button Press
ISR(INT0_vect) {
    if (game_state == 0) {
        counter = 0;
        game_state = 1;
    } else if (game_state == 1) {
        // Success Window Check
        if ((counter == TARGET_VALUE && ms_ticks <= WIN_WINDOW_MS) || 
            (counter == TARGET_VALUE - 1 && ms_ticks >= (1000 - WIN_WINDOW_MS))) {
            game_state = 2; // Win!
        } else {
            game_state = 0; // Missed it.
        }
    }
}

int main() {
    setup();
    // (RTC I2C initialization code would go here to enable SQW)
    
    while (1) {
        if (game_state == 2) {
            PORTB |= (1 << PB5); // Buzzer on
            // Small delay loop (avoiding _delay_ms in ISR)
            for(volatile long i=0; i<200000; i++); 
            PORTB &= ~(1 << PB5); // Buzzer off
            game_state = 0;
            counter = 0;
        }
    }
}
