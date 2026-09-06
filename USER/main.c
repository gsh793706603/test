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

		//Mode 1: 超声波跟随 (>20cm前进, <15cm后退, 读一次距离避免重复阻塞)
		if(Mode == 1)
		{
			int dist = SR04_Distance();
			if(dist > 20)
			{
				Forward();
				delay_ms(50);
			}
			else if(dist < 15)
			{
				Backward();
				delay_ms(50);
			}
			else
			{
				Stop();
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

		//Mode 3: ���������� (�������̽ͷɨ��, ������ԭ��ת)
		if(Mode == 3)
		{
			TIM_SetCompare1(TIM3,80);	//�����ǰ
			delay_ms(200);
			if(SR04_Distance()>25)
			{
				Forward();
				delay_ms(500);
			}
			if(SR04_Distance()<25)
			{
				TIM_SetCompare1(TIM3,50);	//̽ͷת����
				delay_ms(200);
				if(SR04_Distance()>25)
				{
					RotateCW();				//�ҷ����ϰ�, ԭ����ת��ǰ��
					delay_ms(700);
				}
				else
				{
					TIM_SetCompare1(TIM3,110);	//̽ͷת����
					delay_ms(200);
					if(SR04_Distance()>25)
					{
						RotateCCW();			//�����ϰ�, ԭ����ת��ǰ��
						delay_ms(700);
					}
					else
					{
						Backward();
						delay_ms(700);
						RotateCW();
						delay_ms(700);
					}
				}
			}
		}

		//Mode 4: ����ѭ�� (��·������, �����ú�������)
		if(Mode == 4)
		{
			if(HW_1 == 0 && HW_2 == 0 && HW_3 == 0 && HW_4 == 0)
			{
				Forward();
				delay_ms(50);
			}
			if(HW_1 == 0 && HW_2 == 1 && HW_3 == 0 && HW_4 == 0)
			{
				StrafeRight();
				delay_ms(150);
			}
			if(HW_1 == 1 && HW_2 == 0 && HW_3 == 0 && HW_4 == 0)
			{
				StrafeRight();
				delay_ms(250);
			}
			if(HW_1 == 1 && HW_2 == 1 && HW_3 == 0 && HW_4 == 0)
			{
				StrafeRight();
				delay_ms(300);
			}
			if(HW_1 == 0 && HW_2 == 0 && HW_3 == 1 && HW_4 == 0)
			{
				StrafeLeft();
				delay_ms(150);
			}
			if(HW_1 == 0 && HW_2 == 0 && HW_3 == 0 && HW_4 == 1)
			{
				StrafeLeft();
				delay_ms(250);
			}
			if(HW_1 == 0 && HW_2 == 0 && HW_3 == 1 && HW_4 == 1)
			{
				StrafeLeft();
				delay_ms(300);
			}
		}
	 }
 }
