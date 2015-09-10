/* 7z.h -- 7z interface
2013-01-18 : Igor Pavlov : Public domain */

#ifndef __7Z_H
#define __7Z_H

#include "7zTypes.h"

EXTERN_C_BEGIN

#define k7zStartHeaderSize 0x20
#define k7zSignatureSize 6

extern Byte k7zSignature[k7zSignatureSize];

typedef struct
{
  const Byte *Data;
  size_t Size;
} CSzData;

/* CSzCoderInfo & CSzFolder support only default methods */

typedef struct
{
  size_t PropsOffset;
  UInt32 MethodID;
  Byte NumInStreams;
  Byte NumOutStreams;
  Byte PropsSize;
} CSzCoderInfo;

typedef struct
{
  UInt32 InIndex;
  UInt32 OutIndex;
} CSzBindPair;

#define SZ_NUM_CODERS_IN_FOLDER_MAX 4
#define SZ_NUM_BINDS_IN_FOLDER_MAX 3
#define SZ_NUM_PACK_STREAMS_IN_FOLDER_MAX 4
#define SZ_NUM_CODERS_OUT_STREAMS_IN_FOLDER_MAX 4

typedef struct
{
  UInt32 NumCoders;
  UInt32 NumBindPairs;
  UInt32 NumPackStreams;
  UInt32 MainOutStream;
  UInt32 PackStreams[SZ_NUM_PACK_STREAMS_IN_FOLDER_MAX];
  CSzBindPair BindPairs[SZ_NUM_BINDS_IN_FOLDER_MAX];
  CSzCoderInfo Coders[SZ_NUM_CODERS_IN_FOLDER_MAX];
  UInt64 CodersUnpackSizes[SZ_NUM_CODERS_OUT_STREAMS_IN_FOLDER_MAX];
} CSzFolder;

/*
typedef struct
{
  size_t CodersDataOffset;
  size_t UnpackSizeDataOffset;
  // UInt32 StartCoderUnpackSizesIndex;
  UInt32 StartPackStreamIndex;
  // UInt32 IndexOfMainOutStream;
} CSzFolder2;
*/

SRes SzGetNextFolderItem(CSzFolder *f, CSzData *sd, CSzData *sdSizes);

typedef struct
{
  UInt32 Low;
  UInt32 High;
} CNtfsFileTime;

typedef struct
{
  Byte *Defs; /* MSB 0 bit numbering */
  UInt32 *Vals;
} CSzBitUi32s;

typedef struct
{
  Byte *Defs; /* MSB 0 bit numbering */
  // UInt64 *Vals;
  CNtfsFileTime *Vals;
} CSzBitUi64s;

#define SzBitArray_Check(p, i) (((p)[(i) >> 3] & (0x80 >> ((i) & 7))) != 0)

#define SzBitWithVals_Check(p, i) ((p)->Defs && ((p)->Defs[(i) >> 3] & (0x80 >> ((i) & 7))) != 0)

typedef struct
{
  UInt32 NumPackStreams;
  UInt32 NumFolders;

  UInt64 *PackPositions; // NumPackStreams + 1
  CSzBitUi32s FolderCRCs;

  size_t *FoCodersOffsets;
  size_t *FoSizesOffsets;
  // UInt32 StartCoderUnpackSizesIndex;
  UInt32 *FoStartPackStreamIndex;

  // CSzFolder2 *Folders;  // +1 item for sum values
  Byte *CodersData;
  Byte *UnpackSizesData;
  size_t UnpackSizesDataSize;
  // UInt64 *CoderUnpackSizes;
} CSzAr;


SRes SzAr_DecodeFolder(const CSzAr *p, UInt32 folderIndex,
    ILookInStream *stream, UInt64 startPos,
    Byte *outBuffer, size_t outSize,
    ISzAlloc *allocMain);

/*
  SzExtract extracts file from archive

  *outBuffer must be 0 before first call for each new archive.

  Extracting cache:
    If you need to decompress more than one file, you can send
    these values from previous call:
      *blockIndex,
      *outBuffer,
      *outBufferSize
    You can consider "*outBuffer" as cache of solid block. If your archive is solid,
    it will increase decompression speed.
  
    If you use external function, you can declare these 3 cache variables
    (blockIndex, outBuffer, outBufferSize) as static in that external function.
    
    Free *outBuffer and set *outBuffer to 0, if you want to flush cache.
*/

typedef struct
{
  CSzAr db;

  UInt64 startPosAfterHeader;
  UInt64 dataPos;
  
  UInt32 NumFiles;

  UInt64 *UnpackPositions;
  // Byte *IsEmptyFiles;
  Byte *IsDirs;
  CSzBitUi32s CRCs;

  CSzBitUi32s Attribs;
  // CSzBitUi32s Parents;
  CSzBitUi64s MTime;
  CSzBitUi64s CTime;

  // UInt32 *FolderStartPackStreamIndex;
  UInt32 *FolderStartFileIndex; // + 1
  UInt32 *FileIndexToFolderIndexMap;

  size_t *FileNameOffsets; /* in 2-byte steps */
  Byte *FileNames;  /* UTF-16-LE */
} CSzArEx;

#define SzArEx_IsDir(p, i) (SzBitArray_Check((p)->IsDirs, i))

#define SzArEx_GetFileSize(p, i) ((p)->UnpackPositions[(i) + 1] - (p)->UnpackPositions[i])

void SzArEx_Init(CSzArEx *p);
void SzArEx_Free(CSzArEx *p, ISzAlloc *alloc);
UInt64 SzArEx_GetFolderStreamPos(const CSzArEx *p, UInt32 folderIndex, UInt32 indexInFolder);
int SzArEx_GetFolderFullPackSize(const CSzArEx *p, UInt32 folderIndex, UInt64 *resSize);

/*
if dest == NULL, the return value specifies the required size of the buffer,
  in 16-bit characters, including the null-terminating character.
if dest != NULL, the return value specifies the number of 16-bit characters that
  are written to the dest, including the null-terminating character. */

size_t SzArEx_GetFileNameUtf16(const CSzArEx *p, size_t fileIndex, UInt16 *dest);

/*
size_t SzArEx_GetFullNameLen(const CSzArEx *p, size_t fileIndex);
UInt16 *SzArEx_GetFullNameUtf16_Back(const CSzArEx *p, size_t fileIndex, UInt16 *dest);
*/

SRes SzArEx_Extract(
    const CSzArEx *db,
    ILookInStream *inStream,
    UInt32 fileIndex,         /* index of file */
    UInt32 *blockIndex,       /* index of solid block */
    Byte **outBuffer,         /* pointer to pointer to output buffer (allocated with allocMain) */
    size_t *outBufferSize,    /* buffer size for output buffer */
    size_t *offset,           /* offset of stream for required file in *outBuffer */
    size_t *outSizeProcessed, /* size of file in *outBuffer */
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp);


/*
SzArEx_Open Errors:
SZ_ERROR_NO_ARCHIVE
SZ_ERROR_ARCHIVE
SZ_ERROR_UNSUPPORTED
SZ_ERROR_MEM
SZ_ERROR_CRC
SZ_ERROR_INPUT_EOF
SZ_ERROR_FAIL
*/

SRes SzArEx_Open(CSzArEx *p, ILookInStream *inStream,
    ISzAlloc *allocMain, ISzAlloc *allocTemp);

EXTERN_C_END

#endif
