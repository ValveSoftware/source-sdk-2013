// LzmaBench.cpp

#include "StdAfx.h"

#include "LzmaBench.h"

#ifndef _WIN32
#include <time.h>
#endif

#include "../../../Common/CRC.h"
#include "../LZMA/LZMADecoder.h"
#include "../LZMA/LZMAEncoder.h"

static const UInt32 kAdditionalSize = 
#ifdef _WIN32_WCE
(1 << 20);
#else
(6 << 20);
#endif

static const UInt32 kCompressedAdditionalSize = (1 << 10);
static const UInt32 kMaxLzmaPropSize = 10;

class CRandomGenerator
{
  UInt32 A1;
  UInt32 A2;
public:
  CRandomGenerator() { Init(); }
  void Init() { A1 = 362436069; A2 = 521288629;}
  UInt32 GetRnd() 
  {
    return 
      ((A1 = 36969 * (A1 & 0xffff) + (A1 >> 16)) << 16) ^
      ((A2 = 18000 * (A2 & 0xffff) + (A2 >> 16)) );
  }
};

class CBitRandomGenerator
{
  CRandomGenerator RG;
  UInt32 Value;
  int NumBits;
public:
  void Init()
  {
    Value = 0;
    NumBits = 0;
  }
  UInt32 GetRnd(int numBits) 
  {
    if (NumBits > numBits)
    {
      UInt32 result = Value & ((1 << numBits) - 1);
      Value >>= numBits;
      NumBits -= numBits;
      return result;
    }
    numBits -= NumBits;
    UInt32 result = (Value << numBits);
    Value = RG.GetRnd();
    result |= Value & ((1 << numBits) - 1);
    Value >>= numBits;
    NumBits = 32 - numBits;
    return result;
  }
};

class CBenchRandomGenerator
{
  CBitRandomGenerator RG;
  UInt32 Pos;
  UInt32 Rep0;
public:
  UInt32 BufferSize;
  Byte *Buffer;
  CBenchRandomGenerator(): Buffer(0) {} 
  ~CBenchRandomGenerator() { Free(); }
  void Free() 
  { 
    ::MidFree(Buffer);
    Buffer = 0;
  }
  bool Alloc(UInt32 bufferSize) 
  {
    if (Buffer != 0 && BufferSize == bufferSize)
      return true;
    Free();
    Buffer = (Byte *)::MidAlloc(bufferSize);
    Pos = 0;
    BufferSize = bufferSize;
    return (Buffer != 0);
  }
  UInt32 GetRndBit() { return RG.GetRnd(1); }
  /*
  UInt32 GetLogRand(int maxLen)
  {
    UInt32 len = GetRnd() % (maxLen + 1);
    return GetRnd() & ((1 << len) - 1);
  }
  */
  UInt32 GetLogRandBits(int numBits)
  {
    UInt32 len = RG.GetRnd(numBits);
    return RG.GetRnd(len);
  }
  UInt32 GetOffset()
  {
    if (GetRndBit() == 0)
      return GetLogRandBits(4);
    return (GetLogRandBits(4) << 10) | RG.GetRnd(10);
  }
  UInt32 GetLen1() { return RG.GetRnd(1 + (int)RG.GetRnd(2)); }
  UInt32 GetLen2() { return RG.GetRnd(2 + (int)RG.GetRnd(2)); }
  void Generate()
  {
    RG.Init(); 
    Rep0 = 1;
    while(Pos < BufferSize)
    {
      if (GetRndBit() == 0 || Pos < 1)
        Buffer[Pos++] = (Byte)RG.GetRnd(8);
      else
      {
        UInt32 len;
        if (RG.GetRnd(3) == 0)
          len = 1 + GetLen1();
        else
        {
          do
            Rep0 = GetOffset();
          while (Rep0 >= Pos);
          Rep0++;
          len = 2 + GetLen2();
        }
        for (UInt32 i = 0; i < len && Pos < BufferSize; i++, Pos++)
          Buffer[Pos] = Buffer[Pos - Rep0];
      }
    }
  }
};

class CBenchmarkInStream: 
  public ISequentialInStream,
  public CMyUnknownImp
{
  const Byte *Data;
  UInt32 Pos;
  UInt32 Size;
public:
  MY_UNKNOWN_IMP
  void Init(const Byte *data, UInt32 size)
  {
    Data = data;
    Size = size;
    Pos = 0;
  }
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
};

STDMETHODIMP CBenchmarkInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 remain = Size - Pos;
  if (size > remain)
    size = remain;
  for (UInt32 i = 0; i < size; i++)
    ((Byte *)data)[i] = Data[Pos + i];
  Pos += size;
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK;
}
  
class CBenchmarkOutStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
  UInt32 BufferSize;
  FILE *_f;
public:
  UInt32 Pos;
  Byte *Buffer;
  CBenchmarkOutStream(): _f(0), Buffer(0) {} 
  virtual ~CBenchmarkOutStream() { delete []Buffer; }
  void Init(FILE *f, UInt32 bufferSize) 
  {
    delete []Buffer;
    Buffer = 0;
    Buffer = new Byte[bufferSize];
    Pos = 0;
    BufferSize = bufferSize;
    _f = f;
  }
  MY_UNKNOWN_IMP
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

STDMETHODIMP CBenchmarkOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 i;
  for (i = 0; i < size && Pos < BufferSize; i++)
    Buffer[Pos++] = ((const Byte *)data)[i];
  if(processedSize != NULL)
    *processedSize = i;
  if (i != size)
  {
    fprintf(_f, "\nERROR: Buffer is full\n");
    return E_FAIL;
  }
  return S_OK;
}
  
class CCrcOutStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
public:
  CCRC CRC;
  MY_UNKNOWN_IMP
  void Init() { CRC.Init(); }
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

STDMETHODIMP CCrcOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  CRC.Update(data, size);
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK;
}
  
static UInt64 GetTimeCount()
{
  #ifdef _WIN32
  LARGE_INTEGER value;
  if (::QueryPerformanceCounter(&value))
    return value.QuadPart;
  return GetTickCount();
  #else
  return clock();
  #endif 
}

static UInt64 GetFreq()
{
  #ifdef _WIN32
  LARGE_INTEGER value;
  if (::QueryPerformanceFrequency(&value))
    return value.QuadPart;
  return 1000;
  #else
  return CLOCKS_PER_SEC;
  #endif 
}

struct CProgressInfo:
  public ICompressProgressInfo,
  public CMyUnknownImp
{
  UInt64 ApprovedStart;
  UInt64 InSize;
  UInt64 Time;
  void Init()
  {
    InSize = 0;
    Time = 0;
  }
  MY_UNKNOWN_IMP
  STDMETHOD(SetRatioInfo)(const UInt64 *inSize, const UInt64 *outSize);
};

STDMETHODIMP CProgressInfo::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  if (*inSize >= ApprovedStart && InSize == 0)
  {
    Time = ::GetTimeCount();
    InSize = *inSize;
  }
  return S_OK;
}

static const int kSubBits = 8;

static UInt32 GetLogSize(UInt32 size)
{
  for (int i = kSubBits; i < 32; i++)
    for (UInt32 j = 0; j < (1 << kSubBits); j++)
      if (size <= (((UInt32)1) << i) + (j << (i - kSubBits)))
        return (i << kSubBits) + j;
  return (32 << kSubBits);
}

static UInt64 MyMultDiv64(UInt64 value, UInt64 elapsedTime)
{
  UInt64 freq = GetFreq();
  UInt64 elTime = elapsedTime;
  while(freq > 1000000)
  {
    freq >>= 1;
    elTime >>= 1;
  }
  if (elTime == 0)
    elTime = 1;
  return value * freq / elTime;
}

static UInt64 GetCompressRating(UInt32 dictionarySize, UInt64 elapsedTime, UInt64 size)
{
  UInt64 t = GetLogSize(dictionarySize) - (18 << kSubBits);
  UInt64 numCommandsForOne = 1060 + ((t * t * 10) >> (2 * kSubBits));
  UInt64 numCommands = (UInt64)(size) * numCommandsForOne;
  return MyMultDiv64(numCommands, elapsedTime);
}

static UInt64 GetDecompressRating(UInt64 elapsedTime, 
    UInt64 outSize, UInt64 inSize)
{
  UInt64 numCommands = inSize * 220 + outSize * 20;
  return MyMultDiv64(numCommands, elapsedTime);
}

/*
static UInt64 GetTotalRating(
    UInt32 dictionarySize, 
    bool isBT4,
    UInt64 elapsedTimeEn, UInt64 sizeEn,
    UInt64 elapsedTimeDe, 
    UInt64 inSizeDe, UInt64 outSizeDe)
{
  return (GetCompressRating(dictionarySize, isBT4, elapsedTimeEn, sizeEn) + 
    GetDecompressRating(elapsedTimeDe, inSizeDe, outSizeDe)) / 2;
}
*/

static void PrintRating(FILE *f, UInt64 rating)
{
  fprintf(f, "%5d MIPS", (unsigned int)(rating / 1000000));
}

static void PrintResults(
    FILE *f, 
    UInt32 dictionarySize,
    UInt64 elapsedTime, 
    UInt64 size, 
    bool decompressMode, UInt64 secondSize)
{
  UInt64 speed = MyMultDiv64(size, elapsedTime);
  fprintf(f, "%6d KB/s  ", (unsigned int)(speed / 1024));
  UInt64 rating;
  if (decompressMode)
    rating = GetDecompressRating(elapsedTime, size, secondSize);
  else
    rating = GetCompressRating(dictionarySize, elapsedTime, size);
  PrintRating(f, rating);
}

static void ThrowError(FILE *f, HRESULT result, const char *s)
{
  fprintf(f, "\nError: ");
  if (result == E_ABORT)
    fprintf(f, "User break");
  if (result == E_OUTOFMEMORY)
    fprintf(f, "Can not allocate memory");
  else
    fprintf(f, s);
  fprintf(f, "\n");
}

const wchar_t *bt2 = L"BT2";
const wchar_t *bt4 = L"BT4";

int LzmaBenchmark(FILE *f, UInt32 numIterations, UInt32 dictionarySize)
{
  if (numIterations == 0)
    return 0;
  if (dictionarySize < (1 << 18))
  {
    fprintf(f, "\nError: dictionary size for benchmark must be >= 19 (512 KB)\n");
    return 1;
  }
  fprintf(f, "\n       Compressing                Decompressing\n\n");
  NCompress::NLZMA::CEncoder *encoderSpec = new NCompress::NLZMA::CEncoder;
  CMyComPtr<ICompressCoder> encoder = encoderSpec;

  NCompress::NLZMA::CDecoder *decoderSpec = new NCompress::NLZMA::CDecoder;
  CMyComPtr<ICompressCoder> decoder = decoderSpec;

  CBenchmarkOutStream *propStreamSpec = new CBenchmarkOutStream;
  CMyComPtr<ISequentialOutStream> propStream = propStreamSpec;
  propStreamSpec->Init(f, kMaxLzmaPropSize);
  
  PROPID propIDs[] = 
  { 
    NCoderPropID::kDictionarySize
  };
  const int kNumProps = sizeof(propIDs) / sizeof(propIDs[0]);
  PROPVARIANT properties[kNumProps];
  properties[0].vt = VT_UI4;
  properties[0].ulVal = UInt32(dictionarySize);

  const UInt32 kBufferSize = dictionarySize + kAdditionalSize;
  const UInt32 kCompressedBufferSize = (kBufferSize / 2) + kCompressedAdditionalSize;

  if (encoderSpec->SetCoderProperties(propIDs, properties, kNumProps) != S_OK)
  {
    fprintf(f, "\nError: Incorrect command\n");
    return 1;
  }
  encoderSpec->WriteCoderProperties(propStream);

  CBenchRandomGenerator rg;
  if (!rg.Alloc(kBufferSize))
  {
    fprintf(f, "\nError: Can't allocate memory\n");
    return 1;
  }

  rg.Generate();
  CCRC crc;
  crc.Update(rg.Buffer, rg.BufferSize);

  CProgressInfo *progressInfoSpec = new CProgressInfo;
  CMyComPtr<ICompressProgressInfo> progressInfo = progressInfoSpec;

  progressInfoSpec->ApprovedStart = dictionarySize;

  UInt64 totalBenchSize = 0;
  UInt64 totalEncodeTime = 0;
  UInt64 totalDecodeTime = 0;
  UInt64 totalCompressedSize = 0;

  for (UInt32 i = 0; i < numIterations; i++)
  {
    progressInfoSpec->Init();
    CBenchmarkInStream *inStreamSpec = new CBenchmarkInStream;
    inStreamSpec->Init(rg.Buffer, rg.BufferSize);
    CMyComPtr<ISequentialInStream> inStream = inStreamSpec;
    CBenchmarkOutStream *outStreamSpec = new CBenchmarkOutStream;
    outStreamSpec->Init(f, kCompressedBufferSize);
    CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
    HRESULT result = encoder->Code(inStream, outStream, 0, 0, progressInfo);
    UInt64 encodeTime = ::GetTimeCount() - progressInfoSpec->Time;
    UInt32 compressedSize = outStreamSpec->Pos;
    if(result != S_OK)
    {
      ThrowError(f, result, "Encoder Error");
      return 1;
    }
    if (progressInfoSpec->InSize == 0)
    {
      fprintf(f, "\nError: Internal ERROR 1282\n");
      return 1;
    }
  
    ///////////////////////
    // Decompressing
  
    CCrcOutStream *crcOutStreamSpec = new CCrcOutStream;
    CMyComPtr<ISequentialOutStream> crcOutStream = crcOutStreamSpec;
    
    UInt64 decodeTime;
    for (int j = 0; j < 2; j++)
    {
      inStreamSpec->Init(outStreamSpec->Buffer, compressedSize);
      crcOutStreamSpec->Init();
      
      if (decoderSpec->SetDecoderProperties2(propStreamSpec->Buffer, propStreamSpec->Pos) != S_OK)
      {
        fprintf(f, "\nError: Set Decoder Properties Error\n");
        return 1;
      }
      UInt64 outSize = kBufferSize;
      UInt64 startTime = ::GetTimeCount();
      result = decoder->Code(inStream, crcOutStream, 0, &outSize, 0);
      decodeTime = ::GetTimeCount() - startTime;
      if(result != S_OK)
      {
        ThrowError(f, result, "Decode Error");
        return 1;
      }
      if (crcOutStreamSpec->CRC.GetDigest() != crc.GetDigest())
      {
        fprintf(f, "\nError: CRC Error\n");
        return 1;
      }
    }
    UInt64 benchSize = kBufferSize - progressInfoSpec->InSize;
    PrintResults(f, dictionarySize, encodeTime, benchSize, false, 0);
    fprintf(f, "     ");
    PrintResults(f, dictionarySize, decodeTime, kBufferSize, true, compressedSize);
    fprintf(f, "\n");

    totalBenchSize += benchSize;
    totalEncodeTime += encodeTime;
    totalDecodeTime += decodeTime;
    totalCompressedSize += compressedSize;
  }
  fprintf(f, "---------------------------------------------------\n");
  PrintResults(f, dictionarySize, totalEncodeTime, totalBenchSize, false, 0);
  fprintf(f, "     ");
  PrintResults(f, dictionarySize, totalDecodeTime, 
      kBufferSize * numIterations, true, totalCompressedSize);
  fprintf(f, "    Average\n");
  return 0;
}
