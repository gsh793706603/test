#include "motor.h"

/* 开环速度系数: 默认 1.0f; 若直行跑偏, 可单独调小对应轮的系数 */
static const float K_FL = 1.0f;
static const float K_FR = 1.0f;
static const float K_RL = 1.0f;
static const float K_RR = 1.0f;

/* ---- 单轮原语: dir>0 正转, dir<0 反转, dir==0 停止 ---- */
static void Motor_FL_Set(int dir, u16 pwm)
{
	u16 cmp = (u16)((float)pwm * K_FL);
	if(dir > 0)      { FL_IN1 = 1; FL_IN2 = 0; TIM_SetCompare1(TIM4, cmp); }
	else if(dir < 0) { FL_IN1 = 0; FL_IN2 = 1; TIM_SetCompare1(TIM4, cmp); }
	else             { FL_IN1 = 0; FL_IN2 = 0; TIM_SetCompare1(TIM4, 0);   }
}

static void Motor_FR_Set(int dir, u16 pwm)
{
	u16 cmp = (u16)((float)pwm * K_FR);
	if(dir > 0)      { FR_IN1 = 1; FR_IN2 = 0; TIM_SetCompare2(TIM4, cmp); }
	else if(dir < 0) { FR_IN1 = 0; FR_IN2 = 1; TIM_SetCompare2(TIM4, cmp); }
	else             { FR_IN1 = 0; FR_IN2 = 0; TIM_SetCompare2(TIM4, 0);   }
}

static void Motor_RL_Set(int dir, u16 pwm)
{
	u16 cmp = (u16)((float)pwm * K_RL);
	if(dir > 0)      { RL_IN1 = 1; RL_IN2 = 0; TIM_SetCompare3(TIM4, cmp); }
	else if(dir < 0) { RL_IN1 = 0; RL_IN2 = 1; TIM_SetCompare3(TIM4, cmp); }
	else             { RL_IN1 = 0; RL_IN2 = 0; TIM_SetCompare3(TIM4, 0);   }
}

static void Motor_RR_Set(int dir, u16 pwm)
{
	u16 cmp = (u16)((float)pwm * K_RR);
	if(dir > 0)      { RR_IN1 = 1; RR_IN2 = 0; TIM_SetCompare4(TIM4, cmp); }
	else if(dir < 0) { RR_IN1 = 0; RR_IN2 = 1; TIM_SetCompare4(TIM4, cmp); }
	else             { RR_IN1 = 0; RR_IN2 = 0; TIM_SetCompare4(TIM4, 0);   }
}

/* 8个方向脚: PA2/PA3/PA8/PA11 + PB0/PB1/PB12/PB13, 上电先拉低 */
void Motor_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_8 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOA, GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_8 | GPIO_Pin_11);
	GPIO_ResetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_12 | GPIO_Pin_13);
}

/* TIM4 四通道PWM(占空比刻度与旧TIM1电机PWM一致: arr=1999,psc=359 约100Hz) */
void TIM4_PWM_Init(u16 arr, u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	/* PB6/7/8/9 复用推挽 = TIM4 CH1/2/3/4 */
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	TIM_TimeBaseStructure.TIM_Period = arr;
	TIM_TimeBaseStructure.TIM_Prescaler = psc;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM4, &TIM_OCInitStructure);
	TIM_OC2Init(TIM4, &TIM_OCInitStructure);
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);

	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);

	TIM_Cmd(TIM4, ENABLE);
}

/* 统一入口: fl/fr/rl/rr 取 -1(反)/0(停)/1(正), pwm 为占空比刻度值 */
void Motor_Set(int fl, int fr, int rl, int rr, u16 pwm)
{
	Motor_FL_Set(fl, pwm);
	Motor_FR_Set(fr, pwm);
	Motor_RL_Set(rl, pwm);
	Motor_RR_Set(rr, pwm);
}

/* 麦克纳姆运动查表(X型布置):
 * Forward     + + + +      Backward     - - - -
 * StrafeLeft  + - - +      StrafeRight  - + + -
 * RotateCCW   - + - +      RotateCW     + - + -
 */
void Forward(void)    { Motor_Set( 1, 1, 1, 1, SPEED_FULL); }
void Backward(void)   { Motor_Set(-1,-1,-1,-1, SPEED_FULL); }
void StrafeRight(void){ Motor_Set(-1, 1, 1,-1, SPEED_FULL); }
void StrafeLeft(void) { Motor_Set( 1,-1,-1, 1, SPEED_FULL); }
void RotateCW(void)   { Motor_Set( 1,-1, 1,-1, SPEED_FULL); }
void RotateCCW(void)  { Motor_Set(-1, 1,-1, 1, SPEED_FULL); }
void Stop(void)       { Motor_Set( 0, 0, 0, 0, 0); }
