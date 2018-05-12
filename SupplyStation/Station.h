#include <Arduino.h>
#include <Servo.h>

// define the waiting times
#define open_time 1000
#define open_keep_time 500
#define close_time 1000
#define raise_time 5000
#define drop_time 5000

// define box Box
#define CLOSED 0
#define OPENING 1
#define CLOSING -1
#define OPENED 2

#define DOWN 5
#define RAISING 6
#define UP 7
#define DROPPING 8
#define BOXUP false
#define BOXDOWN true
// a Box of one box
// The start time is always the start time of the coresponding Box
typedef struct Box {
	short raise_pin; // the pin number of raising box
	short open_pin; // the pin number of open the box
	short close_pin; // the pin number of close the box

  bool box_position;
	bool door_position;
  short status;
  unsigned long start_time;

	Servo blender;
} Box;
// check if any motor is working
bool is_working(Box);
// change instruction into PWM portion
// 1 and -1 for two instruction, 0 for stop
float inst2PWM(int inst);

// check Status for one box.
void check_job(Box box);

// deploy the jobs
void open_box(Box box);
void close_box(Box box);

void raise_box(Box box);
void stop_box(Box box);
void drop_box(Box box);
