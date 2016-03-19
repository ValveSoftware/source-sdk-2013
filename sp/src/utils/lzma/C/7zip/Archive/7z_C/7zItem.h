/* 7zItem.h */

#ifndef __7Z_ITEM_H
#define __7Z_ITEM_H

#include "7zMethodID.h"
#include "7zHeader.h"
#include "7zBuffer.h"

typedef struct _CCoderInfo
{
  UInt32 NumInStreams;
  UInt32 NumOutStreams;
  CMethodID MethodID;
  CSzByteBuffer Properties;
}CCoderInfo;

void SzCoderInfoInit(CCoderInfo *coder);
void SzCoderInfoFree(CCoderInfo *coder, void (*freeFunc)(void *p));

typedef struct _CBindPair
{
  UInt32 InIndex;
  UInt32 OutIndex;
}CBindPair;

typedef struct _CFolder
{
  UInt32 NumCoders;
  CCoderInfo *Coders;
  UInt32 NumBindPairs;
  CBindPair *BindPairs;
  UInt32 NumPackStreams; 
  UInt32 *PackStreams;
  CFileSize *UnPackSizes;
  int UnPackCRCDefined;
  UInt32 UnPackCRC;

  UInt32 NumUnPackStreams;
}CFolder;

void SzFolderInit(CFolder *folder);
CFileSize SzFolderGetUnPackSize(CFolder *folder);
int SzFolderFindBindPairForInStream(CFolder *folder, UInt32 inStreamIndex);
UInt32 SzFolderGetNumOutStreams(CFolder *folder);
CFileSize SzFolderGetUnPackSize(CFolder *folder);

/* #define CArchiveFileTime UInt64 */

typedef struct _CFileItem
{
  /*
  CArchiveFileTime LastWriteTime;
  CFileSize StartPos;
  UInt32 Attributes; 
  */
  CFileSize Size;
  UInt32 FileCRC;
  char *Name;

  Byte IsFileCRCDefined;
  Byte HasStream;
  Byte IsDirectory;
  Byte IsAnti;
  /*
  int AreAttributesDefined;
  int IsLastWriteTimeDefined;
  int IsStartPosDefined;
  */
}CFileItem;

void SzFileInit(CFileItem *fileItem);

typedef struct _CArchiveDatabase
{
  UInt32 NumPackStreams;
  CFileSize *PackSizes;
  Byte *PackCRCsDefined;
  UInt32 *PackCRCs;
  UInt32 NumFolders;
  CFolder *Folders;
  UInt32 NumFiles;
  CFileItem *Files;
}CArchiveDatabase;

void SzArchiveDatabaseInit(CArchiveDatabase *db);
void SzArchiveDatabaseFree(CArchiveDatabase *db, void (*freeFunc)(void *));


#endif
