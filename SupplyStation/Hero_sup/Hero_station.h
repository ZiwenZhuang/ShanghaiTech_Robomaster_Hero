#include <Arduino.h>
#include <Servo.h>

// define raising box parameters
// define input pins
#define get_trig_pin 52
#define up_detect_pin 45
#define down_detect_pin 44
// offline output HIGH enables the motor
#define offline_pin 31
#define orient_pin 30
#define pwm_pin 2

// define door parameters
// define door trigger parameters
#define approach_pin_0 38
#define approach_pin_1 39
// define door parameters: time in (ms)
#define servo_pin 3
#define trig_wait_time 30
#define door_open_angle 350
#define door_close_angle 85

// put down the box
void put_down();
// raise up the box
void raise_up();

bool should_open();
bool still_here();
// open the door
void door_open(Servo servo);
// close the door
void door_close(Servo servo);
