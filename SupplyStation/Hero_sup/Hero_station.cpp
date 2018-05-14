#include "Hero_station.h"

void put_down() {
  Serial.println("putting down");
  digitalWrite(offline_pin, HIGH);
  digitalWrite(orient_pin, HIGH);
  analogWrite(pwm_pin, 10);
  while (digitalRead(down_detect_pin) == LOW) continue;
  digitalWrite(offline_pin, LOW);
  Serial.println("put down done");
}
void raise_up() {
  Serial.println("putting up");
  digitalWrite(offline_pin, HIGH);
  digitalWrite(orient_pin, LOW);
  analogWrite(pwm_pin, 10);
  while (digitalRead(up_detect_pin) == LOW) continue;
  digitalWrite(offline_pin, LOW);
  Serial.println("put up done...");
}

bool should_open() {
  return (digitalRead(approach_pin_0) == HIGH && digitalRead(approach_pin_1) == HIGH);
}
bool still_here() {
  return (digitalRead(approach_pin_0) == HIGH || digitalRead(approach_pin_1) == HIGH);
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
      delay(5) ;
    }
  }
}
void door_close(Servo servo) {
  Serial.println("door close");
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

