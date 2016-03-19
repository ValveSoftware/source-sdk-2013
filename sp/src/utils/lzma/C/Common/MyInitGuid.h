// Common/MyInitGuid.h

#ifndef __COMMON_MYINITGUID_H
#define __COMMON_MYINITGUID_H

#ifdef _WIN32
#include <initguid.h>
#else
#define INITGUID
#include "MyGuidDef.h"
#endif

#endif
