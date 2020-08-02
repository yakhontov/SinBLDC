#ifndef INC_HALL_H_
#define INC_HALL_H_

#include "stdbool.h"
#include "tim.h"

// Режим работы датчика Холла. Нормальная работа, выключен, калибровка
typedef enum HallMode { HallModeNormal, HallModeDisabled, HallModeCalibration };

// Включает/выключает датчик Холла. Выключенный датчик не оказывает никакого влияния на работу драйвера. Но драйвер может продолжать работу с заданной частотой
void EnableHall(bool ena);
void RunTimer4();
void TIM4_OC_SetPolarity(uint8_t hallState); // Таймер реагирует только на одну полярность входных сигналов. Поэтому после каждого изменения входного сигнала нужно перенастраивать входы таймера. Эта функция настраивает входы таймера в соответствии с текущим состоянием hallState
void SetHallMode(HallMode newMode);

#endif
