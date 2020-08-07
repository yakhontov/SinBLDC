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
//#include <stdlib.h>
//#include <stdio.h>
//#include "stm32f1xx_hal.h"
//#include "tim.h"

// Таблица синусов от 0 (включая) до 90 (включая) градусов с шагом 0.2 градусов. Т.е. 450 значений
#define SINCOUNT (sizeof(sinTable))

int Constrain3600(int deg);
int Sin(int deg); // Возвращает синус умноженный на 255 (в диапазоне [-255..255] ). Аргумент задается в десятых долях градуса

#endif /* INC_SINTABLE_H_ */
