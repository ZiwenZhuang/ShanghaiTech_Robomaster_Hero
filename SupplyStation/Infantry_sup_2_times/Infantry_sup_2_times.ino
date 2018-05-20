#include "Inf_station.h"
Servo servo;

void setup() {
  init_pins();
  servo.attach(servo_pin);
  Serial.begin(9600);
  digitalWrite(led_pin,LOW); // close the led
  //digitalWrite(LED_BUILTIN, LOW);
}

void open_door(void){
  digitalWrite(led_pin,HIGH);
  digitalWrite(door_open_pin, HIGH);
  blend(servo, open_time);
  digitalWrite(door_open_pin, LOW);
}

void close_door(void){
  digitalWrite(door_close_pin, HIGH);
  blend(servo, close_time);
  digitalWrite(door_close_pin, LOW);
  digitalWrite(led_pin,LOW);
}

void hold_door(void){
  digitalWrite(door_close_pin, LOW);
  digitalWrite(door_open_pin, LOW);
  blend(servo, hold_time);
}

void loop() {
  if (should_open()) {
    //Serial.println("open_door");
    //digitalWrite(LED_BUILTIN, HIGH);
    
    // open the gate;
    open_door();
    // hold the gate;
    hold_door();
    // close the gate;
    close_door();

    // open the gate;
    open_door();
    // hold the gate;
    hold_door();
    // close the gate;
    close_door();
    
    // end the mission
    digitalWrite(door_open_pin, LOW);
    digitalWrite(door_close_pin, LOW);
    // wait for the infantry to leave
    Serial.println("wait for leaving");
    while (still_here()) {
      //delay(1);
     
    }
    //Serial.println("infantry leave");
    //digitalWrite(LED_BUILTIN, LOW);
  }
}

