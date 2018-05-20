#include "Inf_station.h"

void init_pins() {
  pinMode(door_open_pin, OUTPUT);
  pinMode(door_close_pin, OUTPUT);
  pinMode(led_pin, OUTPUT);
  digitalWrite(door_open_pin, LOW);
  digitalWrite(door_close_pin, LOW);
  digitalWrite(led_pin, LOW);
  pinMode(approach_pin_0, INPUT);
  pinMode(approach_pin_1, INPUT);
  pinMode(approach_pin_eng, INPUT);
}

// perform blend for certain time, singal thread;
void blend(Servo servo, unsigned int total_time) {
  unsigned int pos;
  // The step has to be the period time, in milli-seconds
  for (unsigned int iter = 0; iter < total_time; iter += 600) {
    Serial.println("blending");
    for (pos = 60; pos <= 120; pos += 1) {
      // in steps of 1 degree
      servo.write(pos);
      delay(5);
    }
    for (pos = 120; pos >= 60; pos -= 1) {
      servo.write(pos);
      delay(5);
    }
  }
}


bool should_open() {
  bool flag2 = (digitalRead(approach_pin_1) == HIGH);
   bool flag1 = (digitalRead(approach_pin_0) == HIGH);
   bool flag3 = (digitalRead(approach_pin_eng) == HIGH);
   return ((flag1 && flag2) || flag3 );
}
// check if the infantry is still at the gate
bool still_here() {
  bool flag2 = (digitalRead(approach_pin_1) == HIGH);
   bool flag1 = (digitalRead(approach_pin_0) == HIGH);
   bool flag3 = (digitalRead(approach_pin_eng) == HIGH);
   return ((flag1 || flag2) || flag3 );
}

