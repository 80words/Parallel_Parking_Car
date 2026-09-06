#include <Servo.h>

// PIN DEFINITIONS
const int TRIG_PIN = A1;
const int ECHO_PIN = A0;
const int SERVO_PIN = A2;

// Left motor (L298N)
const int LEFT_PWM = 5;
const int LEFT_IN1 = 2;
const int LEFT_IN2 = 4;

// Right motor (L298N)
const int RIGHT_PWM = 6;
const int RIGHT_IN1 = 7;
const int RIGHT_IN2 = 8;

Servo scanner;

const int FORWARD_SPEED = 110;
const int PARK_SPEED = 100;

// DISTANCE THRESHOLDS (cm)
const float CAR_DISTANCE = 25.0;    // Obstacle distance
const float OPEN_DISTANCE = 35.0;   // Distance indicating open space
const unsigned long MIN_GAP_TIME = 1200; // Minimum gap duration (ms)

// TIMING SETTINGS (Tune these for your physical space)
const int POSITION_TIME = 300;       // Move past gap before reversing
const int FIRST_REVERSE_TIME = 1400;  // Arc turn into space
const int SECOND_REVERSE_TIME = 500; // Counter-turn to straighten
const int FINAL_PARK_TIME = 400;
enum RobotState {
  SEARCHING,
  MEASURING_GAP,
  POSITIONING,
  FIRST_REVERSE_TURN,
  SECOND_REVERSE_TURN,
  FINAL_REVERSE,
  PARKED
};

RobotState state = SEARCHING;

unsigned long gapStartTime = 0;
unsigned long gapEndTime = 0;

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  if (duration == 0) return 400.0;
  return duration / 58.0;
}

void stopRobot() {
  analogWrite(LEFT_PWM, 0);
  analogWrite(RIGHT_PWM, 0);
  digitalWrite(LEFT_IN1, LOW);
  digitalWrite(LEFT_IN2, LOW);
  digitalWrite(RIGHT_IN1, LOW);
  digitalWrite(RIGHT_IN2, LOW);
}

void goForward(int speed) {
  digitalWrite(LEFT_IN1, LOW);
  digitalWrite(LEFT_IN2, HIGH);
  digitalWrite(RIGHT_IN1, LOW);
  digitalWrite(RIGHT_IN2, HIGH);
  analogWrite(LEFT_PWM, speed);
  analogWrite(RIGHT_PWM, speed);
}

void goBackward(int speed) {
  digitalWrite(LEFT_IN1, HIGH);
  digitalWrite(LEFT_IN2, LOW);
  digitalWrite(RIGHT_IN1, HIGH);
  digitalWrite(RIGHT_IN2, LOW);
  analogWrite(LEFT_PWM, speed);
  analogWrite(RIGHT_PWM, speed);
}

void curveReverseRight(int speed) {
  digitalWrite(LEFT_IN1, HIGH);
  digitalWrite(LEFT_IN2, LOW);
  digitalWrite(RIGHT_IN1, HIGH);
  digitalWrite(RIGHT_IN2, LOW);
  analogWrite(LEFT_PWM, speed);
  analogWrite(RIGHT_PWM, speed / 3);
}

void curveReverseLeft(int speed) {
  digitalWrite(LEFT_IN1, HIGH);
  digitalWrite(LEFT_IN2, LOW);
  digitalWrite(RIGHT_IN1, HIGH);
  digitalWrite(RIGHT_IN2, LOW);
  analogWrite(LEFT_PWM, speed / 3);
  analogWrite(RIGHT_PWM, speed);
}

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(LEFT_PWM, OUTPUT);
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);

  pinMode(RIGHT_PWM, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);

  scanner.attach(SERVO_PIN);
  scanner.write(80); // Set servo facing RIGHT (0°)
  delay(500);       // Give hardware time to position at 0°

  stopRobot();
  delay(1000);
}

void loop() {
  float distance = getDistance();

  switch (state) {
    case SEARCHING:
      scanner.write(80); // Ensure servo remains locked at 0°
      goForward(FORWARD_SPEED);
      if (distance = OPEN_DISTANCE) {
        gapStartTime = millis();
        state = MEASURING_GAP;
      }
      break;

    case MEASURING_GAP:
      scanner.write(80);
      goForward(FORWARD_SPEED);
      if (distance <= CAR_DISTANCE) {
        gapEndTime = millis();
        unsigned long gapDuration = gapEndTime - gapStartTime;

        if (gapDuration >= MIN_GAP_TIME) {
          state = POSITIONING;
        } else {
          state = SEARCHING;
        }
      }
      break;

    case POSITIONING:
      goForward(FORWARD_SPEED);
      delay(POSITION_TIME);
      stopRobot();
      delay(300);
      state = FIRST_REVERSE_TURN;
      break;

    case FIRST_REVERSE_TURN:
      curveReverseRight(PARK_SPEED);
      delay(FIRST_REVERSE_TIME);
      stopRobot();
      delay(300);
      state = SECOND_REVERSE_TURN;
      break;

    case SECOND_REVERSE_TURN:
      curveReverseLeft(PARK_SPEED);
      delay(SECOND_REVERSE_TIME);
      stopRobot();
      delay(300);
      state = FINAL_REVERSE;
      break;

    case FINAL_REVERSE:
      goBackward(PARK_SPEED);
      delay(FINAL_PARK_TIME);
      stopRobot();
      state = PARKED;
      break;

    case PARKED:
      stopRobot();
      break;
  }
}