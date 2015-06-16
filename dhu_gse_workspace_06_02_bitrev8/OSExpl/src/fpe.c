/*
 * fpe.c
 *
 *  Created on: Apr 6, 2015
 *      Author: jal
 */
#include <xil_types.h>
#include <string.h>
#include "platform.h"
#include "xparameters.h"
#include <xbram.h>
#include <xbram_hw.h>
#include <xil_io.h>
#include <xil_printf.h>
#include <xil_types.h>
#include <xscugic.h>
#include <xstatus.h>
#include <xuartps.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>				// prototype for sleep()
#include <string.h>

#include "fpe.h"
#include "dhu.h"

extern volatile u32 cam1IntStatus,cam1SpmIntStatus,cam1DdrReq,cam1DdrRdy,cam1DdrReqCnt;
extern volatile u32 cam2IntStatus,cam2SpmIntStatus;
extern const u32 FpeDefaultBitFile[];
extern XScuGic scuGic;		/* pl/pl fabric generic interrupt controller */

void DoIdle(void);
void cmdRsp(const char *fmt, ...);
int udpGets(char *);

#define DHU_CAM1INT_ID			XPAR_FABRIC_SYSTEM_DHU_CAM1_INT_INTR

//
// Utility function to move BRAM -> FPE
//
u32 FPEWrite(u32 bramOffset,
			 u32 length,
			 FpeMemType type,
			 u32 Immediate)
{
	FPEWriteCtrl_bits fpewctrl;

	fpewctrl.word = 0;
	fpewctrl.bitFields.length = length;
	fpewctrl.bitFields.memType = type;
	fpewctrl.bitFields.regAddr = 0;
	fpewctrl.bitFields.xferType = Immediate;
	DHURegWrite(CAM_FPE_WDATA_BAR, bramOffset);		// Data starts at 0
	DHURegWrite(CAM_FPE_WCTRL, fpewctrl.word);		// Start write operation
	return XST_SUCCESS;
}

void xferBitFile(u32 length, u32 *where)
{
	u32 y;

	//
	// We have to loop in here until the entire buffer is send!
	//
	Xil_ExceptionDisableMask(XIL_EXCEPTION_IRQ);
	cam1IntStatus &= ~0x0002;
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
	DHURegWrite(CAM_FPE_BITFILE_SIZE,length);	// put length of bitfile in words into register

	cam1DdrReq = 0;
	cam1DdrReqCnt = 0;
	y = 0;

	// Copy from bigBuff[] -> BRAM[0 - 1024]
	memmove((void *)(cam1Bram.Config.MemBaseAddress),
			(void *)(&(where[y])),
			(1024 * sizeof(u32)));
	y += 1024;

	// Now tell DHU to transfer....
	// 0 base address
	// .. 0 words (actual length is in CAM_FPE_BITFILE_SIZE
	// .. to bitfile
	// .. immediately
	FPEWrite(0, 0, FPE_BITFILE, 1);

	for (; y<length;)
	{
		// Wait till DHU is ready for more...
		while (cam1DdrReq == 0);
		cam1DdrReq = 0;
		cmdRsp(".");

		// Preload the 2nd 1/2 of BRAM
		// Copy from bigBuff[] -> BRAM[1024 - 2047]
		memmove((void *)(cam1Bram.Config.MemBaseAddress + 4096),
				(void *)(&(where[y])),
				(1024 * sizeof(u32)));
		y += 1024;

		// Wait till DHU is ready for more...
		while (cam1DdrReq == 0);
		cam1DdrReq = 0;
		xil_printf(".");

		// Load 1st 1/2 of BRAM
		memmove((void *)(cam1Bram.Config.MemBaseAddress),
				(void *)(&(where[y])),
				(1024 * sizeof(u32)));
		y += 1024;
	}
}


#if 0
// Not supported w/ 16 bit cam1FrameBuff
void FpeLoadBit(void)
{
	char cmdBuff[128];
	char *value;
	int endflag = 0;
	int length;
	u32 y;

		// Disable/turn off DHU
		DHURegWrite(CAM_CONTROL,0);
		cmdRsp("DHU Disabled before bitfile loads\n\r");

		length = 0;		// Keep track of number of output elements...
		memset((void *)cam1FrameData,0,sizeof(cam1FrameData));

		while ((!endflag ) && (length < (sizeof(cam1FrameData)/sizeof(cam1FrameData[0]))))
		{
			// Get a value....
			if (udpGets(cmdBuff))
			{
				if (strncmp(cmdBuff,"end",3) == 0) endflag = 1;
				else {
					value = strtok(cmdBuff," \n\r");
					if (value != NULL)
					{
						y = strtoul(value, NULL, 0);
						cam1FrameData[length++] = y;
					}
				}
			} else DoIdle();
		}

		//
		// See if we got too much stuff...
		//
		if (length >= (sizeof(cam1FrameData)/sizeof(cam1FrameData[0])))
			xil_printf("Possible bitfile buffer overflow during argument list load!\n\r?> ");

		//
		// Now kick off the DHU->FPE transfer
		// Does the doneflag in the DHU register clear automatically when we kick off
		// a command.
		//
		DHURegWrite(CAM_INTERRUPT_ENA,2);	// Enable FPE_LOAD interrupts
		cam1DdrReqCnt = 0;
		cam1IntStatus &= ~0x00000002;
		cmdRsp("Sending %d words to FPE (Bitfile load)\n\r",length);
		xferBitFile(length, cam1FrameData);

		// Expect a cam1 FPE_LOAD interrupt after everything is done....
		cmdRsp("\n\rWaiting for  bitfile load finish\n\r");
		while ((cam1IntStatus & 0x0002) == 0) DoIdle();
		cmdRsp("cam1DdrReqCnt = %d\n\r",cam1DdrReqCnt);
		cmdRsp("BitFile Load complete\n\r?> ");
}
#endif


void FpeLoadHK(void)
{
	char cmdBuff[128];
	char *value;

    int addr = 0;
	int index = 0;
	int endflag = 0;
	int length;
	u32 x,y;

		length = 0;		// Keep track of number of output elements...
		y=0;

		x = DHURegRead(CAM_CONTROL);

		if (((DHUCamControl_bits *)&x)->ENABLE_ALL == 1)
		{
			xil_printf("Disable DHU before memory loads\n\r?> ");
			return;
		}

		while ((!endflag ) && (addr < 2048))
		{
			// Get a value....
			// ..put into DDR1/2 memory.. for now only DHU1
			// This memory is 8 bit but the BRAM is only word wide so we have to pack
			// 4 input values into one output word...and be sure to catch the last partial
			// write on the end!
			//
			if (udpGets(cmdBuff))
			{
				if (strncmp(cmdBuff,"end",3) == 0) endflag = 1;
				else {
					value = strtok(cmdBuff," \n\r");
					if (value != NULL)
					{
						y = strtoul(value, NULL, 0);
						XBram_Out32(cam1Bram.Config.MemBaseAddress + (u32)(addr<<2), (u32)y);
						addr++;
						length++;
					}
				}
			} else DoIdle();
		}

		// See if there is a partial write that has to be done...
		if (index) {
			XBram_Out32(cam1Bram.Config.MemBaseAddress + (u32)(addr<<2), (u32)y);
			length++;
		}

		//
		// See if we got too much stuff...
		//
		if (addr >= 2048)
			xil_printf("Possible BRAM overflow during argument list load!\n\r?> ");
		cmdRsp("Sending %d words to HSK memory\n\r",length);

		//
		// And wait for a done flag from the DHU
		// Polled mode...
		// To Do - Add timeout so we don't wait forever
		//
		xferFpeBuff(length, (u32 *)(cam1Bram.Config.MemBaseAddress), FPE_HMEM);
		cmdRsp("Housekeeping Memory Load complete\n\r?> ");
}


void FpeLoadProg(void)
{
	char cmdBuff[128];
	char *value;
    int addr = 0;
	int endflag = 0;
	int length;
	u32 y;
	u32 x;


		length = 0;		// Keep track of number of output elements...
		x = DHURegRead(CAM_CONTROL);

		if (((DHUCamControl_bits *)&x)->ENABLE_ALL == 1)
		{
			cmdRsp("Disable DHU before memory loads\n\r?> ");
			return;
		}

		while ((!endflag ) && (addr < 2048))
		{
			// Get a value....
			// ..put into DDR1/2 memory.. for now only DHU1
			// This memory is 64 bit but the BRAM is only word wide so we have to put
			// 2 BRAM locations per input argument
			//
			if (udpGets(cmdBuff))
			{
				if (strncmp(cmdBuff,"end",3) == 0) endflag = 1;
				else {
					value = strtok(cmdBuff," \n\r");
					if (value != NULL)
					{
						length++;
						y = strtoul(value, NULL, 0);
						XBram_Out32(cam1Bram.Config.MemBaseAddress + (u32)(addr<<2), (u32)(y & 0xFFFFFFFF));
						addr++;
					}
				}
			} else DoIdle();
		}

		//
		// See if we got too much stuff...
		//
		if (addr >= 2048)
			xil_printf("Possible BRAM overflow during argument list load!\n\r?> ");
		cmdRsp("Sending %d words to FPE Program memory\n\r",length);

		xferFpeBuff(length, (u32 *)(cam1Bram.Config.MemBaseAddress), FPE_PMEM);
		cmdRsp("Program Memory Load complete\n\r?> ");
}


void FpeLoadClv(void)
{
	char cmdBuff[128];
	int length = 0;		// Keep track of number of output elements...
	char *value;
    int addr = 0;
	int endflag = 0;
	u32 y,x;

	endflag = 0;
	y = 0;

	x = DHURegRead(CAM_CONTROL);
	if (((DHUCamControl_bits *)&x)->ENABLE_ALL == 1)
	{
		cmdRsp("Disable DHU before memory loads\n\r?> ");
		return;
	}

	while ((!endflag ) && (addr < 2048))
	{
		// Get a value....
		// ..put into DDR1/2 memory.. for now only DHU1
		// This memory is 16 bit but the BRAM is only word wide so we have to put
		// 2 input values into each BRAM location.
		//
		if (udpGets(cmdBuff))
		{
			if (strncmp(cmdBuff,"end",3) == 0) endflag = 1;
			else {
				value = strtok(cmdBuff," \n\r");
				if (value != NULL)
				{
					y = strtoul(value, NULL, 0);
					XBram_Out32(cam1Bram.Config.MemBaseAddress + (u32)(addr<<2), (u32)y);
					length++;
					addr++;
				}

			}
		} else DoIdle();
	}


	//
	// See if we got too much stuff...
	//
	if (addr >= 2048)
		xil_printf("Possible BRAM overflow during argument list load!\n\r?> ");
	cmdRsp("Sending %d words to FPE Voltage memory\n\r",length);
	xferFpeBuff(length, (u32 *)(cam1Bram.Config.MemBaseAddress), FPE_VMEM);

	cmdRsp("Voltage Memory Load complete\n\r?> ");
}

void FpeLoadSeq(void)
{
	char cmdBuff[128];
    char *value;
    u32 y;
    int length;
    int endflag = 0;
    int addr=0;
    u32 x;

	length = 0;		// Keep track of number of output elements...

	x = DHURegRead(CAM_CONTROL);

	if (((DHUCamControl_bits *)&x)->ENABLE_ALL == 1)
	{
		cmdRsp("Disable DHU before memory loads\n\r?> ");
		return;
	}

	while ((!endflag ) && (addr < 2048))
	{
		// Get a value....
		// ..put into DDR1/2 memory.. for now only DHU1
		// This memory is 33 bit but the BRAM is only word wide so we have to put
		// 2 BRAM locations per input argument
		//
		if (udpGets(cmdBuff))
		{
			if (strncmp(cmdBuff,"end",3) == 0) endflag = 1;
			else {
				value = strtok(cmdBuff," \n\r");
				if (value != NULL)
				{
					length++;
					y = strtoul(value, NULL, 0);
					XBram_Out32(cam1Bram.Config.MemBaseAddress + (u32)(addr<<2), (u32)(y & 0xFFFFFFFF));
					addr++;
				}
			}
		} else DoIdle();
	}

	//
	// See if we got too much stuff...
	//
	if (addr >= 2048)
		xil_printf("Possible BRAM overflow during argument list load!\n\r?> ");
	cmdRsp("Sending %d words to FPE Sequencer memory\n\r",length);

	xferFpeBuff(length, (u32 *)(cam1Bram.Config.MemBaseAddress), FPE_SMEM);
	cmdRsp("Sequence Memory Load complete\n\r?> ");

}

	//
	// Read/report housekeeping data:  If a numeric argument is provided
	// it will specify the number of registers to report
	//
void FpeReadHsk(char *value)
{
	int length,i;

	length = 128;
	if (value) length = atoi(value);

	ENTER_CRITICAL;
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(1<<2), length);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(2<<2), CAM_FPE_HSK);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(0<<2), 0x55555555);

	//
	// Wait here for PCIe[0] to clear....
	//
	while (XBram_In32(PCIeBram.Config.MemBaseAddress) & 0x0000FFFF);
	EXIT_CRITICAL;

	for (i=0; i<length; i++)
	  cmdRsp("Hsk\[%d] = 0x%08x\n\r",i,XBram_In32(PCIeBram.Config.MemBaseAddress + (u32)((i+1)<<2)));
    cmdRsp("?>");
}

void FpeReadMem(char *value)
{
	int length,i;

	length = 2048;
	if (value) length = atoi(value);

	ENTER_CRITICAL;
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(1<<2), length);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(2<<2), CAM_FPE_MEM_DUMP);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(0<<2), 0x55555555);

	//
	// Wait here for PCIe[0] to clear....
	//
	while (XBram_In32(PCIeBram.Config.MemBaseAddress) & 0x0000FFFF);
	EXIT_CRITICAL;

	for (i=0; i<length; i++)
	  cmdRsp("FpeMem\[%d] = 0x%08x\n\r",i,XBram_In32(PCIeBram.Config.MemBaseAddress + (u32)((i+1)<<2)));

	cmdRsp("?>");
}

void FpeReset(void)
{
	FPEWrite(0, 0, FPE_RESET,1);
	cmdRsp("FPE Rest complete\n\r?>");
}

void FpeLoadReg(void) {
	char cmdBuff[128];
    char *value;
    u32 y;
    int length;
    int endflag = 0;
    int addr=0;
    u32 x;

	length = 0;		// Keep track of number of output elements...

	x = DHURegRead(CAM_CONTROL);

	if (((DHUCamControl_bits *)&x)->ENABLE_ALL == 1)
	{
		cmdRsp("Disable DHU before register loads\n\r?> ");
		return;
	}

	while ((!endflag ) && (addr < 2048))
	{
		// Get a value....
		// ..put into DDR1/2 memory.. for now only DHU1
		// This memory is 33 bit but the BRAM is only word wide so we have to put
		// 2 BRAM locations per input argument
		//
		if (udpGets(cmdBuff))
		{
			if (strncmp(cmdBuff,"end",3) == 0) endflag = 1;
			else {
				value = strtok(cmdBuff," \n\r");
				if (value != NULL)
				{
					length++;
					y = strtoul(value, NULL, 0);
					XBram_Out32(cam1Bram.Config.MemBaseAddress + (u32)(addr<<2), (u32)(y & 0xFFFFFFFF));
					addr++;
					length++;
				}
			}
		} else DoIdle();
	}

	//
	// See if we got too much stuff...
	//
	if (addr >= 2048)
		xil_printf("Possible BRAM overflow during argument list load!\n\r?> ");

	xferFpeBuff(length, (u32 *)(cam1Bram.Config.MemBaseAddress), FPE_REGISTERS);
	cmdRsp("Sequence Memory Load complete\n\r?> ");

}

