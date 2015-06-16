/*
 * dhu.h
 *
 *  Created on: Apr 5, 2015
 *      Author: jal
 */

#ifndef DHU_H_
#define DHU_H_

// Define a bitmap for the DHU Cam control register
typedef struct {
	u32 RESET_ALL :1 ;		// Bit 0 - 0 = not reset, 1 = reset this camera dhu logic
	u32 ENABLE_ALL  :1 ;	// Bit 1 - 0 = Disable Camera, 1 = Enable Camera
	u32 : 0;				// Fill to next boundary
} DHUCamControl_bits;

// Define a bitmap for the DHU Cam control register
typedef union {
	struct {
		u32 DHU_FW_VERSION :8 ;	// DHU Firmware Version
		u32 FPGA_ID : 1;		// ???
		u32 FPE_FW_VERSION :3 ;	// DHU Firmware Version
		u32 FPE_CAM_ID : 5;		//
		u32 TEST_MODE : 2;		// Camera test mode
		u32 INSYNC_B :1;
		u32 INSYNC_A :1;
		u32 PCIE_APP_RDY : 1;
		u32 PCIE_LINKUP : 1;
		u32 DDR_CALIB_COMP : 1;
	} bitField;
	u32 word;
} DHUCamStatus_bits;

// Define a bitmap for the DHU Cam control register
typedef struct {
	u32 SPM0_BUFA_RDY :1 ;		// Buffer B of SPM is ready to transfer
	u32 SPM1_BUFA_RDY :1 ;		// Buffer B of SPM is ready to transfer
	u32 SPM2_BUFA_RDY :1 ;		// Buffer B of SPM is ready to transfer
	u32 SPM3_BUFA_RDY :1 ;		// Buffer B of SPM is ready to transfer
	u32 SPM4_BUFA_RDY :1 ;		// Buffer B of SPM is ready to transfer

	u32 SPM0_BUFB_RDY :1 ;		// Buffer B of SPM is ready to transfer
	u32 SPM1_BUFB_RDY :1 ;		// Buffer B of SPM is ready to transfer
	u32 SPM2_BUFB_RDY :1 ;		// Buffer B of SPM is ready to transfer
	u32 SPM3_BUFB_RDY :1 ;		// Buffer B of SPM is ready to transfer
	u32 SPM4_BUFB_RDY :1 ;		// Buffer B of SPM is ready to transfer

	u32 SPM0_REG_ERR :1 ;		// SPM config register parity error
	u32 SPM1_REG_ERR :1 ;		// SPM config register parity error
	u32 SPM2_REG_ERR :1 ;		// SPM config register parity error
	u32 SPM3_REG_ERR :1 ;		// SPM config register parity error
	u32 SPM4_REG_ERR :1 ;		// SPM config register parity error

	u32 SPM0_BM_ERR :1 ;		// SPM Bit Mask Error
	u32 SPM1_BM_ERR :1 ;		// SPM Bit Mask Error
	u32 SPM2_BM_ERR :1 ;		// SPM Bit Mask Error
	u32 SPM3_BM_ERR :1 ;		// SPM Bit Mask Error
	u32 SPM4_BM_ERR :1 ;		// SPM Bit Mask Error
	u32 : 0;
} DHUCamInterruptSpm_bits;

typedef struct {
	char *Name;
	int  Index;
} DHURegisterType;

typedef enum  {
		CAM_CONTROL=0,
		CAM_STATUS,
		CAM_INTERRUPT,
		CAM_INTERRUPT_ENA,
		CAM_INTERRUPT_FORCE,
		CAM_INTERRUPT_SPM,
		CAM_INTERRUPT_SPM_ENA,
		CAM_INTERRUPT_SPM_FORCE,
		CAM_PPS_DELAY,
		CAM_FRAME_COUNT,
		CAM_ALL_PIXELS,
		CAM_DATA_PIXELS,
		CAM_OC_PIXELS,
		CAM_NUM_BADPKTS_A,
		CAM_NUM_BADPKTS_B,
		CAM_TEST_DATA,
		CAM_FPE_WDATA_BAR,
		CAM_FPE_WCTRL,
		CAM_FPE_RCTRL,
		CAM_FPE_BITFILE_SIZE,
		CAM_SPM0_CTRL = 0x0100,
		CAM_SPM0_CURRENT_INT,
		CAM_SPM0_FRAME_START,
		CAM_SPM0_FRAME_END,
		CAM_SPM0_BM_NUM_PXLS,
		CAM_SPM0_INT_NUM,
		CAM_SPM0_CHUNK_NUM,
		CAM_SPM0_PAUSE_NUM,
		CAM_SPM0_SATURATION,
		CAM_SPM0_BM_BAR,
		CAM_SPM0_MIN_BAR,
		CAM_SPM0_MAX_BAR,
		CAM_SPM0_BAR_DATA_BUFA,
		CAM_SPM0_BAR_DATA_BUFB,
		CAM_SPM1_CTRL = 0x0200,
		CAM_SPM1_CURRENT_INT,
		CAM_SPM1_FRAME_START,
		CAM_SPM1_FRAME_END,
		CAM_SPM1_BM_NUM_PXLS,
		CAM_SPM1_INT_NUM,
		CAM_SPM1_CHUNK_NUM,
		CAM_SPM1_PAUSE_NUM,
		CAM_SPM1_SATURATION,
		CAM_SPM1_BM_BAR,
		CAM_SPM1_MIN_BAR,
		CAM_SPM1_MAX_BAR,
		CAM_SPM1_BAR_DATA_BUFA,
		CAM_SPM1_BAR_DATA_BUFB,
		CAM_SPM2_CTRL = 0x0300,
		CAM_SPM2_CURRENT_INT,
		CAM_SPM2_FRAME_START,
		CAM_SPM2_FRAME_END,
		CAM_SPM2_BM_NUM_PXLS,
		CAM_SPM2_INT_NUM,
		CAM_SPM2_CHUNK_NUM,
		CAM_SPM2_PAUSE_NUM,
		CAM_SPM2_SATURATION,
		CAM_SPM2_BM_BAR,
		CAM_SPM2_MIN_BAR,
		CAM_SPM2_MAX_BAR,
		CAM_SPM2_BAR_DATA_BUFA,
		CAM_SPM2_BAR_DATA_BUFB,
		CAM_SPM3_CTRL = 0x0400,
		CAM_SPM3_CURRENT_INT,
		CAM_SPM3_FRAME_START,
		CAM_SPM3_FRAME_END,
		CAM_SPM3_BM_NUM_PXLS,
		CAM_SPM3_INT_NUM,
		CAM_SPM3_CHUNK_NUM,
		CAM_SPM3_PAUSE_NUM,
		CAM_SPM3_SATURATION,
		CAM_SPM3_BM_BAR,
		CAM_SPM3_MIN_BAR,
		CAM_SPM3_MAX_BAR,
		CAM_SPM3_BAR_DATA_BUFA,
		CAM_SPM3_BAR_DATA_BUFB,
		CAM_SPM4_CTRL = 0x0500,
		CAM_SPM4_CURRENT_INT,
		CAM_SPM4_FRAME_START,
		CAM_SPM4_FRAME_END,
		CAM_SPM4_BM_NUM_PXLS,
		CAM_SPM4_INT_NUM,
		CAM_SPM4_CHUNK_NUM,
		CAM_SPM4_PAUSE_NUM,
		CAM_SPM4_SATURATION,
		CAM_SPM4_BM_BAR,
		CAM_SPM4_MIN_BAR,
		CAM_SPM4_MAX_BAR,
		CAM_SPM4_BAR_DATA_BUFA,
		CAM_SPM4_BAR_DATA_BUFB,
		CAM_FPE_HSK = 0x700,					// 0x600  - 0x67F  Base of 127 registers
		CAM_FPE_MEM_DUMP = 0x800				// 0x800  - 0xFFF  Base of 2048 memory dump registers
} DHURegId;

extern DHURegisterType DHURegisters[];
//
// Map the three BRAMs to their related GSE function
#define idPCI  (XPAR_AXI_BRAM_CTRL_0_DEVICE_ID)		// dual port memory stand in for PCI
#define idCam1 (XPAR_AXI_BRAM_CTRL_1_DEVICE_ID) 	// data to/from FPE 1
#define idCam2 (XPAR_AXI_BRAM_CTRL_2_DEVICE_ID) 	// data to/from FPE 2

extern XBram PCIeBram;				/* The Instance of the BRAM Driver */
extern XBram cam1Bram;				/* The Instance of the BRAM Driver */
extern XBram cam2Bram;				/* The Instance of the BRAM Driver */

// Called from main loop/non-interrupt code
u32	DHURegWrite(u32 regOffset,u32 value);
u32	DHURegRead(u32 regOffset);
// Called from Interrupt service routines....
u32	IntDHURegWrite(u32 regOffset,u32 value);
u32	IntDHURegRead(u32 regOffset);
u32 DHURegLookup(char *regName);

extern XScuGic scuGic;		/* pl/pl fabric generic interrupt controller */
#define DHU_CAM1INT_ID	XPAR_FABRIC_SYSTEM_DHU_CAM1_INT_INTR
#define ENTER_CRITICAL	{XScuGic_Enable(&scuGic, DHU_CAM1INT_ID);}
#define EXIT_CRITICAL	{XScuGic_Enable(&scuGic, DHU_CAM1INT_ID);}
#endif /* DHU_H_ */
