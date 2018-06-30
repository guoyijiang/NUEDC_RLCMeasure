#include "connectFPGA.h"

void SPI1_Init()
{
	GPIO_InitTypeDef GPIO_Init_Inst;
	SPI_InitTypeDef SPI_INIT_Inst;
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_SPI1,  ENABLE);
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB,  ENABLE);
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOG,  ENABLE);
	//PG 8 CS
	GPIO_Init_Inst.GPIO_Pin = GPIO_Pin_8;	
  GPIO_Init_Inst.GPIO_Mode = GPIO_Mode_OUT ;	
	GPIO_Init_Inst.GPIO_OType = GPIO_OType_PP;//�������
  GPIO_Init_Inst.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_Init_Inst.GPIO_PuPd = GPIO_PuPd_NOPULL;//��������
  GPIO_Init(GPIOG, &GPIO_Init_Inst);//��ʼ��
	FPGA_CS = 1;

	//PB14����
	GPIO_Init_Inst.GPIO_Pin = GPIO_Pin_14;
  GPIO_Init_Inst.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
  GPIO_Init_Inst.GPIO_OType = GPIO_OType_PP;//�������
  GPIO_Init_Inst.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_Init_Inst.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOB, &GPIO_Init_Inst);//��ʼ��PB14
	GPIO_SetBits(GPIOB,GPIO_Pin_14);//PB14���1,��ֹSPI FLASH����NRF��ͨ�� 
	
	//PB 3 SCK
	//PB 4 MISO
	//PB 5 MOSI
	
	GPIO_Init_Inst.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;//PB3~5���ù������	
  GPIO_Init_Inst.GPIO_Mode = GPIO_Mode_AF;//���ù���
  GPIO_Init(GPIOB, &GPIO_Init_Inst);//��ʼ��
	
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource3,GPIO_AF_SPI1); //PB3
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource4,GPIO_AF_SPI1); //PB4
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource5,GPIO_AF_SPI1); //PB5
	
	//SPI
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,ENABLE);//��λSPI1
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,DISABLE);//ֹͣ��λSPI1
	
	SPI_INIT_Inst.SPI_Direction = SPI_Direction_2Lines_FullDuplex; 
	SPI_INIT_Inst.SPI_Mode = SPI_Mode_Master;		
	SPI_INIT_Inst.SPI_DataSize = SPI_DataSize_16b;		
	SPI_INIT_Inst.SPI_CPOL = SPI_CPOL_Low;		//SCK����Ϊ��
	SPI_INIT_Inst.SPI_CPHA = SPI_CPHA_2Edge;	//�½��ز����������ظı䣩
	SPI_INIT_Inst.SPI_NSS = SPI_NSS_Soft;		//NSS�ź���Ӳ������
	SPI_INIT_Inst.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;		//84M/16
	SPI_INIT_Inst.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_INIT_Inst.SPI_CRCPolynomial = 1;
	SPI_Init(SPI1, &SPI_INIT_Inst);
 
	SPI_Cmd(SPI1, ENABLE); 
	//SPI1_ReadWriteByte(0xffff);
}
void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler)
{
  assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));//�ж���Ч��
	SPI1->CR1&=0XFFC7;//λ3-5���㣬�������ò�����
	SPI1->CR1|=SPI_BaudRatePrescaler;	//����SPI1�ٶ� 
	SPI_Cmd(SPI1,ENABLE); //ʹ��SPI1
} 

u16 SPI1_ReadWriteByte(u16 TxData)
{		 			 
	
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET){}//�ȴ���������  
	
	SPI_I2S_SendData(SPI1, TxData); //ͨ������SPIx����һ��byte  ����
		
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET){} //�ȴ�������һ��byte  

	return SPI_I2S_ReceiveData(SPI1); //����ͨ��SPIx������յ�����	    
}


/*
*���ݸ�ʽ
* 1 
* +------------+---------------+--------------------------+
* | 01(R)/10(W)|  ...(0000)... |       		addr		  |
* +------------+---------------+--------------------------+
*  	 15 14    		  		  8 7						  0

* 2(W/R)
* +-------------------------------------------------------+
* |			           data[31:16]						  |		
* +-------------------------------------------------------+

* 3(W/R)
* +-------------------------------------------------------+
* |			           data[15:0]						  |	
* +-------------------------------------------------------+
*/
void Fpga_init()
{
	SPI1_Init();
}
void Fpga_WriteReg(u16 addr,u32 data)
{
	FPGA_CS = 0;
	SPI1_ReadWriteByte((u16)(0x8000|addr));
	FPGA_CS = 1;
	delay_us(1);
	
	FPGA_CS = 0;
	SPI1_ReadWriteByte((u16)(0xFFFF&(data >>16)));
	FPGA_CS = 1;
	delay_us(1);
	
	FPGA_CS = 0;
	SPI1_ReadWriteByte((u16)(0xFFFF&(data)));
	FPGA_CS = 1;
	delay_us(1);
}
u32 Fpga_ReadReg(u16 addr)
{
	u32 temp;
	
	FPGA_CS = 0;
	SPI1_ReadWriteByte((u16)(0x4000|addr));
	FPGA_CS = 1;
	delay_us(1);
	
	FPGA_CS = 0;
	temp = SPI1_ReadWriteByte(0x5555)<<16;
	FPGA_CS = 1;
	//DebugPrintf("redH = %x\r\n",temp);
	delay_us(1);
	
	FPGA_CS = 0;
	temp = temp|SPI1_ReadWriteByte(0x5555);
	//DebugPrintf("redL = %x\r\n",temp);
	FPGA_CS = 1;	
	
	return temp;
}
