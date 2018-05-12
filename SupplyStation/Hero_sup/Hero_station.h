#include <Arduino.h>
#include <Servo.h>

// define raising box parameters
// define input pins
#define get_trig_pin 30
#define up_detect_pin 48
#define down_detect_pin 49
// offline output HIGH enables the motor
#define offline_pin 50
#define orient_pin 51
#define pwm_pin 2

// define door parameters
// define door trigger parameters
#define approach_start_pin 52
#define approach_echo_pin A15
#define stable_distance 15
#define distance_difference 2
// define servo output pins and parameters
#define servo_pin 7
#define close_angle 60
#define open_angle 180
// define door parameters: time in (ms)
#define trig_wait_time 30
#define door_open_angle 120
#define door_close_angle 60

// put down the box
void put_down();
// raise up the box
void raise_up();

bool should_open();
// open the door
void door_open(Servo servo);
// close the door
void door_close(Servo servo);
