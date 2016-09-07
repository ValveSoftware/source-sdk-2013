//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// f0.h
//
//******************************************************************************

#ifndef _F0_H_
#define _F0_H_
       
#ifdef __cplusplus
extern "C" {
#endif

typedef struct F0_Data* F0Data;

typedef struct {
  float f0;
  float rms;
  /*  float voicing;
   */
} F0Out;


F0Data F0_NewF0Obj (void);

void   F0_DeleteF0Obj (F0Data* f0Obj);

int    F0_ParseParameter (F0Data f0Obj, const char* str);

float  F0_ParameterValue (F0Data f0Obj, const char* str);

int    F0_Init (F0Data f0Obj, double sampFreq, int* buffsize, int* buffStep);

int    F0_AddDataFrame (F0Data f0Obj, float* data, int nData);

int    F0_OutputLength (F0Data f0Obj);

F0Out  F0_GetOutputFrame (F0Data f0Obj, int idx);

#ifdef __cplusplus
}
#endif

#endif
