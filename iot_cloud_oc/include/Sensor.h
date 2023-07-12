#ifndef __SENSOR_H__
#define __SENSOR_H__

typedef enum
{
    FOR = 0,
    REW = 1,
    OFF
} MotorStatus;

typedef struct
{
    float Pressure;
    float Distance;
} SensorData;

void MotorStatusSet(MotorStatus status);

#endif
