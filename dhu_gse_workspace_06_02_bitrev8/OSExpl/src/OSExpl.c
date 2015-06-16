/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* XILINX CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * Simple serial -> BRAM tool for cat'ing log files to restore system
 * configuration.
 *
 * To Do:
 * - Frame collection (Partially implemented) and relay to network interface
 * - Add cam2 handling
 * - Send pixel frame data in background loop instead of ISR
 * - Look at udp packet loss/thruput issues (increase number/size of pbufs)
 * - Client cannot disconnect/reconnect to board (address in use issue)
 * - Try simulated full frames on dedicated network
 * + Added some interrupt disable/enable around masking of cam1IntStatus
 * Notes:
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   **ps7_uart    115200 (configured by bootrom/bsp)
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
#include <xscutimer.h>

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>				// prototype for sleep()
#include <string.h>
#include <time.h>

#include "fpe.h"
#include "dhu.h"

typedef struct {
	u32 flags;					// 0 = free
	u32 hwPixelCnt;				//
	u32 hwLastPixelCnt;			//
	u32 swPixelCnt;
	u32 hwFrameCnt;
	u32 swFrameCnt;
	u32 rdyIntCnt;
} typeFrameStatus;

typeFrameStatus lastFrameStatus;

#define USE_SERIAL			// Undefine to use UDP console instead
//#define SIMULATE_FRAMES			// Pretend frames are full

//***************************************************************************
// lwip related declarations and variables
//***************************************************************************

#include "netif/xadapter.h"
#define	LWIP_DHCP 1
#define PLATFORM_EMAC_BASEADDR XPAR_XEMACPS_0_BASEADDR

#if LWIP_DHCP==1
#include "lwip/dhcp.h"
#endif

/* defined by each RAW mode application */
void print_app_header();
int start_application();
int transfer_data();
void lwip_init();
void print_ip_settings(struct ip_addr *ip, struct ip_addr *mask, struct ip_addr *gw);

#if LWIP_DHCP==1
extern volatile int dhcp_timoutcntr;
err_t dhcp_start(struct netif *netif);
#endif

static struct netif server_netif;
struct netif *echo_netif;

#ifdef __arm__
#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1	\
    || XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
int ProgramSi5324(void);
int ProgramSfpPhy(void);
#endif
#endif

struct ip_addr ipaddr, netmask, gw;
/* the mac address of the board. this should be unique per board */
//unsigned char mac_ethernet_address[] ={ 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };	// Demitrios
unsigned char mac_ethernet_address[] ={ 0x00, 0x0a, 0x35, 0x00, 0x01, 0x03 };	// Kari


//***************************************************************************
// Global Variables/Macro Definitions....
//***************************************************************************

//
// Map the three BRAMs to their related GSE function
#define idPCI  (XPAR_AXI_BRAM_CTRL_0_DEVICE_ID)		// dual port memory stand in for PCI
#define idCam1 (XPAR_AXI_BRAM_CTRL_1_DEVICE_ID) 	// data to/from FPE 1
#define idCam2 (XPAR_AXI_BRAM_CTRL_2_DEVICE_ID) 	// data to/from FPE 2

// New interrupts per the latest implementation...
#define DDR_REQ_CAM2INT_ID		XPAR_FABRIC_SYSTEM_DDR_REQ_CAM2_INT_INTR
#define DDR_RDY_CAM2INT_ID		XPAR_FABRIC_SYSTEM_DDR_RDY_CAM2_INT_INTR
#define DHU_CAM2INT_ID			XPAR_FABRIC_SYSTEM_DHU_CAM2_INT_INTR
#define DDR_REQ_CAM1INT_ID		XPAR_FABRIC_SYSTEM_DDR_REQ_CAM1_INT_INTR
#define DDR_RDY_CAM1INT_ID		XPAR_FABRIC_SYSTEM_DDR_RDY_CAM1_INT_INTR
#define DHU_CAM1INT_ID			XPAR_FABRIC_SYSTEM_DHU_CAM1_INT_INTR

#define FRAME_BUF_IS_16

//
// Interrupt related variables...
//
u32		caught_interrupt,cam1DhuInterrupt;	/*  Set true in FPGAIntrHandler */
int		status;				/*	Passed to FPGAIntrHandler for TBD use	*/
u32 	fpeFrameNextData;	/*  What we expect for next ramp....		*/
volatile u32 cam1IntStatus,cam1SpmIntStatus,cam1DdrReq,cam1DdrRdy,cam1DdrReqCnt;
volatile u32 cam2IntStatus,cam2SpmIntStatus;
#ifdef FRAME_BUF_IS_16
u16		cam1FrameData[0x4000000];	// 32MB buffer for bitfiles and cam1Frames
u16		cam2FrameData[0x4000000];	// 32MB buffer for bitfiles and cam2Frames
#else
u32		cam1FrameData[0x4000000];	// 64MB buffer for bitfiles and cam1Frames
u32		cam2FrameData[0x4000000];	// 64MB buffer for bitfiles and cam2Frames
#endif
volatile u32 cam1FrameWriteIndex, cam1FrameReadIndex;
volatile u32 cam2FrameWriteIndex, cam2FrameReadIndex;
volatile u32 cam1FrameCnt,cam1FrameSize,cam1FrameRampFaults,cam1FrameTxIndex;
volatile u32 cam2FrameCnt;
u32 	fperestart_req,fpestart_req;
char 	cmdBuff[128];

XBram PCIeBram;				/* The Instance of the BRAM Driver */
XBram cam1Bram;				/* The Instance of the BRAM Driver */
XBram cam2Bram;				/* The Instance of the BRAM Driver */

XScuGic scuGic;		/* pl/pl fabric generic interrupt controller */
XScuGic_Config *IntcConfig;
XScuTimer TimerInstance;

extern struct udp_pcb *last_cmd_pcb_ptr;
extern struct pbuf *last_PB_ptr;
extern struct ip_addr *last_client_addr_ptr;
extern u16_t last_client_src_port;
extern struct udp_pcb *data_pcb_ptr;
extern u32 lwipTimerTicks;
extern char cmdPkt[128];
extern int  cmdLength;		// -1 or 0 is nothing there....

void StartOfFrame(void);
void timer_callback(XScuTimer * TimerInstance);
void EndOfFrame(void);
err_t send_response( struct udp_pcb *cmd_pcb_ptr, const void *cmd_resp, uint16_t resp_bytes );
err_t send_data( const void *data, uint16_t data_bytes );

	//
	// Stub for command responses....
	//
void cmdRsp(const char *fmt, ...)
{
	char tmp[128];
	va_list vargs;

	va_start(vargs,fmt);
	memset(tmp,0,sizeof(tmp));
	vsprintf(tmp,fmt,vargs);

#ifdef USE_SERIAL
	xil_printf("%s",tmp);
#else
	// if its a serial port....Eventually make this go away...
	//if (last_cmd_pcb_ptr == NULL)
	//		xil_printf("%s",tmp);
	// else
		send_response(last_cmd_pcb_ptr, tmp, strlen(tmp));
#endif
}

	//
	// nonBlocking wait for next udp packet from command port.
	// For now we only allow 1 at a time..
	// Returns length of string...
	//
	// OR ...serial port...
	//
int udpGets(char *myCmdBuff)
{
#ifdef USE_SERIAL
	static int x;
	int length;

	if (!XUartPs_IsReceiveData(STDIN_BASEADDRESS)) return 0;
		else {
			cmdBuff[x] = XUartPs_ReadReg(STDIN_BASEADDRESS, XUARTPS_FIFO_OFFSET);
			if (x >= sizeof(cmdBuff)) x = sizeof(cmdBuff) - 1;
			if ((cmdBuff[x] == '\n') || (cmdBuff[x] == '\r'))
			{
				length = x;
				x = 0;
				memcpy(myCmdBuff,cmdBuff,length);
				return(length);
			} else x++;
		}
	return(0);
#else
	int x;

	x = cmdLength;

	if (x > sizeof(cmdBuff)) x = sizeof(cmdBuff);
	if (x > 0)
	{
		memcpy(myCmdBuff,cmdPkt,x);
		cmdLength = 0;
	}
	return(x);
#endif
}


static void
Cam2ReqIntrHandler( void *CallBackRef )
{
	// Don't really know which one is interrupting...clear both for now...
//	cam2IntStatus |= DHURegRead(2);		// Read Interrupt status
//	DHURegWrite(2,DHUIntStatus);	// Write interrupt status to 0
//	caught_interrupt = 1;
    xil_printf( "Cam2 DDR Req. Interrupt\n\r" );
}

static void
Cam2RdyIntrHandler( void *CallBackRef )
{
	// Don't really know which one is interrupting...clear both for now...
//	cam2IntStatus |= DHURegRead(2);		// Read Interrupt status
//	DHURegWrite(2,DHUIntStatus);	// Write interrupt status to 0
//	caught_interrupt = 1;
    xil_printf( "Cam2 DDR Ready Interrupt\n\r" );
}

static void
Cam2DhuIntrHandler( void *CallBackRef )
{
	// Don't really know which one is interrupting...clear both for now...
//	cam2IntStatus |= DHURegRead(2);		// Read Interrupt status
//	DHURegWrite(2,DHUIntStatus);	// Write interrupt status to 0
//	caught_interrupt = 1;
    xil_printf( "Cam2 DHU Interrupt\n\r" );
}

static void
Cam1ReqIntrHandler( void *CallBackRef )
{
	cam1DdrReq = 1;
	cam1DdrReqCnt++;
}

static void
Cam1RdyIntrHandler( void *CallBackRef )
{
#if 0
	u32 *x;
	u32 *y;
	u32 z;

	// This interrupt is a pulse 100ns edge so no clearing required...
	if (cam1FrameWriteIndex >= (sizeof(cam1FrameData)/sizeof(cam1FrameData[0])))
		xil_printf("Frame reception overflow\n\r");
	else {
		x = &cam1FrameData[cam1FrameWriteIndex];
		y = cam1Bram.Config.MemBaseAddress;
		for (z = 0; z<1024; z++) *x++ = *y++;
		cam1FrameWriteIndex += 1024;
		cam1FrameSize = cam1FrameWriteIndex;
	}
#else
	// This interrupt is a pulse 100ns edge so no clearing required...
	if (cam1FrameWriteIndex >= (sizeof(cam1FrameData)/sizeof(cam1FrameData[0])))
		xil_printf("Frame reception overflow\n\r");
	else {
		// faster way but now we need 2x the buffer space!
		memcpy((void *)&cam1FrameData[cam1FrameWriteIndex],
				cam1Bram.Config.MemBaseAddress,
				2048);
//				4096);
// 32 bits/pixel
//		cam1FrameWriteIndex += 1024;
// 16 bits/pixel ...so we move twice as far
//		cam1FrameWriteIndex += 2048;
//16 bits/pixel, but we only move half every time
		cam1FrameWriteIndex += 1024;
		cam1FrameSize = cam1FrameWriteIndex;
	}
#endif

}


static void Cam1DhuIntrHandler( void *CallBackRef )
{
	static u32 iTmp,iTmp2;

	// Don't really know which one is interrupting...clear both for now...
	iTmp = DHURegRead(CAM_INTERRUPT);				// Read Interrupt status
	cam1IntStatus |= iTmp;				// Read Interrupt status
	IntDHURegWrite(CAM_INTERRUPT,iTmp);				// Write interrupt status to clear pending interrupts
	iTmp2 = IntDHURegRead(CAM_INTERRUPT_SPM);		// Read Spm Interrupt status
	cam1SpmIntStatus |= iTmp2;		// Read Spm Interrupt status
	IntDHURegWrite(CAM_INTERRUPT_SPM,iTmp2);		// Write Spm interrupt status to clear pending interrupts
	cam1DhuInterrupt = 1;

	//
	// If the DHU is enabled and we have an FPE_LOADED interrupt then a new PPS has just gone
	// out.   This means start of frame so close the previous frame (if any).
	//
	iTmp = IntDHURegRead(CAM_CONTROL);
	if ((((DHUCamControl_bits *)&iTmp)->ENABLE_ALL) &&
		(cam1IntStatus & 0x000000002))
	{
			StartOfFrame();
			cam1IntStatus &= ~0x02;
	}

	//karih addition
	//FpeReadHsk("32");
	// check housekeeping and read out
	if ((cam1IntStatus & 0x00000004))
	{
//		xil_printf("Received a housekeeping interrupt.\n\r");
		FpeReadHsk("24");		// Just dump 32 for now
		cam1IntStatus &= ~0x04;
	}

	//check end of frame as reported by SPMs
	if (cam1SpmIntStatus & 0x00000001)
	{
//		xil_printf("Received an end-of-frame interrupt on SPM buffer A\n\r");
//		FpeReadHsk("32");
		EndOfFrame();
		cam1SpmIntStatus &= ~0x01;
	} else if (cam1SpmIntStatus & 0x00000002)
	{
//		xil_printf("Received an end-of-frame interrupt on SPM Buffer B\n\r");
		EndOfFrame();
		cam1SpmIntStatus &= ~0x02;
	}

}

	//
	// Check for linearly increasing pixel data from the camera in test mode.
	//
void CheckPixelRamp(void)
{
	int cycles = 0;

#if 0
	// Generate UDP traffic too
	// jal - fix to be sure and send last block of pixels...
	if  ((cam1FrameTxIndex + 256) < cam1FrameSize)
	{
		send_data(&cam1FrameData[cam1FrameTxIndex],1024);
		cam1FrameTxIndex += 256;
	}
#endif


	while (cam1FrameReadIndex < cam1FrameSize)
	{
		//karih modification
		if ((cam1FrameData[cam1FrameReadIndex]&0x0000FFFF) != (fpeFrameNextData&0x0000FFFF))
		{
			//
			// If there are a lot of errors printing will take way too long....
			// Should probably just count stuff and print at the end.
			//
#if 1
			//if (cam1FrameReadIndex<4)
			xil_printf("At 0x%04X Frame Ramp Fault (got/exp) - 0x%08x, 0x%08x\n\r",
					cam1FrameReadIndex,
					cam1FrameData[cam1FrameReadIndex],
					fpeFrameNextData);
			    cam1FrameRampFaults++;
#else

				cam1FrameRampFaults++;
#endif
			fpeFrameNextData = cam1FrameData[cam1FrameReadIndex]&0x0000FFFF;
		}
		fpeFrameNextData++;
		cam1FrameReadIndex++;
		if (++cycles > 100) break;
	}
}


	//
	// All the processing associated with closing an old frame and starting a new one
	//
void SendFrameStartMarker(u32 cnt)
{
#ifndef USE_SERIAL
	char msg[64];

	memset(msg,0,sizeof(msg));
	sprintf(msg,"%d : Starting Frame - %d\n\r",lwipTimerTicks,cnt);
	send_data(msg, strlen(msg));
#endif
}

void StartOfFrame(void)
{
#if 0
	// Check to see if background frame ramp check/udp send is caught up?
	if (cam1FrameReadIndex != cam1FrameSize)
		cmdRsp("Ramp Check/udp send too late!\n\r");
#endif
#if 1
	cmdRsp("%d checks performed\n\r",cam1FrameReadIndex);
	if (cam1FrameRampFaults > 0)
		cmdRsp("%d ramp faults\n\r",cam1FrameRampFaults);
	else
		cmdRsp("no ramp faults\n\r");
#endif
	cam1FrameWriteIndex = 0;
	cam1FrameReadIndex = 0;
	cam1FrameTxIndex = 0;
	// Simulate full size frame
#ifdef SIMULATE_FRAMES
	cam1FrameSize = 17754432;	// full size frame
#else
	cam1FrameSize = 0;
#endif
	cam1FrameRampFaults = 0;
	// This only sends a udp packet if the network interface is configured in.
	SendFrameStartMarker(cam1FrameCnt);
}

//karih addition
void EndOfFrame(void)
{
	static u32 iFrameCntReg,iPixelCntReg;

	iFrameCntReg = IntDHURegRead(CAM_FRAME_COUNT);				// Read Frame Count
	iPixelCntReg = IntDHURegRead(CAM_ALL_PIXELS);				// Read pixel count
	lastFrameStatus.hwLastPixelCnt = lastFrameStatus.hwPixelCnt;
	lastFrameStatus.hwPixelCnt = iPixelCntReg;
	lastFrameStatus.hwFrameCnt = iFrameCntReg;
	lastFrameStatus.swFrameCnt = cam1FrameCnt++;
	lastFrameStatus.swPixelCnt = cam1FrameWriteIndex;			// size rounded up?
	lastFrameStatus.flags = 1;									// Tell main loop to examine lastFrameStatus

	// Discard contents ...if they haven't been transmitted by now its too late.
	cam1FrameWriteIndex = 0;
//	cam1FrameReadIndex = 0; //karih removed to fix ramp issue

}
//end of karih addition

static void
SetUpInterruptSystem(void)
{

    /*  Enable IRQ interrupts in the Processor.  */
	Xil_ExceptionInit();

	//initialize the GIC
	IntcConfig = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
	XScuGic_CfgInitialize(&scuGic, IntcConfig,IntcConfig->CpuBaseAddress);

	//Connect to the Gic to CPU IRQ (Int 5)
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
						(Xil_ExceptionHandler)XScuGic_InterruptHandler,
						&scuGic);

	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

    if (XST_SUCCESS  != XScuGic_SelfTest(&scuGic))
    {
    	xil_printf("scuGic self test failed\n\r");
    } else {

       u8 Priority = 0;			// Start with higherst prior for now...
 	   u8 Trigger = 0x03;		// Configure edge/level sensitivity...

 	   XScuGic_SetPriorityTriggerType(&scuGic, DDR_REQ_CAM2INT_ID, Priority, Trigger);
 	   XScuGic_SetPriorityTriggerType(&scuGic, DDR_RDY_CAM2INT_ID, Priority, Trigger);
 	   XScuGic_SetPriorityTriggerType(&scuGic, DDR_REQ_CAM1INT_ID, Priority, Trigger);
 	   XScuGic_SetPriorityTriggerType(&scuGic, DDR_RDY_CAM1INT_ID, Priority, Trigger);

 	   Trigger = 0x01;			// High/level sensitive...
 	   Priority = 8;			// These two are lower priority than the edge triggered ones.
 	   XScuGic_SetPriorityTriggerType(&scuGic, DHU_CAM2INT_ID, Priority, Trigger);
 	   XScuGic_SetPriorityTriggerType(&scuGic, DHU_CAM1INT_ID, Priority, Trigger);


    	/*  Enable individual interrupt Sources	     */
	   if (XST_SUCCESS != XScuGic_Connect(&scuGic, DHU_CAM1INT_ID,( Xil_ExceptionHandler ) Cam1DhuIntrHandler, &status ))
		   xil_printf("Failed to connect DHU_CAM1INT_ID handler\n\r");
	   XScuGic_Enable(&scuGic, DHU_CAM1INT_ID);

	   if (XST_SUCCESS != XScuGic_Connect(&scuGic, DDR_RDY_CAM1INT_ID,( Xil_ExceptionHandler ) Cam1RdyIntrHandler, &status))
		   xil_printf("Failed to connect DDR_RDY_CAM1INT_ID handler\n\r");
	   XScuGic_Enable(&scuGic, DDR_RDY_CAM1INT_ID);

	   if (XST_SUCCESS != XScuGic_Connect(&scuGic, DDR_REQ_CAM1INT_ID,( Xil_ExceptionHandler ) Cam1ReqIntrHandler, &status ))
		   xil_printf("Failed to connect DDR_REQ_CAM1INT_ID handler\n\r");
	   XScuGic_Enable(&scuGic, DDR_REQ_CAM1INT_ID);

	   if (XST_SUCCESS != XScuGic_Connect(&scuGic, DHU_CAM2INT_ID, ( Xil_ExceptionHandler ) Cam2DhuIntrHandler, &status ))
		   xil_printf("Failed to connect DHU_CAM2INT_ID handler\n\r");
	   XScuGic_Disable(&scuGic, DHU_CAM2INT_ID);
//	   XScuGic_Enable(&scuGic, DHU_CAM2INT_ID);

	   if (XST_SUCCESS != XScuGic_Connect(&scuGic, DDR_RDY_CAM2INT_ID, ( Xil_ExceptionHandler ) Cam2RdyIntrHandler, &status ))
		   xil_printf("Failed to connect DDR_RDY_CAM2INT_ID handler\n\r");
	   XScuGic_Disable(&scuGic, DDR_RDY_CAM2INT_ID);
//	   XScuGic_Enable(&scuGic, DDR_RDY_CAM2INT_ID);

	   if (XST_SUCCESS != XScuGic_Connect(&scuGic, DDR_REQ_CAM2INT_ID,( Xil_ExceptionHandler ) Cam2ReqIntrHandler, &status ))
		   xil_printf("Failed to connect DDR_REQ_CAM2INT_ID handler\n\r");
	   XScuGic_Disable(&scuGic, DDR_REQ_CAM2INT_ID);
//	   XScuGic_Enable(&scuGic, DDR_REQ_CAM2INT_ID);

	   // We have to also enable a timer interrupt for the lwip stack
#ifndef USE_SERIAL
	   Trigger = 0x01;
	   Priority = 16;	// Even lower priority
	   XScuGic_SetPriorityTriggerType(&scuGic, XPAR_SCUTIMER_INTR, Priority, Trigger);
	   if (XST_SUCCESS != XScuGic_Connect(&scuGic, XPAR_SCUTIMER_INTR,
			   	   	   	   (Xil_ExceptionHandler)timer_callback,
			   	   	   	   (void *)&TimerInstance))
		   xil_printf("Failed to connect XPAR_SCUTIMER_INTR handler\n\r");
	   XScuTimer_ClearInterruptStatus(&TimerInstance);
	   XScuGic_Enable(&scuGic, XPAR_SCUTIMER_INTR);
	   XScuTimer_Start(&TimerInstance);
	   XScuTimer_EnableInterrupt(&TimerInstance);
#endif
    }
}

	//
	// Single threaded hack...
	//  while we are waiting for hardware to do something and/or for
	//  inbound command arguments, call this function to keep the ip stack
	//  happy.
	//
void DoIdle(void)
{
	u32 pixdiff;

#ifndef USE_SERIAL
	// Run network stack ....
	xemacif_input(echo_netif);
	transfer_data();
#endif
	if (lastFrameStatus.flags)
	{
		cmdRsp("The FW reported frame count is  0x%02x\n\r",lastFrameStatus.hwFrameCnt);
		pixdiff = lastFrameStatus.hwPixelCnt - lastFrameStatus.hwLastPixelCnt;
		cmdRsp("The FW reported total number of pixels received is  %d\n\r",pixdiff);
		cmdRsp("The SW reported frame count is - %d, size = %d words.\n\r",lastFrameStatus.swFrameCnt,lastFrameStatus.swPixelCnt);
		lastFrameStatus.flags = 0;
	}
}




/****************************************************************************/
/**
*
* This function ensures that ECC in the BRAM is initialized if no hardware
* initialization is available. The ECC bits are initialized by reading and
* writing data in the memory. This code is not optimized to only read data
* in initialized sections of the BRAM.
*
* @param	ConfigPtr is a reference to a structure containing information
*		about a specific BRAM device.
* @param 	EffectiveAddr is the device base address in the virtual memory
*		address space.
*
* @return
*		None
*
* @note		None.
*
*****************************************************************************/
void InitializeECC(XBram_Config *ConfigPtr, u32 EffectiveAddr)
{
	u32 Addr;
	volatile u32 Data;

	if (ConfigPtr->EccPresent &&
	    ConfigPtr->EccOnOffRegister &&
	    ConfigPtr->EccOnOffResetValue == 0 &&
	    ConfigPtr->WriteAccess != 0) {
		for (Addr = ConfigPtr->MemBaseAddress;
		     Addr < ConfigPtr->MemHighAddress; Addr+=4) {
			Data = XBram_In32(Addr);
			XBram_Out32(Addr, Data);
		}
		XBram_WriteReg(EffectiveAddr, XBRAM_ECC_ON_OFF_OFFSET, 1);
	}
}

//
// Simple rw test on a block of memory (including BRAM!)
// a) The memory range is initialized with a single bit set in each location
// b) The memory range is read back to verify that expected values were written
// c) The test bit location is shifted by 1 position and go back to a)
//
// This test prints per cycle results on std out
//
void memtest_walking(u32 *base, u32 length, u32 cycles)
{
	u32 testpattern = 0x00000001;
	u32 *ptrValue;
	u32 errorsThisCycle;
	u32 myCycles;
	u32 i;

	xil_printf("Testing from %08x to %08x\n\r",
			(u32)base, (u32)(base + length));

	for (ptrValue = base, i=0; i < length; i++) XBram_Out32((u32)(ptrValue++),0);

	for (myCycles = 0; myCycles < cycles; myCycles++)
	{
		testpattern = (0x00000001 << (myCycles % 32));
		errorsThisCycle = 0;

		// Initialize memory
		for (ptrValue = base, i=0; i < length; i++)
		{
#ifdef mem_verbose
			xil_printf("Wrote base[%08x] = %08x\n\r",
					(u32)(ptrValue),testpattern);
#endif
			XBram_Out32((u32)(ptrValue++),testpattern);
			testpattern <<= 1;
			if (!testpattern) testpattern = 0x00000001;
		}

		testpattern = (0x00000001 << (myCycles % 32));
		// Verify memory
		for (ptrValue = base, i=0; i < length; i++)
		{
#ifdef mem_verbose
			xil_printf("Read base[%08x] = %08x\n\r",
					(u32)(ptrValue),*ptrValue);
#endif
			if (XBram_In32((u32)ptrValue) != testpattern)
			{
				errorsThisCycle++;
				xil_printf("\n\r\tFault at %08x - got %08x, expected %08x\n\r",
						(u32)ptrValue, *(u32 *)ptrValue, testpattern);
			}
			ptrValue++;
			testpattern <<= 1;
			if (!testpattern) testpattern = 0x00000001;
		}
		xil_printf("Test cycle %d - %d errors\n\r", myCycles, errorsThisCycle);
	}
}



	//
	// Setup all three BRAMs...
	// Generate warnings if initialize fails...
	//
int bramInitialize(void)
{
	int Status;
	XBram_Config *ConfigPtr;

	/*
	 * Initialize the BRAM driver. If an error occurs then exit
	 */
	ConfigPtr = XBram_LookupConfig(XPAR_PCIE_BRAM_CTRL_DEVICE_ID);
	if (ConfigPtr == (XBram_Config *) NULL) {
		return XST_FAILURE;
	}

	Status = XBram_CfgInitialize(&PCIeBram, ConfigPtr,ConfigPtr->CtrlBaseAddress);
	if (Status != XST_SUCCESS) { return XST_FAILURE;	}
    InitializeECC(ConfigPtr, ConfigPtr->CtrlBaseAddress);

	ConfigPtr = XBram_LookupConfig(XPAR_DDR1_BRAM_CTRL_DEVICE_ID);
	if (ConfigPtr == (XBram_Config *) NULL) {
		return XST_FAILURE;
	}

	Status = XBram_CfgInitialize(&cam1Bram, ConfigPtr,ConfigPtr->CtrlBaseAddress);
	if (Status != XST_SUCCESS) { return XST_FAILURE;	}
    InitializeECC(ConfigPtr, ConfigPtr->CtrlBaseAddress);

	ConfigPtr = XBram_LookupConfig(XPAR_DDR2_BRAM_CTRL_DEVICE_ID);
	if (ConfigPtr == (XBram_Config *) NULL) {
		return XST_FAILURE;
	}

	Status = XBram_CfgInitialize(&cam2Bram, ConfigPtr,ConfigPtr->CtrlBaseAddress);
	if (Status != XST_SUCCESS) { return XST_FAILURE;	}
    InitializeECC(ConfigPtr, ConfigPtr->CtrlBaseAddress);

    return XST_SUCCESS;
}

int ProcessConsoleInput(char *cmdBuff)
{
	char *addr,*value;
    int regOffset;
    u32 result;

		// xil_printf("cmd\[] =  %s\n\r",cmdBuff);

		// Figure out what the command means and do it!
		// Handle command to quit
		if (strncmp(cmdBuff,"quit",4) == 0) return XST_FAILURE;

		//
		// These commands all take multiple arguments (sometime a lot!)
		// so we can't expect the whole command on one line.
		//
		// Once invoked,  these commands will continue accepting command line values
		// up until the magic string "end" is received on a line by itself.   Alternatively,
		// if it can be determined that the commands always have a fixed argument list we
		// can do it that way.
		//
		// NOTE :  In theory the argument list could exceed the length of the BRAM so
		//  eventually we will have to be able to send chunks of BRAM within the same
		//  command.
		//
		if (strncmp(cmdBuff,"seqmem",6) == 0) {FpeLoadSeq(); return XST_SUCCESS;}
		if (strncmp(cmdBuff,"prgmem",6) == 0) {FpeLoadProg();return XST_SUCCESS;}
		if (strncmp(cmdBuff,"hskmem",6) == 0) {FpeLoadHK(); return XST_SUCCESS;}
		if (strncmp(cmdBuff,"clvmem",6) == 0) {FpeLoadClv(); return XST_SUCCESS;}
//		if (strncmp(cmdBuff,"bitmem",6) == 0) {FpeLoadBit(); return XST_SUCCESS;}
		if (strncmp(cmdBuff,"camrst",6) == 0) {FpeReset(); return XST_SUCCESS;}
		if (strncmp(cmdBuff,"camreg",6) == 0) {FpeLoadReg(); return XST_SUCCESS;}
		if (strncmp(cmdBuff,"camstrt",7) == 0) {FpeInitDefaults(0); return XST_SUCCESS;}
		if (strncmp(cmdBuff,"camconfig",9) == 0) {FpeInitDefaults(1); return XST_SUCCESS;}
		if (strncmp(cmdBuff,"camreload",6) == 0) {FpeReloadMems(); return XST_SUCCESS;}

		// Might be a manual read/write command
		// Command is "<register name> [value}".   If no value is supplied then its a read
		//  operation else write
		addr = strtok(cmdBuff," ,=\n\r");		// register name
		value = strtok(NULL," =,\n\r");
		// Scan DHU register name list looking for matching name
		if ((regOffset = DHURegLookup(addr)) >= 0)
		{
			if (regOffset == CAM_FPE_HSK) FpeReadHsk(value);
			else if (regOffset == CAM_FPE_MEM_DUMP) FpeReadMem(value);
			else if (!value)
				{
					result = DHURegRead(regOffset);
					cmdRsp("%s = 0x%08x\n\r?> ",addr,result);
				} else {
					sscanf(value,"%x",(unsigned int *)&result);
					DHURegWrite(regOffset,result);
					xil_printf("%s", "Waiting for 100 us between write and read\n\r");
					result = DHURegRead(regOffset);
					cmdRsp("%s = 0x%08x\n\r?> ",addr,result);
				}
		} else xil_printf("\n\r?> ");
		return XST_SUCCESS;
}



int main()
{
	int fatalError;
	DHUCamStatus_bits dhu_status;

	// Hardware platform initialization
    init_platform();

    // Preload cam1FrameData[] with a dummy ramp
    for (cam1FrameWriteIndex = 0; cam1FrameWriteIndex < 17754432; cam1FrameWriteIndex++)
    		cam1FrameData[cam1FrameWriteIndex] = cam1FrameWriteIndex;

    cam1FrameReadIndex = 0;
    cam1FrameWriteIndex = 0;
    cam2FrameReadIndex = 0;
    cam2FrameWriteIndex = 0;
    cam1FrameCnt = 0;
    cam2FrameCnt = 0;


    // Sign on/version message
    xil_printf("Observatory Simulator - BRAM Explorer v0.08 %s %s\n\r",__DATE__,__TIME__);

    // Setup/initialize BRAM's....not done by init_platform()
    if (XST_SUCCESS != bramInitialize())
    {
        xil_printf("BRAM initialization failure ...exiting\n\r");
        goto fatalerror;
    }

    // DHURegRead/Write() interract with xscugic so we have to initialize
    // before calls.
    SetUpInterruptSystem();

	// Read/print DHU status
	dhu_status.word = IntDHURegRead(CAM_STATUS);
	xil_printf("DHU/PL firmware Version = 0x%02x\n\r", dhu_status.bitField.DHU_FW_VERSION);

    // Setup bram related interrupts....
    xil_printf("Observatory Simulator - Setting up the interrupt system.\n\r");
	IntDHURegWrite(CAM_INTERRUPT, 0xFFFFF);	// Clear all the interrupts
	IntDHURegWrite(CAM_INTERRUPT_ENA, 0x0);	// Disable all the interrupts
	// We have to enable interrupts from the cam1 brams or the command channel won't work right..
	DHURegWrite(CAM_INTERRUPT_ENA, 0x0002);

#ifndef USE_SERIAL
	// networking layer bring up ....
	echo_netif = &server_netif;
	#ifdef __arm__
	#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1	\
	|| XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
	ProgramSi5324();
	ProgramSfpPhy();
	#endif
	#endif

#if LWIP_DHCP==1
	ipaddr.addr = 0;
	gw.addr = 0;
	netmask.addr = 0;
#else
	/* initliaze IP addresses to be used */
	IP4_ADDR(&ipaddr,  192, 168,   100, 1);
	IP4_ADDR(&netmask, 255, 255, 255,  0);
	IP4_ADDR(&gw,      192, 168,   100,  2);
#endif
	print_app_header();

	lwip_init();
		/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(echo_netif, &ipaddr, &netmask,
			&gw, mac_ethernet_address,
			PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n\r");
		return -1;
	}
	netif_set_default(echo_netif);

	/* specify that the network if is up */
	netif_set_up(echo_netif);
#if (LWIP_DHCP==1)
	/* Create a new DHCP client for this interface.
	 * Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
	 * the predefined regular intervals after starting the client.
	 */
	dhcp_start(echo_netif);
	dhcp_timoutcntr = 24;

	while(((echo_netif->ip_addr.addr) == 0) && (dhcp_timoutcntr > 0))
	{
		xemacif_input(echo_netif);
	}

	if (dhcp_timoutcntr <= 0) {
		if ((echo_netif->ip_addr.addr) == 0) {
			xil_printf("DHCP Timeout\r\n");
			xil_printf("Configuring default IP of 192.168.1.10\r\n");
			IP4_ADDR(&(echo_netif->ip_addr),  192, 168,  100, 1);
			IP4_ADDR(&(echo_netif->netmask), 255, 255, 255,  0);
			IP4_ADDR(&(echo_netif->gw),      192, 168,   100,  2);
		}
	}

	ipaddr.addr = echo_netif->ip_addr.addr;
	gw.addr = echo_netif->gw.addr;
	netmask.addr = echo_netif->netmask.addr;
#endif

	print_ip_settings(&ipaddr, &netmask, &gw);

	/* start the application (web server, rxtest, txtest, etc..) */
	start_application();
#endif

	cmdRsp("Observatory Simulator - Done setting up the interrupt system.\n\r");
	cmdRsp("Observatory Simulator - Entering Background loop\n\r");
	cmdRsp("?> ");

    for (fatalError = 0; !fatalError;)
    {
    	DoIdle();

    	/* Check if anything is available */
    	if (udpGets(cmdBuff))
    	{
    			ProcessConsoleInput(cmdBuff);
    	}
#if 0
    	CheckPixelRamp();
#endif

    }

fatalerror:
    // Hardware platform cleanup
    xil_printf("Observatory Simulator - Exiting\n\r");
    // Disconnect/disable interrupts from scuGic
    XScuGic_Disable(&scuGic, DHU_CAM1INT_ID);
    XScuGic_Disable(&scuGic, DDR_RDY_CAM1INT_ID);
    XScuGic_Disable(&scuGic, DDR_REQ_CAM1INT_ID);
    XScuGic_Disable(&scuGic, DHU_CAM2INT_ID);
    XScuGic_Disable(&scuGic, DDR_RDY_CAM2INT_ID);
    XScuGic_Disable(&scuGic, DDR_REQ_CAM2INT_ID);
    XScuGic_Disable(&scuGic, XPAR_SCUTIMER_INTR);
    // Disable IRQ input
    Xil_ExceptionDisable( );
    cleanup_platform();
    return 0;
}

