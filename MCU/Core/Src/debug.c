/*
 * VERY ugly debugging code. In heavy need ot refactor. Just works*
 */

#include <main.h>
#include <stdio.h>
#include <stdarg.h>

void SWO_Init(void)
{
  uint32_t swo_baud = 2000000U;

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;

  TPI->SPPR = 2U;
  TPI->ACPR = (SystemCoreClock / swo_baud) - 1U;

  ITM->LAR = 0xC5ACCE55U;
  ITM->TCR = ITM_TCR_ITMENA_Msk;
  ITM->TER = 1U;

  TPI->FFCR = 0x100U;
}

int _write(int32_t file, uint8_t *ptr, int32_t len)
{
  for (int i = 0; i < len; i++)
  {
    ITM_SendChar(*ptr++);
  }
  return len;
}
static volatile uint32_t buf_pos = 0;
static volatile char buf[4096];
void debug(const char *fmt, ...)
{
  return;
  va_list arg;
  va_start(arg, fmt);
  uint32_t chars = vsnprintf(buf+buf_pos, sizeof(buf)-buf_pos, fmt, arg);
  buf_pos += chars > sizeof(buf) - buf_pos ? chars : sizeof(buf) - buf_pos;
  va_end(arg);
}
size_t uint64_to_string(uint64_t value, char *buffer, size_t buffer_size)
{
  return 0;
  char temporary[20];
  size_t length = 0;

  if (buffer_size == 0)
    return 0;

  do {
    temporary[length++] = (char)('0' + value % 10);
    value /= 10;
  } while (value != 0 && length < sizeof temporary);

  if (length + 1 > buffer_size)
    return 0;  // Buffer too small

  for (size_t i = 0; i < length; i++)
    buffer[i] = temporary[length - 1 - i];

  buffer[length] = '\0';
  return length;
}

size_t int64_to_string(int64_t value, char *buffer, size_t buffer_size)
{
  return 0;
  char digits[19];  // At most 19 digits for int64_t
  uint64_t magnitude;
  size_t digit_count = 0;
  size_t position = 0;

  if (buffer_size == 0)
    return 0;

  if (value < 0)
    magnitude = (uint64_t)(-(value + 1)) + 1;
  else
    magnitude = (uint64_t)value;

  do {
    digits[digit_count++] = (char)('0' + magnitude % 10);
    magnitude /= 10;
  } while (magnitude != 0);

  if (value < 0)
    position++;

  if (position + digit_count + 1 > buffer_size)
    return 0;

  position = 0;

  if (value < 0)
    buffer[position++] = '-';

  while (digit_count != 0)
    buffer[position++] = digits[--digit_count];

  buffer[position] = '\0';
  return position;
}

void StartDebugTask(void *argument)
{
  /* USER CODE BEGIN StartDebugTask */
  /* Infinite loop */
  for(;;)
  {
    if (buf_pos) {
      buf[buf_pos - (buf_pos >= sizeof(buf))];
      puts(buf); // Blocking!
      buf_pos = 0;
    }
  }
  /* USER CODE END StartDebugTask */
}