// LzmaRam.cpp

#include "StdAfx.h"
#include "../../../Common/Types.h"
#include "../LZMA/LZMADecoder.h"
#include "../LZMA/LZMAEncoder.h"
#include "LzmaRam.h"

extern "C"
{
#include "../Branch/BranchX86.h"
}

class CInStreamRam: 
  public ISequentialInStream,
  public CMyUnknownImp
{
  const Byte *Data;
  size_t Size;
  size_t Pos;
public:
  MY_UNKNOWN_IMP
  void Init(const Byte *data, size_t size)
  {
    Data = data;
    Size = size;
    Pos = 0;
  }
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
};

STDMETHODIMP CInStreamRam::Read(void *data, UInt32 size, UInt32 *processedSize)
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
  
class COutStreamRam: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
  size_t Size;
public:
  Byte *Data;
  size_t Pos;
  bool Overflow;
  void Init(Byte *data, size_t size)
  {
    Data = data;
    Size = size;
    Pos = 0;
    Overflow = false;
  }
  void SetPos(size_t pos)
  {
    Overflow = false;
    Pos = pos;
  }
  MY_UNKNOWN_IMP
  HRESULT WriteByte(Byte b)
  {
    if (Pos >= Size)
    {
      Overflow = true;
      return E_FAIL;
    }
    Data[Pos++] = b;
    return S_OK;
  }
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

STDMETHODIMP COutStreamRam::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 i;
  for (i = 0; i < size && Pos < Size; i++)
    Data[Pos++] = ((const Byte *)data)[i];
  if(processedSize != NULL)
    *processedSize = i;
  if (i != size)
  {
    Overflow = true;
    return E_FAIL;
  }
  return S_OK;
}
  
#define SZE_FAIL (1)
#define SZE_OUTOFMEMORY (2)
#define SZE_OUT_OVERFLOW (3)

int LzmaRamEncode(
    const Byte *inBuffer, size_t inSize, 
    Byte *outBuffer, size_t outSize, size_t *outSizeProcessed, 
    UInt32 dictionarySize, ESzFilterMode filterMode)
{
  #ifndef _NO_EXCEPTIONS
  try { 
  #endif

  *outSizeProcessed = 0;
  const size_t kIdSize = 1;
  const size_t kLzmaPropsSize = 5;
  const size_t kMinDestSize = kIdSize + kLzmaPropsSize + 8;
  if (outSize < kMinDestSize)
    return SZE_OUT_OVERFLOW;
  NCompress::NLZMA::CEncoder *encoderSpec = new NCompress::NLZMA::CEncoder;
  CMyComPtr<ICompressCoder> encoder = encoderSpec;

  PROPID propIDs[] = 
  { 
    NCoderPropID::kAlgorithm,
    NCoderPropID::kDictionarySize,  
    NCoderPropID::kNumFastBytes,
  };
  const int kNumProps = sizeof(propIDs) / sizeof(propIDs[0]);
  PROPVARIANT properties[kNumProps];
  properties[0].vt = VT_UI4;
  properties[1].vt = VT_UI4;
  properties[2].vt = VT_UI4;
  properties[0].ulVal = (UInt32)2;
  properties[1].ulVal = (UInt32)dictionarySize;
  properties[2].ulVal = (UInt32)64;

  if (encoderSpec->SetCoderProperties(propIDs, properties, kNumProps) != S_OK)
    return 1;
  
  COutStreamRam *outStreamSpec = new COutStreamRam;
  if (outStreamSpec == 0)
    return SZE_OUTOFMEMORY;
  CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
  CInStreamRam *inStreamSpec = new CInStreamRam;
  if (inStreamSpec == 0)
    return SZE_OUTOFMEMORY;
  CMyComPtr<ISequentialInStream> inStream = inStreamSpec;

  outStreamSpec->Init(outBuffer, outSize);
  if (outStreamSpec->WriteByte(0) != S_OK)
    return SZE_OUT_OVERFLOW;

  if (encoderSpec->WriteCoderProperties(outStream) != S_OK)
    return SZE_OUT_OVERFLOW;
  if (outStreamSpec->Pos != kIdSize + kLzmaPropsSize)
    return 1;
  
  int i;
  for (i = 0; i < 8; i++)
  {
    UInt64 t = (UInt64)(inSize);
    if (outStreamSpec->WriteByte((Byte)((t) >> (8 * i))) != S_OK)
      return SZE_OUT_OVERFLOW;
  }

  Byte *filteredStream = 0;

  bool useFilter = (filterMode != SZ_FILTER_NO);
  if (useFilter)
  {
    if (inSize != 0)
    {
      filteredStream = (Byte *)MyAlloc(inSize);
      if (filteredStream == 0)
        return SZE_OUTOFMEMORY;
      memmove(filteredStream, inBuffer, inSize);
    }
    UInt32 _prevMask;
    UInt32 _prevPos;
    x86_Convert_Init(_prevMask, _prevPos);
    x86_Convert(filteredStream, (UInt32)inSize, 0, &_prevMask, &_prevPos, 1);
  }
  
  UInt32 minSize = 0;
  int numPasses = (filterMode == SZ_FILTER_AUTO) ? 3 : 1;
  bool bestIsFiltered = false;
  int mainResult = 0;
  size_t startPos = outStreamSpec->Pos;
  for (i = 0; i < numPasses; i++)
  {
    if (numPasses > 1 && i == numPasses - 1 && !bestIsFiltered)
      break;
    outStreamSpec->SetPos(startPos);
    bool curModeIsFiltered = false;
    if (useFilter && i == 0)
      curModeIsFiltered = true;
    if (numPasses > 1 && i == numPasses - 1)
      curModeIsFiltered = true;

    inStreamSpec->Init(curModeIsFiltered ? filteredStream : inBuffer, inSize);
    
    HRESULT lzmaResult = encoder->Code(inStream, outStream, 0, 0, 0);
    
    mainResult = 0;
    if (lzmaResult == E_OUTOFMEMORY)
    {
      mainResult = SZE_OUTOFMEMORY;
      break;
    } 
    if (i == 0 || outStreamSpec->Pos <= minSize)
    {
      minSize = outStreamSpec->Pos;
      bestIsFiltered = curModeIsFiltered;
    }
    if (outStreamSpec->Overflow)
      mainResult = SZE_OUT_OVERFLOW;
    else if (lzmaResult != S_OK)
    {
      mainResult = SZE_FAIL;
      break;
    } 
  }
  *outSizeProcessed = outStreamSpec->Pos;
  if (bestIsFiltered)
    outBuffer[0] = 1;
  if (useFilter)
    MyFree(filteredStream);
  return mainResult;
  
  #ifndef _NO_EXCEPTIONS
  } catch(...) { return SZE_OUTOFMEMORY; }
  #endif
}
