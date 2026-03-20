void setup() {
  DDRB = (1 << PB1) | (1 << PB2) (1 << PB3); 

  // Timer/Counter 1
  TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10); // CS10 = No Prescaler
  
  ICR1 = 1;    // The smallest possible TOP value (Frequency = 16MHz / (1 * (1+1)))
  OCR1A = 1;   // 50% Duty Cycle
  OCR1B = 1;   // 50% Duty Cycle

  // Timer/Counter2
  TCCR2A = (1 << COM2A0) | (1 << WGM21) | (1 << WGM20); // Toggle OC2A on Match
  TCCR2B = (1 << WGM22) | (1 << CS20);                 // No Prescaler
  
  OCR2A = 0;   // Toggle every clock cycle
}

void loop() {
  
}
