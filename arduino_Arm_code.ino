#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN 150
#define SERVOMAX 600

// Servo angles
int servo1Angle = 90;
int servo2Angle = 90;
int servo3Angle = 90;
int servo4Angle = 90;
int servo6Angle = 90;
int clawAngle = 30;     // closed

// Input states
int rtState = 0;
int ltState = 0;
int rbState = 0;
int lbState = 0;

// Servo1 easing
int servo1Dir = 0;
int servo1Speed = 0;

// Servo6 easing
int servo6Dir = 0;
int servo6Speed = 0;

// Claw toggle
int xState = 0;
int lastXState = 0;
bool clawOpen = false;

// Stabilisation mode
int sqState = 0;
int lastSqState = 0;
bool stabilise = false;

unsigned long lastStepTime = 0;
const int stepDelay = 100;

int angleToPulse(int angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pwm.begin();
  pwm.setPWMFreq(50);

  pwm.setPWM(0, 0, angleToPulse(servo1Angle));
  pwm.setPWM(1, 0, angleToPulse(servo2Angle));
  pwm.setPWM(2, 0, angleToPulse(servo3Angle));
  pwm.setPWM(3, 0, angleToPulse(servo4Angle));
  pwm.setPWM(5, 0, angleToPulse(servo6Angle));
  pwm.setPWM(7, 0, angleToPulse(clawAngle));   // <-- moved to channel 7
}

void loop() {

  // -----------------------------
  // READ SERIAL COMMANDS
  // -----------------------------
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    if (msg.length() < 2) return;

    char type = msg.charAt(0);
    String val = msg.substring(1);

    if (type == 'A') {
      int angle = val.toInt();
      if (angle > 95) servo1Dir = +1;
      else if (angle < 85) servo1Dir = -1;
      else servo1Dir = 0;
    }

    if (type == 'Y') {
      float ry = val.toFloat();
      if (ry > 0.3) servo6Dir = -1;
      else if (ry < -0.3) servo6Dir = +1;
      else servo6Dir = 0;
    }

    if (type == 'R') rtState = val.toInt();
    if (type == 'L') ltState = val.toInt();
    if (type == 'B') rbState = val.toInt();
    if (type == 'C') lbState = val.toInt();

    if (type == 'X') xState = val.toInt();
    if (type == 'Q') sqState = val.toInt();
  }

  // -----------------------------
  // STABILISATION MODE TOGGLE
  // -----------------------------
  if (sqState == 1 && lastSqState == 0) {
    stabilise = !stabilise;
  }
  lastSqState = sqState;

  // -----------------------------
  // CLAW TOGGLE
  // -----------------------------
  if (xState == 1 && lastXState == 0) {
    clawOpen = !clawOpen;
    clawAngle = clawOpen ? 120 : 30;
    pwm.setPWM(7, 0, angleToPulse(clawAngle));   // <-- channel 7
  }
  lastXState = xState;

  // -----------------------------
  // MOVEMENT LOOP
  // -----------------------------
  unsigned long now = millis();
  if (now - lastStepTime >= stepDelay) {
    lastStepTime = now;

    // SERVO1 easing (FASTER NOW)
    if (servo1Dir != 0) {
      if (servo1Speed < 8) servo1Speed++;   // <-- increased speed
    } else {
      if (servo1Speed > 0) servo1Speed--;
    }

    if (servo1Speed > 0) {
      servo1Angle += servo1Dir * servo1Speed;
      servo1Angle = constrain(servo1Angle, 0, 180);
      pwm.setPWM(0, 0, angleToPulse(servo1Angle));
    }

    // SERVO2 (RT/LT)
    if (rtState == 1) servo2Angle = min(180, servo2Angle + 5);
    if (ltState == 1) servo2Angle = max(0, servo2Angle - 5);
    pwm.setPWM(1, 0, angleToPulse(servo2Angle));

    // MIRROR SERVO3
    servo3Angle = 180 - servo2Angle;
    pwm.setPWM(2, 0, angleToPulse(servo3Angle));

    // SERVO4 (RB/LB)
    if (rbState == 1) servo4Angle = min(180, servo4Angle + 5);
    if (lbState == 1) servo4Angle = max(0, servo4Angle - 5);
    pwm.setPWM(3, 0, angleToPulse(servo4Angle));

    // -----------------------------------
    // SERVO 6 — MANUAL OR STABILISED
    // -----------------------------------
    if (stabilise) {
      servo6Angle = 180 - servo2Angle;
      servo6Angle = constrain(servo6Angle, 0, 180);
      pwm.setPWM(5, 0, angleToPulse(servo6Angle));
    } 
    else {
      if (servo6Dir != 0) {
        if (servo6Speed < 5) servo6Speed++;
      } else {
        if (servo6Speed > 0) servo6Speed--;
      }

      if (servo6Speed > 0) {
        servo6Angle += servo6Dir * servo6Speed;
        servo6Angle = constrain(servo6Angle, 0, 180);
        pwm.setPWM(5, 0, angleToPulse(servo6Angle));
      }
    }
  }
}