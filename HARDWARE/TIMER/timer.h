#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"

extern int time;
extern int Distance;
extern uint8_t string[10];

/* SR04 超声波 Trig 引脚(PA0) */
#define SR04 PAout(0)

void TIM3_Int_Init(u16 arr,u16 psc);
void TIM3_PWM_Init(u16 arr,u16 psc);
void TIM2_Cap_Init(u16 arr,u16 psc);
void SR04_GPIO_Init(void);
int SR04_Distance(void);
#endif
