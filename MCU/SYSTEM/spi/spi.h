#ifndef __SPI_H
#define __SPI_H
#include "sys.h"

							  
void SPI3_Init(void);			 //��ʼ��SPI3��
void SPI3_SetSpeed(u8 SpeedSet); //����SPI3�ٶ�   
u16 SPI3_ReadWriteByte(u16 TxData);//SPI3���߶�дһ���ֽ�
		 
#endif

