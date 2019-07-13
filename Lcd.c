/*
 * =====================================================================
 * NAME         : Lcd.c
 *
 * Descriptions : Main routine for S3C2450
 *
 * IDE          : GCC-4.1.0
 *
 * Modification
 *	  
 * =====================================================================
 */
#include <stdarg.h>
#include "2450addr.h"
#include "my_lib.h"
#include "global_var.h"

#include "./fonts/ENG8X16.H"

#define COPY(A,B) for(loop=0;loop<32;loop++) *(B+loop)=*(A+loop);

static unsigned short bfType;
static unsigned int bfSize;
static unsigned int bfOffbits;
static unsigned int biWidth, biWidth2;
static unsigned int biHeight;

static unsigned int Fbuf[2] = {0x33800000, 0x33c00000};

unsigned int fullscreen_buffer[272][480]; 	//background image buffer
unsigned int cookie_buffer[44][44]; 		//cookie image buffer
unsigned int slide_buffer[22][56];			//slide image buffer

void Lcd_Copy(unsigned int from, unsigned int to)
{
	unsigned int i;

	for(i=0; i< (LCD_SIZE_X * LCD_SIZE_Y) ; i++)
	{
		*((unsigned short *)(Fbuf[to])+i) = *((unsigned short *)(Fbuf[from])+i);
	}
}

void Lcd_Select_Frame_Buffer(unsigned int id)
{
    	Fb_ptr = (unsigned short (*)[LCD_SIZE_X])Fbuf[id];
}

void Lcd_Display_Frame_Buffer(unsigned int id)
{
	Lcd_Set_Address(Fbuf[id]);
}

void Lcd_Set_Address(unsigned int fp)
{
	rVIDW00ADD0B0 = fp;					//滚欺林家 
	rVIDW00ADD1B0 = 0;					//辆丰林家 
	rVIDW00ADD2B0 = (0<<13)|((LCD_SIZE_X*4*2)&0x1fff);
	rVIDW00ADD1B0 = 0+(LCD_SIZE_X*LCD_SIZE_Y);	//农扁 
}

void Lcd_Draw_BMP(int x, int y, const unsigned char *fp)
{
	int xx=0, yy=0;	
	unsigned int tmp;
	unsigned char tmpR, tmpG, tmpB;
	
	bfType=*(unsigned short *)(fp+0);
	bfSize=*(unsigned short *)(fp+2);
	tmp=*(unsigned short *)(fp+4);
	bfSize=(tmp<<16)+bfSize;
	bfOffbits=*(unsigned short *)(fp+10);
	biWidth=*(unsigned short *)(fp+18);    
	biHeight=*(unsigned short *)(fp+22);    

	biWidth2=(bfSize-bfOffbits)/biHeight;	

	for(yy=0;yy<biHeight;yy++)
	{
		for(xx=0;xx<biWidth;xx++)
		{
			tmpB=*(unsigned char *)(fp+bfOffbits+(biHeight-yy-1)*biWidth*3+xx*3+0);
			tmpG=*(unsigned char *)(fp+bfOffbits+(biHeight-yy-1)*biWidth*3+xx*3+1);
			tmpR=*(unsigned char *)(fp+bfOffbits+(biHeight-yy-1)*biWidth*3+xx*3+2);
			tmpR>>=3;
			tmpG>>=3;
			tmpB>>=3;
			
			if(xx<biWidth2) LCD_PUT_PIXEL(x+xx,y+yy,(tmpR<<10)+(tmpG<<5)+(tmpB<<0));
		} 
	}   
}

void Lcd_Puts(int x, int y, int color, unsigned int (* buffer)[480], char *str)
{
    	unsigned data;
  	unsigned offset,loop;
	unsigned char xs,ys;
	unsigned char temp[32];
	unsigned char bitmask[]={128,64,32,16,8,4,2,1};   
	while(*str)
	{
		data=*str++;
		offset=(unsigned)(data*16);
		COPY(eng8x16+offset,temp);
		
		for(ys=0;ys<16;ys++)
		{
			for(xs=0;xs<8;xs++)
			{
				if(temp[ys]&bitmask[xs])
				{
					LCD_PUT_PIXEL(x+xs,y+ys,color);
				} 
				else
				{
					LCD_PUT_PIXEL(x+xs,y+ys,buffer[y+ys][x+xs]);
				} 
			}
		}
		x+=8;
	}
} 

void Lcd_Printf(int x, int y, int color, unsigned int (* buffer)[480],char *fmt,...)
{
	va_list ap;
	char string[256];

	va_start(ap,fmt);
	vsprintf(string,fmt,ap);
	Lcd_Puts(x, y, color, buffer, string);
	va_end(ap);
}

void Lcd_Puts_big(int x, int y, int color, unsigned int (* buffer)[480], char *str)
{
    unsigned data;
   	unsigned offset,loop;
	unsigned char xs,ys;
	unsigned char temp[32];
	unsigned char bitmask[]={128,64,32,16,8,4,2,1};     
	
    while(*str)
    {
		data=*str++;
        	offset=(unsigned)(data*16);
		COPY(eng8x16+offset,temp);
		
		for(ys=0;ys<16;ys++)
		{
			for(xs=0;xs<8;xs++)
			{
				if(temp[ys]&bitmask[xs])
				{
					LCD_PUT_PIXEL(x+2*xs,y+2*ys+1,color);
					LCD_PUT_PIXEL(x+2*xs+1,y+2*ys,color);
					LCD_PUT_PIXEL(x+2*xs,y+2*ys,color);
					LCD_PUT_PIXEL(x+2*xs+1,y+2*ys+1,color);
				}
				else
				{
					LCD_PUT_PIXEL(x+2*xs,y+2*ys+1,buffer[y+ys][x+xs]);
					LCD_PUT_PIXEL(x+2*xs+1,y+2*ys,buffer[y+ys][x+xs]);
					LCD_PUT_PIXEL(x+2*xs,y+2*ys,buffer[y+ys][x+xs]);
					LCD_PUT_PIXEL(x+2*xs+1,y+2*ys+1,buffer[y+ys][x+xs]);
				}	   	
			} 
		}
		x+=16;
	}	
} 

/*====================================================================================*/
/*====================================================================================*/
/*====================================================================================*/
/*====================================================================================*/
/*====================================================================================*/

#define MAKE_BUFFER(BUFFER) {\
	int xx=0, yy=0;\
	unsigned int tmp;\
	unsigned char tmpR, tmpG, tmpB;\
	bfType=*(unsigned short *)(fp+0);\
	bfSize=*(unsigned short *)(fp+2);\
	tmp=*(unsigned short *)(fp+4);\
	bfSize=(tmp<<16)+bfSize;\
	bfOffbits=*(unsigned short *)(fp+10);\
	biWidth=*(unsigned short *)(fp+18);\
	biHeight=*(unsigned short *)(fp+22);\
	biWidth2=(bfSize-bfOffbits)/biHeight;\
	for(yy=0;yy<biHeight;yy++)\
	{\
		for(xx=0;xx<biWidth;xx++)\
		{\
			tmpB=*(unsigned char *)(fp+bfOffbits+(biHeight-yy-1)*biWidth*3+xx*3+0);\
			tmpG=*(unsigned char *)(fp+bfOffbits+(biHeight-yy-1)*biWidth*3+xx*3+1);\
			tmpR=*(unsigned char *)(fp+bfOffbits+(biHeight-yy-1)*biWidth*3+xx*3+2);\
			tmpR>>=3;\
			tmpG>>=3;\
			tmpB>>=3;\
			if(xx<biWidth2)\
			{\
				(BUFFER)[y+yy][x+xx] = (unsigned short)((tmpR<<10)+(tmpG<<5)+(tmpB<<0));\
			}\
		}\
	}\
}
void Lcd_Make_fullscreen_Buffer (int x, int y, const unsigned char *fp)
{
	MAKE_BUFFER(fullscreen_buffer)
}

void Lcd_Make_cookie_Buffer (int x, int y, const unsigned char *fp)
{
	MAKE_BUFFER(cookie_buffer)
}

void Lcd_Make_slide_Buffer (int x, int y, const unsigned char *fp)
{
	MAKE_BUFFER(slide_buffer)
}

void Print_cookie()
{
	int i, j;
	int x = 0;
	int y = 0;

	for ( i = 176 ; i < 220 ; i ++)
	{
		for( j = 20; j < 64 ; j ++ )
		{
			LCD_PUT_PIXEL(j, i ,(cookie_buffer[y][x]==0x0000 ? fullscreen_buffer[i][j] : cookie_buffer[y][x]));
			x++;
		}
		x = 0;
		y++;
	}	
}
