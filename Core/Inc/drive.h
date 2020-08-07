#ifndef INC_DRIVE_H_
#define INC_DRIVE_H_

#include "hall.h"
#include "motor.h"

void RunTimers();// { RunTimer1(); RunTimer4(); }
void CalibDrive(int speed, int torq, int time);
void RunDrive(int speed, int torq);

#endif
