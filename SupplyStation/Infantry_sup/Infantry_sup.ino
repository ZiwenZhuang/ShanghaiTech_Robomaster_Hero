#include "Inf_station.h"

Servo servo;

void setup() {
  init_pins();
  servo.attach(servo_pin);
  Serial.begin(9600);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  if (should_open()) {
    Serial.println("open_door");
    digitalWrite(LED_BUILTIN, HIGH);
    // open the gate;
    digitalWrite(door_open_pin, HIGH);
    blend(servo, open_time);
    // hold the gate;
    digitalWrite(door_open_pin, LOW);
    blend(servo, hold_time);
    // close the gate;
    digitalWrite(door_close_pin, HIGH);
    blend(servo, close_time);
    // end the mission
    digitalWrite(door_close_pin, LOW);
    // wait for the infantry to leave
    Serial.println("wait for leaving");
    while (still_here()) {
      digitalWrite(LED_BUILTIN, LOW); delay(100);
      digitalWrite(LED_BUILTIN, HIGH); delay(100);
    }
    Serial.println("infantry leave");
    digitalWrite(LED_BUILTIN, LOW);
  }
}

