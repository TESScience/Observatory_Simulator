/*
 * dhu.c
 *
 *  Created on: Apr 5, 2015
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
#include "dhu.h"

DHURegisterType DHURegisters[] = {
	{"CAM1_CONTROL", CAM_CONTROL},
	{"CAM1_STATUS", CAM_STATUS},
	{"CAM1_INTERRUPT",CAM_INTERRUPT},
	{"CAM1_INTERRUPT_ENA",CAM_INTERRUPT_ENA},
	{"CAM1_INTERRUPT_FORCE",CAM_INTERRUPT_FORCE},
	{"CAM1_INTERRUPT_SPM",CAM_INTERRUPT_SPM},
	{"CAM1_INTERRUPT_SPM_ENA",CAM_INTERRUPT_SPM_ENA},
	{"CAM1_INTERRUPT_SPM_FORCE",CAM_INTERRUPT_SPM_FORCE},
	{"CAM1_PPS_DELAY",CAM_PPS_DELAY},
	{"CAM1_FRAME_COUNT",CAM_FRAME_COUNT},
	{"CAM1_ALL_PIXELS",CAM_ALL_PIXELS},
	{"CAM1_DATA_PIXELS",CAM_DATA_PIXELS},
	{"CAM1_OC_PIXELS",CAM_OC_PIXELS},
	{"CAM1_NUM_BAD_PKTS_A",CAM_NUM_BADPKTS_A},
	{"CAM1_NUM_BAD_PKTS_B",CAM_NUM_BADPKTS_B},
	{"CAM1_TEST_DATA",CAM_TEST_DATA},
	{"CAM1_FPE_WDATA_BAR",CAM_FPE_WDATA_BAR},
	{"CAM1_FPE_WCTRL",CAM_FPE_WCTRL},
	{"CAM1_FPE_RCTRL",CAM_FPE_RCTRL},
	{"CAM1_FPE_BITFILE_SIZE",CAM_FPE_BITFILE_SIZE},
	{"CAM1_SPM0_CTRL",CAM_SPM0_CTRL},
	{"CAM1_SPM0_CURRENT_INT",CAM_SPM0_CURRENT_INT},
	{"CAM1_SPM0_FRAME_START",CAM_SPM0_FRAME_START},
	{"CAM1_SPM0_FRAME_END",CAM_SPM0_FRAME_END},
	{"CAM1_SPM0_BM_NUM_PXLS",CAM_SPM0_BM_NUM_PXLS},
	{"CAM1_SPM0_INT_NUM",CAM_SPM0_INT_NUM},
	{"CAM_SPM0_CHUNK_NUM",CAM_SPM0_CHUNK_NUM},
	{"CAM_SPM0_PAUSE_NUM",CAM_SPM0_PAUSE_NUM},
	{"CAM_SPM0_SATURATION",CAM_SPM0_SATURATION},
	{"CAM_SPM0_BM_BAR",CAM_SPM0_BM_BAR},
	{"CAM_SPM0_MIN_BAR",CAM_SPM0_MIN_BAR},
	{"CAM_SPM0_MAX_BAR",CAM_SPM0_MAX_BAR},
	{"CAM_SPM0_BAR_DATA_BUFA",CAM_SPM0_BAR_DATA_BUFA},
	{"CAM_SPM0_BAR_DATA_BUFB",CAM_SPM0_BAR_DATA_BUFB},

	{"CAM1_SPM1_CTRL",CAM_SPM1_CTRL},
	{"CAM1_SPM1_CURRENT_INT",CAM_SPM1_CURRENT_INT},
	{"CAM1_SPM1_FRAME_START",CAM_SPM1_FRAME_START},
	{"CAM1_SPM1_FRAME_END",CAM_SPM1_FRAME_END},
	{"CAM1_SPM1_BM_NUM_PXLS",CAM_SPM1_BM_NUM_PXLS},
	{"CAM1_SPM1_CHUNK_NUM",CAM_SPM1_CHUNK_NUM},
	{"CAM1_SPM1_PAUSE_NUM",CAM_SPM1_PAUSE_NUM},
	{"CAM1_SPM1_SATURATION",CAM_SPM1_SATURATION},
	{"CAM1_SPM1_BM_BAR",CAM_SPM1_BM_BAR},
	{"CAM1_SPM1_MIN_BAR",CAM_SPM1_MIN_BAR},
	{"CAM1_SPM1_MAX_BAR",CAM_SPM1_MAX_BAR},
	{"CAM1_SPM1_BAR_DATA_BUFA",CAM_SPM1_BAR_DATA_BUFA},
	{"CAM1_SPM1_BAR_DATA_BUFB",CAM_SPM1_BAR_DATA_BUFB},
	{"CAM1_SPM2_CTRL",CAM_SPM2_CTRL},
	{"CAM1_SPM2_CURRENT_INT",CAM_SPM2_CURRENT_INT},
	{"CAM1_SPM2_FRAME_START",CAM_SPM2_FRAME_START},
	{"CAM1_SPM2_FRAME_END",CAM_SPM2_FRAME_END},
	{"CAM1_SPM2_BM_NUM_PXLS",CAM_SPM2_BM_NUM_PXLS},
	{"CAM1_SPM2_INT_NUM",CAM_SPM2_INT_NUM},
	{"CAM1_SPM2_CHUNK_NUM",CAM_SPM2_CHUNK_NUM},
	{"CAM1_SPM2_PAUSE_NUM",CAM_SPM2_PAUSE_NUM},
	{"CAM1_SPM2_SATURATION",CAM_SPM2_SATURATION},
	{"CAM1_SPM2_BM_BAR",CAM_SPM2_BM_BAR},
	{"CAM1_SPM2_MIN_BAR",CAM_SPM2_MIN_BAR},
	{"CAM1_SPM2_MAX_BAR",CAM_SPM2_MAX_BAR},
	{"CAM1_SPM2_BAR_DATA_BUFA",CAM_SPM2_BAR_DATA_BUFA},
	{"CAM1_SPM2_BAR_DATA_BUFB",CAM_SPM2_BAR_DATA_BUFB},
	{"CAM1_SPM3_CTRL",CAM_SPM3_CTRL},
	{"CAM1_SPM3_CURRENT_INT",CAM_SPM3_CURRENT_INT},
	{"CAM1_SPM3_FRAME_START",CAM_SPM3_FRAME_START},
	{"CAM1_SPM3_FRAME_END",CAM_SPM3_FRAME_END},
	{"CAM1_SPM3_BM_NUM_PXLS",CAM_SPM3_BM_NUM_PXLS},
	{"CAM1_SPM3_INT_NUM",CAM_SPM3_INT_NUM},
	{"CAM1_SPM3_CHUNK_NUM",CAM_SPM3_CHUNK_NUM},
	{"CAM1_SPM3_PAUSE_NUM",CAM_SPM3_PAUSE_NUM},
	{"CAM1_SPM3_SATURATION",CAM_SPM3_SATURATION},
	{"CAM1_SPM3_BM_BAR",CAM_SPM3_BM_BAR},
	{"CAM1_SPM3_MIN_BAR",CAM_SPM3_MIN_BAR},
	{"CAM1_SPM3_MAX_BAR",CAM_SPM3_MAX_BAR},
	{"CAM1_SPM3_BAR_DATA_BUFA",CAM_SPM3_BAR_DATA_BUFA},
	{"CAM1_SPM3_BAR_DATA_BUFB",CAM_SPM3_BAR_DATA_BUFB},
	{"CAM1_SPM4_CTRL",CAM_SPM4_CTRL},
	{"CAM1_SPM4_CURRENT_INT",CAM_SPM4_CURRENT_INT},
	{"CAM1_SPM4_FRAME_START",CAM_SPM4_FRAME_START},
	{"CAM1_SPM4_FRAME_END",CAM_SPM4_FRAME_END},
	{"CAM1_SPM4_BM_NUM_PXLS",CAM_SPM4_BM_NUM_PXLS},
	{"CAM1_SPM4_INT_NUM",CAM_FPE_HSK},
	{"CAM1_SPM4_CHUNK_NUM",CAM_SPM4_CHUNK_NUM},
	{"CAM1_SPM4_PAUSE_NUM",CAM_SPM4_PAUSE_NUM},
	{"CAM1_SPM4_SATURATION",CAM_SPM4_SATURATION},
	{"CAM1_SPM4_BM_BAR",CAM_SPM4_BM_BAR},
	{"CAM1_SPM4_MIN_BAR",CAM_SPM4_MIN_BAR},
	{"CAM1_SPM4_MAX_BAR",CAM_SPM4_MAX_BAR},
	{"CAM1_SPM4_BAR_DATA_BUFA",CAM_SPM4_BAR_DATA_BUFA},
	{"CAM1_SPM4_BAR_DATA_BUFB",CAM_SPM4_BAR_DATA_BUFB},
	{"CAM1_FPE_HSK",CAM_FPE_HSK},					// 0x600  - 0x67F  Base of 127 registers
	{"CAM1_FPE_MEM_DUMP",CAM_FPE_MEM_DUMP}			// 0x800  - 0xFFF  Base of 2048 memory dump registers
};

//
// Look up DHU register offset by register name
// returns offset or -1 if name not found.
//
u32 DHURegLookup(char *regName)
{
int i;

for (i=0; i<(sizeof(DHURegisters)/sizeof(DHURegisters[0])); i++)
	if (0 == stricmp(DHURegisters[i].Name,regName)) return(DHURegisters[i].Index);
return(-1);
}

//
// Utility function to Write a DHURegister
// arg 1 - DHURegister number
// arg 2 - 32 bit value to be written
//
// returns value written
//
u32	DHURegWrite(u32 regOffset,u32 value)
{
	// Begin critical section....
	ENTER_CRITICAL;

	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(3<<2), (u32)value);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(1<<2), 1);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(2<<2), regOffset);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(0<<2), 0xAAAAAAAA);

	//
	// Wait here for PCIe[0] to clear....
	//
	while (XBram_In32(PCIeBram.Config.MemBaseAddress) & 0x0000FFFF);

	// End critical section....
	EXIT_CRITICAL;
	return value;
}

//
// Utility function to Read a DHURegister
// arg 1 - DHURegister number
//
// returns value read
//
u32	DHURegRead(u32 regOffset)
{
	u32 x;

	// Begin critical section....
	ENTER_CRITICAL;
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(1<<2), 1);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(2<<2), regOffset);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(0<<2), 0x55555555);
	//
	// Wait here for PCIe[0] to clear....
	//
	while (XBram_In32(PCIeBram.Config.MemBaseAddress) & 0x0000FFFF);

	//
	// Read PCIe[1] for return value
	//
	x =	XBram_In32(PCIeBram.Config.MemBaseAddress + (u32)(1<<2));

	// End critical section....
	EXIT_CRITICAL;
	return 	x;
}

//
// Utility function to Write a DHURegister
// arg 1 - DHURegister number
// arg 2 - 32 bit value to be written
//
// returns value written
//
u32	IntDHURegWrite(u32 regOffset,u32 value)
{
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(3<<2), (u32)value);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(1<<2), 1);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(2<<2), regOffset);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(0<<2), 0xAAAAAAAA);

	//
	// Wait here for PCIe[0] to clear....
	//
	while (XBram_In32(PCIeBram.Config.MemBaseAddress) & 0x0000FFFF);
	return value;
}

//
// Utility function to Read a DHURegister
// arg 1 - DHURegister number
//
// returns value read
//
u32	IntDHURegRead(u32 regOffset)
{
	u32 x;

	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(1<<2), 1);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(2<<2), regOffset);
	XBram_Out32(PCIeBram.Config.MemBaseAddress + (u32)(0<<2), 0x55555555);
	//
	// Wait here for PCIe[0] to clear....
	//
	while (XBram_In32(PCIeBram.Config.MemBaseAddress) & 0x0000FFFF);

	//
	// Read PCIe[1] for return value
	//
	x =	XBram_In32(PCIeBram.Config.MemBaseAddress + (u32)(1<<2));

	return 	x;
}
