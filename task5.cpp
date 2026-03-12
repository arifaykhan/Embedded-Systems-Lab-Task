#include <Arduino.h>
#include <LiquidCrystal.h>

// Pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(7, 8, 9, 10, 11, 12); 

volatile bool soundTriggered = false; 
unsigned long ledOffTime = 0;
const int LED_DURATION = 100; // Keep LED on for 0.5 seconds
const int THRESHOLD = 300; 
unsigned long lastUpdate = 0;
const int interval = 100; 

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  
  DDRB |= (1 << PB5); // LED on Pin 13
  
  // Interrupt on Pin 2
  EICRA |= (1 << ISC01) | (1 << ISC00); 
  EIMSK |= (1 << INT0);  
  sei();

  lcd.print("System Ready...");
  delay(1000);
  lcd.clear();
}

ISR(INT0_vect) {
  soundTriggered = true; 
}

void loop() {
  uint16_t maxSample = 0;
  unsigned long windowStart = millis();

  // 1. Collect data for 50ms to find the loudest peak
  while (millis() - windowStart < 50) {
    uint16_t currentRead = analogRead(A0);
    if (currentRead > maxSample) {
      maxSample = currentRead;
    }
  }

  // 2. Use that peak for the LCD and Serial
  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();
    Serial.println(maxSample);

    lcd.setCursor(0, 0);
    lcd.print("Level: ");
    lcd.print(maxSample);
    lcd.print("    ");

    lcd.setCursor(0, 1);
    if (maxSample > THRESHOLD) {
      lcd.print("STATUS: LOUD!  ");
    } else {
      lcd.print("STATUS: Normal ");
    }
    
    // ... rest of your LED reset logic ...
    if (soundTriggered) {
    PORTB |= (1 << PB5);     // Turn LED ON (Pin 13)
    ledOffTime = millis() + LED_DURATION; // Set future turn-off time
    soundTriggered = false;  // Reset the flag immediately
  }

  // Turn off the LED only when the timer expires
  if (millis() >= ledOffTime && (PORTB & (1 << PB5))) {
    PORTB &= ~(1 << PB5);    // Turn LED OFF
  }
  }
}
