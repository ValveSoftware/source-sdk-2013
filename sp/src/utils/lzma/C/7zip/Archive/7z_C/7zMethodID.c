/* 7zMethodID.c */

#include "7zMethodID.h"

int AreMethodsEqual(CMethodID *a1, CMethodID *a2)
{
  int i;
  if (a1->IDSize != a2->IDSize)
    return 0;
  for (i = 0; i < a1->IDSize; i++)
    if (a1->ID[i] != a2->ID[i])
      return 0;
  return 1;
}
