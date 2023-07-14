#ifndef __HCSR04_H_
#define __HCSR04_H_

// Trig 输出
// Echo 接收
#define Trig 8
#define Echo 7

void Hcsr04_Init(void);
float GetDistance(void);

#endif
