#include "Hero_station.h"

#include <Servo.h>
Servo servo;

void setup() {
  pinMode(get_trig_pin, INPUT);
  pinMode(up_detect_pin, INPUT);
  pinMode(down_detect_pin, INPUT);
  pinMode(offline_pin, OUTPUT);
  pinMode(orient_pin, OUTPUT);
  pinMode(pwm_pin, OUTPUT);
  
  pinMode(approach_pin_0, INPUT);
  pinMode(approach_pin_1, INPUT);
  servo.attach(servo_pin);
  
  digitalWrite(offline_pin, LOW);
  Serial.begin(9600);

  digitalWrite(LED_BUILTIN, LOW);
  put_down();
  Serial.println("initialize done");
}

// define box position, false for down, true for up;
bool box_position = false;
// Only when the engineering comes, the box will put down
void loop() {
  if (digitalRead(get_trig_pin) == HIGH) {
    // perform getting ammo job
    Serial.println("get trig pin HIGH");
    if (box_position) {
      put_down();
      box_position = false;
    } else {
      raise_up();
      box_position = true;
    }
    Serial.println("done moving the box");
  }
  if (should_open()) {
    // perform output ammo job
    door_open(servo);
    door_close(servo);
    while(still_here()) {
      Serial.println("still here");
      digitalWrite(LED_BUILTIN, HIGH); delay(100);
      digitalWrite(LED_BUILTIN, LOW); delay(100);
    }
    Serial.println("door open done");
  }
}
