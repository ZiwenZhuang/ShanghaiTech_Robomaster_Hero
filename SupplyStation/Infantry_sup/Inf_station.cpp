#include "Inf_station.h"

void init_pins() {
  pinMode(door_open_pin, OUTPUT);
  pinMode(door_close_pin, OUTPUT);
  digitalWrite(door_open_pin, LOW);
  digitalWrite(door_close_pin, LOW);
  pinMode(approach_pin_0, INPUT);
  pinMode(approach_pin_1, INPUT);
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

// https://sujuanyan.github.io/FABLAB_DOCUMENTATION/final_project/final_project.html
bool should_open() {
  return (digitalRead(approach_pin_0) == HIGH && digitalRead(approach_pin_1) == HIGH);
}
// check if the infantry is still at the gate
bool still_here() {
  return (digitalRead(approach_pin_0) == HIGH || digitalRead(approach_pin_1) == HIGH);
}

