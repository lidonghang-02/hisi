#include <stdio.h>

#include "Sensor.h"

// 电机状态
void Begin_Clean(Clean status)
{
    if (status == CLEAN)
    {
        printf("MotorStatusSet FOR\r\n");
    }

    if (status == OFF)
    {
        printf("MotorStatusSet OFF\r\n");
    }
}
