#include "hall.h"


HallMode hallMode = HallModeDisabled;

int hallPosDegByState[] = {0, 2350, 880, 1040, 3050, 2510, 360, 0}; //

int hallCalibPosByState[] = {0, 0, 0, 0, 0, 0, 0, 0}; // При калибровке датчиков сюда заносятся данные
const int calibDempDiv = 256; // Делитель демпфера при калибровке. Чем больше значение, тем медленнее будет работать демпфер
const int calibMult = 100;

const uint8_t hallState[] = {0b001, 0b011, 0b010, 0b110, 0b100, 0b101};
							// 0     60     120    180    240    300

int hallPosDeg[8] = {0, 0, 0, 0, 0, 0, 0, 0};


// Инверсия пина B1
void ToggleB1() { GPIOB->ODR ^= 0b10; }

// Возвращает в трех младших битах состояние датчиков Холла. Корректные значения [1..6], любые другие - ошибочные. Хотя теоретически эта функция может вернуть [0..7]
uint8_t ReadHallSensors() { return (GPIOB->IDR >> 6) & 0b111; }

// Устанавливает режим, в котором работают даччики Холла (включены и при каждом изменении уточняют фазу ротора; выключены и не влияют на работу мотора; калибруется положение датчиков, не влияют на работу мотора)
void SetHallMode(HallMode newMode) { hallMode = newMode; }

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

// Настройка таймера 4 и его запуск
void RunTimer4()
{
	TIM4_OC_SetPolarity(ReadHallSensors());
	HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_1);
	HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_2);
	HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_3);
	HAL_TIM_Base_Start(&htim4);
}

// Возвращает позицию датчика в градусах по его индексу. Если индекс не входит в [0-5], то приводит в нужный диапазон
//void int GetHallPosDeg(int8_t hallState)

// Прерывание захвата сигнала в Таймере 4
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	uint16_t currentCCR[3]; // Текущее состояние регистров CCR
	static uint16_t prevCCR[3] = {0, 0, 0}; // Предыдущее состояние регистров CCR
	uint16_t currentTime; // Текущее время в тактах Таймера 4 изменения состояния датчихов Холла
	static uint16_t prevTime = 0; // Время в тактах Таймера 4 предыдущего изменения состояния датчихов Холла
	uint8_t currentHallState; // Текущее состояние датчиков Холла [1..6]
	static uint8_t prevHallState = 0; // Предыдущее состояние датчиков Холла [1..6]
	static int prevRotorPhaseDeg = 0; // Предыдущее положение ротора

	currentHallState = ReadHallSensors(); // Читаем текущее состояние датчиков

	TIM4_OC_SetPolarity(currentHallState); // Устанавливаем правильную полярность для следующего захвата

	// Читаем текущие значения из всех регистров CCR
	currentCCR[0] = TIM4->CCR1;
	currentCCR[1] = TIM4->CCR2;
	currentCCR[2] = TIM4->CCR3;

	ToggleB1(); // Debug
	if(currentHallState < 1 || currentHallState > 6) // Ошибка датчиков
	{
		ToggleB1(); // Error
		ToggleB1();
	}

	// Определяем какой из датчиков Холла изменил свое состояние (определяем индекс датчика)
	uint8_t changedHallId;
	if(prevCCR[0] != currentCCR[0])
		changedHallId = 0;
	else if(prevCCR[1] != currentCCR[1])
		changedHallId = 1;
	else
		changedHallId = 2;
	currentTime = currentCCR[changedHallId]; // и по индексу определяем время последнего импульса
	uint16_t deltaTime = currentTime - prevTime; // Сколько времени прошло с прошлого раза
	if(deltaTime <= 0) deltaTime = 1; // Чтобы в математике не возникало деления на 0

	int currentRotorPhaseDeg = hallPosDegByState[currentHallState]; // Текущее положение ротора в градусах
	int deltaRotorPhaseDeg = currentRotorPhaseDeg - prevRotorPhaseDeg; // Сколько градусов прошло с прошлого раза. Значение может быть отрицательным. Знак числа зависит от направления вращения
	if(deltaRotorPhaseDeg < -1800) // Перескочили через 0 в положительном направлении
		deltaRotorPhaseDeg += 3600;
	//deltaRotorPhaseDeg = (deltaRotorPhaseDeg > 1800)?(deltaRotorPhaseDeg - 1800):((deltaRotorPhaseDeg < -1800)?(deltaRotorPhaseDeg + 1800):deltaRotorPhaseDeg); // Приводим к диапазону -1800..1800

	if(deltaRotorPhaseDeg < -1800 || deltaRotorPhaseDeg > 1800) // Переменная не должна выходить за диапазон, но если вдруг это случилось, то дебажим
			printf("DRErr:%d,%d,%d\n", currentRotorPhaseDeg, prevRotorPhaseDeg, deltaRotorPhaseDeg);
	//if(!deltaRotorPhaseDeg) // Если значение 0, значит сработал тот же датчик, значит изменилось направление вращения. Нужно включить шаговый режим работы, если он еще не включен по таймауту
	//лучше так: if(changedHallId == prevChangedHallId)

	// Скорость вращения ротора мотора в градусах за один период ШИМ. Это значение будет добавляться к положению ротора в каждом прерывании ШИМ. 4800 и 128 - делители таймера 1(ARR) и 4(Prescaler) соответственно
	int currentRotorSpeed =  (4800 * deltaRotorPhaseDeg) / (deltaTime * 128); // Градусы за один шаг датчиков, делитель таймера 4, ARR таймера 1 / время шага = скорость вращения (градусов/один шаг ШИМ)

	//printf("%d,%d   ", currentRotorPhaseDeg, currentRotorSpeed);

	switch(hallMode)
	{
		case HallModeEnabled:
			RotorSetPhaseSpeed(currentRotorPhaseDeg, currentRotorSpeed);
			//printf("%d, ", currentRotorPhaseDeg);
			printf("%d ", deltaTime);
			break;
		case HallModeDisabled:
			break;
		case HallModeCalibration:
			hallCalibPosByState[currentHallState] = (hallCalibPosByState[currentHallState] * (calibDempDiv - 1) + FieldGetPhase() * calibMult) / calibDempDiv;
			break;
	}

	//printf("%d %d %d\n", prevTime, currentTime, c);
	//printf("%d %d %d  %d %d %d\n", currentCCR[0], currentCCR[1], currentCCR[2], prevTime, currentTime, deltaTime);
	//printf("%d %d %d\n", deltaTime, currentSpeed, hallDeg[currentHall]);

	prevHallState = currentHallState; // Сохраняем состояние дачиков для следующего прерывания
	prevTime = currentTime; // Сохраняем время для следующего прерывания
	prevRotorPhaseDeg = currentRotorPhaseDeg; // Сохраняем фазу для следующего прерывания

	// Сохраняем значения регистров CCR
	prevCCR[0] = currentCCR[0];
	prevCCR[1] = currentCCR[1];
	prevCCR[2] = currentCCR[2];
}

void CalibHall(int time)
{
	for(int i = 0; i < 6; i++)
		hallCalibPosByState[i] = hallPosDegByState[i] * calibMult;

	HallMode h = hallMode;
	hallMode = HallModeCalibration;
	HAL_Delay(time);
	hallMode = h;

	for(int i = 1; i < 7; i++)
		printf("%d, ", hallCalibPosByState[i]);
	printf("\n");
}

