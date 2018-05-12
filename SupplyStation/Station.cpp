#include "Station.h"

// check status for one box.
void check_job(Box box) {
  switch (box.status) {
		case OPENING:
			if (millis() - box.start_time > open_time) {
				digitalWrite(box.open_pin, LOW);
				box.status = OPENED;
				box.door_position = OPENED;
				box.start_time = millis();
			}
		break;
		case OPENED:
			if (millis() - box.start_time > open_keep_time) {
				close_box(box);
			}
		break;
		case CLOSING:
			if (millis() - box.start_time > close_time) {
				digitalWrite(box.close_pin, LOW);
				box.status = CLOSED;
				box.door_position = CLOSED;
				box.start_time = millis();
			}
		break;
		case RAISING:
			if (millis() - box.start_time > raise_time) {
				analogWrite(box.raise_pin, inst2PWM(0));
				box.status = UP;
				box.box_position = BOXUP;
				box.start_time = millis();
			}
		break;
		case DROPPING:
			if (millis() - box.start_time > raise_time) {
				analogWrite(box.raise_pin, inst2PWM(0));
				box.status = DOWN;
				box.box_position = BOXDOWN;
				box.start_time = millis();
			}
		break;
  }
}

bool is_working(Box box) {
	return (box.status == OPENING || box.status == CLOSING\
		|| box.status == RAISING || box.status == DROPPING);
}
float inst2PWM(int inst) {
	return 128;
}

void open_box(Box box) {
	if (is_working(box) || box.door_position == OPENED) return;
  digitalWrite(box.close_pin, LOW);
	digitalWrite(box.open_pin, HIGH);
	box.status = OPENING;
	box.start_time = millis();
}
void close_box(Box box) {
	if (is_working(box) || box.door_position == CLOSED) return;
  digitalWrite(box.open_pin, LOW);
	digitalWrite(box.close_pin, HIGH);
	box.status = CLOSING;
	box.start_time = millis();
}

void raise_box(Box box) {
  if (is_working(box) || box.box_position == BOXUP) return;
  analogWrite(box.raise_pin, inst2PWM(1));
	box.status = RAISING;
	box.start_time = millis();
}
void drop_box(Box box) {
  if (is_working(box) || box.box_position == BOXDOWN) return;
  analogWrite(box.raise_pin, inst2PWM(-1));
	box.status = DROPPING;
	box.start_time = millis();
}
