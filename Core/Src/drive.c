#include "drive.h"

// Инверсия пина B0
void ToggleB0() { GPIOB->ODR ^= 0b1; }

// Настройка таймера 1 и его запуск
void RunTimer1(TIM_HandleTypeDef *htim)
{
	HAL_TIM_PWM_Start(htim, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(htim, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(htim, TIM_CHANNEL_3);
	HAL_TIM_Base_Start_IT(&htim1);
}


// Прерывание, вызываемое по достижению Таймером 1 максимального значения. Вызывается с частотой ШИМ в середине импульса
void OnTimer1Top()
{
	const int m = 9;
	int currentStep = currentPosDeg / 600;
	switch(currentStep)
	{
	case 0:
	case 1:
		TIM1->CCR1 = constrToARR(Sin((int16_t)currentPosDeg) * currentTorq);
		TIM1->CCR2 = 0;
		TIM1->CCR3 = constrToARR(-Sin((int16_t)(currentPosDeg - 1200)) * currentTorq);
		break;
	case 2:
	case 3:
		TIM1->CCR1 = constrToARR(-Sin((int16_t)(currentPosDeg - 2400)) * currentTorq);
		TIM1->CCR2 = constrToARR(Sin((int16_t)(currentPosDeg - 1200)) * currentTorq);
		TIM1->CCR3 = 0;
		break;
	case 4:
	case 5:
		TIM1->CCR1 = 0;
		TIM1->CCR2 = constrToARR(-Sin((int16_t)(currentPosDeg)) * currentTorq);
		TIM1->CCR3 = constrToARR(Sin((int16_t)(currentPosDeg - 2400)) * currentTorq);
		break;
	default:
		ToggleB0();
	}

	if(!currentPosDeg)
		ToggleB0();
	currentPosDeg += currentSpeed;
}

// Общее прерывание для нескольких событий таймеров
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM1)
	{
		if(!(htim->Instance->CR1 & TIM_CR1_DIR))
			OnTimer1Top();
	}
}

void CalibDrive(int speed, int torq, int time)
{
	currentSpeed = speed;
	currentTorq = torq;
	enableCalibHallPos = false;
	//printf("Speed: %d, torq %d\n", speed, torq);
	printf("%d, ", speed);
	HAL_Delay(time / 2);
	enableCalibHallPos = true;
	HAL_Delay(time / 2);
	for(int i = 1; i < 7; i++)
		printf("%d, ", hallPosDeg[i]);
	printf("\n");
}

void RunDrive(int speed, int torq)
{
	currentSpeed = speed;
	currentTorq = torq;
	enableCalibHallPos = false;
	enableHall = true;
	printf("Run. Speed: %d, torq %d\n", speed, torq);
}
