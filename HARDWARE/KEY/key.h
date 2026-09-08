#ifndef __KEY_H
#define __KEY_H
#include "sys.h"

#define KEY1  GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_7)//KEY1
#define KEY2  GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_12)//KEY2

/* 红外避障传感器 (0=无障碍, 1=检测到障碍) */
#define IR_LEFT   GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_5)//红外 左
#define IR_RIGHT  GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)//红外 右

#define KEY0_PRES 	1
#define KEY1_PRES	  2
#define WKUP_PRES   3

void KEY_Init(void);
void TCRT5000_Init(void);
u8 KEY_Scan(u8);
#endif
