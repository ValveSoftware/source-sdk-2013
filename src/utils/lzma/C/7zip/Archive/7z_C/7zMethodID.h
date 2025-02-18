/* 7zMethodID.h */

#ifndef __7Z_METHOD_ID_H
#define __7Z_METHOD_ID_H

#include "7zTypes.h"

#define kMethodIDSize 15
  
typedef struct _CMethodID
{
  Byte ID[kMethodIDSize];
  Byte IDSize;
} CMethodID;

int AreMethodsEqual(CMethodID *a1, CMethodID *a2);

#endif
