#include "Inf_station.h"

// perform blend for certain time, singal thread;
void blend(Servo servo, unsigned int total_time) {
  int pos = 0;
  unsigned int end_time;
  end_time = millis() + total_time;
  while (millis() < end_time) {
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
    double duration, range; range = 0;
    for (int i = 0; i < trigger_time; i += 50) {
      for (int j = 0; j < 5; j++) {
        digitalWrite(approach_start_pin, HIGH);
        delay(10);
        digitalWrite(approach_start_pin, LOW);
        duration = pulseIn(approach_echo_pin, HIGH);
        range += duration / 2 / 29.1;
      }
      range = range / 5;
      Serial.println(range);
      if (range > (stable_distance + distance_difference)\
          || range < (stable_distance - distance_difference)) return false;
    }
    return true;
}
