#include "main.h"
#include "StepperMotor.hpp"
#include "acceleration.h"

#define CORE_CLOCK 84000000

// Walks 0.1 mm/s at speed 1
#define COEF CORE_CLOCK / MOTOR_MICROSTEP * MOTOR_SCREWSTEP * 10

PositioningStepperMotor<uint16_t> motor1{TIM9, TIM2, COEF, MOTOR_ENA_GPIO_Port, MOTOR_ENA_Pin, DIR_GPIO_Port, DIR_Pin, 200};

extern "C" void processAcceleration(void) {
  motor1.tick_IT();
}

extern "C" void processPositionCounter(void) {
  motor1.counter_IT();
}

extern "C" void setPos(int64_t position) {
  motor1.set_target_pos(position);
}

extern "C" void resetPos(int64_t position) {
  motor1.reset_target_current_pos(position);
}

extern "C" void setSpeedManual(int32_t speed) {
  if (motor1.is_positioning())
    return;
  if (motor1.set_reverse(speed < 0))
    motor1.target_speed = std::abs(speed);
  else
    motor1.target_speed = 0; // Stop before reversing
}

extern "C" void initPositioning(void) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  htim2.Instance->CNT = htim2.Instance->ARR; // Counting down!
  HAL_TIM_OC_Start(&htim2, TIM_CHANNEL_1);
  htim2.Instance->CR1 &= ~TIM_CR1_CEN;
  htim2.Instance->CNT = htim2.Instance->ARR;
  htim9.Instance->CNT = 0;
  HAL_NVIC_ClearPendingIRQ(TIM1_BRK_TIM9_IRQn); // Some pending interrupt is there, force clear it!
  htim9.Instance->SR = 0;
  HAL_TIM_Base_Start_IT(&htim9);
  __set_PRIMASK(primask);
}

extern "C" int64_t getPos(void) {
  return motor1.get_current_pos();
}

extern "C" int32_t getSpeed(void) {
  return (motor1.is_reversed()) ? -(int32_t)motor1.get_speed() : (int32_t)motor1.get_speed();
}
