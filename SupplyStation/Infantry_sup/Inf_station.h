#include <Arduino.h>
#include <Servo.h>

#ifndef InfStation
#define InfStation

// output pins
#define servo_pin 9
//red
#define door_open_pin 2
//blue
#define door_close_pin 4
// input pins
#define approach_start_pin 7
#define approach_echo_pin A0
// door parameters (ms)
#define open_time 1200
#define hold_time 0
#define close_time 1200
// trigger parameters (ms) (cm)
#define trigger_time 50
#define stable_distance 20
#define distance_difference 1

// perform blend for certain time, singal thread;
void blend(Servo serco, unsigned int total_time);

// check if it is time to open the door
bool should_open();

#endif