#include "Hero_station.h"

void put_down() {
  digitalWrite(offline_pin, HIGH);
  digitalWrite(orient_pin, LOW);
  analogWrite(pwm_pin, 128);
  while (digitalRead(down_detect_pin) == LOW) continue;
  digitalWrite(offline_pin, LOW);
}
void raise_up() {
  digitalWrite(offline_pin, HIGH);
  digitalWrite(orient_pin, HIGH);
  analogWrite(pwm_pin, 128);
  while (digitalRead(up_detect_pin) == LOW) continue;
  digitalWrite(offline_pin, LOW);
}

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
    Serial.println("Should open");
    return true;
}
void door_open(Servo servo) {
  Serial.println("door open");
  if (door_close_angle < door_open_angle) {
    for (unsigned int i = door_close_angle; i < door_open_angle; i++) {
      servo.write(i);
      delay(5);
    }
  } else {
    for (unsigned int i = door_close_angle; i > door_open_angle; i--) {
      servo.write(i);
      delay(5);
    }
  }
}
void door_close(Servo servo) {
  if (door_open_angle < door_close_angle) {
    for (unsigned int i = door_open_angle; i < door_close_angle; i++) {
      servo.write(i);
      delay(5);
    }
  } else {
    for (unsigned int i = door_open_angle; i > door_close_angle; i--) {
      servo.write(i);
      delay(5);
    }
  }
}

