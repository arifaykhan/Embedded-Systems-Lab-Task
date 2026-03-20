void setup() {
  DDRB = (1 << PB1) | (1 << PB2) (1 << PB3); 

  // Timer/Counter 1
  TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10); // no prescaler
  
  ICR1 = 1;    // the smallest possible TOP value (freq = 16MHz / (1 * (1+1)))
  OCR1A = 1;   // 50% duty cycle
  OCR1B = 1;   // 50% duty cycle

  // Timer/Counter2
  TCCR2A = (1 << COM2A0) | (1 << WGM21) | (1 << WGM20); // toggle OC2A on match
  TCCR2B = (1 << WGM22) | (1 << CS20);                 // no prescaler
  
  OCR2A = 0;   // Toggle every clock cycle
}

void loop() {
  
}
