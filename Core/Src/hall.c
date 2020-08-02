#include "hall.h"

HallMode hallMode = HallModeNormal;
int hallPosDegByState[] = {0, 2350, 880, 1040, 3050, 2510, 360, 0}; //

// Инверсия пина B1
void ToggleB1() { GPIOB->ODR ^= 0b10; }

// Возвращает в трех младших битах состояние датчиков Холла
uint8_t ReadHallSensors() { return (GPIOB->IDR >> 6) & 0b111; }

// Настройка таймера 4 и его запуск
void RunTimer4(TIM_HandleTypeDef *htim)
{
	TIM4_OC_SetPolarity(ReadHallSensors());
	HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_1);
	HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_2);
	HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_3);
	HAL_TIM_Base_Start(&htim4);
}

// Таймер реагирует только на одну полярность входных сигналов. Поэтому после каждого изменения входного сигнала нужно перенастраивать входы таймера. Эта функция настраивает входы таймера в соответствии с текущим состоянием hallState
void TIM4_OC_SetPolarity(uint8_t hallState)
{
	//return;
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

// Возвращает позицию датчика в градусах по его индексу. Если индекс не входит в [0-5], то приводит в нужный диапазон
//void int GetHallPosDeg(int8_t hallState)

// Прерывание захвата сигнала в Таймере 4
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	static uint16_t prevCCR[3] = {0, 0, 0}; // Предыдущее состояние регистров CCR
	uint16_t currentCCR[3]; // Текущее состояние регистров CCR
	static uint16_t prevTime = 0; // Время в тактах Таймера 4 предыдущего изменения состояния датчихов Холла
	uint16_t currentTime; // Текущее время в тактах Таймера 4 изменения состояния датчихов Холла
	static uint8_t prevHall = 0;
	uint8_t currentHallState;
	static int prevRotorPhaseDeg = 0;

	currentHallState = ReadHallSensors(); // Читаем текущее состояние датчиков

	TIM4_OC_SetPolarity(currentHallState); // Устанавливаем правильную полярность для следующего захвата

	// Читаем новые значения из всех регистров CCR
	currentCCR[0] = TIM4->CCR1;
	currentCCR[1] = TIM4->CCR2;
	currentCCR[2] = TIM4->CCR3;

	ToggleB1(); // Debug

	// Определяем какой из регистров изменился (его индекс)
	uint8_t changedHallId;
	if(prevCCR[0] != currentCCR[0])
		changedHallId = 0;
	else if(prevCCR[1] != currentCCR[1])
		changedHallId = 1;
	else
		changedHallId = 2;
	currentTime = currentCCR[changedHallId]; // и по нему определяем время последнего импульса
	uint16_t deltaTime = currentTime - prevTime; // Вычисляем время, прошедшее с момента последнего импульса
	if(!deltaTime) deltaTime = 1;

	int currentRotorPhaseDeg = hallPosDegByState[currentHallState]; // Текущее положение ротора в градусах
	int deltaRotorPhaseDeg = currentRotorPhaseDeg - prevRotorPhaseDeg; // Сколько градусов прошло со времени предыдущего прерывания
	deltaRotorPhaseDeg = (deltaRotorPhaseDeg > 1800)?(deltaRotorPhaseDeg - 1800):((deltaRotorPhaseDeg < -1800)?(deltaRotorPhaseDeg + 1800):deltaPhaseDeg);
	//if(!deltaRotorPhaseDeg) // Изменилось направление вращения. Нужно включить шаговый режим работы если он еще не включен по таймауту
	// Скорость вращения ротора мотора в градусах за один период ШИМ. Это значение будет добавляться к положению ротора в каждом прерывании ШИМ. 4800 и 128 - делители таймера 1(ARR) и 4(Prescaler) соответственно
	int rotorSpeedDegPerPeriod =  (4800 * deltaRotorPhaseDeg) / (deltaTime * 128); // Градусы за один шаг датчиков, делитель таймера 4, ARR таймера 1 / время шага = скорость вращения (градусов/один шаг ШИМ)

	switch(hallMode)
	{
		case HallModeNormal:
			RotorSetPhaseSpeed(currentRotorPhaseDeg, rotorSpeedDegPerPeriod);
			break;
		case HallModeDisabled:
			break;
		case HallModeCalibration:
			hallPosDegByState[currentHallState] = RotorGetPhase();
			break;
	}

	//printf("%d %d %d\n", prevTime, currentTime, c);
	//printf("%d %d %d  %d %d %d\n", currentCCR[0], currentCCR[1], currentCCR[2], prevTime, currentTime, deltaTime);
	//printf("%d %d %d\n", deltaTime, currentSpeed, hallDeg[currentHall]);

	prevHall = currentHallState; // Сохраняем состояние дачиков для следующего прерывания
	prevTime = currentTime; // Сохраняем время для следующего прерывания
	prevRotorPhaseDeg = currentRotorPhaseDeg;

	// Сохраняем значения регистров CCR
	prevCCR[0] = currentCCR[0];
	prevCCR[1] = currentCCR[1];
	prevCCR[2] = currentCCR[2];
}

void SetHallMode(HallMode newMode) { hallMode = newMode; }
