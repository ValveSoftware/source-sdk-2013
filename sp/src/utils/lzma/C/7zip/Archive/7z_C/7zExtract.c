/* 7zExtract.c */

#include "7zExtract.h"
#include "7zDecode.h"
#include "7zCrc.h"

SZ_RESULT SzExtract(
    ISzInStream *inStream, 
    CArchiveDatabaseEx *db,
    UInt32 fileIndex,
    UInt32 *blockIndex,
    Byte **outBuffer, 
    size_t *outBufferSize,
    size_t *offset, 
    size_t *outSizeProcessed, 
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp)
{
  UInt32 folderIndex = db->FileIndexToFolderIndexMap[fileIndex];
  SZ_RESULT res = SZ_OK;
  *offset = 0;
  *outSizeProcessed = 0;
  if (folderIndex == (UInt32)-1)
  {
    allocMain->Free(*outBuffer);
    *blockIndex = folderIndex;
    *outBuffer = 0;
    *outBufferSize = 0;
    return SZ_OK;
  }

  if (*outBuffer == 0 || *blockIndex != folderIndex)
  {
    CFolder *folder = db->Database.Folders + folderIndex;
    CFileSize unPackSize = SzFolderGetUnPackSize(folder);
    #ifndef _LZMA_IN_CB
    CFileSize packSize = SzArDbGetFolderFullPackSize(db, folderIndex);
    Byte *inBuffer = 0;
    size_t processedSize;
    #endif
    *blockIndex = folderIndex;
    allocMain->Free(*outBuffer);
    *outBuffer = 0;
    
    RINOK(inStream->Seek(inStream, SzArDbGetFolderStreamPos(db, folderIndex, 0)));
    
    #ifndef _LZMA_IN_CB
    if (packSize != 0)
    {
      inBuffer = (Byte *)allocTemp->Alloc((size_t)packSize);
      if (inBuffer == 0)
        return SZE_OUTOFMEMORY;
    }
    res = inStream->Read(inStream, inBuffer, (size_t)packSize, &processedSize);
    if (res == SZ_OK && processedSize != (size_t)packSize)
      res = SZE_FAIL;
    #endif
    if (res == SZ_OK)
    {
      *outBufferSize = (size_t)unPackSize;
      if (unPackSize != 0)
      {
        *outBuffer = (Byte *)allocMain->Alloc((size_t)unPackSize);
        if (*outBuffer == 0)
          res = SZE_OUTOFMEMORY;
      }
      if (res == SZ_OK)
      {
        size_t outRealSize;
        res = SzDecode(db->Database.PackSizes + 
          db->FolderStartPackStreamIndex[folderIndex], folder, 
          #ifdef _LZMA_IN_CB
          inStream,
          #else
          inBuffer, 
          #endif
          *outBuffer, (size_t)unPackSize, &outRealSize, allocTemp);
        if (res == SZ_OK)
        {
          if (outRealSize == (size_t)unPackSize)
          {
            if (folder->UnPackCRCDefined)
            {
              if (!CrcVerifyDigest(folder->UnPackCRC, *outBuffer, (size_t)unPackSize))
                res = SZE_FAIL;
            }
          }
          else
            res = SZE_FAIL;
        }
      }
    }
    #ifndef _LZMA_IN_CB
    allocTemp->Free(inBuffer);
    #endif
  }
  if (res == SZ_OK)
  {
    UInt32 i; 
    CFileItem *fileItem = db->Database.Files + fileIndex;
    *offset = 0;
    for(i = db->FolderStartFileIndex[folderIndex]; i < fileIndex; i++)
      *offset += (UInt32)db->Database.Files[i].Size;
    *outSizeProcessed = (size_t)fileItem->Size;
    if (*offset + *outSizeProcessed > *outBufferSize)
      return SZE_FAIL;
    {
      if (fileItem->IsFileCRCDefined)
      {
        if (!CrcVerifyDigest(fileItem->FileCRC, *outBuffer + *offset, *outSizeProcessed))
          res = SZE_FAIL;
      }
    }
  }
  return res;
}
