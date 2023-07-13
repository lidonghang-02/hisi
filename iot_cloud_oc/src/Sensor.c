#include <stdio.h>

#include "Sensor.h"

// 电机状态
void MotorStatusSet(MotorStatus status)
{
    if (status == FOR)
    {
        printf("MotorStatusSet FOR\r\n");
    }

    if (status == OFF)
    {
        printf("MotorStatusSet OFF\r\n");
    }

    if (status == REW)
    {
        printf("MotorStatusSet REW\r\n");
    }
}
