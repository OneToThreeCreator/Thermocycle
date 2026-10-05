#ifndef ACCELERATION_H
#define ACCELERATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MOTOR_MICROSTEP 3200
#define MOTOR_SCREWSTEP 4

void processAcceleration(void);
void processPositionCounter(void);
void setPos(int64_t position);
int64_t getPos(void);
void resetPos(int64_t position);
int32_t getSpeed(void);
void initPositioning(void);
void setSpeedManual(int32_t speed);

#ifdef __cplusplus
}
#endif

#endif // ACCELERATION_H
