#ifndef __LED_H
#define __LED_H
#include "sys.h"

/* 电机方向/运动函数已移至 HARDWARE/MOTOR/motor.h;
 * 超声波 Trig(SR04) 宏及初始化已移至 HARDWARE/TIMER/timer.h */
#define LED  PCout(13)	/* PC13 状态指示灯 */

void LED_Init(void);	/* 初始化LED(开漏) */

#endif
