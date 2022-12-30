#include <stdio.h>
#include <stdlib.h>
#include <bget.h>

#include <global.h>
#include <defreg.h>
#include <gpio.h>
#include <uart.h>
#include <tmr.h>
#include <intc.h>
#include <sata.h>
#include <ata.h>
#include <atai.h>
#include <ataopt.h>
#include <ipm.h>
#include <ftl.h>
#include <nand_sys.h>
#include <spi.h>
#include <ftl.h>

extern byte bWBM_DMAWrite(dword nSSect, dword nSectCnt);
extern byte bWBM_DMARead(dword nSSect, dword nSectCnt);
extern byte bWBM_PIOWrite(dword nSSect, dword nSectCnt);
extern byte bWBM_PIORead(dword nSSect, dword nSectCnt);
extern void timerISRCB_ii(void);

void vFlushAll(void);
static void SysDataAccess_trap(bool_T boRead, UINT8 *pSYSinfo);

byte 	boCPUpowerSaving;
dword	gdwSystemTimer; //record the time tick since booting
dword	gdwSystemLastAccess; //record last host access time
dword	gdwSystemInitTime; // record Host reset init time
byte 	gFlashModeMask1, gFlashModeMask2;
byte	SYSTEM_TEMPTURE;
dword	gdwThermalCheckLastTick;
byte 	FlashMask1, FlashMask2;
byte 	bFlagQEGPIO,bFlagSEGPIO;


struct softTimer_S gSysTimer;

//extern void vFTL_InitDefineValue();
dword	gdwDRAMAllocSize;

//for lab3
int P_flag = 0;

extern bool boNAND_Init(int hostif);
/****************************************************************************************/
// system initial function
/****************************************************************************************/
void initialize_system(void)
{
	struct ata_stuff ataStuff;
	dword nCount;
	
	//2009-07-09 force loader to reload program once reset occur
	rF_LDR_PATTERN = 0x0;
	
	/* initialize bget */
	if(CONFIG_PROG_IS_RELOADED)
		bpool((void *) BGET_ALLOC_BASE, BGET_ALLOC_SIZE);
	
	/* initialize peripheral */
	IntcInit();
	UARTinit();
	TmrInit(dwTmrSysClock(0));
	
   if(SPI_BOOT)
	   UARTputs("\rSPI_BOOT\r");
	
	UARTprintf("SysClk:%dMHz\r",(dwTmrSysClock(0))/1000000);
	
	FlashMask1 = rSYS_PIN & MAK_FLASH_MODE1;
	FlashMask2 = rSYS_PIN & MAK_FLASH_MODE2;
 
	// For detect flash transfer mode setting, for 5 Flash ID the same on legacy and toggle mode flash
	gFlashModeMask1 = rSYS_PIN & MAK_FLASH_MODE1;
	gFlashModeMask2 = rSYS_PIN & MAK_FLASH_MODE2;	
	
	TmrRegisterSoftTimer(TmrPeriodicTimer, 1, timerISRCB_ii, &gSysTimer);
	
	gpio_init();
	gpioHDDAledBlink(HDDA_LED_ON);
	
	/* initialize SATA */
	sata_init();

	// Set high temperature for low flash speed
	SYSTEM_TEMPTURE = 80;
	
	gdwDRAMAllocSize = 0;
	vDPD_Init(0);
	vFTL_InitDefineValue();
	
	boNAND_Init(0);
	
	vFTL_Init();
	
	//SYSTEM_TEMPTURE = bGpioIICSMARTTempure();
	
	for(nCount=0; nCount<0x8000; nCount++)
	{
		if(IS_DEV_DECTED_PHY_ESTABLISHED())
		{
			//workaround solution for loader sata_init() setting;
			sataSts_gi.isrSts.DetPHYEstblsh = TRUE;
			break;
			//***************************************
		}
	}
	
	//while(1);
	
	/* initialize ATA */
	ataStuff.IO_ITER_SIZE = 256;
	
	ataStuff.pIdentifyDevTbl = &SATA_IDENTIFY_DEVICE_WORD0;
	ataStuff.nIdentifyDevTblSize = (UINT16)(((UINT32) &SATA_IDENTIFY_DEVICE_WORDn) - ((UINT32) &SATA_IDENTIFY_DEVICE_WORD0)) + 2;
	
	ataStuff.pDCOidentifyInfo = &DCO_IDENTIFY_INFO_WORD0;
	ataStuff.nDCOidentifyInfoSize = (UINT16)(((UINT32) &DCO_IDENTIFY_INFO_WORDn) - ((UINT32) &DCO_IDENTIFY_INFO_WORD0)) + 2;
	
	ataStuff.flushCacheCB = vFlushAll;
	ataStuff.idleCB = vFlushAll;
	ataStuff.standbyCB = vFlushAll;
	ataStuff.sleepCB = vFlushAll;
	ataStuff.SysAccessCB = SysDataAccess_trap;
	ataStuff.CheckEccCB = NULL;
	ataStuff.UpdateEccCB = NULL;
	ataStuff.SaveDramCB = NULL;
	ataStuff.LoadDramCB = NULL;
	
	//vSPI_Init();
	ata_init(&ataStuff);

    // for safety, I put some delay here(Frank@2011/08/18)
    ARM_NOP()
    ARM_NOP()
    ARM_NOP()
    
	boCPUpowerSaving = FALSE;
	
	/* print some information by uart */
	UARTputs("Make date = " __TIME__ " " __DATE__ "\r");
	UARTputs("USERNAME  = " USER_NAME "\r");
	UARTputs("Root Path = " PROG_ROOT_PATH "\r");
//	if (FPGA_MODE)
//		UARTputs("FPGA_MODE\r");
	UARTprintf("SysClk:%dMHz\r",(dwTmrSysClock(0))/1000000);
	UARTprintf("Disk info:\r");
//	
	UARTprintf("  NATIVE_MAX_LBA:%d\r", NATIVE_MAX_LBA);
	UARTprintf("  HDisk size: %d MB\r", SYS_MAX_LBA/(1024*2));
	UARTprintf("=======> %d.%d secs\r", gdwSystemTimer/CONFIG_TMR_TICK_HZ,
                                      	10*(gdwSystemTimer%CONFIG_TMR_TICK_HZ)/CONFIG_TMR_TICK_HZ);
	
	if(boSecurityLocked) UARTprintf("SeLo\r"); 
	 
}


/****************************************************************************************/
// main function
/****************************************************************************************/
byte gbNotMerge = 0;
int main(void)
{
#pragma import(__use_no_semihosting_swi)
#pragma import(__use_realtime_heap)
	dword dwStatus;
	dword dwSystemIdleTime;
	byte  bIdleActFlag;
	register struct ata_io_cmd_info *pIOInfo=NULL;
	struct ata_v2_cmd_info *pATAV2IOInfo;
	byte bSecureEraseStatus;
	byte bGPIO8;
	
	// for SPI function
	if(SPI_BOOT)
	{
	   *(volatile unsigned int *)0xB0004100 &= ~(0x02000000);
	   *(volatile byte *)(0xB001041F) = 0x00;
	}
	bGPIO8 = 0x00;
	bSecureEraseStatus = 0;
	bFlagQEGPIO = 0;
	bFlagSEGPIO = 0;
	rRST_CTRL |= COMRESET_BIT; //disable COMRESET to reset CPU, must add here

	dwStatus = 0;
	gdwSystemTimer = 0;
	initialize_system();
	gdwSystemLastAccess = gdwSystemTimer;
	gdwSystemInitTime = gdwSystemTimer;
	UARTputs("\rTEST\r");
	while(1)
	{
		//for temperature, read thermal sensor per 1sec
        if((gdwSystemTimer - gdwThermalCheckLastTick) >= (1 * CONFIG_TMR_TICK_HZ))
        {
        	//gpioHDDAledBlink(1); //LED blink
        	SYSTEM_TEMPTURE = bGpioIICSMARTTempure();
        		gdwThermalCheckLastTick = gdwSystemTimer;
        }
		
		if(UARTpollCommandReady())
			process_uart_cmd_channel(0x01);
		
//		if(gbHostWriteIdle)
		{
			//poll ata status
			dwStatus = ata_poll_status();
			
			//SATA DMA read/write command
			if(dwStatus & ATA_STS_REQ_EXEC_DMAIO_CMD)
			{
				pIOInfo = &ataCtx_gi.io_cmd_info;

				//DMA write command
				if(!pIOInfo->isRead) 
				{
				    //UARTprintf("\nTrack Write LBA:%d, SectorCount:%d\n", pIOInfo->nSSect, pIOInfo->nSectCnt);
					if(!P_flag){
						gpioHDDAledBlink(LED_MODE_WRITE);										
						bWBM_DMAWrite(pIOInfo->nSSect, pIOInfo->nSectCnt);
					}
				}
				//DMA read command
				else
				{
					//UARTprintf("\nTrack Read LBA:%d, SectorCount:%d\n", pIOInfo->nSSect, pIOInfo->nSectCnt);
					gpioHDDAledBlink(LED_MODE_READ);
					bWBM_DMARead(pIOInfo->nSSect, pIOInfo->nSectCnt);
				}
				
				gdwSystemLastAccess = gdwSystemTimer;
			}
			//SATA PIO read/write command
			else if(dwStatus & ATA_STS_REQ_EXEC_PIO_CMD)
			{
			    UARTputs("\rPIO\r");
				pIOInfo = &ataCtx_gi.io_cmd_info;
				
				//PIO write command
				if(!pIOInfo->isRead) 
				{
				    //UARTprintf("Track PIO Write LBA:%d, SectorCount:%d\n", pIOInfo->nSSect, pIOInfo->nSectCnt);
					gpioHDDAledBlink(LED_MODE_WRITE);
					bWBM_PIOWrite(pIOInfo->nSSect, pIOInfo->nSectCnt);
				}
				//PIO read command
				else
				{
				    //UARTprintf("Track PIO Read LBA:%d, SectorCount:%d\n", pIOInfo->nSSect, pIOInfo->nSectCnt);
					gpioHDDAledBlink(LED_MODE_READ);
					bWBM_PIORead(pIOInfo->nSSect, pIOInfo->nSectCnt);
				}
				
				gdwSystemLastAccess = gdwSystemTimer;
			}
			//SATA non-IO command
			else if(dwStatus & ATA_STS_REQ_EXEC_NONIO_CMD)
			{
				gpioHDDAledBlink(HDDA_LED_OFF);
				ata_exec_nonio_cmd();
			}
		}
		
   
		// Get system idle time
		dwSystemIdleTime = gdwSystemTimer - gdwSystemLastAccess;

	}
}


/****************************************************************************************/
// system memory allocation
/****************************************************************************************/
void *sysSDRAMAlloc(long size)
{
	register void *p;
	
	gdwDRAMAllocSize = gdwDRAMAllocSize + size;
	
//	UARTprintf("sysMalloc %4.4x ", gdwDRAMAllocSize);
	
	p = bget(size);
	
	if(!p)
	{
		UARTprintf("sysMalloc NULL %4.4x\r", gdwDRAMAllocSize);
		while (1);
	}
	
//	UARTprintf("E\r");
	
	return(p);
}

void sysSDRAMFree(void *pMem)
{
	brel(pMem);
}

//flush cache
void vFlushAll(void)
{
	gdwSystemLastAccess = gdwSystemTimer;
	return;
}

void SysDataAccess_trap(bool_T boRead, UINT8 *pSYSinfo)
{
}


