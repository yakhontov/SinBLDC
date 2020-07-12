#include "sintable.h"

// Таблица синусов с амплитудой 255 от 0 до 45 градусов с шагом 0.2 градуса
const uint8_t sinTable[] = {0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 12, 13, 14, 15, 16, 17, 18, 19, 20, 20, 21, 22, 23, 24, 25, 26, 27, 28, 28, 29, 30, 31, 32, 33, 34, 35, 35, 36, 37, 38, 39, 40, 41, 42, 43, 43, 44, 45, 46, 47, 48, 49, 50, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59, 60, 61, 62, 63, 63, 64, 65, 66, 67, 68, 69, 69, 70, 71, 72, 73, 74, 75, 75, 76, 77, 78, 79, 80, 80, 81, 82, 83, 84, 85, 86, 86, 87, 88, 89, 90, 91, 91, 92, 93, 94, 95, 96, 96, 97, 98, 99, 100, 100, 101, 102, 103, 104, 105, 105, 106, 107, 108, 109, 109, 110, 111, 112, 113, 113, 114, 115, 116, 117, 117, 118, 119, 120, 121, 121, 122, 123, 124, 124, 125, 126, 127, 127, 128, 129, 130, 131, 131, 132, 133, 134, 134, 135, 136, 137, 137, 138, 139, 140, 140, 141, 142, 143, 143, 144, 145, 146, 146, 147, 148, 148, 149, 150, 151, 151, 152, 153, 153, 154, 155, 156, 156, 157, 158, 158, 159, 160, 160, 161, 162, 163, 163, 164, 165, 165, 166, 167, 167, 168, 169, 169, 170, 171, 171, 172, 173, 173, 174, 175, 175, 176, 176, 177, 178, 178, 179, 180, 180, 181, 182, 182, 183, 183, 184, 185, 185, 186, 186, 187, 188, 188, 189, 190, 190, 191, 191, 192, 192, 193, 194, 194, 195, 195, 196, 196, 197, 198, 198, 199, 199, 200, 200, 201, 201, 202, 203, 203, 204, 204, 205, 205, 206, 206, 207, 207, 208, 208, 209, 209, 210, 210, 211, 211, 212, 212, 213, 213, 214, 214, 215, 215, 216, 216, 217, 217, 218, 218, 219, 219, 219, 220, 220, 221, 221, 222, 222, 223, 223, 223, 224, 224, 225, 225, 226, 226, 226, 227, 227, 228, 228, 228, 229, 229, 230, 230, 230, 231, 231, 231, 232, 232, 233, 233, 233, 234, 234, 234, 235, 235, 235, 236, 236, 236, 237, 237, 237, 238, 238, 238, 239, 239, 239, 240, 240, 240, 241, 241, 241, 241, 242, 242, 242, 243, 243, 243, 243, 244, 244, 244, 244, 245, 245, 245, 245, 246, 246, 246, 246, 247, 247, 247, 247, 247, 248, 248, 248, 248, 248, 249, 249, 249, 249, 249, 250, 250, 250, 250, 250, 250, 251, 251, 251, 251, 251, 251, 252, 252, 252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
const uint8_t hallState[] = {0b001, 0b011, 0b010, 0b110, 0b100, 0b101};
							// 0     60     120    180    240    300
const uint8_t hallDeg[] = {0, 0, 1200, 600, 2400, 3000, 1800};

uint8_t currentHall = 0; // Состояние дачиков Холла после последнего изменения
uint8_t prevHall = 0; // Состояние дачиков Холла перед последним изменением
volatile int currentPosDeg = 0;
volatile int currentSpeed = 0;

//////////////////////////////////////////////////////////////////////////
int16_t Sin090(uint16_t deg) // Поскольку аргумент должен быть всегда положительным, тип входного параметра будет uint16_t
{
	return sinTable[deg >> 1];
}

//////////////////////////////////////////////////////////////////////////
int16_t Sin(int16_t deg)
{
	if(deg >= 3600 && deg < 7200) // Это наиболее частый сценарий, т.к. при расчетах фазе прибавляется 120 и 240 гдадусов, поэтому для него отдельное условие
		deg -= 3600;
	else
	{
		// Для начала приведем к диапазону [0-3600)
		if(deg < 0)
			deg -= ((deg/3600)-1) * 3600; // В результате получим всегда положительное число
		else
			if(deg >= 3600)
				deg -= (deg/3600) * 3600; // В результате получим число менее 3600
	}
	if(deg >= 2700)
		return -Sin090(3600 - deg);
	if(deg >= 1800)
		return -Sin090(deg - 1800);
	if(deg >= 900)
		return Sin090(1800 - deg);
	return Sin090(deg);
}

uint8_t ReadHallSensors() // Возвращает в трех младших битах состояние датчиков Холла
{
	return (GPIOB->IDR >> 6) & 0b111;
}

void OnTimer1Overflow()
{
	const int m = 9;
	int currentStep = currentPosDeg / 600;
	int c = TIM1->CNT;
	GPIOB->ODR ^= 0b1;
	switch(currentStep)
	{
	case 0:
	case 1:
		TIM1->CCR1 = Sin((int16_t)currentPosDeg) * m;
		TIM1->CCR2 = 0;
		TIM1->CCR3 = -Sin((int16_t)(currentPosDeg - 1200)) * m;
		break;
	case 2:
	case 3:
		TIM1->CCR1 = -Sin((int16_t)(currentPosDeg - 2400)) * m;
		TIM1->CCR2 = Sin((int16_t)(currentPosDeg - 1200)) * m;
		TIM1->CCR3 = 0;
		break;
	case 4:
	case 5:
		TIM1->CCR1 = 0;
		TIM1->CCR2 = -Sin((int16_t)(currentPosDeg)) * m;
		TIM1->CCR3 = Sin((int16_t)(currentPosDeg - 2400)) * m;
		break;
	}
	currentPosDeg += 10;//currentSpeed;
	if(currentPosDeg >= 3600)
		currentPosDeg = 0;
}

void RunTimer1(TIM_HandleTypeDef *htim)
{
	HAL_TIM_PWM_Start(htim, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(htim, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(htim, TIM_CHANNEL_3);
	HAL_TIM_Base_Start_IT(htim);
}

void RunTimer4(TIM_HandleTypeDef *htim)
{
	TIM4_OC_SetPolarity(ReadHallSensors);
	HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_1);
	HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_2);
	HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_3);
	HAL_TIM_Base_Start(htim);
}

void RunTimers()
{
	RunTimer1(&htim1);
	RunTimer4(&htim4);
}

void OnTimer4Overflow()
{

}

void TIM4_OC_SetPolarity(uint8_t hallState)
{
	if(hallState & 0b001)
		TIM4->CCER |= TIM_CCER_CC1P;
	else
		TIM4->CCER &= ~TIM_CCER_CC1P;
	if(hallState & 0b010)
		TIM4->CCER |= TIM_CCER_CC2P;
	else
		TIM4->CCER &= ~TIM_CCER_CC2P;
	if(hallState & 0b100)
		TIM4->CCER |= TIM_CCER_CC3P;
	else
		TIM4->CCER &= ~TIM_CCER_CC3P;
}

//void HAL_TIM_PeriodElapsedHalfCpltCallback(TIM_HandleTypeDef *htim)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM1)
	{
		if(!(htim->Instance->CR1 & TIM_CR1_DIR))
			OnTimer1Overflow();
	}
	else if(htim->Instance == TIM4)
			OnTimer4Overflow();
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	static prevTime = 0;
	uint16_t currentTime = 0;

	prevHall = currentHall;
	currentHall = ReadHallSensors();

	//if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
	GPIOB->ODR ^= 0b10;

	currentTime = HAL_TIM_ReadCapturedValue(htim, htim->Channel);
	uint16_t delta = currentTime - prevTime;
	printf("123\n");//%d", delta);

	currentPosDeg = hallDeg[currentHall];
	if(delta)
		currentSpeed = 600 * 128 * 4800 / delta;
	else
		currentSpeed = 1;
	if(currentSpeed < 1)
		currentSpeed = 1;

	TIM4_OC_SetPolarity(currentHall);

	prevTime = currentTime;
}
