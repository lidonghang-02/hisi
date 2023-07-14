#include "SteeringEngine.h"
#include "hi_time.h"

#define CYCLE   20000
#define COUNT   20

/**
 * @brief Initialize the Steering Engine, including its IO and Function and Directinon
 * 
 */
void SteeringEngine_Init(void)
{
    IoTGpioSetOutputVal(SteeringEngine_IO, IOT_GPIO_VALUE0);
    IoTGpioInit(SteeringEngine_IO);
    IoTGpioSetFunc(SteeringEngine_IO, SteeringEngine_IOFunc);
    IoTGpioSetDir(SteeringEngine_IO, IOT_GPIO_DIR_OUT);

    SteeringEngine_SetAngle(0);
}

/**
 * @brief Simulate PWM pulse for one time to set angle by using duty, and usually used in continuous set
 * 
 * @param duty the duty between 0.5ms and 2.5ms in the cycle of Steering Engine(one cycle is 20ms)
 */
void SteeringEngine_SetAngleOnce(uint16_t duty)
{
    SteeringEngine_IOPullUp;
    hi_udelay(duty);
    SteeringEngine_IOPullDown;
    hi_udelay(CYCLE - duty);
}

/**
 * @brief Set the angle of Steering Engine fovever
 * 
 * @param angle the angle of Steering Engine you would like to set
 */
void SteeringEngine_SetAngle(uint8_t angle)
{
    uint16_t duty = angle * (2500 - 500) / 180 + 500;
    uint8_t i = 0;
    for(i = 0; i < COUNT; i++)
        SteeringEngine_SetAngleOnce(duty);
}