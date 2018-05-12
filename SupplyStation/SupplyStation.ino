#include "Station.h"
// define output pins
#define box0_raise 5 // pwm
#define box1_raise 6 // pwm
#define box0_open 2
#define box0_close 4
#define box1_open 7
#define box1_close 8
#define box0_servo_pin 13
#define box1_servo_pin 14
// define input pins
#define box0_open_trig 9
#define box1_open_trig 10
#define box0_down_trig 11
#define box1_down_trig 12

Box box0;
Box box1;

void setup() {
  pinMode(box0_raise, OUTPUT);
  pinMode(box1_raise, OUTPUT);
  pinMode(box0_open, OUTPUT);
  pinMode(box0_close, OUTPUT);
  pinMode(box1_open, OUTPUT);
  pinMode(box1_close, OUTPUT);
  pinMode(box0_open_trig, INPUT);
  pinMode(box1_open_trig, INPUT);
  box0.box_position = BOXDOWN;
  box0.raise_pin = box0_raise;
  box0.open_pin = box0_open;
  box0.close_pin = box0_close;
  box0.box_position = BOXDOWN;
  box0.raise_pin = box0_raise;
  box0.open_pin = box0_open;
  box0.close_pin = box0_close;
}

void loop() {
  check_job(box0); check_job(box1);
  if (digitalRead(box0_open_trig) == HIGH) open_box(box0);
  if (digitalRead(box1_open_trig) == HIGH) open_box(box1);
  if (digitalRead(box0_down_trig) == HIGH) drop_box(box0);
  else raise_box(box0);
  if (digitalRead(box1_down_trig) == HIGH) drop_box(box1);
  else raise_box(box1);
}

