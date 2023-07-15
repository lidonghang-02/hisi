#ifndef __LITTERCLEANER_H__
#define __LITTERCLEANER_H__


#include "StepMotor/StepMotor.h"

extern char Operation_Sign;
extern MotorStatus StepMotor_Status;

void LitterCleaner_KeyCallback(void);
void LitterCleaner_KeyRegconize(void);
void LimitedKey_1(void);
void LimitedKey_2(void);


#endif