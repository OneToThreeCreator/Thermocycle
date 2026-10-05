#include <FreeRTOS.h>
#include <semphr.h>
#include <stdbool.h>

#include "acceleration.h"
#include "main.h"
#include "Modbus.h"
#include "modbus_defs.h"

uint16_t hregs[MODBUS_HREGS] = {
  [MODBUS_HREG_SECONDPOS_LSW] = 175,
  [MODBUS_HREG_CYCLETIME] = 10,
  [MODBUS_HREG_CYCLESCOUNT] = 3
};
uint16_t coils;
uint16_t iregs[MODBUS_IREGS] ={0};

#define cycles_count hregs[MODBUS_HREG_CYCLESCOUNT]
#define cycle_time hregs[MODBUS_HREG_CYCLETIME]

// Transparently convert between mm and stepper steps

#define first_position  (((uint32_t)hregs[MODBUS_HREG_FIRSTPOS_MSW]  << 16 | hregs[MODBUS_HREG_FIRSTPOS_LSW])  * (MOTOR_MICROSTEP / MOTOR_SCREWSTEP))
#define second_position (((uint32_t)hregs[MODBUS_HREG_SECONDPOS_MSW] << 16 | hregs[MODBUS_HREG_SECONDPOS_LSW]) * (MOTOR_MICROSTEP / MOTOR_SCREWSTEP))

#define _pos_conv(p) ((p + MOTOR_MICROSTEP/MOTOR_SCREWSTEP/2) / (MOTOR_MICROSTEP/MOTOR_SCREWSTEP))
#define set_first_position(p)  (hregs[MODBUS_HREG_FIRSTPOS_MSW]  = (_pos_conv(p) >> 16), hregs[MODBUS_HREG_FIRSTPOS_LSW]  = (_pos_conv(p) & 0xFFFF))
#define set_second_position(p) (hregs[MODBUS_HREG_SECONDPOS_MSW] = (_pos_conv(p) >> 16), hregs[MODBUS_HREG_SECONDPOS_LSW] = (_pos_conv(p) & 0xFFFF))

#define set_current_pos(p) (iregs[MODBUS_IREG_POS_MSW] = (p >> 16), iregs[MODBUS_IREG_POS_LSW] = (p & 0xFFFF))

#define cycles_started ((coils >> MODBUS_COIL_DOCYCLE) & 1)
#define set_cycles_started(v) ((v) ? coils |= (1 << MODBUS_COIL_DOCYCLE) : (coils &= ~(1 << MODBUS_COIL_DOCYCLE)))

#define is_position2 ((coils >> MODBUS_COIL_POSITION2) & 1)
#define set_is_position2(v) ((v) ? coils |= (1 << MODBUS_COIL_POSITION2) : (coils &= ~(1 << MODBUS_COIL_POSITION2)))
#define toggle_is_position2() (coils ^= (1 << MODBUS_COIL_POSITION2))

#define current_cycle iregs[MODBUS_IREG_CYCLE]

#define manual_speed hregs[MODBUS_HREG_MANUAL_SPEED]


modbusHandler_t mb = {
  .uModbusType = MB_SLAVE,
  .port = &huart2,
  .u8id = MODBUS_ADDRESS,
  .EN_Port = USART2_REDE_GPIO_Port,
  .EN_Pin = USART2_REDE_Pin,
  .u16regs = NULL,
  .u16timeOut = TIMEOUT_MODBUS,
  .u16regsize = 0,
  .u16coils = &coils,
  .u16coilsNregs = MODBUS_COILS,
  .u16holdingRegs = hregs,
  .u16holdingRegsNregs = sizeof(hregs)/sizeof(hregs[0]),
  .u16inputRegs = iregs,
  .u16inputRegsNregs = sizeof(iregs)/sizeof(iregs[0]),
  .xTypeHW = USART_HW,
};

osThreadId_t commTaskHandle;
const osThreadAttr_t commTask_attributes = {
  .name = "commTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

uint32_t get_seconds()
{
  RTC_TimeTypeDef time = {
    .SecondFraction = 0
};
  RTC_DateTypeDef date;
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
  uint32_t monthdays[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  uint32_t days = 0;
  days += date.Year * 365 + (date.Year + 3) / 4;
  for (unsigned i = 0; i < date.Month - 1; ++i) {
    days += monthdays[i];
    if (i == 1 && date.Year % 4 == 0)
      days += 1;
  }
  // Still will overflow earlier then uint32_t. Will take a lot of time though
  return (uint32_t)time.Hours   * 3600u
         + (uint32_t)time.Minutes * 60u
         + (uint32_t)time.Seconds
         + (date.Date - 1 + days) * 86400;
}

void reset_seconds() {
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {
    .Date = 1,
    .Month = 1,
    .Year = 0
  };
  HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
  HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
}

void StartCommTask(void *arg);

void init_communication(void) {
  ModbusInit(&mb);
  ModbusStart(&mb);
  // commTaskHandle = osThreadNew(StartCommTask, NULL, &commTask_attributes); // Already started by CubeNX
}

void position_first () {
  uint32_t trgpos = first_position;
  uint32_t curpos = getPos();
  xSemaphoreGive(mb.ModBusSphrHandle);
  for (; curpos != trgpos;
       osDelay(pdMS_TO_TICKS(10)),
       xSemaphoreTake(mb.ModBusSphrHandle, portMAX_DELAY),
       trgpos = first_position, curpos = getPos(),
       xSemaphoreGive(mb.ModBusSphrHandle))
    setPos(trgpos), set_current_pos(curpos);
  xSemaphoreTake(mb.ModBusSphrHandle, portMAX_DELAY);
}

void position_second () {
  uint32_t trgpos = second_position;
  uint32_t curpos = getPos();
  xSemaphoreGive(mb.ModBusSphrHandle);
  for (; curpos != trgpos;
       osDelay(pdMS_TO_TICKS(10)),
       xSemaphoreTake(mb.ModBusSphrHandle, portMAX_DELAY),
       trgpos = second_position, curpos = getPos(),
       xSemaphoreGive(mb.ModBusSphrHandle))
    setPos(trgpos), set_current_pos(curpos);
  xSemaphoreTake(mb.ModBusSphrHandle, portMAX_DELAY);
}

void position_manual() {
  bool pos2 = is_position2;
  if (getSpeed() == 0) {
    (pos2) ? position_second() : position_first();
  }
  uint32_t curpos = getPos();
  int16_t speed = manual_speed;
  set_current_pos(curpos);
  if (speed == 0)
    return;
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  setSpeedManual(speed);
  xSemaphoreGive(mb.ModBusSphrHandle);
  for (; getSpeed() > 0 || speed > 0;
       osDelay(pdMS_TO_TICKS(10)),
       xSemaphoreTake(mb.ModBusSphrHandle, portMAX_DELAY),
       curpos = getPos(),
       speed = manual_speed,
       xSemaphoreGive(mb.ModBusSphrHandle))
    setSpeedManual(speed), set_current_pos(curpos);
  xSemaphoreTake(mb.ModBusSphrHandle, portMAX_DELAY);
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  if (!pos2) {
    resetPos(0);
  } else {
    set_second_position(getPos());
  }
}

void StartCommTask(void *arg) {
  /* Finding zero (considering options). Ignore for now
  for (;;) {

  }//*/
  uint32_t cycle_start_time = get_seconds();
  bool start_is_position2 = is_position2;
  // Initial positioning
  position_first();
  // Main loop
  for (;;
       xSemaphoreGive(mb.ModBusSphrHandle),
       osDelay(pdMS_TO_TICKS(10)),
       xSemaphoreTake(mb.ModBusSphrHandle, portMAX_DELAY)) {
    if (!cycles_started) {
      HAL_GPIO_WritePin(AIR_1_GPIO_Port, AIR_1_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(AIR_2_GPIO_Port, AIR_2_Pin, GPIO_PIN_RESET);
      current_cycle = cycles_count;
      position_manual();
      continue;
    }
    if (cycles_started && (current_cycle >= cycles_count)) {
      reset_seconds();
      current_cycle = 0;
      start_is_position2 = is_position2;
      cycle_start_time = get_seconds();
      (is_position2) ? position_second() : position_first(); // To start properly
    }
    if (get_seconds() - cycle_start_time < cycle_time)
      continue;
    HAL_GPIO_WritePin(AIR_1_GPIO_Port, AIR_1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AIR_2_GPIO_Port, AIR_2_Pin, GPIO_PIN_RESET);
    toggle_is_position2();
    (is_position2) ? position_second() : position_first();
    if (is_position2 == start_is_position2) {
      ++current_cycle;
    }
    HAL_GPIO_WritePin(AIR_1_GPIO_Port, AIR_1_Pin, !is_position2 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AIR_2_GPIO_Port, AIR_2_Pin, is_position2  ? GPIO_PIN_SET : GPIO_PIN_RESET);
    if (cycles_started && (current_cycle >= cycles_count)) {
      set_cycles_started(false);
    }
    cycle_start_time = get_seconds();
  }
}