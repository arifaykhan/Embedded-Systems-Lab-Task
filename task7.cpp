#include <Arduino.h>
#include <Keypad.h>
#include <IRremote.h>
#include <SPI.h>
#include <MFRC522.h>

// --- PIN DEFINITIONS ---
#define IR_RECEIVE_PIN 2
#define RED_LED A0
#define GREEN_LED A1
#define BLUE_LED A2
#define RST_PIN A3   // RFID Reset
#define SS_PIN 10    // RFID SDA (Slave Select)

void updateLEDs();
void handleKeypad();
void handleIR();
void handleRFID();
char translateIR(int command);

// --- STATE MACHINE ---
enum SystemState { WAITING, LOCKED, UNLOCKED };
SystemState currentState = WAITING;

// --- KEYPAD SETUP (Pins 9-3) ---
const byte ROWS = 4; 
const byte COLS = 3; 
char keys[ROWS][COLS] = {
  {'1','2','3'}, {'4','5','6'}, {'7','8','9'}, {'*','0','#'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; 
byte colPins[COLS] = {5, 4, 3}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- RFID SETUP ---
MFRC522 mfrc522(SS_PIN, RST_PIN);

// --- VARIABLES ---
String masterCode = "";
String inputCode = "";

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  IrReceiver.begin(IR_RECEIVE_PIN);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  Serial.println("SYSTEM_START: Enter 4-digit code on Keypad to Lock.");
}

void loop() {
  updateLEDs();

  switch (currentState) {
    case WAITING:
      handleKeypad();
      break;
    case LOCKED:
      handleIR();
      break;
    case UNLOCKED:
      handleRFID();
      break;
  }
}

// --- LOGIC FUNCTIONS ---

void handleKeypad() {
  char key = keypad.getKey();
  if (key) {
    masterCode += key;
    Serial.print("*"); // Masked output
    if (masterCode.length() >= 4) {
      currentState = LOCKED;
      Serial.println("\nSYSTEM_LOCKED");
    }
  }
}

void handleIR() {
  if (IrReceiver.decode()) {
    // Note: You may need to update these HEX codes based on your specific remote
    // This example translates common Elegoo IR codes back to digits
    char pressed = translateIR(IrReceiver.decodedIRData.command);
    
    if (pressed != 'X') {
      inputCode += pressed;
      Serial.print("?"); 
    }

    if (inputCode.length() >= 4) {
      if (inputCode == masterCode) {
        currentState = UNLOCKED;
        inputCode = "";
        Serial.println("\nSYSTEM_UNLOCKED");
      } else {
        Serial.println("\nWRONG_CODE");
        inputCode = ""; // Reset attempt
      }
    }
    IrReceiver.resume();
  }
}

void handleRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Flash Blue to indicate read
  digitalWrite(BLUE_LED, HIGH);
  
  String tagID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    tagID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    tagID += String(mfrc522.uid.uidByte[i], HEX);
  }
  tagID.toUpperCase();

  // Send to Python via Serial
  Serial.println("TAG:" + tagID);
  
  delay(500); 
  digitalWrite(BLUE_LED, LOW);
  mfrc522.PICC_HaltA();
}

void updateLEDs() {
  switch (currentState) {
    case WAITING:  // Blinking Blue
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BLUE_LED, (millis() / 500) % 2); 
      break;
    case LOCKED:   // Solid Red
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
      break;
    case UNLOCKED: // Solid Green
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(BLUE_LED, LOW);
      break;
  }
}

char translateIR(int command) {
  // Common Elegoo Remote Commands (Update if your remote differs)
  switch (command) {
    case 0x16: return '0'; case 0xC:  return '1'; case 0x18: return '2';
    case 0x5E: return '3'; case 0x8:  return '4'; case 0x1C: return '5';
    case 0x5A: return '6'; case 0x42: return '7'; case 0x52: return '8';
    case 0x4A: return '9';
    default: return 'X';
  }
}
