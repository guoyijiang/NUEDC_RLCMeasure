#include "sys.h"
#include "misc.h"
#include "delay.h"
#include "usart.h"
#include "key.h"
#include "led.h"
#include "lcd.h"
#include "lcdpro.h"
#include "connectFPGA.h"
#include "touch.h" 
#include "spi.h"
#include "rlcmeasure.h"
#include "ui1.h"
#include "timer.h"
#include "osci.h"
#include "sram.h"
#include "malloc.h" 
#include "dac8811.h"
#include "usmart.h"

#define	DA8811CS 		PAout(15)  		//CS
#define ADS8866CS   PAout(15)  		//CS

int DA8811_Init()
{
  GPIO_InitTypeDef  GPIO_InitStructure;
 
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOB时钟
//  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);//使能GPIOG时钟

	//GPIOA15
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;//PA15
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//输出
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化

//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;//PG7
//  GPIO_Init(GPIOG, &GPIO_InitStructure);//初始化
 
//	GPIO_SetBits(GPIOG,GPIO_Pin_7);//PG7输出1,防止NRF干扰SPI FLASH的通信 
	
	DA8811CS=1;			//SPI FLASH不选中
	
	SPI3_Init();		   			//初始化SPI
	//SPI3_SetSpeed(SPI_BaudRatePrescaler_4);		//设置为21M时钟,高速模式 
	return 0;
}
void DAC8811_writereg(u16 DATA)
{
	DA8811CS = 0;
	SPI3_ReadWriteByte((u16)DATA);
	DA8811CS = 1;
	//delay_us(2);
}
//+-Vref
int DAC8811_SetVoltage(double voltage)
{
	//y = 0.9763711 x - 0.0635625 
	float x;
	// x= (y+0.0635625)/0.9763711
	
	//if((voltage < DAVREF)&&(voltage > -1.0 * DAVREF))
	x = (voltage+0.0635625)/0.9763711;
	
		DAC8811_writereg( (int16_t)( (x/DAVREF +1)*32768.0) );
	return 0;
}


int ADS8866_Init()
{
	 GPIO_InitTypeDef  GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOB时钟

	//GPIOA15
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;//PA15
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//输出
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
	
	ADS8866CS=1;			//SPI FLASH不选中
	SPI3_Init();		   			//初始化SPI // 
	return 0;
}
u16 ADS8866_readreg()
{
	u16 temp;
	ADS8866CS = 0;
	delay_us(1);
	temp = SPI3_ReadWriteByte(0xaaaa);
	delay_us(1);
	ADS8866CS = 1;
	delay_us(10);
	return temp;
}
double ADS8866_SampleVoltage()
{

	 return (double)((u16)ADS8866_readreg())/65536.0 * ADVREF -2.0;
}

int testADDA_FPAG()
{
	
	u32 data;
	double u;
	
	data = 0x0000ffff & Fpga_ReadReg(16);
	u = ((double)data)/65536.0 * 4.5;
//	DebugPrintf("data= %x\tu= %x\r\n",data,u);
//	delay_ms(300);
	return 0;
}

char timer3ItFlag = 0;
//定时器3中断服务函数
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //溢出中断
	{
		timer3ItFlag = 1;
		LED1=!LED1;//DS1翻转
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位
}

int funtest()
{
	
	u16 i;
	u32 temp;
	double utemp;
	double sum =.0f;
	for(i=0;i<ADNSAMPLE;i++)
	{
		temp = Fpga_ReadReg(AddrRegAD);
		utemp = ((double)temp)/65536.0 * ADVREF;
		sum += utemp;
		delay_us(100);
	}
	
	utemp = sum/ADNSAMPLE;
	DebugStatePrint(5,"v= %f",utemp);
	
	return 0;
}
int main(void)
{
	RLC_Measure RLC;
	KEY1STRUCT key1array[NKEY1];
	
	u32 temp;
	int keyvalue =0;
	int keyvaluetemp =0;

	//系统初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	delay_init(168);
	uart_init(115200);
	usmart_dev.init(84); 	//初始化USMART		
	
	TIM3_Int_Init(3000-1,8400-1); //0.1ms*3000 = 300ms
	
	LED_Init();
	LCD_Init();	
	tp_dev.init();
	
	POINT_COLOR=BLACK;
	BACK_COLOR = WHITE;
	
	KEY_Init();
	SPI1_Init();
	DA8811_Init(); 
	
	rlc_Initial(&RLC);
	delay_ms(5);
		
	//DebugPrintf("Hello from MCU\r\n");
	Fpga_WriteReg(0,520);
	temp = Fpga_ReadReg(0);
	//if(temp == 520) DebugPrintf("ConnectFPGA OK...\r\n");
	
	rlc_SetFreq(&RLC, 1000.0);
	temp = Fpga_ReadReg(2);

	setFDA(65536,0);
	setDAC8811(65536,0);
	keyBoard1_Generate(key1array);
	
	
	while(1)
	{

		keyvaluetemp = keyboard1_Input(key1array);	
		
		if(keyvaluetemp == 0);
		else 
		{
			keyvalue = keyvaluetemp;
			keyvaluetemp =0;
			DebugStatePrint(10,"touch:%d",keyvalue);
		}
		if(timer3ItFlag)
		{
			rlc_Measure(&RLC,keyvalue);
			
			if(keyvalue == 2)
				rlc_SetFreq(&RLC, 1000.0);
			else if(keyvalue == 3)
				rlc_SetFreq(&RLC, 10000.0);
			else if(keyvalue == 4)
				rlc_SetFreq(&RLC, 100000.0);
			keyvalue =0;
			timer3ItFlag =0;
			
			temp = Fpga_ReadReg(2);
			
		}	
	}
	
}
