// LZInWindow.cpp

#include "StdAfx.h"

#include "LZInWindow.h"
#include "../../../Common/MyCom.h"
#include "../../../Common/Alloc.h"

void CLZInWindow::Free()
{
  ::BigFree(_bufferBase);
  _bufferBase = 0;
}

bool CLZInWindow::Create(UInt32 keepSizeBefore, UInt32 keepSizeAfter, UInt32 keepSizeReserv)
{
  _keepSizeBefore = keepSizeBefore;
  _keepSizeAfter = keepSizeAfter;
  UInt32 blockSize = keepSizeBefore + keepSizeAfter + keepSizeReserv;
  if (_bufferBase == 0 || _blockSize != blockSize)
  {
    Free();
    _blockSize = blockSize;
    if (_blockSize != 0)
      _bufferBase = (Byte *)::BigAlloc(_blockSize);
  }
  _pointerToLastSafePosition = _bufferBase + _blockSize - keepSizeAfter;
  if (_blockSize == 0)
    return true;
  return (_bufferBase != 0);
}

void CLZInWindow::SetStream(ISequentialInStream *stream)
{
  _stream = stream;
}

HRESULT CLZInWindow::Init()
{
  _buffer = _bufferBase;
  _pos = 0;
  _streamPos = 0;
  _streamEndWasReached = false;
  return ReadBlock();
}

/*
void CLZInWindow::ReleaseStream()
{
  _stream.Release();
}
*/

///////////////////////////////////////////
// ReadBlock

// In State:
//   (_buffer + _streamPos) <= (_bufferBase + _blockSize)
// Out State:
//   _posLimit <= _blockSize - _keepSizeAfter;
//   if(_streamEndWasReached == false):
//     _streamPos >= _pos + _keepSizeAfter
//     _posLimit = _streamPos - _keepSizeAfter;
//   else
//          
  
HRESULT CLZInWindow::ReadBlock()
{
  if(_streamEndWasReached)
    return S_OK;
  while(true)
  {
    UInt32 size = (UInt32)(_bufferBase - _buffer) + _blockSize - _streamPos;
    if(size == 0)
      return S_OK;
    UInt32 numReadBytes;
    RINOK(_stream->Read(_buffer + _streamPos, size, &numReadBytes));
    if(numReadBytes == 0)
    {
      _posLimit = _streamPos;
      const Byte *pointerToPostion = _buffer + _posLimit;
      if(pointerToPostion > _pointerToLastSafePosition)
        _posLimit = (UInt32)(_pointerToLastSafePosition - _buffer);
      _streamEndWasReached = true;
      return S_OK;
    }
    _streamPos += numReadBytes;
    if(_streamPos >= _pos + _keepSizeAfter)
    {
      _posLimit = _streamPos - _keepSizeAfter;
      return S_OK;
    }
  }
}

void CLZInWindow::MoveBlock()
{
  UInt32 offset = (UInt32)(_buffer - _bufferBase) + _pos - _keepSizeBefore;
  // we need one additional byte, since MovePos moves on 1 byte.
  if (offset > 0)
    offset--; 
  UInt32 numBytes = (UInt32)(_buffer - _bufferBase) + _streamPos -  offset;
  memmove(_bufferBase, _bufferBase + offset, numBytes);
  _buffer -= offset;
}
