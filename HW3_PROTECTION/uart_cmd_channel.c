#include <global.h>
#include <defreg.h>
#include <intc.h>
#include <uart.h>
#include <sata.h>
#include <nand_sys.h>
#include <ftl.h>

#define SATA_ELECTRIC_TEST_CMD

//extern long nCurAllocSize_g;
//extern long nMaxAllocSize_g;
extern UINT32 nFTLTblBase_gi;
bool_T sysDisIdleMergeByUARTCmd_g = FALSE;
UINT8 *pbStr1;
byte SATATestPatternSel=0; // 0:HH 1:MM 2:LL 3:LB

// for lab3
extern int P_flag;


UINT32 dwStrToHexval(UINT8 *pStr)
{
	UINT8 i;
	UINT8 c;
	UINT32 hexval;

	hexval=0;

	if (pStr!=NULL)
		pbStr1=pStr;

	while (*pbStr1==' ')
		pbStr1++;

	c= *pbStr1;

	if (c=='\r' || c=='\n')
		return (0xffff); //end of string

	for (i=0;i<8;i++)
	{
		c= *pbStr1;
		if (c>='0' && c<='9')
			c &= 0x0f;
		else if (c>='A' && c<='F')
			c = (c&0x0f)+9;
		else if (c>='a' && c<='f')
			c = (c&0x0f)+9;
		else if (c=='\r' || c=='\n')	
			return (hexval); //end of string, don't inc-pointer
		else
		{	pbStr1++; //inc-pointer to next
			break; //other unpredict char, return latest value
		}
        
		hexval= hexval*16 + c;
		pbStr1++; // if (end of string) don't inc-pointer
	}

	return (hexval);
}

UINT32 dwStrToHexval2(UINT8 *pStr, UINT8 *pCnt)
{
	UINT8 i;
	UINT8 c;
	UINT32 hexval;

	hexval=0;
	*pCnt=0;

	if (pStr!=NULL)
		pbStr1=pStr;

	while (*pbStr1==' ')
	{
		pbStr1++;
		(*pCnt) +=1 ;
	}

	c= *pbStr1;

	if (c=='\r' || c=='\n')
		return (0xffff); //end of string

	for (i=0;i<8;i++)
	{
		c= *pbStr1;
		if (c>='0' && c<='9')
			c &= 0x0f;
		else if (c>='A' && c<='F')
			c = (c&0x0f)+9;
		else if (c>='a' && c<='f')
			c = (c&0x0f)+9;
		else if (c=='\r' || c=='\n')	
			return (hexval); //end of string, don't inc-pointer
		else
		{
		    pbStr1++; //inc-pointer to next
		    (*pCnt) +=1 ;
			break; //other unpredict char, return latest value
		}
        
		hexval= hexval*16 + c;
		pbStr1++; // if (end of string) don't inc-pointer
	    (*pCnt) +=1 ;
	}

	return (hexval);
}

/***********************************************************
 *          Static Function
 ***********************************************************/
/***********************************************************/
void UARTdumpMemory(UINT32 dwAddress)
{
	UINT8 i;
	UINT8 *pbStrTemp=(UINT8 *)dwAddress;

  	put4HEX(dwAddress);
  	UARTputchar(':');
    
	for (i=0;i<8;i++)
	{
		putHEX(*pbStrTemp++);
		UARTputchar(' ');
	}
	UARTputchar('=');
	for (i=8;i<16;i++)
	{
		putHEX(*pbStrTemp++);
		UARTputchar(' ');
	}
	UARTputchar('\r');
}

/***********************************************************/
void vProcessModify(byte *pCommand)
{
	dword dwOffset;
	dword dwValue;
	dword *pdwCommandData;
	
	dwOffset = dwStrToHexval(pCommand);
	dwValue = dwStrToHexval(NULL);

	dwOffset = dwOffset & 0xFFFFFFFC; //force dword alignment
	pdwCommandData = (dword *)dwOffset;
	pdwCommandData[0] = dwValue;
	dwOffset&=0xFFFFFFF0;
	UARTdumpMemory(dwOffset);
}

/***********************************************************/
void vProcessDumpMemoryLong(byte *pCommand)
{
	dword dwOffset;
	word  wDumpLength;	
		
	dwOffset = dwStrToHexval(pCommand);

	dwOffset &= 0xFFFFFFF0;
	UARTdumpMemory(dwOffset);	

   for( wDumpLength = 0 ; wDumpLength < ( 16 - 1 ) ; wDumpLength++ )
   {
      dwOffset = (dwOffset+0x10) & 0xFFFFFFF0;	// dump 2nd line 16 bytes   
      UARTdumpMemory(dwOffset);
   }
   
//	/* dump 3*16 bytes data */
//	dwOffset &= 0xFFFFFFF0;
//	UARTdumpMemory(dwOffset);				// dump 1st line 16 bytes
//	dwOffset = (dwOffset+0x10) & 0xFFFFFFF0;	// dump 2nd line 16 bytes
//	UARTdumpMemory(dwOffset);
//	dwOffset = (dwOffset+0x10) & 0xFFFFFFF0;	// dump 3rd line 16 bytes
//	UARTdumpMemory(dwOffset);
}

/***********************************************************/
void vProcessDumpMemory(byte *pCommand)
{
	dword dwOffset;
		
	dwOffset = dwStrToHexval(pCommand);
   
	/* dump 3*16 bytes data */
	dwOffset &= 0xFFFFFFF0;
	UARTdumpMemory(dwOffset);				// dump 1st line 16 bytes
	dwOffset = (dwOffset+0x10) & 0xFFFFFFF0;	// dump 2nd line 16 bytes
	UARTdumpMemory(dwOffset);
	dwOffset = (dwOffset+0x10) & 0xFFFFFFF0;	// dump 3rd line 16 bytes
	UARTdumpMemory(dwOffset);
}

#ifdef SATA_ELECTRIC_TEST_CMD
/***********************************************************/

/***********************************************************/

void vProcessReleaseSATAReset(void)
{   
   (*(volatile dword *)(0xB0005048)) = (*(volatile dword *)(0xB0005048)) & (0xFFFFFFF0);
}

void vProcessForceSATAPhyRdy(void)
{
   (*(volatile dword *)(0xB0005028)) = (*(volatile dword *)(0xB0005028)) | (0x00000010);
}
   
void vProcessProgramVendorPattern(void)
{    
   switch(SATATestPatternSel)
   {
      case 0: //HH
         (*(volatile dword *)(0xB0005054))=0xB5B5B5B5;
         break;
      case 1: //MM
         (*(volatile dword *)(0xB0005054))=0x78787878;
         break;
      case 2:
         (*(volatile dword *)(0xB0005054))=0x7E7E7E7E;
         break;
      case 3:
         (*(volatile dword *)(0xB0005054))=0x6B0C8B0C;
         break;
      default:
         (*(volatile dword *)(0xB0005054))=0xB5B5B5B5;
         break;   
   }
}

void vProcessEnableTXVendorPatternForceMode(void)
{    
   (*(volatile dword *)(0xB0005034)) = (*(volatile dword *)(0xB0005034)) | (0x00000001);
}

/***********************************************************/
void vProcessSATAelecTest(byte *pCommand)
{
	word w0,w1;
	byte c0,c1,c2;
	byte ok;
	dword dwTstp0, dwTstp1;
	word	wZonePhyBlkNo[8];
	byte	bPhyPageNo[8];
	word	i;
	
	for(i = 0 ; i < 8 ; i++)
	{
		wZonePhyBlkNo[i] = 0;
		bPhyPageNo[i] = 10;
	}
	
	
	c0= *pCommand++;
	c1= *pCommand++;
	c2= *pCommand++;
	ok=0;
	
	IntcDisableIntr(INTC_SATA);
	
	if ((c0=='H'))
	{	//HFTP, rSATA_BIST_DATA0=0xB5B5B5B5;
		REG_BIST_DATA0 = 0xB5B5B5B5;
		REG_BIST_DATA1 = 0xB5B5B5B5;
		REG_SYS_CTRL = 0x80001839;
		REG_SYS_CTRL = 0x80001039;
		ok=1;
	} else
	if ((c0=='M'))
	{	//MFTP, rSATA_BIST_DATA0=0x78787878;
		REG_BIST_DATA0 = 0x78787878;
		REG_BIST_DATA1 = 0x78787878;
		REG_SYS_CTRL = 0x80001839;
		REG_SYS_CTRL = 0x80001039;
		ok=1;
	} else
	if ((c0=='L'))
	{	//LFTP, rSATA_BIST_DATA0=0x7E7E7E7E;
		REG_BIST_DATA0 = 0x7E7E7E7E;
		REG_BIST_DATA1 = 0x7E7E7E7E;
		REG_SYS_CTRL = 0x80001839;
		REG_SYS_CTRL = 0x80001039;
		ok=1;
	} else
	if ((c0=='B'))
	{	//LBP, rSATA_BIST_DATA0=0x6B0C8B0C;
		REG_BIST_DATA0 = 0x6B0C8B0C;
		REG_BIST_DATA1 = 0x6B0C8B0C;
		REG_SYS_CTRL = 0x80001839;
		REG_SYS_CTRL = 0x80001039;
		ok=1;
	} else
	if ((c0=='D') && (c1=='Y') && (c2=='2'))
	{
		
	} else
	if ((c0=='D') && (c1=='Y'))
	{
		REG_SYS_CTRL = 0x00000030;
		ok=1;
	} else
	if ((c0=='W') && ((c1=='0')||(c1=='1'))	)
	{
		w0 = (word)dwStrToHexval(pCommand);
		w1 = (word)dwStrToHexval(NULL);
		if (c1=='0')
		{	((UINT8 *)&dwTstp0)[0]=(byte)(w0>>8);
			((UINT8 *)&dwTstp0)[1]=(byte)(w0&0xff);
			((UINT8 *)&dwTstp0)[2]=(byte)(w1>>8);
			((UINT8 *)&dwTstp0)[3]=(byte)(w1&0xff);
		} else
		{	((UINT8 *)&dwTstp1)[0]=(byte)(w0>>8);
			((UINT8 *)&dwTstp1)[1]=(byte)(w0&0xff);
			((UINT8 *)&dwTstp1)[2]=(byte)(w1>>8);
			((UINT8 *)&dwTstp1)[3]=(byte)(w1&0xff);
		}
		UARTputs("W:"); put2HEX(w0); put2HEX(w1); UARTputchar('\r');
		ok=1;
	} else
	if ((c0=='V'))
	{
		REG_BIST_DATA0 = dwTstp0;
		REG_BIST_DATA1 = dwTstp1;
		REG_SYS_CTRL = 0x80001839;
		REG_SYS_CTRL = 0x80001039;
		ok=1;
	}
	if (ok==1)
		UARTputs("FinishSATAtstCmd\r");
	else
		UARTputs("TstCmdErr\r");
}
#endif

#ifdef ATA_FEATURE_NCQ
extern void vNCQ_dumpTag(void);
#endif

void vSATAEyeTest(void)
{
   UARTputs("sata eye test ");
   //REG_SYS_CTRL = 0x00000030;
   
   vProcessReleaseSATAReset();
   UARTputs("1 ");
   vProcessForceSATAPhyRdy();
   UARTputs("2 ");
   vProcessProgramVendorPattern();
   UARTputs("3\r");
   vProcessEnableTXVendorPatternForceMode();             
}

void vSATAEyeTestHeavy(void)
{
   byte page[8]={4,4,4,4,4,4,4,4};
   word block[8]={4,4,4,4,4,4,4,4};
   byte bBank;
   
   UARTputs("sata eye test ");
   //REG_SYS_CTRL = 0x00000030;
   
   vProcessReleaseSATAReset();
   UARTputs("1 ");
   vProcessForceSATAPhyRdy();
   UARTputs("2 ");
   vProcessProgramVendorPattern();
   UARTputs("3\r");
   vProcessEnableTXVendorPatternForceMode();  
   UARTputs("4\r"); 
}

void process_uart_cmd_channel(UINT8 bFlag)
{
	//static int ii = 0;
	char *xyz;
	SIO_RX_BUFFER *p;

    
	p = UARTgetCommand();
	xyz = (char*)p->abData;

	while (xyz[0] == '\r' || xyz[0] == '\n')
		xyz += 1;

 	if (strncmp(xyz, "erase", 5) == 0)
 	{
        word wBlkNo;
        
        wBlkNo = dwStrToHexval((byte*)&xyz[5]);
	    wBlkNo &= 0x0000FFFF;

		UARTprintf(" Erase wBlock %d(0x%04Xh)\r", wBlkNo, wBlkNo);

    	// Erase block
		bFTL_BlockErase(wBlkNo);
		UARTputs("end>\r\r");

 	}
 	else if (strncmp(xyz, "write", 5) == 0)
 	{
        UINT8 bIdx;
        UINT8 bCnt;
        UINT8 bPATTERN;
        word wPageNo;
        word wBlkNo;
        dword dwPATTERN;
     	dword *pSRamBuffer;

 	    pSRamBuffer = (dword *)(FLASH_MEM_BASE);

        bIdx = 5;
        wBlkNo = dwStrToHexval2((byte*)&xyz[bIdx], &bCnt);
	    wBlkNo &= 0x0000FFFF;

        bIdx += bCnt;
        dwPATTERN = dwStrToHexval2((byte*)&xyz[bIdx], &bCnt);
	    bPATTERN = dwPATTERN & 0x000000FF;

		UARTprintf(" Write dwBlock %d(0x%04Xh)\r", wBlkNo, wBlkNo);
    	    	
    	// Set Buffer
    	memset((byte *)(FLASH_MEM_BASE), bPATTERN, 128*32);
        pSRamBuffer[0] = 0;

        for( wPageNo = 0 ; wPageNo <= 255 ; wPageNo++ )
        {
		    UARTprintf(" dwPage %d(0x%04Xh)\r", wPageNo, wPageNo);

            pSRamBuffer[0] = (byte) wPageNo;
			UARTprintf(" w1 data\n");
			UARTprintf(" pBuf[0] 0x %08X %08X %08X %08X\n", pSRamBuffer[0], pSRamBuffer[1], pSRamBuffer[2], pSRamBuffer[3]);
			//UARTprintf(" pBuf[4] 0x %08X %08X %08X %08X\n", pSRamBuffer[4], pSRamBuffer[5], pSRamBuffer[6], pSRamBuffer[7]);


            vFTL_PageProgram(wPageNo, wBlkNo);
			//UARTprintf(" w2 data\n");
			//UARTprintf(" pBuf[0] 0x %08X %08X %08X %08X\n", pSRamBuffer[0], pSRamBuffer[1], pSRamBuffer[2], pSRamBuffer[3]);
        }
		UARTputs("end>\r\r");

 	}
 	else if (strncmp(xyz, "read", 4) == 0)
 	{
        UINT8 bIdx;
        UINT8 bCnt;
        byte bPageNo;
        word wBlkNo;
     	dword *pSRamBuffer;
		int i;

 	    pSRamBuffer = (dword *)(FLASH_MEM_BASE);

        bIdx = 4;
        wBlkNo = dwStrToHexval2((byte*)&xyz[bIdx], &bCnt);
	    wBlkNo &= 0x0000FFFF;

        bIdx += bCnt;
        bPageNo = dwStrToHexval2((byte*)&xyz[bIdx], &bCnt);
	    bPageNo &= 0x0000FFFF;

		UARTprintf(" Read dwBlock %d(0x%04Xh)\r", wBlkNo, wBlkNo);
		UARTprintf(" Read dwPage %d(0x%04Xh)\r", bPageNo, bPageNo);

        // Read data
        vFTL_PageRead(bPageNo, wBlkNo);

        UARTprintf(" UART dump SRAM Buf after Read data\n");
        //UARTprintf(" UART pBuf[0] 0x %08X %08X %08X %08X\n", pSRamBuffer[0], pSRamBuffer[1], pSRamBuffer[2], pSRamBuffer[3]);
        //UARTprintf(" UART pBuf[4] 0x %08X %08X %08X %08X\n", pSRamBuffer[4], pSRamBuffer[5], pSRamBuffer[6], pSRamBuffer[7]);

		for(i = 0;i<1024;i++){
			UARTprintf("0x%08x\n",pSRamBuffer[i]);
		}

		UARTputs("end>\r\r");

 	}
	else if(strncmp(xyz, "info", 4) == 0){
		 UARTprintf(" Shannon\n F64081169\r");
		 UARTputs("end>\r\r");
	}
	else if(strncmp(xyz, "protectEnable", 13) == 0){
		 P_flag = 1;
		 UARTprintf(" protectEnable\r");
		 UARTputs("end>\r\r");
	}
	else if(strncmp(xyz, "protectDisable", 14) == 0){
		 P_flag = 0;
		 UARTprintf(" protectDisable\r");
		 UARTputs("end>\r\r");
	}
 	else
	{
		if(bFlag == 0)
			UARTputs("OK>\r");
		else if(bFlag == 0x01)
			UARTputs("command..>\r");
		else if(bFlag == 0xFF)
			UARTputs("DBG>\r");
		else
		{
			UARTputs("WHILE:");putHEX(bFlag);UARTputs(">\r");
		}
	}
		
	p->bLen = 0;
}

