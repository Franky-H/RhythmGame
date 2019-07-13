/*
 * =====================================================================
 * NAME         : Main.c
 *
 * Descriptions : Main routine for S3C2450
 *
 * IDE          : GCC-4.1.0
 *
 * Modification
 *	  
 * =====================================================================
 */
#include "./Images/BG2.H"
#include "./Images/START.H"
#include "./Images/RUN.H"
#include "./Images/SLIDE.H"
#include "./Images/ttt.h"
#include "./Images/tt.h"

#include "iis.c"
#include "2450addr.h"
#include "my_lib.h"
#include "global_var.h"

//16  color
#define BLACK	0x0000
#define WHITE	0xFFFF
#define BLUE	    0x001F		//00000 00000 11111
#define GREEN	0x03E0		//00000 11111 00000
#define RED	0x7C00			//11111 00000 00000

int press_jump = 0;			//jump flag var
int press_slide = 0;		//slide flag var

int random1 = 0;			//1st block rand var
int random2 = 0;			//2nd block rand var

 int random3=0 ;
 int start_x3 =0;	
 int hit_check3=0 ;	


int position = 0;			//current cookie height

int num_life = 0;			//life count
int score = 0;				//current score

unsigned short (* Fb_ptr)[480];	//lcd0 buffer

int start_x1 = 480;			//block1 current Xposition
int start_x2 = 0;			//block2 current Xposition


int hit_check1 = 0;			//block1 hit check var
int hit_check2 = 0;			//block2 hit check var


int temp_start_x1 = 0;		//block1 & block2 distance store var
int temp_start_x2 = 0;

int level = 0;		//current game level var

char ptr[10][10] = {"Perfect","Miss   ","       "};	//current life string

extern unsigned int fullscreen_buffer[272][480];
extern unsigned int cookie_buffer[44][44];

void Main(void)
{	
	MMU_Init();
	Uart_Init(115200);
	Graphic_Init();
	SWITCH_Port_Init();
	Timer_Init();	
	Touch_Init();
	iis_init();
	//pISR_ADC = (unsigned int)Touch_ISR;
	//rINTSUBMSK &= ~(0x1<<9);
	//rINTMSK1 &= ~(0x1<<31);
	
	Lcd_Select_Frame_Buffer(1);
	//Lcd_Draw_BMP(0, 0, start);
	Lcd_Draw_BMP(0, 0, tt);
	Lcd_Copy(1,0);
	Lcd_Select_Frame_Buffer(0);

	
	//Lcd_Make_cookie_Buffer(0,0,run);	
	Lcd_Make_cookie_Buffer(0,0,ttt);	
	//Lcd_Make_slide_Buffer(0,0,slide);
	Lcd_Make_slide_Buffer(0,0,ttt);
	//Lcd_Make_fullscreen_Buffer(0,0,bg2);
	Lcd_Make_fullscreen_Buffer(0,0,ttt);
	
	Lcd_Display_Frame_Buffer(0);	
}
	
void game_over(void)
{
	static int first_score = 0;
	static int second_score = 0;
	static int third_score = 0;

	rTCON &= ~(0x1);			//timer stop
	rTCON &= ~(0x1<<8);
	rTCON &= ~(0x1<<12);
	
	rINTSUBMSK |= (0x1<<9);		//touch off
	rINTMSK1 |= (0x1<<31);
	


	if(score> first_score)
	{
		third_score = second_score;
		second_score = first_score;
		first_score = score;		
	}		
	else if(score> second_score)
	{
		third_score = second_score;
		second_score = score;
	}		
	else if(score> third_score)
		third_score = score;

	Lcd_Puts_big(168,60,0xffff,fullscreen_buffer,"GAME OVER");
	Lcd_Printf(196,96,0xffff,fullscreen_buffer,"== Score ==");
	
	Lcd_Printf(208,126,0xffff,fullscreen_buffer,"1st : %d",first_score);
	Lcd_Printf(208,152,0xffff,fullscreen_buffer,"2nd : %d",second_score);
	Lcd_Printf(208,178,0xffff,fullscreen_buffer,"3rd : %d",third_score);
}
