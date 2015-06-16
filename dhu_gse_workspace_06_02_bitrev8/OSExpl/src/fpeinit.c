/*
 * fpeinit.c
 *
 *  Created on: Apr 5, 2015
 *      Author: jal
 *
 *  Implementation of command to automatically start/initialize a camera useing default
 *  values.
 */

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
extern XBram PCIeBram;				/* The Instance of the BRAM Driver */
extern XBram cam1Bram;				/* The Instance of the BRAM Driver */
extern XBram cam2Bram;				/* The Instance of the BRAM Driver */

u32	DHURegRead(u32 regOffset);
u32	DHURegWrite(u32 regOffset,u32 value);
void xferBitFile(u32 length, u32 *where);
void DoIdle(void);
u32 FPEWrite(u32 bramOffset,
			 u32 length,
			 FpeMemType type,
			 u32 Immediate);
void cmdRsp(const char *fmt, ...);

const u32 FpeDefaultBitFile[] = {
#include "FPE_Wrapper.h"
};

const u32 FpeDefaultRegMem[] = {
#include "RegMem.h"
};

const u32 FpeDefaultHskMem[] = {
#include "HskMem.h"
};

const u32 FpeDefaultClvMem[] = {
#include "ClvMem.h"
};

const u32 FpeDefaultPrgMem[] = {
#include "PrgMem.h"
};

const u32 FpeDefaultSeqMem[] = {
#include "SeqMem.h"
};


void xferFpeBuff(u32 length, u32 *where, FpeMemType type)
{
	u32 x;

	//
	// We have to loop in here until the entire buffer is send!
	//
	Xil_ExceptionDisableMask(XIL_EXCEPTION_IRQ);
	cam1IntStatus &= ~0x0002;
	cam1DdrReq = 0;
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	// Copy from where[] -> BRAM[0 - length]
	memmove((void *)(cam1Bram.Config.MemBaseAddress),
			(void *)where,
			(length * sizeof(unsigned int)));

	x = DHURegRead(CAM_CONTROL);
	if (((DHUCamControl_bits *)&x)->ENABLE_ALL == 0)
	{
		FPEWrite(0, length, type, 1);
		// Immediate loads we'll get an DDR Req interrupt
		while (cam1DdrReq == 0) DoIdle();
	} else {
		FPEWrite(0, length, type, 0);
		while ((cam1IntStatus & 0x0002) == 0) DoIdle();
	}
}

//karih modification  -- added restart parameter
void FpeInitDefaults(u32 restart)
//end modification
{

	// Reset DHU
	DHURegWrite(CAM_CONTROL,0x01);
	sleep(2);
	cmdRsp("DHU Reset\n\r");


	// Enable the FPE_LOADED Interrupt
	DHURegWrite(CAM_INTERRUPT_ENA,2);

	//karih addition
	if (restart)
	{
//		DHURegWrite(CAM_FPE_WCTRL,0xC00000); //set the FPE for reprogramming
		// Program the bitfile....
		cmdRsp("FPE bitfile ...");
		xferBitFile(sizeof(FpeDefaultBitFile)/sizeof(FpeDefaultBitFile[0]), (u32 *)FpeDefaultBitFile);
		cmdRsp(" loaded\n\r");
	}
	sleep(2);

	// Load CLV Memory
	cmdRsp("CLV Memory....");
	xferFpeBuff(sizeof(FpeDefaultClvMem)/sizeof(FpeDefaultClvMem[0]),
			(u32 *)FpeDefaultClvMem,
			FPE_VMEM);
	cmdRsp("Loaded\n\r");
	sleep(2);

	// Load Hsk Memory
	cmdRsp("Hsk Memory ....");
	xferFpeBuff(sizeof(FpeDefaultHskMem)/sizeof(FpeDefaultHskMem[0]),
			(u32 *)FpeDefaultHskMem,
			FPE_HMEM);
	cmdRsp("Loaded\n\r");
	sleep(2);

	// Load Sequence Memory
	cmdRsp("Seq Memory ....");
	xferFpeBuff(sizeof(FpeDefaultSeqMem)/sizeof(FpeDefaultSeqMem[0]),
			(u32 *)FpeDefaultSeqMem,
			FPE_SMEM);
	cmdRsp("Loaded\n\r");
	sleep(2);

	// Load Prog Memory
	cmdRsp("Prog Memory ....");
	xferFpeBuff(sizeof(FpeDefaultPrgMem)/sizeof(FpeDefaultPrgMem[0]),
			(u32 *)FpeDefaultPrgMem,
			FPE_PMEM);
	cmdRsp("Loaded\n\r");
	sleep(2);

	// Load Reg Memory
	cmdRsp("FPE Registers.....");
	xferFpeBuff(sizeof(FpeDefaultRegMem)/sizeof(FpeDefaultRegMem[0]),
			(u32 *)FpeDefaultRegMem,
			FPE_REGISTERS);
	cmdRsp("Loaded\n\r");
	sleep(2);

	// Configure DHU for frame collection.
	DHURegWrite(CAM_INTERRUPT_SPM_ENA,3);
	DHURegWrite(CAM_SPM0_CTRL,1);
	DHURegWrite(CAM_CONTROL,2);
	cmdRsp("Starting frames...\n\r?> ");
}

void FpeReloadMems(void)
{
    cmdRsp("Stop the frames, and reset the FPE\n\r");
    DHURegWrite(CAM_CONTROL,0); // stop the frames from going to FPE
    sleep(2);

    FPEWrite(0, 0, FPE_RESET, 1); // FPE reset
    sleep(2);

    // Load CLV Memory
    cmdRsp("CLV Memory....");
    xferFpeBuff(sizeof(FpeDefaultClvMem)/sizeof(FpeDefaultClvMem[0]),
            (u32 *)FpeDefaultClvMem,
            FPE_VMEM);
    cmdRsp("Loaded\n\r");
    sleep(2);

    // Load Hsk Memory
    cmdRsp("Hsk Memory ....");
    xferFpeBuff(sizeof(FpeDefaultHskMem)/sizeof(FpeDefaultHskMem[0]),
            (u32 *)FpeDefaultHskMem,
            FPE_HMEM);
    cmdRsp("Loaded\n\r");
    sleep(2);

    // Load Sequence Memory
    cmdRsp("Seq Memory ....");
    xferFpeBuff(sizeof(FpeDefaultSeqMem)/sizeof(FpeDefaultSeqMem[0]),
            (u32 *)FpeDefaultSeqMem,
            FPE_SMEM);
    cmdRsp("Loaded\n\r");
    sleep(2);

    // Load Prog Memory
    cmdRsp("Prog Memory ....");
    xferFpeBuff(sizeof(FpeDefaultPrgMem)/sizeof(FpeDefaultPrgMem[0]),
            (u32 *)FpeDefaultPrgMem,
            FPE_PMEM);
    cmdRsp("Loaded\n\r");
    sleep(2);

    // Load Reg Memory
    cmdRsp("FPE Registers.....");
    xferFpeBuff(sizeof(FpeDefaultRegMem)/sizeof(FpeDefaultRegMem[0]),
            (u32 *)FpeDefaultRegMem,
            FPE_REGISTERS);
    cmdRsp("Loaded\n\r");
    sleep(2);

    // Configure DHU for frame collection.
    DHURegWrite(CAM_CONTROL,2);
    cmdRsp("Starting frames...\n\r?> ");
}
