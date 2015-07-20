//
//  Structs and definitions for interpretting TESS camera HK values
//

#ifndef __TESS_HK
#define __TESS_HK

#include <stdint.h>

#define NUM_CAMERA_HK_VALS 10

typedef struct {
  int index;
  int group;
  int  WriteToHeader;
  char *name;
  char func;
  double P[4];
  double Limits[4];
  char *format;
} CAMERA_HK_DEF;

int readHKdefs(char *fname, CAMERA_HK_DEF *HK);
double convertHK(uint16_t value, CAMERA_HK_DEF *HK, int index, int group);
char *HKtoString(uint16_t value, CAMERA_HK_DEF *HK, int index, int group);
#endif    
