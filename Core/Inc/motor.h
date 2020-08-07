#ifndef INC_MOTOR_H_
#define INC_MOTOR_H_

#include "tim.h"

void RunTimer1();
int RotorGetPhase();
int FieldGetPhase();
void RotorSetPhaseSpeed(int phase, int speed);

#endif /* INC_MOTOR_H_ */
