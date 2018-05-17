#ifndef agc_h__
#define agc_h__
//#define AGC_DEBUG
#ifdef AGC_DEBUG
#include "stdio.h"
#endif
#define HEADER_SIZE 5
#define TRAILER_SIZE 8
#define VAD_WIN_SIZE 10
#define TOTAL_FRAME 10

#define SPEECH2SLI_OTHER 8
#define SLI2SPEECH_OTHER 4

typedef struct DigitalAGCVar_t DigitalAGCVar;

#define Subframe_Size 8
#define NSubframe 10

int DigitalAGC_QueryNeedSize(int flag);
// 6, 6, 0, -33, -33
int DigitalAGC_Init( DigitalAGCVar *agc, int TargetLevel,int MaxGain,int MinGain,int AgcNoiseThr,int AgcSpeechThr);
int DigitalAGC_ProcessBlock(DigitalAGCVar *agc, const short *in, short *out);
#endif // agc_h__
