#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "timer.h"
#include "exti.h"
#include "key.h"
#include "oled.h"
#include "adc.h"
#include "motor.h"
#include "protocol.h"

int time = 0;
int Distance = 0;
int g_USART1_FLAG = 0;
int g_USART3_FLAG = 0;
extern u8  TIM2CH2_CAPTURE_STA;	//���벶��״̬
extern u16 TIM2CH2_CAPTURE_VAL;	//���벶��ֵ
int Mode = 0;
uint8_t string[10] = {0};

 int main(void)
 {
	 uint16_t adcx;
	 float temp;

	 NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);		//�ж����ȼ�����
	 delay_init();	    //��ʱ��ʼ��
	 uart1_init(115200);	//����1��ʼ��
	 uart3_init(115200);	//�������ڳ�ʼ��
	 LED_Init();	  	//״ָ̬ʾ�Ƴ�ʼ��
	 Motor_GPIO_Init();	//L298N�������ų�ʼ��
	 EXTIX_Init();		//�ⲿ�ж�(����/ģʽ)��ʼ��
	 TCRT5000_Init();	//����ѭ����ʼ��
	 OLED_Init();
	 OLED_Clear();
	 Adc_Init();
	 TIM4_PWM_Init(1999,359);		//4·���PWM(TIM4)��ʼ��, 100Hz
	 TIM3_PWM_Init(999,1439);		//���PWM(TIM3)��ʼ��, 50Hz
	 Stop();						//�ϵ缴ͣ(PWM=0), ��ֹ����
	 SR04_GPIO_Init();
	 TIM2_Cap_Init(0xFFFF,71);		//�������ز����벶��, 1MHz

	 while(1)
	 {
		//OLED ��ʾ��ص�ѹ
		adcx=Get_Adc_Average(ADC_Channel_4,10);
		temp=(float)adcx*(3.3/4096);
		sprintf((char *)string,"U:%.2f   ",(temp*7));
		OLED_ShowString(12,0,string,16);

		//OLED ��ʾ����������
		sprintf((char *)string,"D:%d      ",SR04_Distance());
		OLED_ShowString(12,3,string,16);

		//OLED ��ʾ��ǰģʽ
		if(Mode == 5)
			sprintf((char *)string,"Mode:RPi  ");
		else
			sprintf((char *)string,"Mode:%d   ",Mode);
		OLED_ShowString(12,6,string,16);

		UsartPrintf(USART3,"Mode:%d",Mode);

		//Mode 0: ֹͣ
		if(Mode == 0)
		{
			TIM_SetCompare1(TIM3,80);	//�������
			delay_ms(200);
			Stop();
		}

		//Mode 1: 增强跟随 (超声波 + 红外避障)
		if(Mode == 1)
		{
			// 红外紧急避障优先
			if(IR_LEFT == 1 && IR_RIGHT == 1)
			{
				Backward();
				delay_ms(300);
			}
			else if(IR_LEFT == 1)
			{
				RotateCW();		//左边有障碍, 右转
				delay_ms(200);
			}
			else if(IR_RIGHT == 1)
			{
				RotateCCW();	//右边有障碍, 左转
				delay_ms(200);
			}
			else
			{
				// 超声波跟随
				int dist = SR04_Distance();
				if(dist > 30)
				{
					Forward();
					delay_ms(50);
				}
				else if(dist > 15)
				{
					Forward();
					delay_ms(30);	//慢速靠近
				}
				else if(dist < 10)
				{
					Backward();
					delay_ms(50);
				}
				else
				{
					Stop();			//10~15cm 停止跟随
				}
			}
		}

		//Mode 2: �������� (USART3), ����=����
		if(Mode == 2)
		{
			if(g_USART3_FLAG == 1)		//ǰ
			{
				LED = ~LED;
				Forward();
				delay_ms(100);
				g_USART3_FLAG = 0;
			}
			if(g_USART3_FLAG == 2)		//��
			{
				LED = ~LED;
				Backward();
				delay_ms(100);
				g_USART3_FLAG = 0;
			}
			if(g_USART3_FLAG == 3)		//�Һ���
			{
				LED = ~LED;
				StrafeRight();
				delay_ms(100);
				g_USART3_FLAG = 0;
			}
			if(g_USART3_FLAG == 4)		//�����
			{
				LED = ~LED;
				StrafeLeft();
				delay_ms(100);
				g_USART3_FLAG = 0;
			}
			if(g_USART3_FLAG == 5)		//ֹͣ
			{
				LED = ~LED;
				Stop();
				delay_ms(100);
				g_USART3_FLAG = 0;
			}
		}

		//Mode 3: 增强避障 (超声波 + 红外, 无舵机)
		if(Mode == 3)
		{
			int dist = SR04_Distance();

			// 红外近距紧急处理
			if(IR_LEFT == 1 && IR_RIGHT == 1)
			{
				Backward();
				delay_ms(400);
				RotateCW();
				delay_ms(500);
			}
			else if(IR_LEFT == 1)
			{
				RotateCW();		//左边有障碍, 右转
				delay_ms(400);
			}
			else if(IR_RIGHT == 1)
			{
				RotateCCW();	//右边有障碍, 左转
				delay_ms(400);
			}
			else if(dist < 30)
			{
				// 超声波检测到前方障碍
				RotateCCW();
				delay_ms(500);
			}
			else
			{
				Forward();
				delay_ms(100);
			}
		}

		//Mode 5: 树莓派上位机控制模式
		if(Mode == 5)
		{
			Protocol_Poll();  // 轮询处理来自树莓派的协议命令
		}
	 }
 }
