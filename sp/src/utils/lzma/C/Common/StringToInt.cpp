// Common/StringToInt.cpp

#include "StdAfx.h"

#include "StringToInt.h"

UInt64 ConvertStringToUInt64(const char *s, const char **end)
{
  UInt64 result = 0;
  while(true)
  {
    char c = *s;
    if (c < '0' || c > '9')
    {
      if (end != NULL)
        *end = s;
      return result;
    }
    result *= 10;
    result += (c - '0');
    s++;
  }
}

UInt64 ConvertOctStringToUInt64(const char *s, const char **end)
{
  UInt64 result = 0;
  while(true)
  {
    char c = *s;
    if (c < '0' || c > '7')
    {
      if (end != NULL)
        *end = s;
      return result;
    }
    result <<= 3;
    result += (c - '0');
    s++;
  }
}


UInt64 ConvertStringToUInt64(const wchar_t *s, const wchar_t **end)
{
  UInt64 result = 0;
  while(true)
  {
    wchar_t c = *s;
    if (c < '0' || c > '9')
    {
      if (end != NULL)
        *end = s;
      return result;
    }
    result *= 10;
    result += (c - '0');
    s++;
  }
}


Int64 ConvertStringToInt64(const char *s, const char **end)
{
  if (*s == '-')
    return -(Int64)ConvertStringToUInt64(s + 1, end);
  return ConvertStringToUInt64(s, end);
}
