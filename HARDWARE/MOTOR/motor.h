#ifndef __MOTOR_H
#define __MOTOR_H
#include "sys.h"

/* ---------------------------------------------------------------------------
 * 麦克纳姆轮小车电机控制 (2 x L298N + 4 x JGB37-520)
 * PWM : TIM4 CH1=PB6(FL) CH2=PB7(FR) CH3=PB8(RL) CH4=PB9(RR)
 * 方向: FL=PB0/PB1   FR=PB12/PB13   RL=PA8/PA11   RR=PA2/PA3
 * 若某轮转向与预期相反, 交换该轮的 IN1/IN2 两个宏即可
 * ------------------------------------------------------------------------- */
#define FL_IN1  PBout(0)
#define FL_IN2  PBout(1)
#define FR_IN1  PBout(12)
#define FR_IN2  PBout(13)
#define RL_IN1  PAout(8)
#define RL_IN2  PAout(11)
#define RR_IN1  PAout(2)
#define RR_IN2  PAout(3)

#define SPEED_FULL 1500		/* 全速占空比(与旧TIM1刻度一致, arr=1999) */
#define SPEED_MIN  600		/* 起步最小占空比(克服减速电机静摩擦) */

void Motor_GPIO_Init(void);					/* 8个方向脚初始化 */
void TIM4_PWM_Init(u16 arr, u16 psc);		/* PB6~PB9 = TIM4 CH1~CH4 电机PWM */
void Motor_Set(int fl, int fr, int rl, int rr, u16 pwm);	/* 统一入口: dir=-1反/0停/1正 */
void Forward(void);			/* 前进 */
void Backward(void);		/* 后退 */
void StrafeLeft(void);		/* 左横移 */
void StrafeRight(void);		/* 右横移 */
void RotateCW(void);		/* 原地右转(顺时针) */
void RotateCCW(void);		/* 原地左转(逆时针) */
void Stop(void);			/* 停止 */

#endif
