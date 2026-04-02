#include <Arduino.h>
#include <Servo.h>
#include <Stepper.h>

void startRound();
void checkFalseStart();
void triggerBuzzer();
void checkReaction();
void resolveRound(int winner, bool isFalseStart, long reactionTime = 0);
void victorySpin();

// --- Configuration ---
const int STEPS_PER_REV = 2048; 
Stepper myStepper(STEPS_PER_REV, 8, 10, 9, 11); // Note the sequence for ULN2003
Servo myServo;

const int btn1 = 2;
const int btn2 = 3;
const int buzzer = 5;
const int servoPin = 6;

// --- Game Variables ---
enum State { IDLE, COUNTDOWN, WAITING_FOR_REACTION, ROUND_OVER };
State currentState = IDLE;

unsigned long startTime;
unsigned long targetWaitTime;
int p1Score = 0;
int p2Score = 0;

void setup() {
  Serial.begin(9600);
  myServo.attach(servoPin);
  myServo.write(90); // Center the pointer
  
  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  
  myStepper.setSpeed(10); // RPM
}

void loop() {
  // 1. Listen for commands from the Python GUI
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd.startsWith("START")) {
      // GUI sends "START:[seconds]"
      int seconds = cmd.substring(6).toInt();
      targetWaitTime = (unsigned long)seconds * 1000;
      startRound();
    }
  }

  switch (currentState) {
    case COUNTDOWN:
      checkFalseStart();
      if (millis() - startTime >= targetWaitTime) {
        triggerBuzzer();
      }
      break;

    case WAITING_FOR_REACTION:
      checkReaction();
      break;
      
    default:
      break;
  }
}

// --- Game Functions ---

void startRound() {
  currentState = COUNTDOWN;
  startTime = millis();
  tone(buzzer, 440, 100); // Beep to signify start of wait
}

void checkFalseStart() {
  if (digitalRead(btn1) == LOW) resolveRound(2, true); // P2 wins by P1 error
  if (digitalRead(btn2) == LOW) resolveRound(1, true); // P1 wins by P2 error
}

void triggerBuzzer() {
  currentState = WAITING_FOR_REACTION;
  startTime = millis(); // Reset timer to measure reaction time
  tone(buzzer, 1000, 200); // "GO!" sound
}

void checkReaction() {
  if (digitalRead(btn1) == LOW) {
    long reactionTime = millis() - startTime;
    resolveRound(1, false, reactionTime);
  } else if (digitalRead(btn2) == LOW) {
    long reactionTime = millis() - startTime;
    resolveRound(2, false, reactionTime);
  }
}

void resolveRound(int winner, bool isFalseStart, long reactionTime = 0) {
  currentState = ROUND_OVER;
  
  // 1. Move Servo Pointer
  if (winner == 1) {
    myServo.write(180);
    myStepper.step(200); // Tug-of-war increment
    p1Score++;
  } else {
    myServo.write(0);
    myStepper.step(-200);
    p2Score++;
  }

  // 2. Report to GUI
  // Format: WINNER:[1/2]|TIME:[ms]|FALSE:[0/1]
  Serial.print("RESULT|W:");
  Serial.print(winner);
  Serial.print("|T:");
  Serial.print(reactionTime);
  Serial.print("|F:");
  Serial.println(isFalseStart ? "1" : "0");

  // 3. Check for Match Victory
  if (p1Score >= 3 || p2Score >= 3) {
    victorySpin();
    p1Score = 0;
    p2Score = 0;
  }
  
  delay(2000); // Pause so players can see the result
  myServo.write(90); // Reset pointer
  currentState = IDLE;
}

void victorySpin() {
  myStepper.setSpeed(15);
  myStepper.step(STEPS_PER_REV); // Full 360 victory lap
}
