/* 7zArcIn.c -- 7z Input functions
2014-06-16 : Igor Pavlov : Public domain */

#include "Precomp.h"

#include <string.h>

#include "7z.h"
#include "7zBuf.h"
#include "7zCrc.h"
#include "CpuArch.h"

#define MY_ALLOC(T, p, size, alloc) { if ((size) == 0) p = 0; else \
  if ((p = (T *)IAlloc_Alloc(alloc, (size) * sizeof(T))) == 0) return SZ_ERROR_MEM; }

#define k7zMajorVersion 0

enum EIdEnum
{
  k7zIdEnd,
  k7zIdHeader,
  k7zIdArchiveProperties,
  k7zIdAdditionalStreamsInfo,
  k7zIdMainStreamsInfo,
  k7zIdFilesInfo,
  k7zIdPackInfo,
  k7zIdUnpackInfo,
  k7zIdSubStreamsInfo,
  k7zIdSize,
  k7zIdCRC,
  k7zIdFolder,
  k7zIdCodersUnpackSize,
  k7zIdNumUnpackStream,
  k7zIdEmptyStream,
  k7zIdEmptyFile,
  k7zIdAnti,
  k7zIdName,
  k7zIdCTime,
  k7zIdATime,
  k7zIdMTime,
  k7zIdWinAttrib,
  k7zIdComment,
  k7zIdEncodedHeader,
  k7zIdStartPos,
  k7zIdDummy
  // k7zNtSecure,
  // k7zParent,
  // k7zIsReal
};

Byte k7zSignature[k7zSignatureSize] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C};

#define NUM_FOLDER_CODERS_MAX 32
#define NUM_CODER_STREAMS_MAX 32

/*
static int SzFolder_FindBindPairForInStream(const CSzFolder *p, UInt32 inStreamIndex)
{
  UInt32 i;
  for (i = 0; i < p->NumBindPairs; i++)
    if (p->BindPairs[i].InIndex == inStreamIndex)
      return i;
  return -1;
}
*/

#define SzBitUi32s_Init(p) { (p)->Defs = 0; (p)->Vals = 0; }

static SRes SzBitUi32s_Alloc(CSzBitUi32s *p, size_t num, ISzAlloc *alloc)
{
  MY_ALLOC(Byte, p->Defs, (num + 7) >> 3, alloc);
  MY_ALLOC(UInt32, p->Vals, num, alloc);
  return SZ_OK;
}

void SzBitUi32s_Free(CSzBitUi32s *p, ISzAlloc *alloc)
{
  IAlloc_Free(alloc, p->Defs); p->Defs = 0;
  IAlloc_Free(alloc, p->Vals); p->Vals = 0;
}

#define SzBitUi64s_Init(p) { (p)->Defs = 0; (p)->Vals = 0; }

void SzBitUi64s_Free(CSzBitUi64s *p, ISzAlloc *alloc)
{
  IAlloc_Free(alloc, p->Defs); p->Defs = 0;
  IAlloc_Free(alloc, p->Vals); p->Vals = 0;
}

static void SzAr_Init(CSzAr *p)
{
  p->NumPackStreams = 0;
  p->NumFolders = 0;
  p->PackPositions = 0;
  SzBitUi32s_Init(&p->FolderCRCs);
  // p->Folders = 0;
  p->FoCodersOffsets = 0;
  p->FoSizesOffsets = 0;
  p->FoStartPackStreamIndex = 0;

  p->CodersData = 0;
  // p->CoderUnpackSizes = 0;
  p->UnpackSizesData = 0;
}

static void SzAr_Free(CSzAr *p, ISzAlloc *alloc)
{
  IAlloc_Free(alloc, p->UnpackSizesData);
  IAlloc_Free(alloc, p->CodersData);
  // IAlloc_Free(alloc, p->CoderUnpackSizes);

  IAlloc_Free(alloc, p->PackPositions);
 
  // IAlloc_Free(alloc, p->Folders);
  IAlloc_Free(alloc, p->FoCodersOffsets);
  IAlloc_Free(alloc, p->FoSizesOffsets);
  IAlloc_Free(alloc, p->FoStartPackStreamIndex);
  
  SzBitUi32s_Free(&p->FolderCRCs, alloc);

  SzAr_Init(p);
}


void SzArEx_Init(CSzArEx *p)
{
  SzAr_Init(&p->db);
  p->NumFiles = 0;
  p->dataPos = 0;
  // p->Files = 0;
  p->UnpackPositions = 0;
  // p->IsEmptyFiles = 0;
  p->IsDirs = 0;
  // p->FolderStartPackStreamIndex = 0;
  // p->PackStreamStartPositions = 0;
  p->FolderStartFileIndex = 0;
  p->FileIndexToFolderIndexMap = 0;
  p->FileNameOffsets = 0;
  p->FileNames = 0;
  SzBitUi32s_Init(&p->CRCs);
  SzBitUi32s_Init(&p->Attribs);
  // SzBitUi32s_Init(&p->Parents);
  SzBitUi64s_Init(&p->MTime);
  SzBitUi64s_Init(&p->CTime);
}

void SzArEx_Free(CSzArEx *p, ISzAlloc *alloc)
{
  // IAlloc_Free(alloc, p->FolderStartPackStreamIndex);
  // IAlloc_Free(alloc, p->PackStreamStartPositions);
  IAlloc_Free(alloc, p->FolderStartFileIndex);
  IAlloc_Free(alloc, p->FileIndexToFolderIndexMap);

  IAlloc_Free(alloc, p->FileNameOffsets);
  IAlloc_Free(alloc, p->FileNames);

  SzBitUi64s_Free(&p->CTime, alloc);
  SzBitUi64s_Free(&p->MTime, alloc);
  SzBitUi32s_Free(&p->CRCs, alloc);
  // SzBitUi32s_Free(&p->Parents, alloc);
  SzBitUi32s_Free(&p->Attribs, alloc);
  IAlloc_Free(alloc, p->IsDirs);
  // IAlloc_Free(alloc, p->IsEmptyFiles);
  IAlloc_Free(alloc, p->UnpackPositions);
  // IAlloc_Free(alloc, p->Files);

  SzAr_Free(&p->db, alloc);
  SzArEx_Init(p);
}

static int TestSignatureCandidate(Byte *testBytes)
{
  size_t i;
  for (i = 0; i < k7zSignatureSize; i++)
    if (testBytes[i] != k7zSignature[i])
      return 0;
  return 1;
}

#define SzData_Clear(p) { (p)->Data = 0; (p)->Size = 0; }

static SRes SzReadByte(CSzData *sd, Byte *b)
{
  if (sd->Size == 0)
    return SZ_ERROR_ARCHIVE;
  sd->Size--;
  *b = *sd->Data++;
  return SZ_OK;
}

#define SZ_READ_BYTE_SD(_sd_, dest) if ((_sd_)->Size == 0) return SZ_ERROR_ARCHIVE; (_sd_)->Size--; dest = *(_sd_)->Data++;
#define SZ_READ_BYTE(dest) SZ_READ_BYTE_SD(sd, dest)
#define SZ_READ_BYTE_2(dest) if (sd.Size == 0) return SZ_ERROR_ARCHIVE; sd.Size--; dest = *sd.Data++;

#define SKIP_DATA(sd, size) { sd->Size -= (size_t)(size); sd->Data += (size_t)(size); }
#define SKIP_DATA2(sd, size) { sd.Size -= (size_t)(size); sd.Data += (size_t)(size); }

#define SZ_READ_32(dest) if (sd.Size < 4) return SZ_ERROR_ARCHIVE; \
   dest = GetUi32(sd.Data); SKIP_DATA2(sd, 4);

static MY_NO_INLINE SRes ReadNumber(CSzData *sd, UInt64 *value)
{
  Byte firstByte, mask;
  unsigned i;
  UInt32 v;

  SZ_READ_BYTE(firstByte);
  if ((firstByte & 0x80) == 0)
  {
    *value = firstByte;
    return SZ_OK;
  }
  SZ_READ_BYTE(v);
  if ((firstByte & 0x40) == 0)
  {
    *value = (((UInt32)firstByte & 0x3F) << 8) | v;
    return SZ_OK;
  }
  SZ_READ_BYTE(mask);
  *value = v | ((UInt32)mask << 8);
  mask = 0x20;
  for (i = 2; i < 8; i++)
  {
    Byte b;
    if ((firstByte & mask) == 0)
    {
      UInt64 highPart = firstByte & (mask - 1);
      *value |= (highPart << (8 * i));
      return SZ_OK;
    }
    SZ_READ_BYTE(b);
    *value |= ((UInt64)b << (8 * i));
    mask >>= 1;
  }
  return SZ_OK;
}

/*
static MY_NO_INLINE const Byte *SzReadNumbers(const Byte *data, const Byte *dataLim, UInt64 *values, UInt32 num)
{
  for (; num != 0; num--)
  {
    Byte firstByte;
    Byte mask;

    unsigned i;
    UInt32 v;
    UInt64 value;
   
    if (data == dataLim)
      return NULL;
    firstByte = *data++;

    if ((firstByte & 0x80) == 0)
    {
      *values++ = firstByte;
      continue;
    }
    if (data == dataLim)
      return NULL;
    v = *data++;
    if ((firstByte & 0x40) == 0)
    {
      *values++ = (((UInt32)firstByte & 0x3F) << 8) | v;
      continue;
    }
    if (data == dataLim)
      return NULL;
    value = v | ((UInt32)*data++ << 8);
    mask = 0x20;
    for (i = 2; i < 8; i++)
    {
      if ((firstByte & mask) == 0)
      {
        UInt64 highPart = firstByte & (mask - 1);
        value |= (highPart << (8 * i));
        break;
      }
      if (data == dataLim)
        return NULL;
      value |= ((UInt64)*data++ << (8 * i));
      mask >>= 1;
    }
    *values++ = value;
  }
  return data;
}
*/

static MY_NO_INLINE SRes SzReadNumber32(CSzData *sd, UInt32 *value)
{
  Byte firstByte;
  UInt64 value64;
  if (sd->Size == 0)
    return SZ_ERROR_ARCHIVE;
  firstByte = *sd->Data;
  if ((firstByte & 0x80) == 0)
  {
    *value = firstByte;
    sd->Data++;
    sd->Size--;
    return SZ_OK;
  }
  RINOK(ReadNumber(sd, &value64));
  if (value64 >= (UInt32)0x80000000 - 1)
    return SZ_ERROR_UNSUPPORTED;
  if (value64 >= ((UInt64)(1) << ((sizeof(size_t) - 1) * 8 + 4)))
    return SZ_ERROR_UNSUPPORTED;
  *value = (UInt32)value64;
  return SZ_OK;
}

#define ReadID(sd, value) ReadNumber(sd, value)

static SRes SkipData(CSzData *sd)
{
  UInt64 size;
  RINOK(ReadNumber(sd, &size));
  if (size > sd->Size)
    return SZ_ERROR_ARCHIVE;
  SKIP_DATA(sd, size);
  return SZ_OK;
}

static SRes WaitId(CSzData *sd, UInt64 id)
{
  for (;;)
  {
    UInt64 type;
    RINOK(ReadID(sd, &type));
    if (type == id)
      return SZ_OK;
    if (type == k7zIdEnd)
      return SZ_ERROR_ARCHIVE;
    RINOK(SkipData(sd));
  }
}

static SRes RememberBitVector(CSzData *sd, UInt32 numItems, const Byte **v)
{
  UInt32 numBytes = (numItems + 7) >> 3;
  if (numBytes > sd->Size)
    return SZ_ERROR_ARCHIVE;
  *v = sd->Data;
  SKIP_DATA(sd, numBytes);
  return SZ_OK;
}

static UInt32 CountDefinedBits(const Byte *bits, UInt32 numItems)
{
  Byte b = 0;
  unsigned m = 0;
  UInt32 sum = 0;
  for (; numItems != 0; numItems--)
  {
    if (m == 0)
    {
      b = *bits++;
      m = 8;
    }
    m--;
    sum += ((b >> m) & 1);
  }
  return sum ;
}

static MY_NO_INLINE SRes ReadBitVector(CSzData *sd, UInt32 numItems, Byte **v, ISzAlloc *alloc)
{
  Byte allAreDefined;
  UInt32 i;
  Byte *v2;
  UInt32 numBytes = (numItems + 7) >> 3;
  RINOK(SzReadByte(sd, &allAreDefined));
  if (allAreDefined == 0)
  {
    if (numBytes > sd->Size)
      return SZ_ERROR_ARCHIVE;
    MY_ALLOC(Byte, *v, numBytes, alloc);
    memcpy(*v, sd->Data, numBytes);
    SKIP_DATA(sd, numBytes);
    return SZ_OK;
  }
  MY_ALLOC(Byte, *v, numBytes, alloc);
  v2 = *v;
  for (i = 0; i < numBytes; i++)
    v2[i] = 0xFF;
  {
    unsigned numBits = (unsigned)numItems & 7;
    if (numBits != 0)
      v2[numBytes - 1] = (Byte)((((UInt32)1 << numBits) - 1) << (8 - numBits));
  }
  return SZ_OK;
}

static MY_NO_INLINE SRes ReadUi32s(CSzData *sd2, UInt32 numItems, CSzBitUi32s *crcs, ISzAlloc *alloc)
{
  UInt32 i;
  CSzData sd;
  UInt32 *vals;
  const Byte *defs;
  MY_ALLOC(UInt32, crcs->Vals, numItems, alloc);
  sd = *sd2;
  defs = crcs->Defs;
  vals = crcs->Vals;
  for (i = 0; i < numItems; i++)
    if (SzBitArray_Check(defs, i))
    {
      SZ_READ_32(vals[i]);
    }
    else
      vals[i] = 0;
  *sd2 = sd;
  return SZ_OK;
}

static SRes ReadBitUi32s(CSzData *sd, UInt32 numItems, CSzBitUi32s *crcs, ISzAlloc *alloc)
{
  SzBitUi32s_Free(crcs, alloc);
  RINOK(ReadBitVector(sd, numItems, &crcs->Defs, alloc));
  return ReadUi32s(sd, numItems, crcs, alloc);
}

static SRes SkipBitUi32s(CSzData *sd, UInt32 numItems)
{
  Byte allAreDefined;
  UInt32 numDefined = numItems;
  RINOK(SzReadByte(sd, &allAreDefined));
  if (!allAreDefined)
  {
    size_t numBytes = (numItems + 7) >> 3;
    if (numBytes > sd->Size)
      return SZ_ERROR_ARCHIVE;
    numDefined = CountDefinedBits(sd->Data, numItems);
    SKIP_DATA(sd, numBytes);
  }
  if (numDefined > (sd->Size >> 2))
    return SZ_ERROR_ARCHIVE;
  SKIP_DATA(sd, (size_t)numDefined * 4);
  return SZ_OK;
}

static SRes ReadPackInfo(CSzAr *p, CSzData *sd, ISzAlloc *alloc)
{
  RINOK(SzReadNumber32(sd, &p->NumPackStreams));

  RINOK(WaitId(sd, k7zIdSize));
  MY_ALLOC(UInt64, p->PackPositions, (size_t)p->NumPackStreams + 1, alloc);
  {
    UInt64 sum = 0;
    UInt32 i;
    UInt32 numPackStreams = p->NumPackStreams;
    for (i = 0; i < numPackStreams; i++)
    {
      UInt64 packSize;
      p->PackPositions[i] = sum;
      RINOK(ReadNumber(sd, &packSize));
      sum += packSize;
      if (sum < packSize)
        return SZ_ERROR_ARCHIVE;
    }
    p->PackPositions[i] = sum;
  }

  for (;;)
  {
    UInt64 type;
    RINOK(ReadID(sd, &type));
    if (type == k7zIdEnd)
      return SZ_OK;
    if (type == k7zIdCRC)
    {
      /* CRC of packed streams is unused now */
      RINOK(SkipBitUi32s(sd, p->NumPackStreams));
      continue;
    }
    RINOK(SkipData(sd));
  }
}

/*
static SRes SzReadSwitch(CSzData *sd)
{
  Byte external;
  RINOK(SzReadByte(sd, &external));
  return (external == 0) ? SZ_OK: SZ_ERROR_UNSUPPORTED;
}
*/

#define SZ_NUM_IN_STREAMS_IN_FOLDER_MAX 16

SRes SzGetNextFolderItem(CSzFolder *f, CSzData *sd, CSzData *sdSizes)
{
  UInt32 numCoders, numBindPairs, numPackStreams, i;
  UInt32 numInStreams = 0, numOutStreams = 0;
  const Byte *dataStart = sd->Data;
  Byte inStreamUsed[SZ_NUM_IN_STREAMS_IN_FOLDER_MAX];
  
  RINOK(SzReadNumber32(sd, &numCoders));
  if (numCoders > SZ_NUM_CODERS_IN_FOLDER_MAX)
    return SZ_ERROR_UNSUPPORTED;
  f->NumCoders = numCoders;
  
  for (i = 0; i < numCoders; i++)
  {
    Byte mainByte;
    CSzCoderInfo *coder = f->Coders + i;
    unsigned idSize, j;
    UInt64 id;
    RINOK(SzReadByte(sd, &mainByte));
    if ((mainByte & 0xC0) != 0)
      return SZ_ERROR_UNSUPPORTED;
    idSize = (unsigned)(mainByte & 0xF);
    if (idSize > sizeof(id))
      return SZ_ERROR_UNSUPPORTED;
    if (idSize > sd->Size)
      return SZ_ERROR_ARCHIVE;
    id = 0;
    for (j = 0; j < idSize; j++)
    {
      id = ((id << 8) | *sd->Data);
      sd->Data++;
      sd->Size--;
    }
    if (id > (UInt32)0xFFFFFFFF)
      return SZ_ERROR_UNSUPPORTED;
    coder->MethodID = (UInt32)id;
    
    coder->NumInStreams = 1;
    coder->NumOutStreams = 1;
    coder->PropsOffset = 0;
    coder->PropsSize = 0;
    
    if ((mainByte & 0x10) != 0)
    {
      UInt32 numStreams;
      RINOK(SzReadNumber32(sd, &numStreams));
      if (numStreams > NUM_CODER_STREAMS_MAX)
        return SZ_ERROR_UNSUPPORTED;
      coder->NumInStreams = (Byte)numStreams;
      RINOK(SzReadNumber32(sd, &numStreams));
      if (numStreams > NUM_CODER_STREAMS_MAX)
        return SZ_ERROR_UNSUPPORTED;
      coder->NumOutStreams = (Byte)numStreams;
    }
    if ((mainByte & 0x20) != 0)
    {
      UInt32 propsSize = 0;
      RINOK(SzReadNumber32(sd, &propsSize));
      if (propsSize >= 0x40)
        return SZ_ERROR_UNSUPPORTED;
      if (propsSize > sd->Size)
        return SZ_ERROR_ARCHIVE;
      coder->PropsOffset = sd->Data - dataStart;
      coder->PropsSize = (Byte)propsSize;
      sd->Data += (size_t)propsSize;
      sd->Size -= (size_t)propsSize;
    }
    numInStreams += coder->NumInStreams;
    numOutStreams += coder->NumOutStreams;
  }

  if (numOutStreams == 0)
    return SZ_ERROR_UNSUPPORTED;

  f->NumBindPairs = numBindPairs = numOutStreams - 1;
  if (numInStreams < numBindPairs)
    return SZ_ERROR_ARCHIVE;
  if (numInStreams > SZ_NUM_IN_STREAMS_IN_FOLDER_MAX)
    return SZ_ERROR_UNSUPPORTED;
  f->MainOutStream = 0;
  f->NumPackStreams = numPackStreams = numInStreams - numBindPairs;
  if (numPackStreams > SZ_NUM_PACK_STREAMS_IN_FOLDER_MAX)
    return SZ_ERROR_UNSUPPORTED;
  for (i = 0; i < numInStreams; i++)
    inStreamUsed[i] = False;
  if (numBindPairs != 0)
  {
    Byte outStreamUsed[SZ_NUM_CODERS_OUT_STREAMS_IN_FOLDER_MAX];

    if (numBindPairs > SZ_NUM_BINDS_IN_FOLDER_MAX)
      return SZ_ERROR_UNSUPPORTED;

    for (i = 0; i < numOutStreams; i++)
      outStreamUsed[i] = False;

    for (i = 0; i < numBindPairs; i++)
    {
      CSzBindPair *bp = f->BindPairs + i;
      RINOK(SzReadNumber32(sd, &bp->InIndex));
      if (bp->InIndex >= numInStreams)
        return SZ_ERROR_ARCHIVE;
      inStreamUsed[bp->InIndex] = True;
      RINOK(SzReadNumber32(sd, &bp->OutIndex));
      if (bp->OutIndex >= numInStreams)
        return SZ_ERROR_ARCHIVE;
      outStreamUsed[bp->OutIndex] = True;
    }
    for (i = 0; i < numOutStreams; i++)
      if (!outStreamUsed[i])
      {
        f->MainOutStream = i;
        break;
      }
    if (i == numOutStreams)
      return SZ_ERROR_ARCHIVE;
  }

  if (numPackStreams == 1)
  {
    for (i = 0; i < numInStreams; i++)
      if (!inStreamUsed[i])
        break;
    if (i == numInStreams)
      return SZ_ERROR_ARCHIVE;
    f->PackStreams[0] = i;
  }
  else
    for (i = 0; i < numPackStreams; i++)
    {
      RINOK(SzReadNumber32(sd, f->PackStreams + i));
    }

  for (i = 0; i < numOutStreams; i++)
  {
    RINOK(ReadNumber(sdSizes, f->CodersUnpackSizes + i));
  }
  
  return SZ_OK;
}

static MY_NO_INLINE SRes SkipNumbers(CSzData *sd2, UInt32 num)
{
  CSzData sd;
  sd = *sd2;
  for (; num != 0; num--)
  {
    Byte firstByte, mask;
    unsigned i;
    SZ_READ_BYTE_2(firstByte);
    if ((firstByte & 0x80) == 0)
      continue;
    if ((firstByte & 0x40) == 0)
    {
      if (sd.Size == 0)
        return SZ_ERROR_ARCHIVE;
      sd.Size--;
      sd.Data++;
      continue;
    }
    mask = 0x20;
    for (i = 2; i < 8 && (firstByte & mask) != 0; i++)
      mask >>= 1;
    if (i > sd.Size)
      return SZ_ERROR_ARCHIVE;
    SKIP_DATA2(sd, i);
  }
  *sd2 = sd;
  return SZ_OK;
}

#define k_InStreamUsed_MAX 64
#define k_OutStreamUsed_MAX 64

static SRes ReadUnpackInfo(CSzAr *p,
    CSzData *sd2,
    UInt32 numFoldersMax, const CBuf *tempBufs, UInt32 numTempBufs,
    ISzAlloc *alloc)
{
  CSzData sd;
  Byte inStreamUsed[k_InStreamUsed_MAX];
  Byte outStreamUsed[k_OutStreamUsed_MAX];
  UInt32 fo, numFolders, numCodersOutStreams, packStreamIndex;
  const Byte *startBufPtr;
  Byte external;
  
  RINOK(WaitId(sd2, k7zIdFolder));
  RINOK(SzReadNumber32(sd2, &numFolders));
  if (p->NumFolders > numFoldersMax)
    return SZ_ERROR_UNSUPPORTED;
  p->NumFolders = numFolders;

  SZ_READ_BYTE_SD(sd2, external);
  if (external == 0)
    sd = *sd2;
  else
  {
    UInt32 index;
    SzReadNumber32(sd2, &index);
    if (index >= numTempBufs)
      return SZ_ERROR_ARCHIVE;
    sd.Data = tempBufs[index].data;
    sd.Size = tempBufs[index].size;
  }
  
  MY_ALLOC(size_t, p->FoCodersOffsets, (size_t)numFolders + 1, alloc);
  MY_ALLOC(size_t, p->FoSizesOffsets, (size_t)numFolders + 1, alloc);
  MY_ALLOC(UInt32, p->FoStartPackStreamIndex, (size_t)numFolders + 1, alloc);
  
  startBufPtr = sd.Data;
  
  packStreamIndex = 0;
  numCodersOutStreams = 0;

  for (fo = 0; fo < numFolders; fo++)
  {
    UInt32 numCoders, ci, numInStreams = 0, numOutStreams = 0;
    
    p->FoCodersOffsets[fo] = sd.Data - startBufPtr;
    RINOK(SzReadNumber32(&sd, &numCoders));
    if (numCoders > NUM_FOLDER_CODERS_MAX)
      return SZ_ERROR_UNSUPPORTED;
    
    for (ci = 0; ci < numCoders; ci++)
    {
      Byte mainByte;
      unsigned idSize;
      UInt32 coderInStreams, coderOutStreams;
      
      SZ_READ_BYTE_2(mainByte);
      if ((mainByte & 0xC0) != 0)
        return SZ_ERROR_UNSUPPORTED;
      idSize = (mainByte & 0xF);
      if (idSize > 8)
        return SZ_ERROR_UNSUPPORTED;
      if (idSize > sd.Size)
        return SZ_ERROR_ARCHIVE;
      SKIP_DATA2(sd, idSize);
      
      coderInStreams = 1;
      coderOutStreams = 1;
      if ((mainByte & 0x10) != 0)
      {
        RINOK(SzReadNumber32(&sd, &coderInStreams));
        RINOK(SzReadNumber32(&sd, &coderOutStreams));
        if (coderInStreams > NUM_CODER_STREAMS_MAX ||
            coderOutStreams > NUM_CODER_STREAMS_MAX)
          return SZ_ERROR_UNSUPPORTED;
      }
      numInStreams += coderInStreams;
      numOutStreams += coderOutStreams;
      if ((mainByte & 0x20) != 0)
      {
        UInt32 propsSize;
        RINOK(SzReadNumber32(&sd, &propsSize));
        if (propsSize > sd.Size)
          return SZ_ERROR_ARCHIVE;
        SKIP_DATA2(sd, propsSize);
      }
    }
    
    {
      UInt32 indexOfMainStream = 0;
      UInt32 numPackStreams = 1;
      if (numOutStreams != 1 || numInStreams != 1)
      {
        UInt32 i;
        UInt32 numBindPairs = numOutStreams - 1;
        if (numOutStreams == 0 || numInStreams < numBindPairs)
          return SZ_ERROR_ARCHIVE;
        
        if (numInStreams > k_InStreamUsed_MAX ||
            numOutStreams > k_OutStreamUsed_MAX)
          return SZ_ERROR_UNSUPPORTED;
        
        for (i = 0; i < numInStreams; i++)
          inStreamUsed[i] = False;
        for (i = 0; i < numOutStreams; i++)
          outStreamUsed[i] = False;
        
        for (i = 0; i < numBindPairs; i++)
        {
          UInt32 index;
          RINOK(SzReadNumber32(&sd, &index));
          if (index >= numInStreams || inStreamUsed[index])
            return SZ_ERROR_ARCHIVE;
          inStreamUsed[index] = True;
          RINOK(SzReadNumber32(&sd, &index));
          if (index >= numInStreams || outStreamUsed[index])
            return SZ_ERROR_ARCHIVE;
          outStreamUsed[index] = True;
        }
        
        numPackStreams = numInStreams - numBindPairs;
        
        if (numPackStreams != 1)
          for (i = 0; i < numPackStreams; i++)
          {
            UInt32 temp;
            RINOK(SzReadNumber32(&sd, &temp));
            if (temp >= numInStreams)
              return SZ_ERROR_ARCHIVE;
          }
          
        for (i = 0; i < numOutStreams; i++)
          if (!outStreamUsed[i])
          {
            indexOfMainStream = i;
            break;
          }
 
        if (i == numOutStreams)
          return SZ_ERROR_ARCHIVE;
      }
      p->FoStartPackStreamIndex[fo] = packStreamIndex;
      p->FoSizesOffsets[fo] = (numOutStreams << 8) | indexOfMainStream;
      numCodersOutStreams += numOutStreams;
      if (numCodersOutStreams < numOutStreams)
        return SZ_ERROR_UNSUPPORTED;
      packStreamIndex += numPackStreams;
      if (packStreamIndex < numPackStreams)
        return SZ_ERROR_UNSUPPORTED;
      if (packStreamIndex > p->NumPackStreams)
        return SZ_ERROR_ARCHIVE;
    }
  }
  
  {
    size_t dataSize = sd.Data - startBufPtr;
    p->FoStartPackStreamIndex[fo] = packStreamIndex;
    p->FoCodersOffsets[fo] = dataSize;
    MY_ALLOC(Byte, p->CodersData, dataSize, alloc);
    memcpy(p->CodersData, startBufPtr, dataSize);
  }
  
  if (external != 0)
  {
    if (sd.Size != 0)
      return SZ_ERROR_ARCHIVE;
    sd = *sd2;
  }
  
  RINOK(WaitId(&sd, k7zIdCodersUnpackSize));
  
  // MY_ALLOC(UInt64, p->CoderUnpackSizes, (size_t)numCodersOutStreams, alloc);
  {
    size_t dataSize = sd.Size;
    /*
    UInt32 i;
    for (i = 0; i < numCodersOutStreams; i++)
    {
    RINOK(ReadNumber(&sd, p->CoderUnpackSizes + i));
    }
    */
    RINOK(SkipNumbers(&sd, numCodersOutStreams));
    dataSize -= sd.Size;
    MY_ALLOC(Byte, p->UnpackSizesData, dataSize, alloc);
    memcpy(p->UnpackSizesData, sd.Data - dataSize, dataSize);
    p->UnpackSizesDataSize = dataSize;
    /*
    const Byte *data = SzReadNumbers(sd.Data, sd.Data + sd.Size, p->CoderUnpackSizes, numCodersOutStreams);
    if (data == NULL)
    return SZ_ERROR_ARCHIVE;
    sd.Size = sd.Data + sd.Size - data;
    sd.Data = data;
    */
  }

  for (;;)
  {
    UInt64 type;
    RINOK(ReadID(&sd, &type));
    if (type == k7zIdEnd)
    {
      *sd2 = sd;
      return SZ_OK;
    }
    if (type == k7zIdCRC)
    {
      RINOK(ReadBitUi32s(&sd, numFolders, &p->FolderCRCs, alloc));
      continue;
    }
    RINOK(SkipData(&sd));
  }
}

typedef struct
{
  UInt32 NumTotalSubStreams;
  UInt32 NumSubDigests;
  CSzData sdNumSubStreams;
  CSzData sdSizes;
  CSzData sdCRCs;
} CSubStreamInfo;

#define SzUi32IndexMax (((UInt32)1 << 31) - 2)

static SRes ReadSubStreamsInfo(CSzAr *p, CSzData *sd, CSubStreamInfo *ssi)
{
  UInt64 type = 0;
  UInt32 i;
  UInt32 numSubDigests = 0;
  UInt32 numFolders = p->NumFolders;
  UInt32 numUnpackStreams = numFolders;
  UInt32 numUnpackSizesInData = 0;

  for (;;)
  {
    RINOK(ReadID(sd, &type));
    if (type == k7zIdNumUnpackStream)
    {
      ssi->sdNumSubStreams.Data = sd->Data;
      numUnpackStreams = 0;
      numSubDigests = 0;
      for (i = 0; i < numFolders; i++)
      {
        UInt32 numStreams;
        RINOK(SzReadNumber32(sd, &numStreams));
        if (numUnpackStreams > numUnpackStreams + numStreams)
          return SZ_ERROR_UNSUPPORTED;
        numUnpackStreams += numStreams;
        if (numStreams != 0)
          numUnpackSizesInData += (numStreams - 1);
        if (numStreams != 1 || !SzBitWithVals_Check(&p->FolderCRCs, i))
          numSubDigests += numStreams;
      }
      ssi->sdNumSubStreams.Size = sd->Data - ssi->sdNumSubStreams.Data;
      continue;
    }
    if (type == k7zIdCRC || type == k7zIdSize || type == k7zIdEnd)
      break;
    RINOK(SkipData(sd));
  }

  if (!ssi->sdNumSubStreams.Data)
  {
    numSubDigests = numFolders;
    if (p->FolderCRCs.Defs)
      numSubDigests = numFolders - CountDefinedBits(p->FolderCRCs.Defs, numFolders);
  }
  
  ssi->NumTotalSubStreams = numUnpackStreams;
  ssi->NumSubDigests = numSubDigests;

  if (type == k7zIdSize)
  {
    ssi->sdSizes.Data = sd->Data;
    RINOK(SkipNumbers(sd, numUnpackSizesInData));
    ssi->sdSizes.Size = sd->Data - ssi->sdSizes.Data;
    RINOK(ReadID(sd, &type));
  }

  for (;;)
  {
    if (type == k7zIdEnd)
      return SZ_OK;
    if (type == k7zIdCRC)
    {
      ssi->sdCRCs.Data = sd->Data;
      RINOK(SkipBitUi32s(sd, numSubDigests));
      ssi->sdCRCs.Size = sd->Data - ssi->sdCRCs.Data;
    }
    else
    {
      RINOK(SkipData(sd));
    }
    RINOK(ReadID(sd, &type));
  }
}

static SRes SzReadStreamsInfo(CSzAr *p,
    CSzData *sd,
    UInt32 numFoldersMax, const CBuf *tempBufs, UInt32 numTempBufs,
    UInt64 *dataOffset,
    CSubStreamInfo *ssi,
    ISzAlloc *alloc)
{
  UInt64 type;

  SzData_Clear(&ssi->sdSizes);
  SzData_Clear(&ssi->sdCRCs);
  SzData_Clear(&ssi->sdNumSubStreams);

  *dataOffset = 0;
  RINOK(ReadID(sd, &type));
  if (type == k7zIdPackInfo)
  {
    RINOK(ReadNumber(sd, dataOffset));
    RINOK(ReadPackInfo(p, sd, alloc));
    RINOK(ReadID(sd, &type));
  }
  if (type == k7zIdUnpackInfo)
  {
    RINOK(ReadUnpackInfo(p, sd, numFoldersMax, tempBufs, numTempBufs, alloc));
    RINOK(ReadID(sd, &type));
  }
  if (type == k7zIdSubStreamsInfo)
  {
    RINOK(ReadSubStreamsInfo(p, sd, ssi));
    RINOK(ReadID(sd, &type));
  }
  else
  {
    ssi->NumTotalSubStreams = p->NumFolders;
    // ssi->NumSubDigests = 0;
  }

  return (type == k7zIdEnd ? SZ_OK : SZ_ERROR_UNSUPPORTED);
}

static SRes SzReadAndDecodePackedStreams(
    ILookInStream *inStream,
    CSzData *sd,
    CBuf *tempBufs,
    UInt32 numFoldersMax,
    UInt64 baseOffset,
    CSzAr *p,
    ISzAlloc *allocTemp)
{
  UInt64 dataStartPos;
  UInt32 fo;
  CSubStreamInfo ssi;
  CSzData sdCodersUnpSizes;

  RINOK(SzReadStreamsInfo(p, sd, numFoldersMax, NULL, 0, &dataStartPos, &ssi, allocTemp));
  
  dataStartPos += baseOffset;
  if (p->NumFolders == 0)
    return SZ_ERROR_ARCHIVE;
 
  sdCodersUnpSizes.Data = p->UnpackSizesData;
  sdCodersUnpSizes.Size = p->UnpackSizesDataSize;
  for (fo = 0; fo < p->NumFolders; fo++)
    Buf_Init(tempBufs + fo);
  for (fo = 0; fo < p->NumFolders; fo++)
  {
    CBuf *tempBuf = tempBufs + fo;
    // folder = p->Folders;
    // unpackSize = SzAr_GetFolderUnpackSize(p, 0);
    UInt32 mix = (UInt32)p->FoSizesOffsets[fo];
    UInt32 mainIndex = mix & 0xFF;
    UInt32 numOutStreams = mix >> 8;
    UInt32 si;
    UInt64 unpackSize = 0;
    p->FoSizesOffsets[fo] = sdCodersUnpSizes.Data - p->UnpackSizesData;
    for (si = 0; si < numOutStreams; si++)
    {
      UInt64 curSize;
      RINOK(ReadNumber(&sdCodersUnpSizes, &curSize));
      if (si == mainIndex)
      {
        unpackSize = curSize;
        break;
      }
    }
    if (si == numOutStreams)
      return SZ_ERROR_FAIL;
    if ((size_t)unpackSize != unpackSize)
      return SZ_ERROR_MEM;
    if (!Buf_Create(tempBuf, (size_t)unpackSize, allocTemp))
      return SZ_ERROR_MEM;
  }
  p->FoSizesOffsets[fo] = sdCodersUnpSizes.Data - p->UnpackSizesData;
    
  for (fo = 0; fo < p->NumFolders; fo++)
  {
    const CBuf *tempBuf = tempBufs + fo;
    RINOK(LookInStream_SeekTo(inStream, dataStartPos));
    RINOK(SzAr_DecodeFolder(p, fo, inStream, dataStartPos, tempBuf->data, tempBuf->size, allocTemp));
    if (SzBitWithVals_Check(&p->FolderCRCs, fo))
      if (CrcCalc(tempBuf->data, tempBuf->size) != p->FolderCRCs.Vals[fo])
        return SZ_ERROR_CRC;
  }
  return SZ_OK;
}

static SRes SzReadFileNames(const Byte *data, size_t size, UInt32 numFiles, size_t *offsets)
{
  size_t pos = 0;
  *offsets++ = 0;
  if (numFiles == 0)
    return (size == 0) ? SZ_OK : SZ_ERROR_ARCHIVE;
  if (data[size - 2] != 0 || data[size - 1] != 0)
    return SZ_ERROR_ARCHIVE;
  do
  {
    const Byte *p;
    if (pos == size)
      return SZ_ERROR_ARCHIVE;
    for (p = data + pos;
      #ifdef _WIN32
      *(const UInt16 *)p != 0
      #else
      p[0] != 0 || p[1] != 0
      #endif
      ; p += 2);
    pos = p - data + 2;
    *offsets++ = (pos >> 1);
  }
  while (--numFiles);
  return (pos == size) ? SZ_OK : SZ_ERROR_ARCHIVE;
}

static MY_NO_INLINE SRes ReadTime(CSzBitUi64s *p, UInt32 num,
    CSzData *sd2,
    const CBuf *tempBufs, UInt32 numTempBufs,
    ISzAlloc *alloc)
{
  CSzData sd;
  UInt32 i;
  CNtfsFileTime *vals;
  Byte *defs;
  Byte external;
  RINOK(ReadBitVector(sd2, num, &p->Defs, alloc));
  RINOK(SzReadByte(sd2, &external));
  if (external == 0)
    sd = *sd2;
  else
  {
    UInt32 index;
    SzReadNumber32(sd2, &index);
    if (index >= numTempBufs)
      return SZ_ERROR_ARCHIVE;
    sd.Data = tempBufs[index].data;
    sd.Size = tempBufs[index].size;
  }
  MY_ALLOC(CNtfsFileTime, p->Vals, num, alloc);
  vals = p->Vals;
  defs = p->Defs;
  for (i = 0; i < num; i++)
    if (SzBitArray_Check(defs, i))
    {
      if (sd.Size < 8)
        return SZ_ERROR_ARCHIVE;
      vals[i].Low = GetUi32(sd.Data);
      vals[i].High = GetUi32(sd.Data + 4);
      SKIP_DATA2(sd, 8);
    }
    else
      vals[i].High = vals[i].Low = 0;
  if (external == 0)
    *sd2 = sd;
  return SZ_OK;
}

#define NUM_ADDITIONAL_STREAMS_MAX 8

static SRes SzReadHeader2(
    CSzArEx *p,   /* allocMain */
    CSzData *sd,
    // Byte **emptyStreamVector, /* allocTemp */
    // Byte **emptyFileVector,   /* allocTemp */
    // Byte **lwtVector,         /* allocTemp */
    ILookInStream *inStream,
    CBuf *tempBufs,
    UInt32 *numTempBufs,
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp
    )
{
  UInt64 type;
  UInt32 numFiles = 0;
  UInt32 numEmptyStreams = 0;
  UInt32 i;
  CSubStreamInfo ssi;
  const Byte *emptyStreams = 0;
  const Byte *emptyFiles = 0;

  SzData_Clear(&ssi.sdSizes);
  SzData_Clear(&ssi.sdCRCs);
  SzData_Clear(&ssi.sdNumSubStreams);

  ssi.NumSubDigests = 0;
  ssi.NumTotalSubStreams = 0;

  RINOK(ReadID(sd, &type));

  if (type == k7zIdArchiveProperties)
  {
    for (;;)
    {
      UInt64 type;
      RINOK(ReadID(sd, &type));
      if (type == k7zIdEnd)
        break;
      RINOK(SkipData(sd));
    }
    RINOK(ReadID(sd, &type));
  }

  // if (type == k7zIdAdditionalStreamsInfo)     return SZ_ERROR_UNSUPPORTED;

  if (type == k7zIdAdditionalStreamsInfo)
  {
    CSzAr tempAr;
    SRes res;
    UInt32 numTempFolders;
    
    SzAr_Init(&tempAr);
    res = SzReadAndDecodePackedStreams(inStream, sd, tempBufs, NUM_ADDITIONAL_STREAMS_MAX,
        p->startPosAfterHeader, &tempAr, allocTemp);
    numTempFolders = tempAr.NumFolders;
    SzAr_Free(&tempAr, allocTemp);
    if (res != SZ_OK)
      return res;
    *numTempBufs = numTempFolders;
    RINOK(ReadID(sd, &type));
  }

  if (type == k7zIdMainStreamsInfo)
  {
    RINOK(SzReadStreamsInfo(&p->db, sd, (UInt32)1 << 30, tempBufs, *numTempBufs,
        &p->dataPos, &ssi, allocMain));
    p->dataPos += p->startPosAfterHeader;
    RINOK(ReadID(sd, &type));
  }

  if (type == k7zIdEnd)
  {
    // *sd2 = sd;
    return SZ_OK;
  }
  if (type != k7zIdFilesInfo)
    return SZ_ERROR_ARCHIVE;
  
  RINOK(SzReadNumber32(sd, &numFiles));
  p->NumFiles = numFiles;

  for (;;)
  {
    UInt64 type;
    UInt64 size;
    RINOK(ReadID(sd, &type));
    if (type == k7zIdEnd)
      break;
    RINOK(ReadNumber(sd, &size));
    if (size > sd->Size)
      return SZ_ERROR_ARCHIVE;
    if ((UInt64)(int)type != type)
    {
      SKIP_DATA(sd, size);
    }
    else switch((int)type)
    {
      case k7zIdName:
      {
        size_t namesSize;
        const Byte *namesData;
        Byte external;

        SZ_READ_BYTE(external);
        if (external == 0)
        {
          namesSize = (size_t)size - 1;
          namesData = sd->Data;
        }
        else
        {
          UInt32 index;
          SzReadNumber32(sd, &index);
          if (index >= *numTempBufs)
            return SZ_ERROR_ARCHIVE;
          namesData = (tempBufs)[index].data;
          namesSize = (tempBufs)[index].size;
        }

        if ((namesSize & 1) != 0)
          return SZ_ERROR_ARCHIVE;
        MY_ALLOC(Byte, p->FileNames, namesSize, allocMain);
        MY_ALLOC(size_t, p->FileNameOffsets, numFiles + 1, allocMain);
        memcpy(p->FileNames, namesData, namesSize);
        RINOK(SzReadFileNames(p->FileNames, namesSize, numFiles, p->FileNameOffsets))
        if (external == 0)
        {
          SKIP_DATA(sd, namesSize);
        }
        break;
      }
      case k7zIdEmptyStream:
      {
        RINOK(RememberBitVector(sd, numFiles, &emptyStreams));
        numEmptyStreams = CountDefinedBits(emptyStreams, numFiles);
        break;
      }
      case k7zIdEmptyFile:
      {
        RINOK(RememberBitVector(sd, numEmptyStreams, &emptyFiles));
        break;
      }
      case k7zIdWinAttrib:
      {
        Byte external;
        CSzData sdSwitch;
        CSzData *sdPtr;
        SzBitUi32s_Free(&p->Attribs, allocMain);
        RINOK(ReadBitVector(sd, numFiles, &p->Attribs.Defs, allocMain));

        SZ_READ_BYTE(external);
        if (external == 0)
          sdPtr = sd;
        else
        {
          UInt32 index;
          SzReadNumber32(sd, &index);
          if (index >= *numTempBufs)
            return SZ_ERROR_ARCHIVE;
          sdSwitch.Data = (tempBufs)[index].data;
          sdSwitch.Size = (tempBufs)[index].size;
          sdPtr = &sdSwitch;
        }
        RINOK(ReadUi32s(sdPtr, numFiles, &p->Attribs, allocMain));
        break;
      }
      /*
      case k7zParent:
      {
        SzBitUi32s_Free(&p->Parents, allocMain);
        RINOK(ReadBitVector(sd, numFiles, &p->Parents.Defs, allocMain));
        RINOK(SzReadSwitch(sd));
        RINOK(ReadUi32s(sd, numFiles, &p->Parents, allocMain));
        break;
      }
      */
      case k7zIdMTime: RINOK(ReadTime(&p->MTime, numFiles, sd, tempBufs, *numTempBufs, allocMain)); break;
      case k7zIdCTime: RINOK(ReadTime(&p->CTime, numFiles, sd, tempBufs, *numTempBufs, allocMain)); break;
      default:
      {
        SKIP_DATA(sd, size);
      }
    }
  }

  if (numFiles - numEmptyStreams != ssi.NumTotalSubStreams)
    return SZ_ERROR_ARCHIVE;

  for (;;)
  {
    UInt64 type;
    RINOK(ReadID(sd, &type));
    if (type == k7zIdEnd)
      break;
    RINOK(SkipData(sd));
  }

  {
    UInt32 emptyFileIndex = 0;

    UInt32 folderIndex = 0;
    UInt32 indexInFolder = 0;
    UInt64 unpackPos = 0;
    const Byte *digestsDefs = 0;
    const Byte *digestsVals = 0;
    UInt32 digestsValsIndex = 0;
    UInt32 digestIndex;
    Byte allDigestsDefined = 0;
    UInt32 curNumSubStreams = (UInt32)(Int32)-1;
    Byte isDirMask = 0;
    Byte crcMask = 0;
    Byte mask = 0x80;
    // size_t unpSizesOffset = 0;
    CSzData sdCodersUnpSizes;
    sdCodersUnpSizes.Data = p->db.UnpackSizesData;
    sdCodersUnpSizes.Size = p->db.UnpackSizesDataSize;
    
    MY_ALLOC(UInt32, p->FolderStartFileIndex, p->db.NumFolders + 1, allocMain);
    MY_ALLOC(UInt32, p->FileIndexToFolderIndexMap, p->NumFiles, allocMain);
    MY_ALLOC(UInt64, p->UnpackPositions, p->NumFiles + 1, allocMain);
    MY_ALLOC(Byte, p->IsDirs, (p->NumFiles + 7) >> 3, allocMain);

    RINOK(SzBitUi32s_Alloc(&p->CRCs, p->NumFiles, allocMain));

    if (ssi.sdCRCs.Size != 0)
    {
      RINOK(SzReadByte(&ssi.sdCRCs, &allDigestsDefined));
      if (allDigestsDefined)
        digestsVals = ssi.sdCRCs.Data;
      else
      {
        size_t numBytes = (ssi.NumSubDigests + 7) >> 3;
        digestsDefs = ssi.sdCRCs.Data;
        digestsVals = digestsDefs + numBytes;
      }
    }

    digestIndex = 0;
    for (i = 0; i < numFiles; i++, mask >>= 1)
    {
      if (mask == 0)
      {
        UInt32 byteIndex = (i - 1) >> 3;
        p->IsDirs[byteIndex] = isDirMask;
        p->CRCs.Defs[byteIndex] = crcMask;
        isDirMask = 0;
        crcMask = 0;
        mask = 0x80;
      }

      p->UnpackPositions[i] = unpackPos;
      p->CRCs.Vals[i] = 0;
      // p->CRCs.Defs[i] = 0;
      if (emptyStreams && SzBitArray_Check(emptyStreams , i))
      {
        if (!emptyFiles || !SzBitArray_Check(emptyFiles, emptyFileIndex))
          isDirMask |= mask;
        emptyFileIndex++;
        if (indexInFolder == 0)
        {
          p->FileIndexToFolderIndexMap[i] = (UInt32)-1;
          continue;
        }
      }
      if (indexInFolder == 0)
      {
        /*
        v3.13 incorrectly worked with empty folders
        v4.07: Loop for skipping empty folders
        */
        for (;;)
        {
          if (folderIndex >= p->db.NumFolders)
            return SZ_ERROR_ARCHIVE;
          p->FolderStartFileIndex[folderIndex] = i;
          if (curNumSubStreams == (UInt32)(Int32)-1);
          {
            curNumSubStreams = 1;
            if (ssi.sdNumSubStreams.Data != 0)
            {
              RINOK(SzReadNumber32(&ssi.sdNumSubStreams, &curNumSubStreams));
            }
          }
          if (curNumSubStreams != 0)
            break;
          curNumSubStreams = (UInt32)(Int32)-1;
          folderIndex++; // check it
        }
      }
      p->FileIndexToFolderIndexMap[i] = folderIndex;
      if (emptyStreams && SzBitArray_Check(emptyStreams , i))
        continue;
      
      indexInFolder++;
      if (indexInFolder >= curNumSubStreams)
      {
        UInt64 folderUnpackSize = 0;
        UInt64 startFolderUnpackPos;
        {
          UInt32 mix = (UInt32)p->db.FoSizesOffsets[folderIndex];
          UInt32 mainIndex = mix & 0xFF;
          UInt32 numOutStreams = mix >> 8;
          UInt32 si;
          p->db.FoSizesOffsets[folderIndex] = sdCodersUnpSizes.Data - p->db.UnpackSizesData;
          for (si = 0; si < numOutStreams; si++)
          {
            UInt64 curSize;
            RINOK(ReadNumber(&sdCodersUnpSizes, &curSize));
            if (si == mainIndex)
            {
              folderUnpackSize = curSize;
              break;
            }
          }
          if (si == numOutStreams)
            return SZ_ERROR_FAIL;
        }

        // UInt64 folderUnpackSize = SzAr_GetFolderUnpackSize(&p->db, folderIndex);
        startFolderUnpackPos = p->UnpackPositions[p->FolderStartFileIndex[folderIndex]];
        if (folderUnpackSize < unpackPos - startFolderUnpackPos)
          return SZ_ERROR_ARCHIVE;
        unpackPos = startFolderUnpackPos + folderUnpackSize;

        if (curNumSubStreams == 1 && SzBitWithVals_Check(&p->db.FolderCRCs, i))
        {
          p->CRCs.Vals[i] = p->db.FolderCRCs.Vals[folderIndex];
          crcMask |= mask;
        }
        else if (allDigestsDefined || (digestsDefs && SzBitArray_Check(digestsDefs, digestIndex)))
        {
          p->CRCs.Vals[i] = GetUi32(digestsVals + (size_t)digestsValsIndex * 4);
          digestsValsIndex++;
          crcMask |= mask;
        }
        folderIndex++;
        indexInFolder = 0;
      }
      else
      {
        UInt64 v;
        RINOK(ReadNumber(&ssi.sdSizes, &v));
        unpackPos += v;
        if (allDigestsDefined || (digestsDefs && SzBitArray_Check(digestsDefs, digestIndex)))
        {
          p->CRCs.Vals[i] = GetUi32(digestsVals + (size_t)digestsValsIndex * 4);
          digestsValsIndex++;
          crcMask |= mask;
        }
      }
    }
    if (mask != 0x80)
    {
      UInt32 byteIndex = (i - 1) >> 3;
      p->IsDirs[byteIndex] = isDirMask;
      p->CRCs.Defs[byteIndex] = crcMask;
    }
    p->UnpackPositions[i] = unpackPos;
    p->FolderStartFileIndex[folderIndex] = i;
    p->db.FoSizesOffsets[folderIndex] = sdCodersUnpSizes.Data - p->db.UnpackSizesData;
  }
  return SZ_OK;
}

static SRes SzReadHeader(
    CSzArEx *p,
    CSzData *sd,
    ILookInStream *inStream,
    ISzAlloc *allocMain
    ,ISzAlloc *allocTemp
    )
{
  // Byte *emptyStreamVector = 0;
  // Byte *emptyFileVector = 0;
  // Byte *lwtVector = 0;
  UInt32 i;
  UInt32 numTempBufs = 0;
  SRes res;
  CBuf tempBufs[NUM_ADDITIONAL_STREAMS_MAX];

  for (i = 0; i < NUM_ADDITIONAL_STREAMS_MAX; i++)
    Buf_Init(tempBufs + i);
  // SzBitUi32s_Init(&digests);
  
  res = SzReadHeader2(p, sd,
      // &emptyStreamVector,
      // &emptyFileVector,
      // &lwtVector,
      inStream,
      tempBufs, &numTempBufs,
      allocMain, allocTemp
      );
  
  for (i = 0; i < numTempBufs; i++)
    Buf_Free(tempBufs + i, allocTemp);

  // IAlloc_Free(allocTemp, emptyStreamVector);
  // IAlloc_Free(allocTemp, emptyFileVector);
  // IAlloc_Free(allocTemp, lwtVector);

  RINOK(res);
  {
    if (sd->Size != 0)
      return SZ_ERROR_FAIL;
  }

  return res;
}

/*
static UInt64 SzAr_GetFolderUnpackSize(const CSzAr *p, UInt32 folderIndex)
{
  const CSzFolder2 *f = p->Folders + folderIndex;

  // return p->CoderUnpackSizes[f->StartCoderUnpackSizesIndex + f->IndexOfMainOutStream];

  UInt32 si;
  CSzData sdCodersUnpSizes;
  sdCodersUnpSizes.Data = p->UnpackSizesData + f->UnpackSizeDataOffset;
  sdCodersUnpSizes.Size = p->UnpackSizesDataSize - f->UnpackSizeDataOffset;
  for (si = 0; si < numOutStreams; si++)
  {
    UInt64 curSize;
    ReadNumber(&sdCodersUnpSizes, &curSize);
    if (si == mainIndex)
      return curSize;
  }
  return 0;
}
*/

static SRes SzArEx_Open2(
    CSzArEx *p,
    ILookInStream *inStream,
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp)
{
  Byte header[k7zStartHeaderSize];
  Int64 startArcPos;
  UInt64 nextHeaderOffset, nextHeaderSize;
  size_t nextHeaderSizeT;
  UInt32 nextHeaderCRC;
  CBuf buf;
  SRes res;

  startArcPos = 0;
  RINOK(inStream->Seek(inStream, &startArcPos, SZ_SEEK_CUR));

  RINOK(LookInStream_Read2(inStream, header, k7zStartHeaderSize, SZ_ERROR_NO_ARCHIVE));

  if (!TestSignatureCandidate(header))
    return SZ_ERROR_NO_ARCHIVE;
  if (header[6] != k7zMajorVersion)
    return SZ_ERROR_UNSUPPORTED;

  nextHeaderOffset = GetUi64(header + 12);
  nextHeaderSize = GetUi64(header + 20);
  nextHeaderCRC = GetUi32(header + 28);

  p->startPosAfterHeader = startArcPos + k7zStartHeaderSize;
  
  if (CrcCalc(header + 12, 20) != GetUi32(header + 8))
    return SZ_ERROR_CRC;

  nextHeaderSizeT = (size_t)nextHeaderSize;
  if (nextHeaderSizeT != nextHeaderSize)
    return SZ_ERROR_MEM;
  if (nextHeaderSizeT == 0)
    return SZ_OK;
  if (nextHeaderOffset > nextHeaderOffset + nextHeaderSize ||
      nextHeaderOffset > nextHeaderOffset + nextHeaderSize + k7zStartHeaderSize)
    return SZ_ERROR_NO_ARCHIVE;

  {
    Int64 pos = 0;
    RINOK(inStream->Seek(inStream, &pos, SZ_SEEK_END));
    if ((UInt64)pos < startArcPos + nextHeaderOffset ||
        (UInt64)pos < startArcPos + k7zStartHeaderSize + nextHeaderOffset ||
        (UInt64)pos < startArcPos + k7zStartHeaderSize + nextHeaderOffset + nextHeaderSize)
      return SZ_ERROR_INPUT_EOF;
  }

  RINOK(LookInStream_SeekTo(inStream, startArcPos + k7zStartHeaderSize + nextHeaderOffset));

  if (!Buf_Create(&buf, nextHeaderSizeT, allocTemp))
    return SZ_ERROR_MEM;

  res = LookInStream_Read(inStream, buf.data, nextHeaderSizeT);
  if (res == SZ_OK)
  {
    res = SZ_ERROR_ARCHIVE;
    if (CrcCalc(buf.data, nextHeaderSizeT) == nextHeaderCRC)
    {
      CSzData sd;
      UInt64 type;
      sd.Data = buf.data;
      sd.Size = buf.size;
      res = ReadID(&sd, &type);
      if (res == SZ_OK && type == k7zIdEncodedHeader)
      {
        CSzAr tempAr;
        CBuf tempBuf;
        Buf_Init(&tempBuf);
        
        SzAr_Init(&tempAr);
        res = SzReadAndDecodePackedStreams(inStream, &sd, &tempBuf, 1, p->startPosAfterHeader, &tempAr, allocTemp);
        SzAr_Free(&tempAr, allocTemp);
       
        if (res != SZ_OK)
        {
          Buf_Free(&tempBuf, allocTemp);
        }
        else
        {
          Buf_Free(&buf, allocTemp);
          buf.data = tempBuf.data;
          buf.size = tempBuf.size;
          sd.Data = buf.data;
          sd.Size = buf.size;
          res = ReadID(&sd, &type);
        }
      }
      if (res == SZ_OK)
      {
        if (type == k7zIdHeader)
        {
          CSzData sd2;
          int ttt;
          for (ttt = 0; ttt < 1; ttt++)
          // for (ttt = 0; ttt < 40000; ttt++)
          {
            SzArEx_Free(p, allocMain);
            sd2 = sd;
            res = SzReadHeader(p, &sd2, inStream, allocMain, allocTemp
              );
            if (res != SZ_OK)
              break;
          }

          // res = SzReadHeader(p, &sd, allocMain, allocTemp);
        }
        else
          res = SZ_ERROR_UNSUPPORTED;
      }
    }
  }
  Buf_Free(&buf, allocTemp);
  return res;
}

// #include <stdio.h>

SRes SzArEx_Open(CSzArEx *p, ILookInStream *inStream,
    ISzAlloc *allocMain, ISzAlloc *allocTemp)
{
  SRes res = SzArEx_Open2(p, inStream, allocMain, allocTemp);
  if (res != SZ_OK)
    SzArEx_Free(p, allocMain);
  // printf ("\nrrr=%d\n", rrr);
  return res;
}

SRes SzArEx_Extract(
    const CSzArEx *p,
    ILookInStream *inStream,
    UInt32 fileIndex,
    UInt32 *blockIndex,
    Byte **tempBuf,
    size_t *outBufferSize,
    size_t *offset,
    size_t *outSizeProcessed,
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp)
{
  UInt32 folderIndex = p->FileIndexToFolderIndexMap[fileIndex];
  SRes res = SZ_OK;
  *offset = 0;
  *outSizeProcessed = 0;
  if (folderIndex == (UInt32)-1)
  {
    IAlloc_Free(allocMain, *tempBuf);
    *blockIndex = folderIndex;
    *tempBuf = 0;
    *outBufferSize = 0;
    return SZ_OK;
  }

  if (*tempBuf == 0 || *blockIndex != folderIndex)
  {
    // UInt64 unpackSizeSpec = SzAr_GetFolderUnpackSize(&p->db, folderIndex);
    UInt64 unpackSizeSpec =
        p->UnpackPositions[p->FolderStartFileIndex[folderIndex + 1]] -
        p->UnpackPositions[p->FolderStartFileIndex[folderIndex]];
    size_t unpackSize = (size_t)unpackSizeSpec;

    if (unpackSize != unpackSizeSpec)
      return SZ_ERROR_MEM;
    *blockIndex = folderIndex;
    IAlloc_Free(allocMain, *tempBuf);
    *tempBuf = 0;
    
    // RINOK(LookInStream_SeekTo(inStream, startOffset));
    
    if (res == SZ_OK)
    {
      *outBufferSize = unpackSize;
      if (unpackSize != 0)
      {
        *tempBuf = (Byte *)IAlloc_Alloc(allocMain, unpackSize);
        if (*tempBuf == 0)
          res = SZ_ERROR_MEM;
      }
      if (res == SZ_OK)
      {
        res = SzAr_DecodeFolder(&p->db, folderIndex,
          inStream,
          p->dataPos,
          *tempBuf, unpackSize, allocTemp);
        if (res == SZ_OK)
        {
          if (SzBitWithVals_Check(&p->db.FolderCRCs, folderIndex))
          {
            if (CrcCalc(*tempBuf, unpackSize) != p->db.FolderCRCs.Vals[folderIndex])
              res = SZ_ERROR_CRC;
          }
        }
      }
    }
  }
  if (res == SZ_OK)
  {
    UInt64 unpackPos = p->UnpackPositions[fileIndex];
    *offset = (size_t)(unpackPos - p->UnpackPositions[p->FolderStartFileIndex[folderIndex]]);
    *outSizeProcessed = (size_t)(p->UnpackPositions[fileIndex + 1] - unpackPos);
    if (*offset + *outSizeProcessed > *outBufferSize)
      return SZ_ERROR_FAIL;
    if (SzBitWithVals_Check(&p->CRCs, fileIndex) && CrcCalc(*tempBuf + *offset, *outSizeProcessed) != p->CRCs.Vals[fileIndex])
      res = SZ_ERROR_CRC;
  }
  return res;
}


size_t SzArEx_GetFileNameUtf16(const CSzArEx *p, size_t fileIndex, UInt16 *dest)
{
  size_t offs = p->FileNameOffsets[fileIndex];
  size_t len = p->FileNameOffsets[fileIndex + 1] - offs;
  if (dest != 0)
  {
    size_t i;
    const Byte *src = p->FileNames + offs * 2;
    for (i = 0; i < len; i++)
      dest[i] = GetUi16(src + i * 2);
  }
  return len;
}

/*
size_t SzArEx_GetFullNameLen(const CSzArEx *p, size_t fileIndex)
{
  size_t len;
  if (!p->FileNameOffsets)
    return 1;
  len = 0;
  for (;;)
  {
    UInt32 parent = (UInt32)(Int32)-1;
    len += p->FileNameOffsets[fileIndex + 1] - p->FileNameOffsets[fileIndex];
    if SzBitWithVals_Check(&p->Parents, fileIndex)
      parent = p->Parents.Vals[fileIndex];
    if (parent == (UInt32)(Int32)-1)
      return len;
    fileIndex = parent;
  }
}

UInt16 *SzArEx_GetFullNameUtf16_Back(const CSzArEx *p, size_t fileIndex, UInt16 *dest)
{
  Bool needSlash;
  if (!p->FileNameOffsets)
  {
    *(--dest) = 0;
    return dest;
  }
  needSlash = False;
  for (;;)
  {
    UInt32 parent = (UInt32)(Int32)-1;
    size_t curLen = p->FileNameOffsets[fileIndex + 1] - p->FileNameOffsets[fileIndex];
    SzArEx_GetFileNameUtf16(p, fileIndex, dest - curLen);
    if (needSlash)
      *(dest - 1) = '/';
    needSlash = True;
    dest -= curLen;

    if SzBitWithVals_Check(&p->Parents, fileIndex)
      parent = p->Parents.Vals[fileIndex];
    if (parent == (UInt32)(Int32)-1)
      return dest;
    fileIndex = parent;
  }
}
*/
