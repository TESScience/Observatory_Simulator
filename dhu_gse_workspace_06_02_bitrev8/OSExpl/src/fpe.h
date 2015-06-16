/*
 * fpe.h
 *
 *  Created on: Apr 5, 2015
 *      Author: jal
 */

#ifndef FPE_H_
#define FPE_H_

typedef enum {
		FPE_PMEM = 0x00,
		FPE_SMEM = 0x01,
		FPE_HMEM = 0x02,
		FPE_VMEM = 0x03,		// 011 -
		FPE_REGISTERS = 0x04,   // 100 -  load
		FPE_RESET = 0x05,		// 101 -
		FPE_BITFILE = 0x07		// 111 -
} FpeMemType;


// Define a bitmap for the fpe write control register
typedef union {
	struct {
		u32 xferType :1 ;		// Bit 0 - 0 = next pps, 1 = immediate
		u32 regAddr  :4 ;		// Bit 1 to 4 - FPE register address (ignored for memory operations
		u32 length	 :12;		// Bit 5 to 16 - number of elements to transfer
		FpeMemType  memType:3;			// Bit 17 to 19
							// 000 - PMEM load (program )
							// 001 - SMEM load (sequencer )
							// 010 - HMEM load (houskeeping)
							// 011 - VMEM load (clock voltage )
							// 100 - registers load
							// 101 - Reset FPE
							// 111 - FPGA bit file load
		u32 : 0;
	} bitFields;
	u32 word;
} FPEWriteCtrl_bits;

extern u16		cam1FrameData[0x4000000];	// 64MB buffer for bitfiles and cam1Frames
extern u16		cam2FrameData[0x4000000];	// 64MB buffer for bitfiles and cam2Frames

//karih modification -- add ability to restart
void FpeInitDefaults(u32 restart);
//end modification
void xferFpeBuff(u32 length, u32 *where, FpeMemType type);
u32 FPEWrite(u32 bramOffset, u32 length, FpeMemType type, u32 Immediate);
void FpeReadHsk(char *value);
void FpeReadMem(char *value);
void FpeLoadHsk(void);
void FpeLoadClv(void);
void FpeLoadPrg(void);
void FpeLoadSeq(void);
void FpeReset(void);
void FpeLoadReg(void);
void FpeLoadBit(void);
void FpeLoadHK(void);
void FpeLoadProg(void);
void FpeReloadMems(void);

#endif /* FPE_H_ */
