#ifndef INC_HALL_H_
#define INC_HALL_H_

#include <stdbool.h>
#include <stdio.h>
#include "tim.h"
#include "motor.h"

// Режим работы датчика Холла. Нормальная работа, выключен, калибровка
typedef enum HallMode { HallModeEnabled, HallModeDisabled, HallModeCalibration };

// Включает/выключает датчик Холла. Выключенный датчик не оказывает никакого влияния на работу драйвера. Но драйвер может продолжать работу с заданной частотой
void EnableHall(bool ena);
void RunTimer4();
void SetHallMode(HallMode newMode);

#endif
