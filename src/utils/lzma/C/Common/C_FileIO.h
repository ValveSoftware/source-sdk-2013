// Common/C_FileIO.h

#ifndef __COMMON_C_FILEIO_H
#define __COMMON_C_FILEIO_H

#include <stdio.h>
#include <sys/types.h>

#include "Types.h"
#include "MyWindows.h"

namespace NC {
namespace NFile {
namespace NIO {

class CFileBase
{
protected:
  int _handle;
  bool OpenBinary(const char *name, int flags);
public:
  CFileBase(): _handle(-1) {};
  ~CFileBase() { Close(); }
  bool Close();
  bool GetLength(UInt64 &length) const;
  off_t Seek(off_t distanceToMove, int moveMethod) const;
};

class CInFile: public CFileBase
{
public:
  bool Open(const char *name);
  ssize_t Read(void *data, size_t size);
};

class COutFile: public CFileBase
{
public:
  bool Create(const char *name, bool createAlways);
  ssize_t Write(const void *data, size_t size);
};

}}}

#endif
