#include "motor.h"


volatile int currentRotorPhaseDeg = 0; // Текущее положение ротора двигателя
volatile int currentRotorSpeed = 0; // Текущая скорость ротора двигателя в градусах на период ШИМ
volatile int currentFieldPhaseDeg = 0; // Текущая фаза вектора магнитного поля. При выключенных датчиках Холла и отсутствии нагрузки на валу фазы ротора и магнитного поля должны сопадать. В таких условиях можно откалибровать положения датчиков
volatile int currentTorq = 0;


// Инверсия пина B0
void ToggleB0() { GPIOB->ODR ^= 0b1; }

// Настройка таймера 1 и его запуск
void RunTimer1()
{
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
	HAL_TIM_Base_Start_IT(&htim1);
}

// Ограничить значение до диапазона [0..2400]
int Constrain2400(int val) { return (val < 0)?0:((val > 2400)?2400:val); }

int FieldGetPhase() { return currentFieldPhaseDeg; }

int RotorGetPhase() { return currentRotorPhaseDeg; }

void RotorSetPhaseSpeed(int phase, int speed) { currentRotorPhaseDeg = Constrain3600(phase); currentRotorSpeed = speed; }

// Прерывание, вызываемое по достижению Таймером 1 максимального значения. Вызывается с частотой ШИМ в середине импульса ШИМ
void OnTimer1Top()
{
	// До момента вызова этого прерывания уже прошло некоторое время, кроме того, это значения перенесутся в рабочие регистры их теневых только спустя половину периода ШИМ
	// так что инкрементировать фазу будем с опережением, ДО выполнения расчетов
	currentRotorPhaseDeg += currentRotorSpeed; // Пересчитываем положение ротора с учетом его скорости
	if(currentRotorPhaseDeg < 0 || currentRotorPhaseDeg >= 3600) // Сделали оборот
		ToggleB0(); // Для целей дебага дрыгнули ножкой
	currentRotorPhaseDeg = Constrain3600(currentRotorPhaseDeg);
	currentFieldPhaseDeg = Constrain3600(currentRotorPhaseDeg + 900); // Считаем фазу магнитного поля. Для масимального крутящего момента она должна быть сдвинута на 90 градусов вперед относительно положения ротора
	int currentStep = currentFieldPhaseDeg / 600; // Текущий шаг в шаговом режиме (60гр сектор, в котором находится вектор). Для каждого шага отличаются расчеты тока в обмотках

	switch(currentStep)
	{
	case 0:
	case 1:
		TIM1->CCR1 = Constrain2400(Sin((int16_t)currentFieldPhaseDeg) * currentTorq);
		TIM1->CCR2 = 0;
		TIM1->CCR3 = Constrain2400(-Sin((int16_t)(currentFieldPhaseDeg - 1200)) * currentTorq);
		break;
	case 2:
	case 3:
		TIM1->CCR1 = Constrain2400(-Sin((int16_t)(currentFieldPhaseDeg - 2400)) * currentTorq);
		TIM1->CCR2 = Constrain2400(Sin((int16_t)(currentFieldPhaseDeg - 1200)) * currentTorq);
		TIM1->CCR3 = 0;
		break;
	case 4:
	case 5:
		TIM1->CCR1 = 0;
		TIM1->CCR2 = Constrain2400(-Sin((int16_t)(currentFieldPhaseDeg)) * currentTorq);
		TIM1->CCR3 = Constrain2400(Sin((int16_t)(currentFieldPhaseDeg - 2400)) * currentTorq);
		break;
	default:
		ToggleB0(); // Дрыгаем ножкой дважды, чтобы показать ошибку
		ToggleB0();
	}
}

// Общее прерывание для нескольких событий таймеров
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM1) // Поскольку прерывание общее для нескольких таймеров, нужно среагировать только на нужный таймер
	{
		if(!(htim->Instance->CR1 & TIM_CR1_DIR)) // и определить только нужное событие
			OnTimer1Top();
	}
}


void RunDrive(int speed, int torq)
{
	currentRotorSpeed = speed;
	currentTorq = torq;
	printf("Run. Speed: %d, torq %d\n", speed, torq);
}
