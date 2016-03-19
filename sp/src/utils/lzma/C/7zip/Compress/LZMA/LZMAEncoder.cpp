// LZMA/Encoder.cpp

#include "StdAfx.h"

#include "../../../Common/Defs.h"
#include "../../Common/StreamUtils.h"

#include "LZMAEncoder.h"

// for minimal compressing code size define these:
// #define COMPRESS_MF_BT
// #define COMPRESS_MF_BT4

#if !defined(COMPRESS_MF_BT) && !defined(COMPRESS_MF_HC)
#define COMPRESS_MF_BT
#define COMPRESS_MF_HC
#endif

#ifdef COMPRESS_MF_BT
#if !defined(COMPRESS_MF_BT2) && !defined(COMPRESS_MF_BT3) && !defined(COMPRESS_MF_BT4)
#define COMPRESS_MF_BT2
#define COMPRESS_MF_BT3
#define COMPRESS_MF_BT4
#endif
#ifdef COMPRESS_MF_BT2
#include "../LZ/BinTree/BinTree2.h"
#endif
#ifdef COMPRESS_MF_BT3
#include "../LZ/BinTree/BinTree3.h"
#endif
#ifdef COMPRESS_MF_BT4
#include "../LZ/BinTree/BinTree4.h"
#endif
#endif

#ifdef COMPRESS_MF_HC
#include "../LZ/HashChain/HC4.h"
#endif

#ifdef COMPRESS_MF_MT
#include "../LZ/MT/MT.h"
#endif

namespace NCompress {
namespace NLZMA {

const int kDefaultDictionaryLogSize = 22;
const UInt32 kNumFastBytesDefault = 0x20;

enum 
{
  kBT2,
  kBT3,
  kBT4,
  kHC4
};

static const wchar_t *kMatchFinderIDs[] = 
{
  L"BT2",
  L"BT3",
  L"BT4",
  L"HC4"
};

Byte g_FastPos[1 << 11];

class CFastPosInit
{
public:
  CFastPosInit() { Init(); }
  void Init()
  {
    const Byte kFastSlots = 22;
    int c = 2;
    g_FastPos[0] = 0;
    g_FastPos[1] = 1;

    for (Byte slotFast = 2; slotFast < kFastSlots; slotFast++)
    {
      UInt32 k = (1 << ((slotFast >> 1) - 1));
      for (UInt32 j = 0; j < k; j++, c++)
        g_FastPos[c] = slotFast;
    }
  }
} g_FastPosInit;


void CLiteralEncoder2::Encode(NRangeCoder::CEncoder *rangeEncoder, Byte symbol)
{
  UInt32 context = 1;
  int i = 8;
  do 
  {
    i--;
    UInt32 bit = (symbol >> i) & 1;
    _encoders[context].Encode(rangeEncoder, bit);
    context = (context << 1) | bit;
  }
  while(i != 0);
}

void CLiteralEncoder2::EncodeMatched(NRangeCoder::CEncoder *rangeEncoder, 
    Byte matchByte, Byte symbol)
{
  UInt32 context = 1;
  int i = 8;
  do 
  {
    i--;
    UInt32 bit = (symbol >> i) & 1;
    UInt32 matchBit = (matchByte >> i) & 1;
    _encoders[0x100 + (matchBit << 8) + context].Encode(rangeEncoder, bit);
    context = (context << 1) | bit;
    if (matchBit != bit)
    {
      while(i != 0)
      {
        i--;
        UInt32 bit = (symbol >> i) & 1;
        _encoders[context].Encode(rangeEncoder, bit);
        context = (context << 1) | bit;
      }
      break;
    }
  }
  while(i != 0);
}

UInt32 CLiteralEncoder2::GetPrice(bool matchMode, Byte matchByte, Byte symbol) const
{
  UInt32 price = 0;
  UInt32 context = 1;
  int i = 8;
  if (matchMode)
  {
    do 
    {
      i--;
      UInt32 matchBit = (matchByte >> i) & 1;
      UInt32 bit = (symbol >> i) & 1;
      price += _encoders[0x100 + (matchBit << 8) + context].GetPrice(bit);
      context = (context << 1) | bit;
      if (matchBit != bit)
        break;
    }
    while (i != 0);
  }
  while(i != 0)
  {
    i--;
    UInt32 bit = (symbol >> i) & 1;
    price += _encoders[context].GetPrice(bit);
    context = (context << 1) | bit;
  }
  return price;
};


namespace NLength {

void CEncoder::Init(UInt32 numPosStates)
{
  _choice.Init();
  _choice2.Init();
  for (UInt32 posState = 0; posState < numPosStates; posState++)
  {
    _lowCoder[posState].Init();
    _midCoder[posState].Init();
  }
  _highCoder.Init();
}

void CEncoder::Encode(NRangeCoder::CEncoder *rangeEncoder, UInt32 symbol, UInt32 posState)
{
  if(symbol < kNumLowSymbols)
  {
    _choice.Encode(rangeEncoder, 0);
    _lowCoder[posState].Encode(rangeEncoder, symbol);
  }
  else
  {
    _choice.Encode(rangeEncoder, 1);
    if(symbol < kNumLowSymbols + kNumMidSymbols)
    {
      _choice2.Encode(rangeEncoder, 0);
      _midCoder[posState].Encode(rangeEncoder, symbol - kNumLowSymbols);
    }
    else
    {
      _choice2.Encode(rangeEncoder, 1);
      _highCoder.Encode(rangeEncoder, symbol - kNumLowSymbols - kNumMidSymbols);
    }
  }
}

void CEncoder::SetPrices(UInt32 posState, UInt32 numSymbols, UInt32 *prices) const
{
  UInt32 a0 = _choice.GetPrice0();
  UInt32 a1 = _choice.GetPrice1();
  UInt32 b0 = a1 + _choice2.GetPrice0();
  UInt32 b1 = a1 + _choice2.GetPrice1();
  UInt32 i = 0;
  for (i = 0; i < kNumLowSymbols; i++)
  {
    if (i >= numSymbols)
      return;
    prices[i] = a0 + _lowCoder[posState].GetPrice(i);
  }
  for (; i < kNumLowSymbols + kNumMidSymbols; i++)
  {
    if (i >= numSymbols)
      return;
    prices[i] = b0 + _midCoder[posState].GetPrice(i - kNumLowSymbols);
  }
  for (; i < numSymbols; i++)
    prices[i] = b1 + _highCoder.GetPrice(i - kNumLowSymbols - kNumMidSymbols);
}

}
CEncoder::CEncoder():
  _numFastBytes(kNumFastBytesDefault),
  _distTableSize(kDefaultDictionaryLogSize * 2),
  _posStateBits(2),
  _posStateMask(4 - 1),
  _numLiteralPosStateBits(0),
  _numLiteralContextBits(3),
  _dictionarySize(1 << kDefaultDictionaryLogSize),
  _dictionarySizePrev(UInt32(-1)),
  _numFastBytesPrev(UInt32(-1)),
  _matchFinderCycles(0),
  _matchFinderIndex(kBT4),
   #ifdef COMPRESS_MF_MT
  _multiThread(false),
   #endif
  _writeEndMark(false),
  setMfPasses(0)
{
  // _maxMode = false;
  _fastMode = false;
}

HRESULT CEncoder::Create()
{
  if (!_rangeEncoder.Create(1 << 20))
    return E_OUTOFMEMORY;
  if (!_matchFinder)
  {
    switch(_matchFinderIndex)
    {
      #ifdef COMPRESS_MF_BT
      #ifdef COMPRESS_MF_BT2
      case kBT2:
      {
        NBT2::CMatchFinder *mfSpec = new NBT2::CMatchFinder;
        setMfPasses = mfSpec;
        _matchFinder = mfSpec;
        break;
      }
      #endif
      #ifdef COMPRESS_MF_BT3
      case kBT3:
      {
        NBT3::CMatchFinder *mfSpec = new NBT3::CMatchFinder;
        setMfPasses = mfSpec;
        _matchFinder = mfSpec;
        break;
      }
      #endif
      #ifdef COMPRESS_MF_BT4
      case kBT4:
      {
        NBT4::CMatchFinder *mfSpec = new NBT4::CMatchFinder;
        setMfPasses = mfSpec;
        _matchFinder = mfSpec;
        break;
      }
      #endif
      #endif
      
      #ifdef COMPRESS_MF_HC
      case kHC4:
      {
        NHC4::CMatchFinder *mfSpec = new NHC4::CMatchFinder;
        setMfPasses = mfSpec;
        _matchFinder = mfSpec;
        break;
      }
      #endif
    }
    if (_matchFinder == 0)
      return E_OUTOFMEMORY;

    #ifdef COMPRESS_MF_MT
    if (_multiThread && !(_fastMode && (_matchFinderIndex == kHC4)))
    {
      CMatchFinderMT *mfSpec = new CMatchFinderMT;
      if (mfSpec == 0)
        return E_OUTOFMEMORY;
      CMyComPtr<IMatchFinder> mf = mfSpec;
      RINOK(mfSpec->SetMatchFinder(_matchFinder));
      _matchFinder.Release();
      _matchFinder = mf;
    }
    #endif
  }
  
  if (!_literalEncoder.Create(_numLiteralPosStateBits, _numLiteralContextBits))
    return E_OUTOFMEMORY;

  if (_dictionarySize == _dictionarySizePrev && _numFastBytesPrev == _numFastBytes)
    return S_OK;
  RINOK(_matchFinder->Create(_dictionarySize, kNumOpts, _numFastBytes, kMatchMaxLen + 1)); // actually it's + _numFastBytes - _numFastBytes
  if (_matchFinderCycles != 0 && setMfPasses != 0)
    setMfPasses->SetNumPasses(_matchFinderCycles);
  _dictionarySizePrev = _dictionarySize;
  _numFastBytesPrev = _numFastBytes;
  return S_OK;
}

static bool AreStringsEqual(const wchar_t *base, const wchar_t *testString)
{
  while (true)
  {
    wchar_t c = *testString;
    if (c >= 'a' && c <= 'z')
      c -= 0x20;
    if (*base != c)
      return false;
    if (c == 0)
      return true;
    base++;
    testString++;
  }
}

static int FindMatchFinder(const wchar_t *s)
{
  for (int m = 0; m < (int)(sizeof(kMatchFinderIDs) / sizeof(kMatchFinderIDs[0])); m++)
    if (AreStringsEqual(kMatchFinderIDs[m], s))
      return m;
  return -1;
}

STDMETHODIMP CEncoder::SetCoderProperties(const PROPID *propIDs, 
    const PROPVARIANT *properties, UInt32 numProperties)
{
  for (UInt32 i = 0; i < numProperties; i++)
  {
    const PROPVARIANT &prop = properties[i];
    switch(propIDs[i])
    {
      case NCoderPropID::kNumFastBytes:
      {
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        UInt32 numFastBytes = prop.ulVal;
        if(numFastBytes < 5 || numFastBytes > kMatchMaxLen)
          return E_INVALIDARG;
        _numFastBytes = numFastBytes;
        break;
      }
      case NCoderPropID::kMatchFinderCycles:
      {
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        _matchFinderCycles = prop.ulVal;
        break;
      }
      case NCoderPropID::kAlgorithm:
      {
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        UInt32 maximize = prop.ulVal;
        _fastMode = (maximize == 0); 
        // _maxMode = (maximize >= 2);
        break;
      }
      case NCoderPropID::kMatchFinder:
      {
        if (prop.vt != VT_BSTR)
          return E_INVALIDARG;
        int matchFinderIndexPrev = _matchFinderIndex;
        int m = FindMatchFinder(prop.bstrVal);
        if (m < 0)
          return E_INVALIDARG;
        _matchFinderIndex = m;
        if (_matchFinder && matchFinderIndexPrev != _matchFinderIndex)
        {
          _dictionarySizePrev = (UInt32)-1;
          ReleaseMatchFinder();
        }
        break;
      }
      #ifdef COMPRESS_MF_MT
      case NCoderPropID::kMultiThread:
      {
        if (prop.vt != VT_BOOL)
          return E_INVALIDARG;
        bool newMultiThread = (prop.boolVal == VARIANT_TRUE);
        if (newMultiThread != _multiThread)
        {
          _dictionarySizePrev = (UInt32)-1;
          ReleaseMatchFinder();
          _multiThread = newMultiThread;
        }
        break;
      }
      case NCoderPropID::kNumThreads:
      {
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        bool newMultiThread = (prop.ulVal > 1);
        if (newMultiThread != _multiThread)
        {
          _dictionarySizePrev = (UInt32)-1;
          ReleaseMatchFinder();
          _multiThread = newMultiThread;
        }
        break;
      }
      #endif
      case NCoderPropID::kDictionarySize:
      {
        const int kDicLogSizeMaxCompress = 30;
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        UInt32 dictionarySize = prop.ulVal;
        if (dictionarySize < UInt32(1 << kDicLogSizeMin) ||
            dictionarySize > UInt32(1 << kDicLogSizeMaxCompress))
          return E_INVALIDARG;
        _dictionarySize = dictionarySize;
        UInt32 dicLogSize;
        for(dicLogSize = 0; dicLogSize < (UInt32)kDicLogSizeMaxCompress; dicLogSize++)
          if (dictionarySize <= (UInt32(1) << dicLogSize))
            break;
        _distTableSize = dicLogSize * 2;
        break;
      }
      case NCoderPropID::kPosStateBits:
      {
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        UInt32 value = prop.ulVal;
        if (value > (UInt32)NLength::kNumPosStatesBitsEncodingMax)
          return E_INVALIDARG;
        _posStateBits = value;
        _posStateMask = (1 << _posStateBits) - 1;
        break;
      }
      case NCoderPropID::kLitPosBits:
      {
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        UInt32 value = prop.ulVal;
        if (value > (UInt32)kNumLitPosStatesBitsEncodingMax)
          return E_INVALIDARG;
        _numLiteralPosStateBits = value;
        break;
      }
      case NCoderPropID::kLitContextBits:
      {
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        UInt32 value = prop.ulVal;
        if (value > (UInt32)kNumLitContextBitsMax)
          return E_INVALIDARG;
        _numLiteralContextBits = value;
        break;
      }
      case NCoderPropID::kEndMarker:
      {
        if (prop.vt != VT_BOOL)
          return E_INVALIDARG;
        SetWriteEndMarkerMode(prop.boolVal == VARIANT_TRUE);
        break;
      }
      default:
        return E_INVALIDARG;
    }
  }
  return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
{ 
  const UInt32 kPropSize = 5;
  Byte properties[kPropSize];
  properties[0] = (_posStateBits * 5 + _numLiteralPosStateBits) * 9 + _numLiteralContextBits;
  for (int i = 0; i < 4; i++)
    properties[1 + i] = Byte(_dictionarySize >> (8 * i));
  return WriteStream(outStream, properties, kPropSize, NULL);
}

STDMETHODIMP CEncoder::SetOutStream(ISequentialOutStream *outStream)
{
  _rangeEncoder.SetStream(outStream);
  return S_OK;
}

STDMETHODIMP CEncoder::ReleaseOutStream()
{
  _rangeEncoder.ReleaseStream();
  return S_OK;
}

HRESULT CEncoder::Init()
{
  CBaseState::Init();

  // RINOK(_matchFinder->Init(inStream));
  _rangeEncoder.Init();

  for(int i = 0; i < kNumStates; i++)
  {
    for (UInt32 j = 0; j <= _posStateMask; j++)
    {
      _isMatch[i][j].Init();
      _isRep0Long[i][j].Init();
    }
    _isRep[i].Init();
    _isRepG0[i].Init();
    _isRepG1[i].Init();
    _isRepG2[i].Init();
  }

  _literalEncoder.Init();

  {
    for(UInt32 i = 0; i < kNumLenToPosStates; i++)
      _posSlotEncoder[i].Init();
  }
  {
    for(UInt32 i = 0; i < kNumFullDistances - kEndPosModelIndex; i++)
      _posEncoders[i].Init();
  }

  _lenEncoder.Init(1 << _posStateBits);
  _repMatchLenEncoder.Init(1 << _posStateBits);

  _posAlignEncoder.Init();

  _longestMatchWasFound = false;
  _optimumEndIndex = 0;
  _optimumCurrentIndex = 0;
  _additionalOffset = 0;

  return S_OK;
}

HRESULT CEncoder::MovePos(UInt32 num)
{
  if (num == 0)
    return S_OK;
  _additionalOffset += num;
  return _matchFinder->Skip(num);
}

UInt32 CEncoder::Backward(UInt32 &backRes, UInt32 cur)
{
  _optimumEndIndex = cur;
  UInt32 posMem = _optimum[cur].PosPrev;
  UInt32 backMem = _optimum[cur].BackPrev;
  do
  {
    if (_optimum[cur].Prev1IsChar)
    {
      _optimum[posMem].MakeAsChar();
      _optimum[posMem].PosPrev = posMem - 1;
      if (_optimum[cur].Prev2)
      {
        _optimum[posMem - 1].Prev1IsChar = false;
        _optimum[posMem - 1].PosPrev = _optimum[cur].PosPrev2;
        _optimum[posMem - 1].BackPrev = _optimum[cur].BackPrev2;
      }
    }
    UInt32 posPrev = posMem;
    UInt32 backCur = backMem;

    backMem = _optimum[posPrev].BackPrev;
    posMem = _optimum[posPrev].PosPrev;

    _optimum[posPrev].BackPrev = backCur;
    _optimum[posPrev].PosPrev = cur;
    cur = posPrev;
  }
  while(cur != 0);
  backRes = _optimum[0].BackPrev;
  _optimumCurrentIndex  = _optimum[0].PosPrev;
  return _optimumCurrentIndex; 
}

/*
Out:
  (lenRes == 1) && (backRes == 0xFFFFFFFF) means Literal
*/

HRESULT CEncoder::GetOptimum(UInt32 position, UInt32 &backRes, UInt32 &lenRes)
{
  if(_optimumEndIndex != _optimumCurrentIndex)
  {
    const COptimal &optimum = _optimum[_optimumCurrentIndex];
    lenRes = optimum.PosPrev - _optimumCurrentIndex;
    backRes = optimum.BackPrev;
    _optimumCurrentIndex = optimum.PosPrev;
    return S_OK;
  }
  _optimumCurrentIndex = _optimumEndIndex = 0;
  
  UInt32 lenMain, numDistancePairs;
  if (!_longestMatchWasFound)
  {
    RINOK(ReadMatchDistances(lenMain, numDistancePairs));
  }
  else
  {
    lenMain = _longestMatchLength;
    numDistancePairs = _numDistancePairs;
    _longestMatchWasFound = false;
  }

  const Byte *data = _matchFinder->GetPointerToCurrentPos() - 1;
  UInt32 numAvailableBytes = _matchFinder->GetNumAvailableBytes() + 1;
  if (numAvailableBytes < 2)
  {
    backRes = (UInt32)(-1);
    lenRes = 1;
    return S_OK;
  }
  if (numAvailableBytes > kMatchMaxLen)
    numAvailableBytes = kMatchMaxLen;

  UInt32 reps[kNumRepDistances];
  UInt32 repLens[kNumRepDistances];
  UInt32 repMaxIndex = 0;
  UInt32 i;
  for(i = 0; i < kNumRepDistances; i++)
  {
    reps[i] = _repDistances[i];
    UInt32 backOffset = reps[i] + 1;
    if (data[0] != data[(size_t)0 - backOffset] || data[1] != data[(size_t)1 - backOffset])
    {
      repLens[i] = 0;
      continue;
    }
    UInt32 lenTest;
    for (lenTest = 2; lenTest < numAvailableBytes && 
        data[lenTest] == data[(size_t)lenTest - backOffset]; lenTest++);
    repLens[i] = lenTest;
    if (lenTest > repLens[repMaxIndex])
      repMaxIndex = i;
  }
  if(repLens[repMaxIndex] >= _numFastBytes)
  {
    backRes = repMaxIndex;
    lenRes = repLens[repMaxIndex];
    return MovePos(lenRes - 1);
  }

  UInt32 *matchDistances = _matchDistances + 1;
  if(lenMain >= _numFastBytes)
  {
    backRes = matchDistances[numDistancePairs - 1] + kNumRepDistances; 
    lenRes = lenMain;
    return MovePos(lenMain - 1);
  }
  Byte currentByte = *data;
  Byte matchByte = data[(size_t)0 - reps[0] - 1];

  if(lenMain < 2 && currentByte != matchByte && repLens[repMaxIndex] < 2)
  {
    backRes = (UInt32)-1;
    lenRes = 1;
    return S_OK;
  }

  _optimum[0].State = _state;

  UInt32 posState = (position & _posStateMask);

  _optimum[1].Price = _isMatch[_state.Index][posState].GetPrice0() + 
      _literalEncoder.GetSubCoder(position, _previousByte)->GetPrice(!_state.IsCharState(), matchByte, currentByte);
  _optimum[1].MakeAsChar();

  UInt32 matchPrice = _isMatch[_state.Index][posState].GetPrice1();
  UInt32 repMatchPrice = matchPrice + _isRep[_state.Index].GetPrice1();

  if(matchByte == currentByte)
  {
    UInt32 shortRepPrice = repMatchPrice + GetRepLen1Price(_state, posState);
    if(shortRepPrice < _optimum[1].Price)
    {
      _optimum[1].Price = shortRepPrice;
      _optimum[1].MakeAsShortRep();
    }
  }
  UInt32 lenEnd = ((lenMain >= repLens[repMaxIndex]) ? lenMain : repLens[repMaxIndex]);

  if(lenEnd < 2)
  {
    backRes = _optimum[1].BackPrev;
    lenRes = 1;
    return S_OK;
  }

  _optimum[1].PosPrev = 0;
  for (i = 0; i < kNumRepDistances; i++)
    _optimum[0].Backs[i] = reps[i];

  UInt32 len = lenEnd;
  do
    _optimum[len--].Price = kIfinityPrice;
  while (len >= 2);

  for(i = 0; i < kNumRepDistances; i++)
  {
    UInt32 repLen = repLens[i];
    if (repLen < 2)
      continue;
    UInt32 price = repMatchPrice + GetPureRepPrice(i, _state, posState);
    do
    {
      UInt32 curAndLenPrice = price + _repMatchLenEncoder.GetPrice(repLen - 2, posState);
      COptimal &optimum = _optimum[repLen];
      if (curAndLenPrice < optimum.Price) 
      {
        optimum.Price = curAndLenPrice;
        optimum.PosPrev = 0;
        optimum.BackPrev = i;
        optimum.Prev1IsChar = false;
      }
    }
    while(--repLen >= 2);
  }

  UInt32 normalMatchPrice = matchPrice + _isRep[_state.Index].GetPrice0();

  len = ((repLens[0] >= 2) ? repLens[0] + 1 : 2);
  if (len <= lenMain)
  {
    UInt32 offs = 0;
    while (len > matchDistances[offs])
      offs += 2;
    for(; ; len++)
    {
      UInt32 distance = matchDistances[offs + 1];
      UInt32 curAndLenPrice = normalMatchPrice + GetPosLenPrice(distance, len, posState);
      COptimal &optimum = _optimum[len];
      if (curAndLenPrice < optimum.Price) 
      {
        optimum.Price = curAndLenPrice;
        optimum.PosPrev = 0;
        optimum.BackPrev = distance + kNumRepDistances;
        optimum.Prev1IsChar = false;
      }
      if (len == matchDistances[offs])
      {
        offs += 2;
        if (offs == numDistancePairs)
          break;
      }
    }
  }

  UInt32 cur = 0;

  while(true)
  {
    cur++;
    if(cur == lenEnd)
    {
      lenRes = Backward(backRes, cur);
      return S_OK;
    }
    UInt32 newLen, numDistancePairs;
    RINOK(ReadMatchDistances(newLen, numDistancePairs));
    if(newLen >= _numFastBytes)
    {
      _numDistancePairs = numDistancePairs;
      _longestMatchLength = newLen;
      _longestMatchWasFound = true;
      lenRes = Backward(backRes, cur);
      return S_OK;
    }
    position++;
    COptimal &curOptimum = _optimum[cur];
    UInt32 posPrev = curOptimum.PosPrev;
    CState state;
    if (curOptimum.Prev1IsChar)
    {
      posPrev--;
      if (curOptimum.Prev2)
      {
        state = _optimum[curOptimum.PosPrev2].State;
        if (curOptimum.BackPrev2 < kNumRepDistances)
          state.UpdateRep();
        else
          state.UpdateMatch();
      }
      else
        state = _optimum[posPrev].State;
      state.UpdateChar();
    }
    else
      state = _optimum[posPrev].State;
    if (posPrev == cur - 1)
    {
      if (curOptimum.IsShortRep())
        state.UpdateShortRep();
      else
        state.UpdateChar();
    }
    else
    {
      UInt32 pos;
      if (curOptimum.Prev1IsChar && curOptimum.Prev2)
      {
        posPrev = curOptimum.PosPrev2;
        pos = curOptimum.BackPrev2;
        state.UpdateRep();
      }
      else
      {
        pos = curOptimum.BackPrev;
        if (pos < kNumRepDistances)
          state.UpdateRep();
        else
          state.UpdateMatch();
      }
      const COptimal &prevOptimum = _optimum[posPrev];
      if (pos < kNumRepDistances)
      {
        reps[0] = prevOptimum.Backs[pos];
    		UInt32 i;
        for(i = 1; i <= pos; i++)
          reps[i] = prevOptimum.Backs[i - 1];
        for(; i < kNumRepDistances; i++)
          reps[i] = prevOptimum.Backs[i];
      }
      else
      {
        reps[0] = (pos - kNumRepDistances);
        for(UInt32 i = 1; i < kNumRepDistances; i++)
          reps[i] = prevOptimum.Backs[i - 1];
      }
    }
    curOptimum.State = state;
    for(UInt32 i = 0; i < kNumRepDistances; i++)
      curOptimum.Backs[i] = reps[i];
    UInt32 curPrice = curOptimum.Price; 
    const Byte *data = _matchFinder->GetPointerToCurrentPos() - 1;
    const Byte currentByte = *data;
    const Byte matchByte = data[(size_t)0 - reps[0] - 1];

    UInt32 posState = (position & _posStateMask);

    UInt32 curAnd1Price = curPrice +
        _isMatch[state.Index][posState].GetPrice0() +
        _literalEncoder.GetSubCoder(position, data[(size_t)0 - 1])->GetPrice(!state.IsCharState(), matchByte, currentByte);

    COptimal &nextOptimum = _optimum[cur + 1];

    bool nextIsChar = false;
    if (curAnd1Price < nextOptimum.Price) 
    {
      nextOptimum.Price = curAnd1Price;
      nextOptimum.PosPrev = cur;
      nextOptimum.MakeAsChar();
      nextIsChar = true;
    }

    UInt32 matchPrice = curPrice + _isMatch[state.Index][posState].GetPrice1();
    UInt32 repMatchPrice = matchPrice + _isRep[state.Index].GetPrice1();
    
    if(matchByte == currentByte &&
        !(nextOptimum.PosPrev < cur && nextOptimum.BackPrev == 0))
    {
      UInt32 shortRepPrice = repMatchPrice + GetRepLen1Price(state, posState);
      if(shortRepPrice <= nextOptimum.Price)
      {
        nextOptimum.Price = shortRepPrice;
        nextOptimum.PosPrev = cur;
        nextOptimum.MakeAsShortRep();
        nextIsChar = true;
      }
    }
    /*
    if(newLen == 2 && matchDistances[2] >= kDistLimit2) // test it maybe set 2000 ?
      continue;
    */

    UInt32 numAvailableBytesFull = _matchFinder->GetNumAvailableBytes() + 1;
    numAvailableBytesFull = MyMin(kNumOpts - 1 - cur, numAvailableBytesFull);
    UInt32 numAvailableBytes = numAvailableBytesFull;

    if (numAvailableBytes < 2)
      continue;
    if (numAvailableBytes > _numFastBytes)
      numAvailableBytes = _numFastBytes;
    if (!nextIsChar && matchByte != currentByte) // speed optimization
    {
      // try Literal + rep0
      UInt32 backOffset = reps[0] + 1;
      UInt32 limit = MyMin(numAvailableBytesFull, _numFastBytes + 1);
      UInt32 temp;
      for (temp = 1; temp < limit && 
          data[temp] == data[(size_t)temp - backOffset]; temp++);
      UInt32 lenTest2 = temp - 1;
      if (lenTest2 >= 2)
      {
        CState state2 = state;
        state2.UpdateChar();
        UInt32 posStateNext = (position + 1) & _posStateMask;
        UInt32 nextRepMatchPrice = curAnd1Price + 
            _isMatch[state2.Index][posStateNext].GetPrice1() +
            _isRep[state2.Index].GetPrice1();
        // for (; lenTest2 >= 2; lenTest2--)
        {
          UInt32 offset = cur + 1 + lenTest2;
          while(lenEnd < offset)
            _optimum[++lenEnd].Price = kIfinityPrice;
          UInt32 curAndLenPrice = nextRepMatchPrice + GetRepPrice(
              0, lenTest2, state2, posStateNext);
          COptimal &optimum = _optimum[offset];
          if (curAndLenPrice < optimum.Price) 
          {
            optimum.Price = curAndLenPrice;
            optimum.PosPrev = cur + 1;
            optimum.BackPrev = 0;
            optimum.Prev1IsChar = true;
            optimum.Prev2 = false;
          }
        }
      }
    }
    
    UInt32 startLen = 2; // speed optimization 
    for(UInt32 repIndex = 0; repIndex < kNumRepDistances; repIndex++)
    {
      // UInt32 repLen = _matchFinder->GetMatchLen(0 - 1, reps[repIndex], newLen); // test it;
      UInt32 backOffset = reps[repIndex] + 1;
      if (data[0] != data[(size_t)0 - backOffset] ||
          data[1] != data[(size_t)1 - backOffset])
        continue;
      UInt32 lenTest;
      for (lenTest = 2; lenTest < numAvailableBytes && 
          data[lenTest] == data[(size_t)lenTest - backOffset]; lenTest++);
      while(lenEnd < cur + lenTest)
        _optimum[++lenEnd].Price = kIfinityPrice;
      UInt32 lenTestTemp = lenTest;
      UInt32 price = repMatchPrice + GetPureRepPrice(repIndex, state, posState);
      do
      {
        UInt32 curAndLenPrice = price + _repMatchLenEncoder.GetPrice(lenTest - 2, posState);
        COptimal &optimum = _optimum[cur + lenTest];
        if (curAndLenPrice < optimum.Price) 
        {
          optimum.Price = curAndLenPrice;
          optimum.PosPrev = cur;
          optimum.BackPrev = repIndex;
          optimum.Prev1IsChar = false;
        }
      }
      while(--lenTest >= 2);
      lenTest = lenTestTemp;
      
      if (repIndex == 0)
        startLen = lenTest + 1;
        
      // if (_maxMode)
        {
          UInt32 lenTest2 = lenTest + 1;
          UInt32 limit = MyMin(numAvailableBytesFull, lenTest2 + _numFastBytes);
          for (; lenTest2 < limit && 
              data[lenTest2] == data[(size_t)lenTest2 - backOffset]; lenTest2++);
          lenTest2 -= lenTest + 1;
          if (lenTest2 >= 2)
          {
            CState state2 = state;
            state2.UpdateRep();
            UInt32 posStateNext = (position + lenTest) & _posStateMask;
            UInt32 curAndLenCharPrice = 
                price + _repMatchLenEncoder.GetPrice(lenTest - 2, posState) + 
                _isMatch[state2.Index][posStateNext].GetPrice0() +
                _literalEncoder.GetSubCoder(position + lenTest, data[(size_t)lenTest - 1])->GetPrice(
                true, data[(size_t)lenTest - backOffset], data[lenTest]);
            state2.UpdateChar();
            posStateNext = (position + lenTest + 1) & _posStateMask;
            UInt32 nextRepMatchPrice = curAndLenCharPrice + 
                _isMatch[state2.Index][posStateNext].GetPrice1() +
                _isRep[state2.Index].GetPrice1();
            
            // for(; lenTest2 >= 2; lenTest2--)
            {
              UInt32 offset = cur + lenTest + 1 + lenTest2;
              while(lenEnd < offset)
                _optimum[++lenEnd].Price = kIfinityPrice;
              UInt32 curAndLenPrice = nextRepMatchPrice + GetRepPrice(
                  0, lenTest2, state2, posStateNext);
              COptimal &optimum = _optimum[offset];
              if (curAndLenPrice < optimum.Price) 
              {
                optimum.Price = curAndLenPrice;
                optimum.PosPrev = cur + lenTest + 1;
                optimum.BackPrev = 0;
                optimum.Prev1IsChar = true;
                optimum.Prev2 = true;
                optimum.PosPrev2 = cur;
                optimum.BackPrev2 = repIndex;
              }
            }
          }
        }
      }
    
    //    for(UInt32 lenTest = 2; lenTest <= newLen; lenTest++)
    if (newLen > numAvailableBytes)
    {
      newLen = numAvailableBytes;
      for (numDistancePairs = 0; newLen > matchDistances[numDistancePairs]; numDistancePairs += 2);
      matchDistances[numDistancePairs] = newLen;
      numDistancePairs += 2;
    }
    if (newLen >= startLen)
    {
      UInt32 normalMatchPrice = matchPrice + _isRep[state.Index].GetPrice0();
      while(lenEnd < cur + newLen)
        _optimum[++lenEnd].Price = kIfinityPrice;

      UInt32 offs = 0;
      while(startLen > matchDistances[offs])
        offs += 2;
      UInt32 curBack = matchDistances[offs + 1];
      UInt32 posSlot = GetPosSlot2(curBack);
      for(UInt32 lenTest = /*2*/ startLen; ; lenTest++)
      {
        UInt32 curAndLenPrice = normalMatchPrice;
        UInt32 lenToPosState = GetLenToPosState(lenTest);
        if (curBack < kNumFullDistances)
          curAndLenPrice += _distancesPrices[lenToPosState][curBack];
        else
          curAndLenPrice += _posSlotPrices[lenToPosState][posSlot] + _alignPrices[curBack & kAlignMask];
  
        curAndLenPrice += _lenEncoder.GetPrice(lenTest - kMatchMinLen, posState);
        
        COptimal &optimum = _optimum[cur + lenTest];
        if (curAndLenPrice < optimum.Price) 
        {
          optimum.Price = curAndLenPrice;
          optimum.PosPrev = cur;
          optimum.BackPrev = curBack + kNumRepDistances;
          optimum.Prev1IsChar = false;
        }

        if (/*_maxMode && */lenTest == matchDistances[offs])
        {
          // Try Match + Literal + Rep0
          UInt32 backOffset = curBack + 1;
          UInt32 lenTest2 = lenTest + 1;
          UInt32 limit = MyMin(numAvailableBytesFull, lenTest2 + _numFastBytes);
          for (; lenTest2 < limit && 
              data[lenTest2] == data[(size_t)lenTest2 - backOffset]; lenTest2++);
          lenTest2 -= lenTest + 1;
          if (lenTest2 >= 2)
          {
            CState state2 = state;
            state2.UpdateMatch();
            UInt32 posStateNext = (position + lenTest) & _posStateMask;
            UInt32 curAndLenCharPrice = curAndLenPrice + 
                _isMatch[state2.Index][posStateNext].GetPrice0() +
                _literalEncoder.GetSubCoder(position + lenTest, data[(size_t)lenTest - 1])->GetPrice( 
                true, data[(size_t)lenTest - backOffset], data[lenTest]);
            state2.UpdateChar();
            posStateNext = (posStateNext + 1) & _posStateMask;
            UInt32 nextRepMatchPrice = curAndLenCharPrice + 
                _isMatch[state2.Index][posStateNext].GetPrice1() +
                _isRep[state2.Index].GetPrice1();
            
            // for(; lenTest2 >= 2; lenTest2--)
            {
              UInt32 offset = cur + lenTest + 1 + lenTest2;
              while(lenEnd < offset)
                _optimum[++lenEnd].Price = kIfinityPrice;
              UInt32 curAndLenPrice = nextRepMatchPrice + GetRepPrice(0, lenTest2, state2, posStateNext);
              COptimal &optimum = _optimum[offset];
              if (curAndLenPrice < optimum.Price) 
              {
                optimum.Price = curAndLenPrice;
                optimum.PosPrev = cur + lenTest + 1;
                optimum.BackPrev = 0;
                optimum.Prev1IsChar = true;
                optimum.Prev2 = true;
                optimum.PosPrev2 = cur;
                optimum.BackPrev2 = curBack + kNumRepDistances;
              }
            }
          }
          offs += 2;
          if (offs == numDistancePairs)
            break;
          curBack = matchDistances[offs + 1];
          if (curBack >= kNumFullDistances)
            posSlot = GetPosSlot2(curBack);
        }
      }
    }
  }
}

static inline bool ChangePair(UInt32 smallDist, UInt32 bigDist)
{
  return ((bigDist >> 7) > smallDist);
}


HRESULT CEncoder::ReadMatchDistances(UInt32 &lenRes, UInt32 &numDistancePairs)
{
  lenRes = 0;
  RINOK(_matchFinder->GetMatches(_matchDistances));
  numDistancePairs = _matchDistances[0];
  if (numDistancePairs > 0)
  {
    lenRes = _matchDistances[1 + numDistancePairs - 2];
    if (lenRes == _numFastBytes)
      lenRes += _matchFinder->GetMatchLen(lenRes - 1, _matchDistances[1 + numDistancePairs - 1], 
          kMatchMaxLen - lenRes);
  }
  _additionalOffset++;
  return S_OK;
}

HRESULT CEncoder::GetOptimumFast(UInt32 position, UInt32 &backRes, UInt32 &lenRes)
{
  UInt32 lenMain, numDistancePairs;
  if (!_longestMatchWasFound)
  {
    RINOK(ReadMatchDistances(lenMain, numDistancePairs));
  }
  else
  {
    lenMain = _longestMatchLength;
    numDistancePairs = _numDistancePairs;
    _longestMatchWasFound = false;
  }

  const Byte *data = _matchFinder->GetPointerToCurrentPos() - 1;
  UInt32 numAvailableBytes = _matchFinder->GetNumAvailableBytes() + 1;
  if (numAvailableBytes > kMatchMaxLen)
    numAvailableBytes = kMatchMaxLen;
  if (numAvailableBytes < 2)
  {
    backRes = (UInt32)(-1);
    lenRes = 1;
    return S_OK;
  }

  UInt32 repLens[kNumRepDistances];
  UInt32 repMaxIndex = 0;

  for(UInt32 i = 0; i < kNumRepDistances; i++)
  {
    UInt32 backOffset = _repDistances[i] + 1;
    if (data[0] != data[(size_t)0 - backOffset] || data[1] != data[(size_t)1 - backOffset])
    {
      repLens[i] = 0;
      continue;
    }
    UInt32 len;
    for (len = 2; len < numAvailableBytes && data[len] == data[(size_t)len - backOffset]; len++);
    if(len >= _numFastBytes)
    {
      backRes = i;
      lenRes = len;
      return MovePos(lenRes - 1);
    }
    repLens[i] = len;
    if (len > repLens[repMaxIndex])
      repMaxIndex = i;
  }
  UInt32 *matchDistances = _matchDistances + 1;
  if(lenMain >= _numFastBytes)
  {
    backRes = matchDistances[numDistancePairs - 1] + kNumRepDistances; 
    lenRes = lenMain;
    return MovePos(lenMain - 1);
  }

  UInt32 backMain = 0; // for GCC
  if (lenMain >= 2)
  {
    backMain = matchDistances[numDistancePairs - 1];
    while (numDistancePairs > 2 && lenMain == matchDistances[numDistancePairs - 4] + 1)
    {
      if (!ChangePair(matchDistances[numDistancePairs - 3], backMain))
        break;
      numDistancePairs -= 2;
      lenMain = matchDistances[numDistancePairs - 2];
      backMain = matchDistances[numDistancePairs - 1];
    }
    if (lenMain == 2 && backMain >= 0x80)
      lenMain = 1;
  }

  if (repLens[repMaxIndex] >= 2)
  {
    if (repLens[repMaxIndex] + 1 >= lenMain || 
        ( repLens[repMaxIndex] + 2 >= lenMain && (backMain > (1 << 9)) ) ||
        ( repLens[repMaxIndex] + 3 >= lenMain && (backMain > (1 << 15)) ) )
    {
      backRes = repMaxIndex;
      lenRes = repLens[repMaxIndex];
      return MovePos(lenRes - 1);
    }
  }
  
  if (lenMain >= 2 && numAvailableBytes > 2)
  {
    RINOK(ReadMatchDistances(_longestMatchLength, _numDistancePairs));
    if (_longestMatchLength >= 2)
    {
      UInt32 newDistance = matchDistances[_numDistancePairs - 1];
      if (( _longestMatchLength >= lenMain && newDistance < backMain ) || 
          ( _longestMatchLength == lenMain + 1 && !ChangePair(backMain, newDistance) ) ||
          ( _longestMatchLength > lenMain + 1 ) ||
          ( _longestMatchLength + 1 >= lenMain && lenMain >= 3 && ChangePair(newDistance, backMain) ) )
      {
        _longestMatchWasFound = true;
        backRes = UInt32(-1);
        lenRes = 1;
        return S_OK;
      }
    }
    data++;
    numAvailableBytes--;
    for(UInt32 i = 0; i < kNumRepDistances; i++)
    {
      UInt32 backOffset = _repDistances[i] + 1;
      if (data[1] != data[(size_t)1 - backOffset] || data[2] != data[(size_t)2 - backOffset])
      {
        repLens[i] = 0;
        continue;
      }
      UInt32 len;
      for (len = 2; len < numAvailableBytes && data[len] == data[(size_t)len - backOffset]; len++);
      if (len + 1 >= lenMain)
      {
        _longestMatchWasFound = true;
        backRes = UInt32(-1);
        lenRes = 1;
        return S_OK;
      }
    }
    backRes = backMain + kNumRepDistances; 
    lenRes = lenMain;
    return MovePos(lenMain - 2);
  }
  backRes = UInt32(-1);
  lenRes = 1;
  return S_OK;
}

HRESULT CEncoder::Flush(UInt32 nowPos)
{
  ReleaseMFStream();
  WriteEndMarker(nowPos & _posStateMask);
  _rangeEncoder.FlushData();
  return _rangeEncoder.FlushStream();
}

void CEncoder::WriteEndMarker(UInt32 posState)
{
  // This function for writing End Mark for stream version of LZMA. 
  // In current version this feature is not used.
  if (!_writeEndMark)
    return;

  _isMatch[_state.Index][posState].Encode(&_rangeEncoder, 1);
  _isRep[_state.Index].Encode(&_rangeEncoder, 0);
  _state.UpdateMatch();
  UInt32 len = kMatchMinLen; // kMatchMaxLen;
  _lenEncoder.Encode(&_rangeEncoder, len - kMatchMinLen, posState, !_fastMode);
  UInt32 posSlot = (1 << kNumPosSlotBits)  - 1;
  UInt32 lenToPosState = GetLenToPosState(len);
  _posSlotEncoder[lenToPosState].Encode(&_rangeEncoder, posSlot);
  UInt32 footerBits = 30;
  UInt32 posReduced = (UInt32(1) << footerBits) - 1;
  _rangeEncoder.EncodeDirectBits(posReduced >> kNumAlignBits, footerBits - kNumAlignBits);
  _posAlignEncoder.ReverseEncode(&_rangeEncoder, posReduced & kAlignMask);
}

HRESULT CEncoder::CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress)
{
  _needReleaseMFStream = false;
  CCoderReleaser coderReleaser(this);
  RINOK(SetStreams(inStream, outStream, inSize, outSize));
  while(true)
  {
    UInt64 processedInSize;
    UInt64 processedOutSize;
    Int32 finished;
    RINOK(CodeOneBlock(&processedInSize, &processedOutSize, &finished));
    if (finished != 0)
      return S_OK;
    if (progress != 0)
    {
      RINOK(progress->SetRatioInfo(&processedInSize, &processedOutSize));
    }
  }
}

HRESULT CEncoder::SetStreams(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UInt64 *inSize, const UInt64 *outSize)
{
  _inStream = inStream;
  _finished = false;
  RINOK(Create());
  RINOK(SetOutStream(outStream));
  RINOK(Init());
  
  // CCoderReleaser releaser(this);

  /*
  if (_matchFinder->GetNumAvailableBytes() == 0)
    return Flush();
  */

  if (!_fastMode)
  {
    FillDistancesPrices();
    FillAlignPrices();
  }

  _lenEncoder.SetTableSize(_numFastBytes + 1 - kMatchMinLen);
  _lenEncoder.UpdateTables(1 << _posStateBits);
  _repMatchLenEncoder.SetTableSize(_numFastBytes + 1 - kMatchMinLen);
  _repMatchLenEncoder.UpdateTables(1 << _posStateBits);

  nowPos64 = 0;
  return S_OK;
}

HRESULT CEncoder::CodeOneBlock(UInt64 *inSize, UInt64 *outSize, Int32 *finished)
{
  if (_inStream != 0)
  {
    RINOK(_matchFinder->SetStream(_inStream));
    RINOK(_matchFinder->Init());
    _needReleaseMFStream = true;
    _inStream = 0;
  }


  *finished = 1;
  if (_finished)
    return S_OK;
  _finished = true;

  if (nowPos64 == 0)
  {
    if (_matchFinder->GetNumAvailableBytes() == 0)
      return Flush(UInt32(nowPos64));
    UInt32 len, numDistancePairs;
    RINOK(ReadMatchDistances(len, numDistancePairs));
    UInt32 posState = UInt32(nowPos64) & _posStateMask;
    _isMatch[_state.Index][posState].Encode(&_rangeEncoder, 0);
    _state.UpdateChar();
    Byte curByte = _matchFinder->GetIndexByte(0 - _additionalOffset);
    _literalEncoder.GetSubCoder(UInt32(nowPos64), _previousByte)->Encode(&_rangeEncoder, curByte);
    _previousByte = curByte;
    _additionalOffset--;
    nowPos64++;
  }

  UInt32 nowPos32 = (UInt32)nowPos64;
  UInt32 progressPosValuePrev = nowPos32;

  if (_matchFinder->GetNumAvailableBytes() == 0)
    return Flush(nowPos32);

  while(true)
  {
    #ifdef _NO_EXCEPTIONS
    if (_rangeEncoder.Stream.ErrorCode != S_OK)
      return _rangeEncoder.Stream.ErrorCode;
    #endif
    UInt32 pos, len;
    HRESULT result;
    if (_fastMode)
      result = GetOptimumFast(nowPos32, pos, len);
    else
      result = GetOptimum(nowPos32, pos, len);
    RINOK(result);

    UInt32 posState = nowPos32 & _posStateMask;
    if(len == 1 && pos == 0xFFFFFFFF)
    {
      _isMatch[_state.Index][posState].Encode(&_rangeEncoder, 0);
      Byte curByte = _matchFinder->GetIndexByte(0 - _additionalOffset);
      CLiteralEncoder2 *subCoder = _literalEncoder.GetSubCoder(nowPos32, _previousByte);
      if(_state.IsCharState())
        subCoder->Encode(&_rangeEncoder, curByte);
      else
      {
        Byte matchByte = _matchFinder->GetIndexByte(0 - _repDistances[0] - 1 - _additionalOffset);
        subCoder->EncodeMatched(&_rangeEncoder, matchByte, curByte);
      }
      _state.UpdateChar();
      _previousByte = curByte;
    }
    else
    {
      _isMatch[_state.Index][posState].Encode(&_rangeEncoder, 1);
      if(pos < kNumRepDistances)
      {
        _isRep[_state.Index].Encode(&_rangeEncoder, 1);
        if(pos == 0)
        {
          _isRepG0[_state.Index].Encode(&_rangeEncoder, 0);
          _isRep0Long[_state.Index][posState].Encode(&_rangeEncoder, ((len == 1) ? 0 : 1));
        }
        else
        {
          UInt32 distance = _repDistances[pos];
          _isRepG0[_state.Index].Encode(&_rangeEncoder, 1);
          if (pos == 1)
            _isRepG1[_state.Index].Encode(&_rangeEncoder, 0);
          else
          {
            _isRepG1[_state.Index].Encode(&_rangeEncoder, 1);
            _isRepG2[_state.Index].Encode(&_rangeEncoder, pos - 2);
            if (pos == 3)
              _repDistances[3] = _repDistances[2];
            _repDistances[2] = _repDistances[1];
          }
          _repDistances[1] = _repDistances[0];
          _repDistances[0] = distance;
        }
        if (len == 1)
          _state.UpdateShortRep();
        else
        {
          _repMatchLenEncoder.Encode(&_rangeEncoder, len - kMatchMinLen, posState, !_fastMode);
          _state.UpdateRep();
        }
      }
      else
      {
        _isRep[_state.Index].Encode(&_rangeEncoder, 0);
        _state.UpdateMatch();
        _lenEncoder.Encode(&_rangeEncoder, len - kMatchMinLen, posState, !_fastMode);
        pos -= kNumRepDistances;
        UInt32 posSlot = GetPosSlot(pos);
        _posSlotEncoder[GetLenToPosState(len)].Encode(&_rangeEncoder, posSlot);
        
        if (posSlot >= kStartPosModelIndex)
        {
          UInt32 footerBits = ((posSlot >> 1) - 1);
          UInt32 base = ((2 | (posSlot & 1)) << footerBits);
          UInt32 posReduced = pos - base;

          if (posSlot < kEndPosModelIndex)
            NRangeCoder::ReverseBitTreeEncode(_posEncoders + base - posSlot - 1, 
                &_rangeEncoder, footerBits, posReduced);
          else
          {
            _rangeEncoder.EncodeDirectBits(posReduced >> kNumAlignBits, footerBits - kNumAlignBits);
            _posAlignEncoder.ReverseEncode(&_rangeEncoder, posReduced & kAlignMask);
            _alignPriceCount++;
          }
        }
        _repDistances[3] = _repDistances[2];
        _repDistances[2] = _repDistances[1];
        _repDistances[1] = _repDistances[0];
        _repDistances[0] = pos;
        _matchPriceCount++;
      }
      _previousByte = _matchFinder->GetIndexByte(len - 1 - _additionalOffset);
    }
    _additionalOffset -= len;
    nowPos32 += len;
    if (_additionalOffset == 0)
    {
      if (!_fastMode)
      {
        if (_matchPriceCount >= (1 << 7))
          FillDistancesPrices();
        if (_alignPriceCount >= kAlignTableSize)
          FillAlignPrices();
      }
      if (_matchFinder->GetNumAvailableBytes() == 0)
        return Flush(nowPos32);
      if (nowPos32 - progressPosValuePrev >= (1 << 14))
      {
        nowPos64 += nowPos32 - progressPosValuePrev;
        *inSize = nowPos64;
        *outSize = _rangeEncoder.GetProcessedSize();
        _finished = false;
        *finished = 0;
        return S_OK;
      }
    }
  }
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  #ifndef _NO_EXCEPTIONS
  try 
  { 
  #endif
    return CodeReal(inStream, outStream, inSize, outSize, progress); 
  #ifndef _NO_EXCEPTIONS
  }
  catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(...) { return E_FAIL; }
  #endif
}
  
void CEncoder::FillDistancesPrices()
{
  UInt32 tempPrices[kNumFullDistances];
  for (UInt32 i = kStartPosModelIndex; i < kNumFullDistances; i++)
  { 
    UInt32 posSlot = GetPosSlot(i);
    UInt32 footerBits = ((posSlot >> 1) - 1);
    UInt32 base = ((2 | (posSlot & 1)) << footerBits);
    tempPrices[i] = NRangeCoder::ReverseBitTreeGetPrice(_posEncoders + 
      base - posSlot - 1, footerBits, i - base);
  }

  for (UInt32 lenToPosState = 0; lenToPosState < kNumLenToPosStates; lenToPosState++)
  {
	  UInt32 posSlot;
    NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumPosSlotBits> &encoder = _posSlotEncoder[lenToPosState];
    UInt32 *posSlotPrices = _posSlotPrices[lenToPosState];
    for (posSlot = 0; posSlot < _distTableSize; posSlot++)
      posSlotPrices[posSlot] = encoder.GetPrice(posSlot);
    for (posSlot = kEndPosModelIndex; posSlot < _distTableSize; posSlot++)
      posSlotPrices[posSlot] += ((((posSlot >> 1) - 1) - kNumAlignBits) << NRangeCoder::kNumBitPriceShiftBits);

    UInt32 *distancesPrices = _distancesPrices[lenToPosState];
	  UInt32 i;
    for (i = 0; i < kStartPosModelIndex; i++)
      distancesPrices[i] = posSlotPrices[i];
    for (; i < kNumFullDistances; i++)
      distancesPrices[i] = posSlotPrices[GetPosSlot(i)] + tempPrices[i];
  }
  _matchPriceCount = 0;
}

void CEncoder::FillAlignPrices()
{
  for (UInt32 i = 0; i < kAlignTableSize; i++)
    _alignPrices[i] = _posAlignEncoder.ReverseGetPrice(i);
  _alignPriceCount = 0;
}

}}
