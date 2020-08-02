/*
 * sintable.h
 *
 *  Created on: Jul 9, 2020
 *      Author: Al
 */

#ifndef INC_SINTABLE_H_
#define INC_SINTABLE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "stm32f1xx_hal.h"
#include "tim.h"

void EnableHall(bool ena);
void CalibDrive(int speed, int torq, int time);
void RunDrive(int speed, int torq);
void RunTimer1(TIM_HandleTypeDef *htim);
void RunTimers();
uint8_t ReadHallSensors(); // Возвращает в трех младших битах состояние датчиков Холла
//void TIM4_OC_SetPolarity() { TIM4_OC_SetPolarity(ReadHallSensors()); }
int16_t Sin(int16_t deg); // Возвращает синус умноженный на 255 (в диапазоне от -255 до 255). Аргумент задается в десятых долях градуса
int constrainDeg(int deg);

// Таблица синусов от 0 (включая) до 90 (включая) градусов с шагом 0.2 градусов. Т.е. 450 значений
#define SINCOUNT (sizeof(sinTable))

#endif /* INC_SINTABLE_H_ */
