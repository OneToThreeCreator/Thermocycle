#ifndef STEPPERMOTOR_H
#define STEPPERMOTOR_H

#include <assert.h>
#include <algorithm>
#include <limits>
#include <stdint.h>
#include <stdlib.h>
#include "main.h"
#include <type_traits>

template <typename T>
std::make_unsigned_t<T> absdiff(T x, T y) {
  return (x > y)
          ? static_cast<std::make_unsigned_t<T>>(x) - static_cast<std::make_unsigned_t<T>>(y)
          : static_cast<std::make_unsigned_t<T>>(y) - static_cast<std::make_unsigned_t<T>>(x);
}

template <typename T, std::enable_if_t<std::is_unsigned_v<T>, T> = 0>
T add_ovf(T x, T y, T lim=std::numeric_limits<T>::max()) {
  return (x > lim || lim - x < y) ? lim : x + y;
}

template <typename T, std::enable_if_t<std::is_unsigned_v<T>, T> = 0>
T sub_ovf(T x, T y, T lim=std::numeric_limits<T>::min()) {
  return (x < lim || x - lim < y) ? lim : x - y;
}

template <bool startstop_pin_inverted = 0>
class StepperMotor {
  private:
  volatile uint32_t coefficient = 0;
  volatile uint32_t current_speed = 0;
  TIM_TypeDef *timer;
  GPIO_TypeDef *ena_port;
  uint16_t ena_pin;
  /**/
  uint32_t speed_to_arr(uint32_t speed) {
    assert(speed > 0);
    // Must never happen. Coef = UINT32_MAX at 1 GHz is one step per ~4.2 seconds - way too slow for anything real. Even longer for slower clocks
    assert(coefficient <= UINT32_MAX - speed);
    return std::max((coefficient + speed/2) / speed, (uint32_t)1) - 1u;
  }
  protected:
  constexpr static uint32_t UPDATE_TIME = 1; // MS
  constexpr static uint32_t MINSPEED = 1;
  constexpr static uint32_t ACCELERATION_STEP = 1;
  void start_motor(uint32_t speed) {
    timer->ARR = speed_to_arr(speed);
    timer->CNT = timer->ARR;
    ena_port->BSRR = (uint32_t)ena_pin << ((unsigned)startstop_pin_inverted * 16);
    timer->CR1 |= TIM_CR1_CEN;
    current_speed = speed;
  }
  void stop_motor() {
    timer->CR1 &= ~TIM_CR1_CEN;
    ena_port->BSRR = (uint32_t)ena_pin << ((unsigned)!startstop_pin_inverted * 16);
    timer->CNT = timer->ARR;
    current_speed = 0;
  }
  public:
  volatile uint32_t target_speed = 0;
  StepperMotor(TIM_TypeDef *timer, uint32_t coefficient, GPIO_TypeDef *ena_port, uint16_t ena_pin)
    : timer{timer}, coefficient{coefficient}, ena_port{ena_port}, ena_pin{ena_pin} {
    if (coefficient == 0)
      abort();
  }
  void tick_IT() {
    uint32_t local_target_speed = target_speed;
    uint32_t speed = current_speed;
    if (local_target_speed == speed)
      return;
    if (local_target_speed == 0) {
      if (speed <= MINSPEED) {
        stop_motor();
        return;
      }
      local_target_speed = MINSPEED;
    }
    if (speed == 0) {
      start_motor(MINSPEED);
      return;
    } else {
      if (local_target_speed > speed)
        speed = add_ovf<uint32_t>(speed, ACCELERATION_STEP, local_target_speed);
      else
        speed = sub_ovf<uint32_t>(speed, ACCELERATION_STEP, MINSPEED);
    }
    timer->ARR = speed_to_arr(speed);
    current_speed = speed;
    return;
  }

  uint32_t get_speed() const {
    return current_speed;
  }

  uint64_t get_stop_steps() const  {
    // Assuming slowdown every ms. Using arithmetic progression. Rounding.
    static_assert(ACCELERATION_STEP == 1); // FIXME: take into account acceleration_step
    if (current_speed < MINSPEED)
      return 0;
    return ((uint64_t)(SystemCoreClock/(1000*UPDATE_TIME*2))*((uint64_t)(MINSPEED + current_speed)*(current_speed - MINSPEED + 1)) + coefficient/2)/coefficient;
  }
};

template <typename countertimer_type = uint16_t,
         bool startstop_pin_inverted = 0>
class PositioningStepperMotor : public StepperMotor<startstop_pin_inverted> {
private:
  volatile int64_t current_pos = 0;
  volatile int64_t target_pos = 0;
  TIM_TypeDef *counter;
  GPIO_TypeDef *dir_port;
  uint64_t stop_steps_cache = 0;
  uint32_t speed_cache = 0;
  enum State: uint32_t {
    STATE_EXT_CONTROLLED = 0,
    STATE_POSITIONING_FAST = 1,
    STATE_POSITIONING_SLOW = 2,
    STATE_BRAKING_PAST_TARGET = 3,
    STATE_START_DELAYED = 4,
    STATE_MASK = 0x0F,
    FLAG_REVERSED = 0x80
  };
  uint32_t state = STATE_EXT_CONTROLLED;
  uint16_t dir_pin;

  void update_stop_steps_cache() {
    if (this->get_speed() != speed_cache) {
      speed_cache = this->get_speed();
      stop_steps_cache = this->get_stop_steps(); // Division is slow, avoid if possible
    }
  }
  inline void sync_pos_offset() {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t result = counter->CNT;
    counter->CNT = 0;
    counter->ARR -= result;
    current_pos = !is_reversed() ? current_pos + result : current_pos - result;
    __set_PRIMASK(primask);
  }
  void set_counter_target(uint64_t steps) {
    if (steps == 0)
      abort();
    counter->ARR = static_cast<uint32_t>(
      std::min<uint64_t>(
        steps - 1,
        std::numeric_limits<countertimer_type>::max()));
  }
protected:
  constexpr static uint32_t MAX_STOP_SPEED = 15;
  constexpr static uint32_t CYCLE_MINSTEPS = 1024;
public:
  bool set_reverse(bool reverse) {
    if (is_reversed() == reverse)
      return true;
    if ((state & STATE_MASK) != STATE_EXT_CONTROLLED)
      return false;
    if (this->target_speed > 0 || this->get_speed() > 0)
      return false;
    bool sign = !is_reversed();
    dir_port->BSRR = (uint32_t)dir_pin << ((unsigned)!sign * 16);
    state = (state & STATE_MASK) | (sign ? FLAG_REVERSED : 0);
    return true;
  }
  volatile uint32_t positioning_speed;
  PositioningStepperMotor(
    TIM_TypeDef *counter, TIM_TypeDef *timer,
    uint32_t coefficient, GPIO_TypeDef *ena_port, uint16_t ena_pin,
    GPIO_TypeDef *dir_port, uint16_t dir_pin, uint32_t position_speed = 100)
    : StepperMotor<startstop_pin_inverted>(timer, coefficient, ena_port, ena_pin)
    , counter{counter}, dir_port{dir_port}, dir_pin{dir_pin}, positioning_speed{position_speed} {
    if (position_speed < this->MINSPEED)
      abort();
  }
  void tick_IT() {
    StepperMotor<startstop_pin_inverted> *super = this;
    int64_t pos = get_current_pos();
    bool sign = target_pos < pos;
    uint64_t diff = absdiff(target_pos, pos);
    switch (state & STATE_MASK) {
    case STATE_EXT_CONTROLLED: {
      //if (this->get_speed() || this->target_speed) //- Really needed?
      //  break;
      sync_pos_offset();
      set_counter_target(-1); // Pevent counter_IT from being ever called
      target_pos = current_pos;
      break;
    }
    case STATE_POSITIONING_SLOW: {
      if (sign != is_reversed()) {
        state = (state & ~STATE_MASK) | STATE_BRAKING_PAST_TARGET;
        super->target_speed = 0;
        break;
      }
      super->target_speed = this->MINSPEED;
      break;
    }
    case STATE_POSITIONING_FAST: {
      if (sign == is_reversed()) {
        update_stop_steps_cache();
        if (diff > stop_steps_cache) {
          super->target_speed = positioning_speed;
          break;
        }
        state = (state & ~STATE_MASK) | STATE_POSITIONING_SLOW;
        super->target_speed = this->MINSPEED;
        break;
      }
      state = (state & ~STATE_MASK) | STATE_BRAKING_PAST_TARGET;
      [[fallthrough]]; // If just started, reverses immediately
    }
    case STATE_BRAKING_PAST_TARGET: {
      if (super->get_speed() != 0) {
        super->target_speed = 0;
        break;
      }
      sync_pos_offset();
      dir_port->BSRR = (uint32_t)dir_pin << ((unsigned)!sign * 16);
      state = ((sign) ? FLAG_REVERSED : 0) | STATE_START_DELAYED; // Enforce at least 1ms delay before starting stepper
      break;
    }
    case STATE_START_DELAYED:
      if (diff == 0) {
        state = (state & ~STATE_MASK) | STATE_EXT_CONTROLLED;
        super->target_speed = 0;
        break;
      }
      if (is_reversed() != sign) {
        super->target_speed = 0;
      }
      set_counter_target(diff);
      state = (state & ~STATE_MASK) | STATE_POSITIONING_FAST;
      super->target_speed = positioning_speed;
      break;
    }
    super->tick_IT();
  }
  void set_target_pos(int64_t new_target) {
    uint32_t primask  = __get_PRIMASK();
    __disable_irq();
    if ((state & STATE_MASK) == STATE_EXT_CONTROLLED) {
      sync_pos_offset();
      uint64_t diff = absdiff(new_target, current_pos);
      if (diff > 0) {
        set_counter_target(diff);
      }
      else if (this->get_speed() < MAX_STOP_SPEED) {
        this->stop_motor();
        this->target_speed = 0;
        target_pos = new_target;
        __set_PRIMASK(primask);
        return;
      }
    } else if (target_pos == new_target) {
      __set_PRIMASK(primask);
      return;
    }
    target_pos = new_target;
    state = (state & ~STATE_MASK) | STATE_POSITIONING_FAST; // Can be SLOW if not manually reset
    __set_PRIMASK(primask);

  }
  int64_t get_current_pos() const {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    int64_t result = !is_reversed() ? current_pos + counter->CNT : current_pos - counter->CNT;
    __set_PRIMASK(primask);
    return result;
  }
  void reset_target_current_pos(int64_t pos) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    current_pos = pos;
    target_pos = pos;
    __set_PRIMASK(primask);
  }
  bool is_reversed() const {
    return (state & FLAG_REVERSED) != 0;
  }
  bool is_positioning() const {
    return (state & STATE_MASK) != STATE_EXT_CONTROLLED;
  }
  void counter_IT() {
    current_pos += !is_reversed() ? (int64_t)counter->ARR+1 : -(int64_t)counter->ARR-1;
    if ((state & STATE_MASK) == STATE_EXT_CONTROLLED) {
      target_pos = current_pos;
      return;
    }
    uint64_t diff = absdiff(target_pos, current_pos);
    if (diff > 0) {
      if ((target_pos < current_pos) != is_reversed()) {
        set_counter_target(-1); // Don't bother with interrupts if already missed. ~STATE_BRAKING_PAST_TARGET
        return;
      }
      if (diff - 1 > std::numeric_limits<countertimer_type>::max()
          && diff - 1 - std::numeric_limits<countertimer_type>::max() < CYCLE_MINSTEPS) {
        set_counter_target((std::numeric_limits<countertimer_type>::max()+1) / 2); // Prevent too short cycles (if we can)
      } else {
        set_counter_target(diff);
      }
      return;
    }
    set_counter_target(-1);
    if (this->get_speed() > MAX_STOP_SPEED) {
      state = (state & ~STATE_MASK) | STATE_BRAKING_PAST_TARGET;
      return;
    }
    this->stop_motor();
    this->target_speed = 0;
    state = (state & ~STATE_MASK) | STATE_EXT_CONTROLLED;
  }
};
#endif // STEPPERMOTOR_H