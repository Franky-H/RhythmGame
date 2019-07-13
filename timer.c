#include "2450addr.h"
#include "my_lib.h"
#include "global_var.h"

extern unsigned int  HandleTIMER0;
extern unsigned int  HandleTIMER1;
extern unsigned int  HandleTIMER2;

extern unsigned int fullscreen_buffer[272][480]; 
extern unsigned int cookie_buffer[44][44]; 
extern unsigned int slide_buffer[22][56]; 



void Timer_Init(void)
{
	rTCFG0 = ((rTCFG0 & (~0xff)) | (0xff));				//timer0,1 
	rTCFG0 = ((rTCFG0 & (~(0xff<<8))) | (0xff<<8));		//timer2,3,4

	rTCFG1 = ((rTCFG1 & (~(0xf<<4))) | (0x3<<4));		//timer1
	rTCFG1 = ((rTCFG1 & (~(0xf<<8))) | (0x3<<8));		//timer2



	rTCON |= (0x1<<11);		//timer1 auto_reload
	
	rTCON |= (0x1<<15);		//timer2 auto_reload
	
	rTCNTB0 = 0;
	rTCMPB0 = 0;
	
	rTCNTB1 = 0;
	rTCMPB1 = 0;
		
	rTCNTB2 = 0;
	rTCMPB2 = 0;
		
	HandleTIMER1 = (unsigned int)timer1_handler;
	HandleTIMER2 = (unsigned int)timer2_handler;
	

	rINTMSK1 &= ~BIT_TIMER1;
	rINTMSK1 &= ~BIT_TIMER2;
}


void Timer1_start(int msec)
{
	rTCNTB1 = msec;
	rTCON |= (1<<9) | (0x0<<8);
	rTCON &= ~(1<<9);
	rTCON |= (0x1<<8);	
}

void Timer2_start(int msec)
{
	rTCNTB2 = 16.113*msec;	
	rTCON |= (1<<13) | (0x0<<12);
	rTCON &= ~(1<<13);
	rTCON |= (0x1<<12);
}

#define MAKE_BUMP(START_X,RANDOM,HIT_CHECK,COLOR) {\
	int y;\
	if(START_X >=0 && (START_X <=480))\
	{\
		switch(RANDOM)\
		{\
			case 0:\
				for(y=10;y<55;y++)\
					LCD_PUT_PIXEL(START_X,y,COLOR);\
				break;\
			case 1:\
				for(y=70;y<120;y++)\
					LCD_PUT_PIXEL(START_X,y,COLOR);\
				break;\
			case 2:\
				for(y=140;y<195;y++)\
					LCD_PUT_PIXEL(START_X,y,COLOR);\
				break;\
			case 3:\
				for(y=210;y<260;y++)\
					LCD_PUT_PIXEL(START_X,y,COLOR);\
				break;\
		}\
	}\
	if((START_X+SIZE_OF_BUMB) >=0 && (START_X+SIZE_OF_BUMB) <= 480)\
	{\
		int temp = START_X+SIZE_OF_BUMB;\
		switch(RANDOM)\
		{\
			case 0:\
				for(y=10;y<55;y++)\
					LCD_PUT_PIXEL(temp,y,fullscreen_buffer[y][temp]);\
				break;\
			case 1:\
				for(y=70;y<120;y++)\
					LCD_PUT_PIXEL(temp,y,fullscreen_buffer[y][temp]);\
				break;\
			case 2:\
				for(y=140;y<195;y++)\
					LCD_PUT_PIXEL(temp,y,fullscreen_buffer[y][temp]);\
				break;\
			case 3:\
				for(y=210;y<260;y++)\
					LCD_PUT_PIXEL(temp,y,fullscreen_buffer[y][temp]);\
				break;\
		}\
	}\
	START_X--;\
		if(HIT_CHECK == 0 && (START_X>14 && START_X<50))\
		{\
			switch(RANDOM)\
			{\
				case 0:\
					if(position==1)\
					{\
						Uart_Printf("Perfect\n");\
						score += 100;\
						Lcd_Printf(234,10,0xffff,fullscreen_buffer,"%s",ptr[0]);\
						HIT_CHECK++;\
					}\
					else\
					{\
						Lcd_Printf(234,10,0xffff,fullscreen_buffer,"%s",ptr[1]);\
						HIT_CHECK--;\
					}\
					break;\
				case 1:\
					if(position==2)\
					{\
						Uart_Printf("Perfect\n");\
						score += 100;\
						HIT_CHECK++;\
						Lcd_Printf(234,10,0xffff,fullscreen_buffer,"%s",ptr[0]);\
					}\
					else\
					{\
						Lcd_Printf(234,10,0xffff,fullscreen_buffer,"%s",ptr[1]);\
						HIT_CHECK--;\
					}\
					break;\
				case 2:\
					if(position==3)\
					{\
						Uart_Printf("Perfect\n");\
						score += 100;\
						HIT_CHECK++;\
						Lcd_Printf(234,10,0xffff,fullscreen_buffer,"%s",ptr[0]);\
					}\
					else\
					{\
						Lcd_Printf(234,10,0xffff,fullscreen_buffer,"%s",ptr[1]);\
						HIT_CHECK--;\
					}\
					break;\
				case 3:\
					if(position==4)\
					{\
						Uart_Printf("Perfect\n");\
						score += 100;\
						HIT_CHECK++;\
						Lcd_Printf(234,10,0xffff,fullscreen_buffer,"%s",ptr[0]);\
					}\
					else\
					{\
						Lcd_Printf(234,10,0xffff,fullscreen_buffer,"%s",ptr[1]);\
						HIT_CHECK--;\
					}\
					break;\
		}\
	}\
}\




void timer1_handler(void)
{
	rINTMSK1 |= BIT_TIMER1;
	
	rSRCPND1 |= BIT_TIMER1;
	rINTPND1 |= BIT_TIMER1;
	
		MAKE_BUMP(start_x1,random1,hit_check1,0x001F)
		if(start_x1 < -SIZE_OF_BUMB)
		{
			start_x1 =  480;
			if(level == 1)
				random1 = rand()%4;
			else
				random1 = rand()%4;
			if(hit_check1<0)
				score-=10;
			hit_check1 = 0;
		}
	
	if(level == 1)
	{
		MAKE_BUMP(start_x2,random2,hit_check2,0x7C00)
		if(start_x2 < -SIZE_OF_BUMB)
		{
			start_x2 =  480;
			if(random1 == 3)
				random2 = 0;
			else
				random2 = rand()%4;
			if(hit_check2<0)
				score-=10;
			hit_check2 = 0;
		}
	}
	


	rINTMSK1 &= ~BIT_TIMER1;
}

void timer2_handler(void)
{
	rINTMSK1 |= BIT_TIMER2;
	
	rSRCPND1 |= BIT_TIMER2;
	rINTPND1 |= BIT_TIMER2;
	int d_min,d_sec = 0;
	
	d_min = (rBCDMIN & 0xf) + 10 * ((rBCDMIN>> 4) & 0x7);
	d_sec = (rBCDSEC & 0xf) + 10 * ((rBCDSEC >> 4) & 0x7);
	
	if(level == 0 && d_sec >= 7)
	{
		level = 1;
		temp_start_x1 = start_x1;
		if(temp_start_x1 >= 240)
			start_x2 = 240+temp_start_x1;
		else
			start_x2 = 480+temp_start_x1;
	}

	if(score<-100)
	{
		game_over();
	}

	if(d_min==1 && d_sec==30)
	{
		game_over();
	} 

	Lcd_Printf(86,10,0xffff,fullscreen_buffer,"%02d:%02d",d_min,d_sec);	
	Lcd_Printf(234,10,0xffff,fullscreen_buffer,"%s",ptr[2]);
	Lcd_Printf(394,10,0xffff,fullscreen_buffer,"%5d",score);
	
	rINTMSK1 &= ~BIT_TIMER2;
}

