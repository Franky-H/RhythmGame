//=============================================================================
// File Name : sdi.c
// Function  : S3C2440A sdi control c source
//=============================================================================

#include <stdio.h>
#include <string.h>
#include "Def.h"
#include "option.h"
#include "2450addr.h"
#include "sdi.h"



//YH 0811#define INICLK	300000
#define INICLK	300000
//#define NORCLK	25000000
#define SDCLK	24000000	//PCLK=49.392MHz
#define MMCCLK	15000000	//PCLK=49.392MHz

#define POL	0
#define INT	1
#define DMA	2

int CMD13(void);    // Send card status
int CMD9(void);
// Global variables

//YH 0728 int *Tx_buffer;	//128[word]*16[blk]=8192[byte]
//YH 0728 int *Rx_buffer;	//128[word]*16[blk]=8192[byte]

unsigned int *Tx_buffer;	//128[word]*16[blk]=8192[byte]
unsigned int *Rx_buffer;	//128[word]*16[blk]=8192[byte]
volatile unsigned int rd_cnt;
volatile unsigned int wt_cnt;
volatile unsigned int block;
volatile unsigned int TR_end=0;


void Rd_Int(void) __attribute__((interrupt("IRQ")));
void Wt_Int(void) __attribute__((interrupt("IRQ")));
void DMA_end(void) __attribute__((interrupt("IRQ")));

#define	ClearPending1(bit) {\
			rSRCPND1 = bit;\
			rINTPND1 = bit;\
			rINTPND1;\
		}		




int Wide=0; // 0:1bit, 1:4bit
int MMC=0;  // 0:SD  , 1:MMC

int  Maker_ID;
char Product_Name[7]; 
int  Serial_Num;

volatile int RCA;

void Test_SDI(U8 sel)
{
	U32 save_rGPEUDP, save_rGPECON;
	int k;
    RCA=0;
    MMC=0;
    block=3072;   //3072Blocks=1.5MByte, ((2Block=1024Byte)*1024Block=1MByte)
//    block=13;

	save_rGPEUDP=rGPEUDP;
	save_rGPECON=rGPECON;
	
    rGPEUDP  = 0xf83f;     // SDCMD, SDDAT[3:0] => PU En.
    rGPECON = 0xaaaaaaaa;	//SDCMD, SDDAT[3:0]
//    rGPECON = 0xaaa800aa;	//SDCMD, SDDAT[3:0] => Input


    Uart_Printf("\n[SDI test]\n");
    
    if(!SD_card_init())
	return;
    
    TR_Buf_new();

    Wt_Block(sel);
	
    Rd_Block(sel);
    View_Rx_buf();

    if(MMC)
	TR_Buf_new();
    
    if(MMC)
    {
		rSDICON |=(1<<5);    // YH 0519, MMC Type SDCLK

		Wt_Stream();
		Rd_Stream();
		View_Rx_buf();
    }

    Card_sel_desel(0);	// Card deselect

    if(!CMD9())
	Uart_Printf("Get CSD fail!!!\n");
    rSDIDCON=0;//tark???
    rSDICSTA=0xffff;
	rGPEUDP=save_rGPEUDP;
	rGPECON=save_rGPECON;	
}

void TR_Buf_new(void)
{
    //-- Tx & Rx Buffer initialize
    int i, j;
    int start = 0x03020100;

    Tx_buffer=(unsigned int *)0x31000000;

    j=0;
    for(i=0;i<2048;i++)	//128[word]*16[blk]=8192[byte]
	*(Tx_buffer+i)=i+j;
    Flush_Rx_buf();
}

void Flush_Rx_buf(void)
{
    //-- Flushing Rx buffer 
    int i;

    Rx_buffer=(unsigned int *)0x31800000;

    for(i=0;i<2048;i++)	//128[word]*16[blk]=8192[byte]
	*(Rx_buffer+i)=0;
    Uart_Printf("\n--End Rx buffer flush\n");
}

void View_Rx_buf()
{
    //-- Display Rx buffer 
    int i,error=0;
    U8 ch=0;

    Tx_buffer=(unsigned int *)0x31000000;
    Rx_buffer=(unsigned int *)0x31800000;

    Uart_Printf("Check Rx data\n");

    for(i=0;i<128*block;i++)
    {
    }
    if(!error)
	Uart_Printf(" O.K.\n");
	
	Uart_Printf("[if you want view Tx/Rx buffer. Hit 'a' key!]\n");
	ch = Uart_Get_Char();
	if((ch =='a')|(ch =='A'))
		for(i=0;i<128;i++) Uart_Printf("%d:Tx-0x%08x, Rx-0x%08x\n",i,Tx_buffer[i], Rx_buffer[i]);
}

void View_Tx_buf(void)
{
    //-- Display Tx buffer 
    int i;

    //for(i=0;i<2048;i++)
	//Uart_Printf("TB[%02x]=%x,",Tx_buffer[i]);
}


int SD_card_init(void)
{
//-- SD controller & card initialize 
    int i;
    char key;


    /* Important notice for MMC test condition */
    /* Cmd & Data lines must be enabled by pull up resister */

//YH 0816, Org    rSDIPRE=PCLK/(2*INICLK)-1;	// 400KHz
    rSDIPRE=PCLK/(INICLK)-1;	// 400KHz
	Uart_Printf("Init. Frequency is %dHz\n",(PCLK/(rSDIPRE+1)));
    
//    rSDICON=(1<<4)|(1<<1)|1;	// Type B, FIFO reset, clk enable
	rSDICON=(1<<4)|1;	// Type B, clk enable
	rSDIFSTA=rSDIFSTA|(1<<16);	//YH 040223 FIFO reset
    rSDIBSIZE=0x200;		// 512byte(128word)
    rSDIDTIMER=0x7fffff;		// Set timeout count

    for(i=0;i<0x1000;i++);  // Wait 74SDCLK for MMC card

    //Uart_Printf("rSDIRSP0=0x%x\n",rSDIRSP0);
    CMD0();
    Uart_Printf("\nIn idle\n");
/*
    //-- Check MMC card OCR
    if(Chk_MMC_OCR()) 
    {
	Uart_Printf("\nIn MMC ready\n");
	MMC=1;
	goto RECMD2;
    }
*/
    Uart_Printf("MMC check end!!\n");
    //-- Check SD card OCR
    if(Chk_SD_OCR()) 
        Uart_Printf("\nIn SD ready\n");

    else
    {
		Uart_Printf("\nInitialize fail\nNo Card assertion\n");
        return 0;
    }

RECMD2:
    //-- Check attaced cards, it makes card identification state
    rSDICARG=0x0;   // CMD2(stuff bit)
    rSDICCON=(0x1<<10)|(0x1<<9)|(0x1<<8)|0x42; //lng_resp, wait_resp, start, CMD2

    //-- Check end of CMD2
    if(!Chk_CMDend(2, 1)) 
	goto RECMD2;
    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

    Uart_Printf("\nEnd id\n");

RECMD3:
    //--Send RCA
    rSDICARG=MMC<<16;	    // CMD3(MMC:Set RCA, SD:Ask RCA-->SBZ)
    rSDICCON=(0x1<<9)|(0x1<<8)|0x43;	// sht_resp, wait_resp, start, CMD3

    //-- Check end of CMD3
    if(!Chk_CMDend(3, 1)) 
	goto RECMD3;
    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

    //--Publish RCA
    if(MMC) {
	RCA=1;
//YH 0812        rSDIPRE=(PCLK/(MMCCLK)+1)-1;	// YH 0716, Normal clock=20MHz
        rSDIPRE=(PCLK/MMCCLK)-1;	// YH 0812, Normal clock=20MHz

        Uart_Printf("MMC Frequency is %dHz\n",(PCLK/(rSDIPRE+1)));
    	}
    else 
    	{
			RCA=( rSDIRSP0 & 0xffff0000 )>>16;
		    Uart_Printf("RCA=0x%x\n",RCA);

			rSDIPRE=PCLK/(SDCLK)-1;	// Normal clock=25MHz
			Uart_Printf("SD Frequency is %dHz\n",(PCLK/(rSDIPRE+1)));
    	}	//YH 0716
    //--State(stand-by) check
    if( rSDIRSP0 & 0x1e00!=0x600 )  // CURRENT_STATE check
	goto RECMD3;
    
    Uart_Printf("\nIn stand-by\n");
    
//    rSDIPRE=PCLK/(2*NORCLK)-1;	// Normal clock=25MHz
//YH 0716    rSDIPRE=PCLK/(SDCLK)-1;	// Normal clock=25MHz
//YH 0716    //Uart_Printf("SD Frequency is %dMHz\n",(PCLK/(rSDIPRE+1)));

    Card_sel_desel(1);	// Select

    if(!MMC)
	Set_4bit_bus();
    else
	Set_1bit_bus();

    return 1;
}

void Card_sel_desel(char sel_desel)
{
    //-- Card select or deselect
    if(sel_desel)
    {
RECMDS7:	
	rSDICARG=RCA<<16;	// CMD7(RCA,stuff bit)
	rSDICCON= (0x1<<9)|(0x1<<8)|0x47;   // sht_resp, wait_resp, start, CMD7

	//-- Check end of CMD7
	if(!Chk_CMDend(7, 1))
	    goto RECMDS7;
	rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

	//--State(transfer) check
	if( rSDIRSP0 & 0x1e00!=0x800 )
	    goto RECMDS7;
    }
    else
    {
RECMDD7:	
	rSDICARG=0<<16;		//CMD7(RCA,stuff bit)
	rSDICCON=(0x1<<8)|0x47;	//no_resp, start, CMD7

	//-- Check end of CMD7
	if(!Chk_CMDend(7, 0))
	    goto RECMDD7;
	rSDICSTA=0x800;	// Clear cmd_end(no rsp)
    }
}

void  Rd_Int(void)
{
    U32 i,status;

    status=rSDIFSTA;
    if( (status&0x200) == 0x200 )	// Check Last interrupt?
    {
	for(i=(status & 0x7f)/4;i>0;i--)
	{
	    *Rx_buffer++=rSDIDAT;
	    rd_cnt++;
	}
//	Uart_Printf("status1=0x%x\n", status);
//	status=rSDIFSTA&0x200;	//If you do like this, rSDIFSTA register will not be updated., YH 040221
	rSDIFSTA=rSDIFSTA&0x200;	//Clear Rx FIFO Last data Ready, YH 040221
//	Uart_Printf("rSDIFSTA=0x%x\n", rSDIFSTA);
    }
    else if( (status&0x80) == 0x80 )	// Check Half interrupt?
    {
        for(i=0;i<8;i++)
        {
    	    *Rx_buffer++=rSDIDAT;
	    rd_cnt++;
	}
    }
//	Uart_Printf("rSDIFSTA=0x%x\n", rSDIFSTA);	//YH 040221

    ClearPending1(BIT_SDI);
}

void  Wt_Int(void)
{
    ClearPending1(BIT_SDI);

    rSDIDAT=*Tx_buffer++;
    wt_cnt++;

    if(wt_cnt==128*block)
    {
	rINTMSK1 |= BIT_SDI;
	rSDIDAT=*Tx_buffer;
	TR_end=1;
    }
}

void  DMA_end(void)
{
    ClearPending1(BIT_DMA0);
    
    TR_end=1;
}

void Rd_Block(U8 sel)
{
    U32 mode;
    int status;

    rd_cnt=0;    
    Uart_Printf("[Block read test]\n");

RE0:
//    Uart_Printf("0:Polling read   1:Interrupt read   2:DMA read\nSelect the test mode?");
    mode=sel;

	Uart_Printf("\n");

    if(mode>2)
	goto RE0;

//    rSDICON = rSDICON|(1<<1);	// FIFO reset
	rSDIFSTA=rSDIFSTA|(1<<16);	// FIFO reset

    if(mode!=2)
	rSDIDCON=(2<<22)|(1<<19)|(1<<17)|(Wide<<16)|(1<<14)|(2<<12)|(block<<0);	//YH 040220
		//Word Rx, Rx after cmd, blk, 4bit bus, Rx start, blk num, data start, data transmit mode

    rSDICARG=0x0;	// CMD17/18(addr)

RERDCMD:
    switch(mode)
    {
	case POL:
	    if(block<2)	// SINGLE_READ
	    {
		rSDICCON=(0x1<<9)|(0x1<<8)|0x51;    // sht_resp, wait_resp, dat, start, CMD17
		if(!Chk_CMDend(17, 1))	//-- Check end of CMD17
		    goto RERDCMD;	    
	    }
	    else	// MULTI_READ
	    {
		rSDICCON=(0x1<<9)|(0x1<<8)|0x52;    // sht_resp, wait_resp, dat, start, CMD18
		if(!Chk_CMDend(18, 1))	//-- Check end of CMD18 
		    goto RERDCMD;
	    }

	    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)	    

	    while(rd_cnt<128*block)	// 512*block bytes
	    {
		if((rSDIDSTA&0x20)==0x20) // Check timeout 
		{
		    rSDIDSTA=(0x1<<0x5);  // Clear timeout flag
		    break;
		}
		status=rSDIFSTA;
		if((status&0x1000)==0x1000)	// Is Rx data?
		{
		    *Rx_buffer++=rSDIDAT;
		    rd_cnt++;
		}
	    }
	    break;
	
	case INT:
	    pISR_SDI=(unsigned)Rd_Int;
	    rINTMSK1 = ~(BIT_SDI);
	    
	    rSDIIMSK=5;	// Last & Rx FIFO half int.

	    if(block<2)	// SINGLE_READ
	    {
		rSDICCON=(0x1<<9)|(0x1<<8)|0x51;    // sht_resp, wait_resp, dat, start, CMD17
		if(!Chk_CMDend(17, 1))	//-- Check end of CMD17
		    goto RERDCMD;	    
	    }
	    else	// MULTI_READ
	    {
		rSDICCON=(0x1<<9)|(0x1<<8)|0x52;    // sht_resp, wait_resp, dat, start, CMD18
		if(!Chk_CMDend(18, 1))	//-- Check end of CMD18 
		    goto RERDCMD;
	    }
    
	    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

	    while(rd_cnt<128*block);

	    rINTMSK1 |= (BIT_SDI);
	    rSDIIMSK=0;	// All mask
	    break;

	case DMA:
	    pISR_DMA0=(unsigned)DMA_end;
	    rINTMSK1 = ~(BIT_DMA0);
		rSDIDCON=rSDIDCON|(1<<24); //YH 040227, Burst4 Enable

	    rDISRC0=(int)(SDIDAT);	// SDIDAT
	    rDISRCC0=(1<<1)+(1<<0);	// APB, fix
	    rDIDST0=(U32)(Rx_buffer);	// Rx_buffer
	    rDIDSTC0=(0<<1)+(0<<0);	// AHB, inc
	    rDCON0=(1<<31)+(0<<30)+(1<<29)+(0<<28)+(0<<27)+(2<<24)+(1<<23)+(1<<22)+(2<<20)+128*block;
	    //handshake, sync PCLK, TC int, single tx, single service, SDI, H/W request, 
	    //auto-reload off, word, 128blk*num
	    rDMASKTRIG0=(0<<2)+(1<<1)+0;    //no-stop, DMA2 channel on, no-sw trigger 

//	    rSDIDCON=(2<<22)|(1<<19)|(1<<17)|(Wide<<16)|(1<<15)|(2<<12)|(block<<0);
	    rSDIDCON=(2<<22)|(1<<19)|(1<<17)|(Wide<<16)|(1<<15)|(1<<14)|(2<<12)|(block<<0);
		    // Word Rx, Rx after rsp, blk, 4bit bus, dma enable, Rx start, blk num, Data start, data receive mode, YH 040220
	    if(block<2)	// SINGLE_READ
	    {
		rSDICCON=(0x1<<9)|(0x1<<8)|0x51;    // sht_resp, wait_resp, dat, start, CMD17
		if(!Chk_CMDend(17, 1))	//-- Check end of CMD17
		    goto RERDCMD;	    
	    }
	    else	// MULTI_READ
	    {
		rSDICCON=(0x1<<9)|(0x1<<8)|0x52;    // sht_resp, wait_resp, dat, start, CMD18
		if(!Chk_CMDend(18, 1))	//-- Check end of CMD18 
		    goto RERDCMD;
	    }

	    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
	    while(!TR_end);
		//Uart_Printf("rSDIFSTA=0x%x\n",rSDIFSTA);
	    rINTMSK1 |= (BIT_DMA0);
	    TR_end=0;
	    rDMASKTRIG0=(1<<2);	//DMA0 stop
	    break;

	default:
	    break;
    }
    //-- Check end of DATA
    if(!Chk_DATend()) 
	Uart_Printf("dat error\n");

	rSDIDCON=rSDIDCON&~(7<<12);		//YH 040220, Clear Data Transfer mode => no operation, Cleata Data Transfer start
	rSDIFSTA=rSDIFSTA&0x200;	//Clear Rx FIFO Last data Ready, YH 040221
    rSDIDSTA=0x10;	// Clear data Tx/Rx end detect

    if(block>1)
    {
RERCMD12:    
	//--Stop cmd(CMD12)
	rSDICARG=0x0;	    //CMD12(stuff bit)
	rSDICCON=(0x1<<9)|(0x1<<8)|0x4c;//sht_resp, wait_resp, start, CMD12

	//-- Check end of CMD12
	if(!Chk_CMDend(12, 1)) 
	    goto RERCMD12;
	rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
    }
}


void Rd_Stream(void)	// only for MMC, 3blk read
{
    int i;
    int status, rd_cnt=0;

    if(MMC!=1)
    {
	Uart_Printf("Stream read command supports only MMC!\n");
	return;
    }    
    Uart_Printf("\n[Stream read test]\n");
    
RECMD11:
//    rSDIDCON=(2<<22)|(1<<19)|(0<<17)|(0<<16)|(2<<12);
	  rSDIDCON=(2<<22)|(1<<19)|(0<<17)|(0<<16)|(1<<14)|(2<<12);


    rSDICARG=0x0;   // CMD11(addr)
    rSDICCON=(0x1<<9)|(0x1<<8)|0x4b;   //sht_resp, wait_resp, dat, start, CMD11

    while(rd_cnt<128*block)
    {
	if( (rSDIDSTA&0x20) == 0x20 )
	{
	    Uart_Printf("Rread timeout error");
	    return ;
	}
	    
	status=rSDIFSTA;
	if((status&0x1000)==0x1000)
	{
	    //*Rx_buffer++=rSDIDAT;
	    //rd_cnt++;
	    Rx_buffer[rd_cnt++]=rSDIDAT;
	}
    }

    //-- Check end of CMD11
    if(!Chk_CMDend(11, 1)) 
	goto RECMD11;
    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

    //-- Check end of DATA
//    rSDIDCON=(2<<22)|(1<<19)|(0<<17)|(0<<16)|(1<<14)|(0<<12);
    rSDIDCON=(2<<22)|(1<<19)|(0<<17)|(0<<16);	//YH 040220
    rSDIDCON=rSDIDCON&~(7<<12);		//YH 040220, no operation, data ready

    while( rSDIDSTA&0x3 !=0x0 );
    if(rSDIDSTA!=0) 
	Uart_Printf("rSDIDSTA=0x%x\n", rSDIDSTA);
    rSDIDSTA=0xff;	//YH 040221

STRCMD12:    
    //--Stop cmd(CMD12)
    rSDICARG=0x0;	    //CMD12(stuff bit)
    rSDICCON=(0x1<<9)|(0x1<<8)|0x4c; //sht_resp, wait_resp, start, CMD12

    //-- Check end of CMD12
    if(!Chk_CMDend(12, 1)) 
	goto STRCMD12;
    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
    rSDIFSTA=rSDIFSTA&0x200;	//Clear Rx FIFO Last data Ready, YH 040221
/*
    //-- Display Rx data
    //Uart_Printf("\nRx data\n");
    for(i=0;i<128*2;i++)
    {
        if(Rx_buffer[i] != Tx_buffer[i])
	{
	    //Uart_Printf("%08x, %08x\n",Tx_buffer[i], Rx_buffer[i]);
	    break;
        }
    }
*/

	Uart_Printf("rSDIFSTA1=0x%x\n", rSDIFSTA);	//YH 040221
  	rSDIFSTA=rSDIFSTA&0x200;	//Clear Rx FIFO Last data Ready, YH 040221
	Uart_Printf("rSDIFSTA2=0x%x\n", rSDIFSTA);	//YH 040221

    Uart_Printf("\n--End stream read test\n");
}


void Wt_Block(U8 sel)
{
    U32 mode;
    int i,status;

    wt_cnt=0;    
    Uart_Printf("[Block write test]\n");

RE1:
//Uart_Printf("0:Polling write   1:Interrupt write   2:DMA write\nSelect the test mode?");
    mode=sel;

    if(mode>2)
	goto RE1;

//    rSDICON |= rSDICON|(1<<1);	// FIFO reset
	rSDIFSTA=rSDIFSTA|(1<<16);	//YH 040223 FIFO reset

    if(mode!=2)
//	rSDIDCON=(2<<22)|(1<<20)|(1<<17)|(Wide<<16)|(3<<12)|(block<<0);
	rSDIDCON=(2<<22)|(1<<20)|(1<<17)|(Wide<<16)|(1<<14)|(3<<12)|(block<<0);	//YH 040220

		    //Word Tx, Tx after rsp, blk, 4bit bus, Tx start, blk num

    rSDICARG=0x0;	    // CMD24/25(addr)

REWTCMD:
    switch(mode)
    {
	case POL:
	    if(block<2)	// SINGLE_WRITE
	    {
		rSDICCON=(0x1<<9)|(0x1<<8)|0x58;	//sht_resp, wait_resp, dat, start, CMD24
		if(!Chk_CMDend(24, 1))	//-- Check end of CMD24
		    goto REWTCMD;
	    }
	    else	// MULTI_WRITE
	    {
			rSDICCON=(0x1<<9)|(0x1<<8)|0x59;	//sht_resp, wait_resp, dat, start, CMD25
			if(!Chk_CMDend(25, 1))	//-- Check end of CMD25
			goto REWTCMD;		
	    }

	    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
	    
	    while(wt_cnt<128*block)
	    {
		status=rSDIFSTA;
		if((status&0x2000)==0x2000) 
		{
		    rSDIDAT=*Tx_buffer++;
		    wt_cnt++;
			//Uart_Printf("Block No.=%d, wt_cnt=%d\n",block,wt_cnt);
		}
	    }
	    break;
	
	case INT:
	    pISR_SDI=(unsigned)Wt_Int;
	    rINTMSK1 = ~(BIT_SDI);

	    rSDIIMSK=0x10;  // Tx FIFO half int.

	    if(block<2)	    // SINGLE_WRITE
	    {
		rSDICCON=(0x1<<9)|(0x1<<8)|0x58;    //sht_resp, wait_resp, dat, start, CMD24
		if(!Chk_CMDend(24, 1))	//-- Check end of CMD24
		    goto REWTCMD;
	    }
	    else	    // MULTI_WRITE
	    {
		rSDICCON=(0x1<<9)|(0x1<<8)|0x59;    //sht_resp, wait_resp, dat, start, CMD25
		if(!Chk_CMDend(25, 1))	//-- Check end of CMD25 
		    goto REWTCMD;
	    }

	    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

	    while(!TR_end);
	    //while(wt_cnt<128);

	    rINTMSK1 |= (BIT_SDI);
	    TR_end=0;
	    rSDIIMSK=0;	// All mask
	    break;

	case DMA:
	    pISR_DMA0=(unsigned)DMA_end;
	    rINTMSK1 = ~(BIT_DMA0);
		rSDIDCON=rSDIDCON|(1<<24); //YH 040227, Burst4 Enable

	    rDISRC0=(int)(Tx_buffer);	// Tx_buffer
	    rDISRCC0=(0<<1)+(0<<0);	// AHB, inc
	    rDIDST0=(U32)(SDIDAT);	// SDIDAT
	    rDIDSTC0=(1<<1)+(1<<0);	// APB, fix
	    rDCON0=(1<<31)+(0<<30)+(1<<29)+(0<<28)+(0<<27)+(2<<24)+(1<<23)+(1<<22)+(2<<20)+128*block;
	    //handshake, sync PCLK, TC int, single tx, single service, SDI, H/W request, 
	    //auto-reload off, word, 128blk*num
	    rDMASKTRIG0=(0<<2)+(1<<1)+0;    //no-stop, DMA0 channel on, no-sw trigger
	    
//	    rSDIDCON=(2<<22)|(1<<20)|(1<<17)|(Wide<<16)|(1<<15)|(3<<12)|(block<<0);
		rSDIDCON=(2<<22)|(1<<20)|(1<<17)|(Wide<<16)|(1<<15)|(1<<14)|(3<<12)|(block<<0);	//YH 040220

		    // Word Tx, Tx after rsp, blk, 4bit bus, dma enable, Tx start, blk num
	    if(block<2)	    // SINGLE_WRITE
	    {
		rSDICCON=(0x1<<9)|(0x1<<8)|0x58;    //sht_resp, wait_resp, dat, start, CMD24
		if(!Chk_CMDend(24, 1))	//-- Check end of CMD24
		    goto REWTCMD;	    
	    }
	    else	    // MULTI_WRITE
	    {
		rSDICCON=(0x1<<9)|(0x1<<8)|0x59;    //sht_resp, wait_resp, dat, start, CMD25
		if(!Chk_CMDend(25, 1))	//-- Check end of CMD25 
		    goto REWTCMD;	    
	    }

	    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

	    while(!TR_end);		

	    rINTMSK1 |= (BIT_DMA0);
	    TR_end=0;
	    rDMASKTRIG0=(1<<2);	//DMA0 stop

	    break;

	default:
	    break;
    }
    
    //-- Check end of DATA
    if(!Chk_DATend()) 
	Uart_Printf("dat error\n");

	rSDIDCON=rSDIDCON&~(7<<12);		//YH 040220, Clear Data Transfer mode => no operation, Cleata Data Transfer start
    rSDIDSTA=0x10;	// Clear data Tx/Rx end

    if(block>1)
    {
	//--Stop cmd(CMD12)
REWCMD12:    
//	rSDIDCON=(1<<18)|(1<<17)|(0<<16)|(1<<12)|(block<<0);
	rSDIDCON=(1<<18)|(1<<17)|(0<<16)|(1<<14)|(1<<12)|(block<<0); 	//YH  040220

	
	rSDICARG=0x0;	    //CMD12(stuff bit)
	rSDICCON=(0x1<<9)|(0x1<<8)|0x4c;    //sht_resp, wait_resp, start, CMD12

	//-- Check end of CMD12
	if(!Chk_CMDend(12, 1)) 
	    goto REWCMD12;
	rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

	//-- Check end of DATA(with busy state)
	if(!Chk_BUSYend()) 
	    Uart_Printf("error\n");
	rSDIDSTA=0x08;	//! Should be cleared by writing '1'.
    }
}

void Wt_Stream(void)	// only for MMC, 3blk write
{
    int status, wt_cnt=0;

    if(MMC!=1)
    {
	Uart_Printf("Stream write command supports only MMC!\n");
	return;
    }
    Uart_Printf("\n[Stream write test]\n");
RECMD20:
//    rSDIDCON=(2<<22)|(1<<20)|(0<<17)|(0<<16)|(3<<12);  // stream mode
    rSDIDCON=(2<<22)|(1<<20)|(0<<17)|(0<<16)|(1<<14)|(3<<12);  // stream mode


    rSDICARG=0x0;	// CMD20(addr)
    rSDICCON=(0x1<<9)|(0x1<<8)|0x54;    //sht_resp, wait_resp, dat, start, CMD20

    //-- Check end of CMD25
    if(!Chk_CMDend(20, 1)) 
	goto RECMD20;
    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

    while(wt_cnt<128*block)
    {
	status=rSDIFSTA;
	if((status&0x2000)==0x2000) 
	    rSDIDAT=Tx_buffer[wt_cnt++];
    }

    //-- Check end of DATA
    while( rSDIFSTA&0x400 );
        // for the empty of DATA line(Hardware)
//    rSDIDCON=(1<<20)|(0<<17)|(0<<16)|(1<<14)|(0<<12);
    rSDIDCON=(1<<20)|(0<<17)|(0<<16);	//YH 040220
    rSDIDCON=rSDIDCON&~(7<<12);		//YH 040220, no operation, data ready

    while( (rSDIDSTA&0x3)!=0x0 );
    if(rSDIDSTA!=0x0) 
	Uart_Printf("rSDIDSTA=0x%x\n", rSDIDSTA);

    rSDIDSTA=0xff;	//Clear rSDIDSTA

STWCMD12:    
    //--Stop cmd(CMD12)
//    rSDIDCON=(1<<18)|(1<<17)|(0<<16)|(1<<12);
    rSDIDCON=(1<<18)|(1<<17)|(0<<16)|(1<<14)|(1<<12);


    rSDICARG=0x0;	    //CMD12(stuff bit)
    rSDICCON=(0x1<<9)|(0x1<<8)|0x4c;   //sht_resp, wait_resp, start, CMD12

    //-- Check end of CMD12
    if(!Chk_CMDend(12, 1)) 
	goto STWCMD12;
    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

    //-- Check end of DATA(with busy state)
    if(!Chk_BUSYend()) 
	Uart_Printf("error\n");
    rSDIDSTA=0x08;

    Uart_Printf("\n--End Stream write test\n");
}

/*0,0*/
int Chk_CMDend(int cmd, int be_resp)
//0: Timeout
{
    int finish0;
    Uart_Printf("Here\n");
    if(!be_resp)    // No response
    {
    	Uart_Printf("Here1.1\n");
    	finish0=rSDICSTA;
		while((finish0&0x800)!=0x800)	// Check cmd end
		{	
	    finish0=rSDICSTA;
	    Uart_Printf("finish0:%d\n",finish0);
	}
	rSDICSTA=finish0;// Clear cmd end state
	Uart_Printf("Here2\n");
	return 1;
    }
    else	// With response
    {
    	Uart_Printf("Here3\n");
    	finish0=rSDICSTA;
	while( !( ((finish0&0x200)==0x200) | ((finish0&0x400)==0x400) ))    // Check cmd/rsp end
//	while( !( ((finish0&0x200)==0x200) | ((finish0&0x400)==0x800) ))	// Check cmd/rsp end
//	while((finish0&0x1800)!=0x1800) //YH 0811 Check cmd end
//	Uart_Printf("CMD%d:rSDICSTA=0x%x\n", cmd, rSDICSTA);	//YH 0811
		Uart_Printf("Here4\n");
		finish0=rSDICSTA;



//YH 0624	if(cmd==1 | cmd==9 | cmd==41)	// CRC no check
	if(cmd==1 | cmd==41)	// CRC no check, CMD9 is a long Resp. command.

	{
		Uart_Printf("Here5\n");
	    if( (finish0&0xf00) != 0xa00 )  // Check error
	    {
	    	Uart_Printf("Here6\n");
		rSDICSTA=finish0;   // Clear error state

		if(((finish0&0x400)==0x400))
			Uart_Printf("Here7\n");
		    return 0;	// Timeout error
    	    }
	    rSDICSTA=finish0;	// Clear cmd & rsp end state
	}
	else	// CRC check
	{
		Uart_Printf("Here8\n");
	    if( (finish0&0x1f00) != 0xa00 )	// Check error
	    {
	    Uart_Printf("Here9\n");	
		Uart_Printf("CMD%d:rSDICSTA=0x%x, rSDIRSP0=0x%x\n",cmd, rSDICSTA, rSDIRSP0);
		rSDICSTA=finish0;   // Clear error state

		if(((finish0&0x400)==0x400))
			Uart_Printf("Here10\n");
		    return 0;	// Timeout error
    	    }
	    rSDICSTA=finish0;
	}
	return 1;
    }
}

int Chk_DATend(void)
{
    int finish;

    finish=rSDIDSTA;
    while( !( ((finish&0x10)==0x10) | ((finish&0x20)==0x20) ))	
	// Chek timeout or data end
	finish=rSDIDSTA;

    if( (finish&0xfc) != 0x10 )
    {
        Uart_Printf("DATA:finish=0x%x\n", finish);
        rSDIDSTA=0xec;  // Clear error state
        return 0;
    }
    return 1;
}

int Chk_BUSYend(void)
{
    int finish;

    finish=rSDIDSTA;
    while( !( ((finish&0x08)==0x08) | ((finish&0x20)==0x20) ))
	finish=rSDIDSTA;

    if( (finish&0xfc) != 0x08 )
    {
        Uart_Printf("DATA:finish=0x%x\n", finish);
        rSDIDSTA=0xf4;  //clear error state
        return 0;
    }
    return 1;
}

void CMD0(void)
{
    //-- Make card idle state 
    rSDICARG=0x0;	    // CMD0(stuff bit)
    rSDICCON=(1<<8)|0x40;   // No_resp, start, CMD0

    //-- Check end of CMD0
    Chk_CMDend(0, 0);
    rSDICSTA=0x800;	    // Clear cmd_end(no rsp)
}

int Chk_MMC_OCR(void)
{
    int i;


    //-- Negotiate operating condition for MMC, it makes card ready state
    for(i=0;i<100;i++)	//Negotiation time is dependent on CARD Vendors.
    {
//    	rSDICARG=0xffc000;	    	    //CMD1(MMC OCR:2.6V~3.6V)
		rSDICARG=0xff8000;	    	    //CMD1(SD OCR:2.7V~3.6V)
    	rSDICCON=(0x1<<9)|(0x1<<8)|0x41;    //sht_resp, wait_resp, start, CMD1

    	//-- Check end of CMD1
// 	if(Chk_CMDend(1, 1) & rSDIRSP0==0x80ffc000)	//[31]:Card Power up status bit (busy)
		//0xffc000 is Voltage window
	if(Chk_CMDend(1, 1) && (rSDIRSP0>>16)==0x80ff) //YH 0903 [31]:Card Power up status bit (busy)
//	if(Chk_CMDend(1, 1) & rSDIRSP0==0x80ff8000) 
	{
	    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
	    return 1;	// Success
	}
    }
    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
    return 0;		// Fail
}

int Chk_SD_OCR(void)
{
    int i,j=0;

    //-- Negotiate operating condition for SD, it makes card ready state
    for(i=0;i<50;i++)	//If this time is short, init. can be fail.
    {
    	CMD55();    // Make ACMD

    	rSDICARG=0xff8000;	//ACMD41(SD OCR:2.7V~3.6V)
//    	rSDICARG=0xffc000;	//ACMD41(MMC OCR:2.6V~3.6V)
    	rSDICCON=(0x1<<9)|(0x1<<8)|0x69;//sht_resp, wait_resp, start, ACMD41

	//-- Check end of ACMD41
    	if( Chk_CMDend(41, 1) & rSDIRSP0==0x80ff8000 ) 
	{
	    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

	    return 1;	// Success	    
	}
	 // Wait Card power up status
    }
    //Uart_Printf("SDIRSP0=0x%x\n",rSDIRSP0);
    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
    return 0;		// Fail
}

int CMD55(void)
{
    //--Make ACMD
    rSDICARG=RCA<<16;			//CMD7(RCA,stuff bit)
    rSDICCON=(0x1<<9)|(0x1<<8)|0x77;	//sht_resp, wait_resp, start, CMD55

    //-- Check end of CMD55
    if(!Chk_CMDend(55, 1)) 
	return 0;

    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
    return 1;
}

int CMD13(void)//SEND_STATUS
{
    int response0;
    U8 sel;

    rSDICARG=RCA<<16;			// CMD13(RCA,stuff bit)
    rSDICCON=(0x1<<9)|(0x1<<8)|0x4d;	// sht_resp, wait_resp, start, CMD13

    //-- Check end of CMD13
    if(!Chk_CMDend(13, 1)) 
	return 0;
    //Uart_Printf("rSDIRSP0=0x%x\n", rSDIRSP0);
    if(rSDIRSP0&0x100)
	//Uart_Printf("Ready for Data\n");
//    else 
	//Uart_Printf("Not Ready\n");
    response0=rSDIRSP0;
    response0 &= 0x3c00;
    response0 = response0 >> 9;
    //Uart_Printf("Current Status=%d\n", response0);
    if(response0==6)
	Test_SDI(sel);

    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
    return 1;
}

int CMD9(void)//SEND_CSD
{
    rSDICARG=RCA<<16;				// CMD9(RCA,stuff bit)
    rSDICCON=(0x1<<10)|(0x1<<9)|(0x1<<8)|0x49;	// long_resp, wait_resp, start, CMD9

    Uart_Printf("\n****CSD register****\n");
    //-- Check end of CMD9
    if(!Chk_CMDend(9, 1)) 
	return 0;

    Uart_Printf(" SDIRSP0=0x%x\n SDIRSP1=0x%x\n SDIRSP2=0x%x\n SDIRSP3=0x%x\n", rSDIRSP0,rSDIRSP1,rSDIRSP2,rSDIRSP3);
    return 1;
}

void Set_1bit_bus(void)
{
    Wide=0;
    if(!MMC)
	SetBus();
    Uart_Printf("\n****1bit bus****\n");
}

void Set_4bit_bus(void)
{
    Wide=1;
    SetBus();
    Uart_Printf("\n****4bit bus****\n");
}

void SetBus(void)
{
SET_BUS:
    CMD55();	// Make ACMD
    //-- CMD6 implement
    rSDICARG=Wide<<1;	    //Wide 0: 1bit, 1: 4bit
    rSDICCON=(0x1<<9)|(0x1<<8)|0x46;	//sht_resp, wait_resp, start, CMD55

    if(!Chk_CMDend(6, 1))   // ACMD6
	goto SET_BUS;
    rSDICSTA=0xa00;	    // Clear cmd_end(with rsp)
}

void Set_Prt(void)
{
    //-- Set protection addr.0 ~ 262144(32*16*512) 
    Uart_Printf("[Set protection(addr.0 ~ 262144) test]\n");

RECMD28:
    //--Make ACMD
    rSDICARG=0;	    // CMD28(addr) 
    rSDICCON=(0x1<<9)|(0x1<<8)|0x5c;	//sht_resp, wait_resp, start, CMD28

    //-- Check end of CMD28
    if(!Chk_CMDend(28, 1)) 
	goto RECMD28;
    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
}

void Clr_Prt(void)
{
    //-- Clear protection addr.0 ~ 262144(32*16*512) 
    //Uart_Printf("[Clear protection(addr.0 ~ 262144) test]\n");

RECMD29:
    //--Make ACMD
    rSDICARG=0;	    // CMD29(addr)
    rSDICCON=(0x1<<9)|(0x1<<8)|0x5d;	//sht_resp, wait_resp, start, CMD29

    //-- Check end of CMD29
    if(!Chk_CMDend(29, 1)) 
	goto RECMD29;
    rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
}

