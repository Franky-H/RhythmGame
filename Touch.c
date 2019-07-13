#include "2450addr.h"
#include "my_lib.h"
#include "global_var.h"

void Touch_Init(void);

extern unsigned int HandleADC;
extern unsigned int fullscreen_buffer[272][480];
extern unsigned int cookie_buffer[44][44];
extern unsigned int slide_buffer[22][56]; 

void Touch_Init(void)
{
	rADCDLY = (50000); 
	
	rADCCON |= (0x1<<14);
	rADCCON |= (39<<6);
	rADCCON &= ~(0x1<<3);
	rADCCON &= ~(0x1<<2);
	rADCCON &= ~(0x1<<1);
	rADCCON &= ~(0x1);

	rADCTSC = (0x0d3);	//interrupt mode & detect stylus down 
		
	HandleADC = (unsigned int)Touch_ISR;
}

volatile  int ADC_x, ADC_y;

void Touch_ISR()
{
	rINTSUBMSK |= (0x1<<9);
	rINTMSK1 |= (0x1<<31);	
	
	rSUBSRCPND |= (0x1<<9);
	rSRCPND1 |= (0x1<<31);
	rINTPND1 |= (0x1<<31);

	if(rADCTSC & 0x100)
	{
		rADCTSC &= (0xff); 
		position =0;
	}
	
	else
	{
		rADCTSC &=~(0x1<<8);				//detect stylus down
		
		rADCTSC &= ~(0x3);
		rADCTSC |= (0x1<<2);
		
		rADCCON |= (0x1);
		while(!(rADCCON & (1<<15)));
		rADCCON &= ~(0x1);
		
		ADC_x = (rADCDAT0 & 0x3ff);
		ADC_y = (rADCDAT1 & 0x3ff);
		
		if((ADC_x >=764) && (ADC_x <= 838) && (ADC_y >=598) && (ADC_y <= 660))
		{
			position =1;
		}
		if((ADC_x >=760) && (ADC_x <= 841) && (ADC_y >=508) && (ADC_y <= 582))
		{
			position =2;
		}
		if((ADC_x >=758) && (ADC_x <= 840) && (ADC_y >=421) && (ADC_y <= 489))
		{
			position =3;
		}
		if((ADC_x >=763) && (ADC_x <= 832) && (ADC_y >=342) && (ADC_y <= 402))
		{
			position =4;
		}
		
		/*if(press_slide == 0 && press_jump < 2 && ADC_x >= 650 && ADC_y <= 440)
		{
			CLEAR_COOKIE
			press_jump++;
		}
		else if(press_jump == 0 && ADC_x <= 360 && ADC_y <= 440)
		{
			CLEAR_COOKIE
			press_slide = 1;
		}*/
		rADCTSC = (0x1d3);	//interrupt mode & detect stylus up 
	}
	rINTSUBMSK &= ~(0x1<<9);
	rINTMSK1 &= ~(0x1<<31);
}
