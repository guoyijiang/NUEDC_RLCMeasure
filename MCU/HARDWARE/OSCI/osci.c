#include "osci.h"
#include "lcd.h"
#include "lcdpro.h"
#include "connectFPGA.h"
#include "sram.h"
#include "malloc.h" 


u8 osciPoint[OSCIDATALENGTH];
u8* osciDataP = NULL;
u8* osciDataPl = NULL;
u8 osciLINE = 0;
u8 osciRUN =0;
u8 osciSINGLE = 0;

_osci osci;
_oscipoint* osciPrintArrayP;
_oscipoint* osciPrintArrayPl;

u32 osciArryUsed;
u32 osciArrylUsed;
int osci_Init(void) //��ʼ�����鲢 ��UI�ͱ���
{
	osciDataP = osciPoint;
	osciPrintArrayP = (_oscipoint*)mymalloc(SRAMEX,sizeof(_oscipoint)*OSCIARRAYLENGTH);
	if(osciPrintArrayP == NULL)
	{
		DebugPrintf("ini osciPrintArrayP wrong!\r\n");
		while(1);
	}
	osciPrintArrayPl =(_oscipoint*)mymalloc(SRAMEX,sizeof(_oscipoint)*OSCIARRAYLENGTH);
	if(osciPrintArrayPl == NULL)
	{
		DebugPrintf("ini osciPrintArrayPl wrong!\r\n");
		while(1);		
	}
	
	osciArryUsed =0;
	osciArrylUsed =0;
	
	osci_DrawUI();
	osci_DrawBack();
//	osci_DrawData();
	osci_Sample();
	return 0;
}
int osci_DrawGrid(void)//������
{
	u16 i = 0;
	u16 tempx = 0;
	
	//��ֱ����8div
	for(i=0;i<=8;i++)
	{
		if((i == 0)||(i == 4)||(i == 8) ) POINT_COLOR=BLUE;
		else POINT_COLOR= MYGRAY1;
		LCD_DrawLine(0 ,OSCISY + OSCIHIGH/8*i,OSCILENGTH-1, OSCIHIGH/8*i);
		POINT_COLOR= WHITE;
	}
	//ˮƽ10div
	for(i=0;i<=10;i++)
	{
		if((i == 0)||(i == 5)||(i == 10) ) POINT_COLOR=BLUE;
		else POINT_COLOR= MYGRAY1;
		LCD_DrawLine(OSCILENGTH/10*i ,OSCISY ,OSCILENGTH/10*i, OSCIHIGH-1);
	}	
	POINT_COLOR = WHITE;	
	return 0;
}
int osci_DrawBack(void) // ���ɫ�ͻ�����
{
	LCD_Fill(0,0,OSCILENGTH-1,OSCIHIGH-1,BLACK);
	osci_DrawGrid();
	return 0;
}
int osci_DrawData(void)//���²���ʾ���ݵ� �����»�����
{
	u16 iold =0;
	u16 inew =0;
	u16 oldx=0;
	
	osci_PrintToArray(); //�����������ݺ�used���
	
	//��������
	if(osciSINGLE == 1)	//���δ����Ȱ���һ������Ĩȥ
	{
		while(iold < osciArrylUsed)
		{
			oldx = osciPrintArrayPl[iold].x;
			POINT_COLOR= BLACK;
			while((osciPrintArrayPl[iold].x <= oldx)&&(iold<osciArrylUsed))
			{
				LCD_DrawPoint( osciPrintArrayPl[iold].x,osciPrintArrayPl[iold].y);
				iold++;
			}					
			
		}

	}
	else
	{
		while(iold < osciArrylUsed)
		{
			oldx = osciPrintArrayPl[iold].x;
			
			//Ĩȥ������ 
			POINT_COLOR= BLACK;
			while((osciPrintArrayPl[iold].x <= oldx)&&(iold<osciArrylUsed))
			{
				LCD_DrawPoint( osciPrintArrayPl[iold].x,osciPrintArrayPl[iold].y);
				iold++;
			}
			//��ӡ������
			POINT_COLOR= MYYELLOW1;
			while((osciPrintArrayP[inew].x <= oldx)&&(inew<osciArryUsed))
			{		
				LCD_DrawPoint( osciPrintArrayP[inew].x,osciPrintArrayP[inew].y);
				inew++;
			}
		}
	}
	//����
	for(;inew<osciArryUsed;inew++)
	{
		LCD_DrawPoint( osciPrintArrayP[inew].x,osciPrintArrayP[inew].y);
	}
	osci_DrawGrid();
	if(osciSINGLE ==1) osciSINGLE =2;
	else if(osciSINGLE ==2) //���δ�������
	{
		osciSINGLE =0;
		osciRUN =0;
		POINT_COLOR = WHITE;
		LCD_ShowStringCentre(620,250,690,310,24,"RUN");
		LCD_ShowStringCentre(710,320,780,380,16,"SINGLE");
	}
	POINT_COLOR = WHITE; 
	return 0;
}

int osci_Sample(void) //�����ź� Ҫ��ʱ�����read
{
		Fpga_WriteReg(3,1);
	return 0;
}
int osci_Read(void) //��ȡ����
{
	u32 i;
	u8 datatemp;
	//osci_GetState();
	if(osciSINGLE ==2)
	{
		while(1)
		{
			osci_GetState();
			if(osci.full) break;
		}
	}
	Fpga_WriteReg(3,2);//׼����ȡ
	Fpga_ReadReg(13);//��һ��������
	if(osciRUN)
	{
		for(i=0;i<OSCIDATALENGTH;i++)
		{
			
				datatemp = (u8)Fpga_ReadReg(13) + 128;
				osciDataP[i] = datatemp;
		}
	}
	else for(i=0;i<OSCIDATALENGTH;i++) Fpga_ReadReg(13);
	
	//osci_GetState();
	return 0;
}
int osci_GetState(void) //��ȡ״̬ ������
{
	//RREG[12] <={26'b0,ReadFifoState,WriteFifoState,stack_empty,stack_full}
	u8 temp;
	temp = Fpga_ReadReg(12);
	osci.full = 0x01 & temp; //1��
	osci.empty = 0x01 & (temp>>1); // 1��
	osci.writeState = 0x03&(temp>>2); //0���� 1Ԥ�� 2�ȴ����� 3����
	osci.readsState = 0x03&(temp>>4);//0�ȴ�FIFO�� 1�ɶ��ȴ���ȡ 2��������
		
	DebugStatePrint(5,"ReadState:%d  WriteState:%d  empty:%d full:%d",osci.readsState,osci.writeState,osci.empty,osci.full);
	return 0;
}
int osci_SetLinable(u8 en)//������� debug��
{
	en ? (osciLINE=1):(osciLINE=0);
	return 0;
}

int osci_DrawPointToArray(u16 x,u16 y) // �������ӡ���� ��
{
	osciPrintArrayP[osciArryUsed].x = x;
	osciPrintArrayP[osciArryUsed].y = y;
	osciArryUsed++;   //�Ƶ���
	return 0;
}
int osci_DrawLineToArray(u16 x1, u16 y1, u16 x2, u16 y2) // �������ӡ���� ��
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	delta_x=x2-x1; //������������ 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //���õ������� 
	else if(delta_x==0)incx=0;//��ֱ�� 
	else {incx=-1;delta_x=-delta_x;} 
//	else return 1;
		
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//ˮƽ�� 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //ѡȡ�������������� 
	else distance=delta_y; 
	//distance=delta_x;//��ʾ�㷨Ҫ��
	for(t=0;t<=distance+1;t++ )//������� 
	{  
		osci_DrawPointToArray(uRow,uCol);//���� 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}
  return 0;  
}  
int osci_PrintToArray(void) //��ӡ���ݵ����� 
{
	int i;
	_oscipoint* temparrayP =NULL;
	
		//��һ�εĲ��δ�Ϊ�ϲ��Σ�used��Ϊ��used �µ�used��0��ʼ
		temparrayP = osciPrintArrayP;
		osciPrintArrayP = osciPrintArrayPl;
		osciPrintArrayPl = temparrayP;
		
		osciArrylUsed = osciArryUsed;
		osciArryUsed =0;
		
		if(!osciLINE)
			for(i=0;i<OSCIDATALENGTH;i++)
				osci_DrawPointToArray( (u16)((float)i*(float)OSCILENGTH/(float)OSCIDATALENGTH-0.5f), OSCIHIGH - (u16)((float)osciDataP[i]*(float)OSCIHIGH/(float)OSCIDATARANGE)-0.5f);		
		else
			for(i=0;i<OSCIDATALENGTH -1;i++)
				osci_DrawLineToArray((u16)((float)i*(float)OSCILENGTH/(float)OSCIDATALENGTH),OSCIHIGH - (u16)((float)osciDataP[i]*(float)OSCIHIGH/(float)OSCIDATARANGE), (u16)((float)(i+1)*(float)OSCILENGTH/(float)OSCIDATALENGTH),OSCIHIGH - (u16)((float)osciDataP[i+1]*(float)OSCIHIGH/(float)OSCIDATARANGE));

	return 0;
}
int osci_DrawUI(void) // ���ƺ���ʾ����
{
		LCD_Clear(BLACK);
	
	
		POINT_COLOR = BLUE;
		//��ѹ��
		LCD_DrawRectangle(0,400,60,479);//+  0,49	400,479
	
		LCD_DrawRectangle(60 ,400,240,439);// 60,239 400 439
		LCD_DrawRectangle(60 ,439,240,479);// 60,239 440 479
	
		LCD_DrawRectangle(240,400,300,479);//- 240,299 400 479
	
		//ʱ����
		LCD_DrawRectangle(300,400,360,479);//+ 300��359 400 479
		
		LCD_DrawRectangle(360,400,540,439);// 360,539 400 439
		LCD_DrawRectangle(360,439,540,479);// 360,539 440 479
		
		LCD_DrawRectangle(540,400,600,479);//-	540,599 400,479

		POINT_COLOR = WHITE;
		LCD_ShowString(24,428,24,24,24,"+");
		LCD_ShowString(61,408,168,24,24," v/div");
		LCD_ShowString(61,448,168,24,24," mv");
		LCD_ShowString(264,428,24,24,24,"-");
	
		LCD_ShowString(324,428,24,24,24,"+");
		LCD_ShowString(361,408,168,24,24," s/div");
		LCD_ShowString(361,448,168,24,24," s");
		LCD_ShowString(564,428,24,24,24,"-");
		
		POINT_COLOR = BLUE;
		//�����load
		LCD_DrawRectangle(620,1,690,60);//SAVE
		LCD_DrawRectangle(710,1,780,60);//LOAD
		//��ʾ����	
		LCD_DrawRectangle(675,70,725,120);//Up
		LCD_DrawRectangle(675,130,725,180);//O
		LCD_DrawRectangle(615,130,665,180);//l	
		LCD_DrawRectangle(735,130,785,180);//r	
		LCD_DrawRectangle(675,190,725,240);//down
		
		LCD_DrawRectangle(620,250,690,310);//RUN
		LCD_DrawRectangle(710,250,780,310);//interpotation
		
		LCD_DrawRectangle(620,320,690,380);//AUTO
		LCD_DrawRectangle(710,320,780,380);//SINGLE		
		
		POINT_COLOR = WHITE;
		//����
		LCD_ShowStringCentre(620,1,690,60,24,"SAVE");
		LCD_ShowStringCentre(710,1,780,60,24,"LOAD");
		LCD_ShowStringCentre(675,70,725,120,24,"U");
		LCD_ShowStringCentre(675,130,725,180,24,"O");
		LCD_ShowStringCentre(615,130,665,180,24,"L");
		LCD_ShowStringCentre(735,130,785,180,24,"R");
		LCD_ShowStringCentre(675,190,725,240,24,"D");
		LCD_ShowStringCentre(620,250,690,310,24,"RUN");
		LCD_ShowStringCentre(710,250,780,310,24,"LINE");
		LCD_ShowStringCentre(620,320,690,380,24,"AUTO");
		LCD_ShowStringCentre(710,320,780,380,16,"SINGLE");
	
	
	return 0;
}

u8 osci_KeyScan(void)
{
	int i;
	int xlst,ylst;
	tp_dev.scan(0);
				if((tp_dev.sta)&(1<<0))//��0�㴥��
				{
					if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)//��LCD��Χ��
					{
						
					//	if((tp_dev.x[0] == xlst)&&(tp_dev.y[0] == ylst)) return 0;
						xlst = tp_dev.x[0];
						ylst = tp_dev.y[0];
						while(1) //ֱ������
						{
							tp_dev.scan(0);
							if((tp_dev.x[0] != xlst)&&(tp_dev.y[0] != ylst)) break;
						}
						tp_dev.x[0] = xlst;
						tp_dev.y[0] = ylst;
						
						
						if(JUDGEKEY(620,1,690,60,tp_dev.x[0],tp_dev.y[0])) return 1;	//SAVE
						if(JUDGEKEY(710,1,780,60,tp_dev.x[0],tp_dev.y[0])) return 2;	//LOAD
						if(JUDGEKEY(675,70,725,120,tp_dev.x[0],tp_dev.y[0])) return 3;	//U
						if(JUDGEKEY(675,130,725,180,tp_dev.x[0],tp_dev.y[0])) return 4;	//O
						if(JUDGEKEY(615,130,665,180,tp_dev.x[0],tp_dev.y[0])) return 5;	//L
						if(JUDGEKEY(735,130,785,180,tp_dev.x[0],tp_dev.y[0])) return 6;	//R
						if(JUDGEKEY(675,190,725,240,tp_dev.x[0],tp_dev.y[0])) return 7;	//D
						if(JUDGEKEY(620,250,690,310,tp_dev.x[0],tp_dev.y[0])) return 8;	//RUN
						if(JUDGEKEY(710,250,780,310,tp_dev.x[0],tp_dev.y[0])) return 9;	//LINE

						if(JUDGEKEY(620,320,690,380,tp_dev.x[0],tp_dev.y[0])) return 10;	//AUTO
						if(JUDGEKEY(710,320,780,380,tp_dev.x[0],tp_dev.y[0])) return 11;	//SINGLE
						
						if(JUDGEKEY(0,400,60,479,tp_dev.x[0],tp_dev.y[0])) return 12;	//v+
						if(JUDGEKEY(240,400,300,479,tp_dev.x[0],tp_dev.y[0])) return 13;	//v-
						if(JUDGEKEY(300,400,360,479,tp_dev.x[0],tp_dev.y[0])) return 14;	//t+
						if(JUDGEKEY(540,400,600,479,tp_dev.x[0],tp_dev.y[0])) return 15;	//t-
//						
//						if((tp_dev.x[0]>620)&&(tp_dev.x[0]<690)&&(tp_dev.y[0]>250)&&(tp_dev.y[0]<310)) return 8;//RUN
//						if((tp_dev.x[0]>710)&&(tp_dev.x[0]<780)&&(tp_dev.y[0]>250)&&(tp_dev.y[0]<310)) return 9;//LINE
						
					  return 0;
					}
				}
			//}
	return 0;	
	
	
}
int osci_KeyDeal(u8 keyvalue)
{
		if(keyvalue !=0)
		{
			POINT_COLOR = WHITE;
			DebugStatePrint(4,"touch:%d",keyvalue);
//			if(keyvalue==1);
//			if(keyvalue==2);
//			if(keyvalue==3);
//			if(keyvalue==4);
//			if(keyvalue==5);
//			if(keyvalue==6);
//			if(keyvalue==7);
			if(keyvalue==8)	
			{
				osciRUN = osciRUN ? 0:1;	
				POINT_COLOR = osciRUN ? MYYELLOW1:WHITE;
				LCD_ShowStringCentre(620,250,690,310,24,"RUN");
				return 0;
			}
			if(keyvalue==9)
			{
				osciLINE = osciLINE ? 0:1;
				POINT_COLOR = osciLINE ? MYYELLOW1:WHITE;
				LCD_ShowStringCentre(710,250,780,310,24,"LINE");
				return 0;
			}
			if(keyvalue==10);
			if(keyvalue==11)
			{
				osciSINGLE = 1;	
				osciRUN = 1;
				POINT_COLOR = MYYELLOW1;
				LCD_ShowStringCentre(710,320,780,380,16,"SINGLE");
				return 0;
			}
		}
		return 0;
}

