#include "protocol.h"
#include "usart.h"
#include "motor.h"
#include "timer.h"
#include "adc.h"
#include "key.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

/* --------------------------------------------------------------------------
 * 树莓派上位机通信协议 (USART1)
 *
 * 命令格式: "CMD\n" 或 "CMD,arg1,arg2,...\n"
 * 响应格式: "OK\n"  "OK,DIST,25cm\n"  "ERR,描述\n"
 *
 * 支持的命令:
 *   FR,speed    前进 (speed 0~100)
 *   BK,speed    后退
 *   SL,speed    左平移
 *   SR,speed    右平移
 *   RL,speed    原地左转
 *   RR,speed    原地右转
 *   ST          停止
 *   MW,fl,fr,rl,rr,speed   单轮控制 (1=正/0=停/-1=反)
 *   SV,position 舵机 (CCR 40~120)
 *   DIST        读超声波距离
 *   BATT        读电池电压
 *   IR          读红外避障传感器 (返回 左,右)
 *   ALL         读所有传感器
 *   MD,n        切换模式 (0=停,1=跟随,2=遥控,3=避障,5=树莓派控制)
 * -------------------------------------------------------------------------- */

/* 将速度百分比(0~100)映射到 PWM 值(0~1999) */
static u16 SpeedToPWM(int speed)
{
	if(speed < 0)   speed = 0;
	if(speed > 100) speed = 100;
	return (u16)((long)speed * 1999 / 100);
}

/* 字符串比较前缀 */
static int StartsWith(const char *str, const char *prefix)
{
	while(*prefix)
	{
		if(*str != *prefix) return 0;
		str++;
		prefix++;
	}
	return 1;
}

/* 处理运动命令: CMD,speed */
static void HandleMove(const char *cmd)
{
	const char *p;
	int speed;
	u16 pwm;

	p = strchr(cmd, ',');
	if(p == NULL)
	{
		UsartPrintf(USART1, "ERR,缺少速度参数\n");
		return;
	}
	speed = atoi(p + 1);
	pwm = SpeedToPWM(speed);

	if(StartsWith(cmd, "FR"))      Motor_Set( 1,  1,  1,  1, pwm);
	else if(StartsWith(cmd, "BK")) Motor_Set(-1, -1, -1, -1, pwm);
	else if(StartsWith(cmd, "SL")) Motor_Set( 1, -1, -1,  1, pwm);
	else if(StartsWith(cmd, "SR")) Motor_Set(-1,  1,  1, -1, pwm);
	else if(StartsWith(cmd, "RL")) Motor_Set(-1,  1, -1,  1, pwm);
	else if(StartsWith(cmd, "RR")) Motor_Set( 1, -1,  1, -1, pwm);

	UsartPrintf(USART1, "OK\n");
}

/* 处理单轮控制: MW,fl,fr,rl,rr,speed */
static void HandleMotorWheel(const char *cmd)
{
	int fl, fr, rl, rr, speed;
	u16 pwm;
	const char *p = cmd + 3;

	fl = atoi(p);   p = strchr(p, ','); if(!p) goto err; p++;
	fr = atoi(p);   p = strchr(p, ','); if(!p) goto err; p++;
	rl = atoi(p);   p = strchr(p, ','); if(!p) goto err; p++;
	rr = atoi(p);   p = strchr(p, ','); if(!p) goto err; p++;
	speed = atoi(p);

	pwm = SpeedToPWM(speed);
	Motor_Set(fl, fr, rl, rr, pwm);
	UsartPrintf(USART1, "OK\n");
	return;

err:
	UsartPrintf(USART1, "ERR,MW参数错误\n");
}

/* 处理舵机: SV,position */
static void HandleServo(const char *cmd)
{
	const char *p;
	int pos;

	p = strchr(cmd, ',');
	if(p == NULL)
	{
		UsartPrintf(USART1, "ERR,缺少舵机位置\n");
		return;
	}
	pos = atoi(p + 1);
	if(pos < 40)  pos = 40;
	if(pos > 120) pos = 120;
	TIM_SetCompare1(TIM3, (u16)pos);
	UsartPrintf(USART1, "OK\n");
}

/* 处理读距离 */
static void HandleDist(void)
{
	int dist = SR04_Distance();
	UsartPrintf(USART1, "OK,DIST,%dcm\n", dist);
}

/* 处理读电池 */
static void HandleBatt(void)
{
	u16 adcx = Get_Adc_Average(ADC_Channel_4, 5);
	float voltage = (float)adcx * (3.3f / 4096.0f) * 7.0f;
	UsartPrintf(USART1, "OK,BATT,%.2fV\n", voltage);
}

/* 处理读红外避障 */
static void HandleIR(void)
{
	int left  = IR_LEFT  ? 1 : 0;
	int right = IR_RIGHT ? 1 : 0;
	UsartPrintf(USART1, "OK,IR,%d%d\n", left, right);
}

/* 处理读全部传感器 */
static void HandleAll(void)
{
	int dist = SR04_Distance();
	u16 adcx = Get_Adc_Average(ADC_Channel_4, 5);
	float voltage = (float)adcx * (3.3f / 4096.0f) * 7.0f;
	int left  = IR_LEFT  ? 1 : 0;
	int right = IR_RIGHT ? 1 : 0;
	UsartPrintf(USART1, "OK,ALL,%d,%.2f,%d%d\n", dist, voltage, left, right);
}

/* 处理模式切换: MD,n */
static void HandleMode(const char *cmd)
{
	const char *p;
	int mode;

	p = strchr(cmd, ',');
	if(p == NULL)
	{
		UsartPrintf(USART1, "ERR,缺少模式号\n");
		return;
	}
	mode = atoi(p + 1);
	if(mode < 0 || mode > 5 || mode == 4)
	{
		UsartPrintf(USART1, "ERR,模式范围0~3,5\n");
		return;
	}
	Mode = mode;
	UsartPrintf(USART1, "OK\n");
}

/* ==================== 主解析入口 ==================== */
static void Protocol_Parse(char *cmd)
{
	/* 去除末尾空白 */
	int len = strlen(cmd);
	while(len > 0 && (cmd[len-1] == '\r' || cmd[len-1] == '\n' || cmd[len-1] == ' '))
	{
		cmd[len-1] = '\0';
		len--;
	}
	if(len == 0) return;

	/* 分发命令 */
	if(StartsWith(cmd, "FR,") || StartsWith(cmd, "BK,") ||
	   StartsWith(cmd, "SL,") || StartsWith(cmd, "SR,") ||
	   StartsWith(cmd, "RL,") || StartsWith(cmd, "RR,"))
	{
		HandleMove(cmd);
	}
	else if(StartsWith(cmd, "ST"))
	{
		Stop();
		UsartPrintf(USART1, "OK\n");
	}
	else if(StartsWith(cmd, "MW,"))
	{
		HandleMotorWheel(cmd);
	}
	else if(StartsWith(cmd, "SV,"))
	{
		HandleServo(cmd);
	}
	else if(StartsWith(cmd, "DIST"))
	{
		HandleDist();
	}
	else if(StartsWith(cmd, "BATT"))
	{
		HandleBatt();
	}
	else if(StartsWith(cmd, "IR"))
	{
		HandleIR();
	}
	else if(StartsWith(cmd, "ALL"))
	{
		HandleAll();
	}
	else if(StartsWith(cmd, "MD,"))
	{
		HandleMode(cmd);
	}
	else
	{
		UsartPrintf(USART1, "ERR,未知命令\n");
	}
}

/* ==================== 轮询接口 ==================== */
void Protocol_Poll(void)
{
	/* 检查 USART1 行接收完成标志 (bit15) */
	if(USART_RX_STA & 0x8000)
	{
		u16 len = USART_RX_STA & 0x3FFF;
		USART_RX_BUF[len] = '\0';  /* 确保字符串结尾 */
		Protocol_Parse((char *)USART_RX_BUF);
		USART_RX_STA = 0;          /* 清除标志, 准备接收下一行 */
	}
}
