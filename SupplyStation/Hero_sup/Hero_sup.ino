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
  
  pinMode(approach_start_pin, INPUT);
  pinMode(approach_echo_pin, OUTPUT);
  servo.attach(servo_pin);
  
  digitalWrite(offline_pin, LOW);
  Serial.begin(9600);
}

// Only when the engineering comes, the box will put down
void loop() {
  if (digitalRead(get_trig_pin) == HIGH) {
    // perform getting ammo job
    put_down();
    while (digitalRead(get_trig_pin) == LOW) continue;
    raise_up();
  } else if (should_open()) {
    // perform output ammo job
    door_open(servo);
    door_close(servo);
  }
}
