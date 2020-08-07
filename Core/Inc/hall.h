#ifndef INC_HALL_H_
#define INC_HALL_H_

#include <stdbool.h>
#include <stdio.h>
#include "tim.h"
#include "motor.h"

// Режим работы датчика Холла. Нормальная работа, выключен, калибровка
typedef enum { HallModeEnabled, HallModeDisabled, HallModeCalibration } HallMode;

void RunTimer4();
// Режим работы датчика Холла. Нормальная работа, выключен, калибровка
void SetHallMode(HallMode newMode);
void CalibHall(int time);

#endif
