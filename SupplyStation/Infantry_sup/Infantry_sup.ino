#include "Inf_station.h"

Servo servo;

void setup() {
  pinMode(door_open_pin, OUTPUT);
  pinMode(door_close_pin, OUTPUT);
  digitalWrite(door_open_pin, LOW);
  digitalWrite(door_close_pin, LOW);
  pinMode(approach_start_pin, OUTPUT);
  pinMode(approach_echo_pin, INPUT);
  servo.attach(servo_pin);
  Serial.begin(9600);
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
    while (should_open()) continue;
    digitalWrite(LED_BUILTIN, LOW);
  }
}

