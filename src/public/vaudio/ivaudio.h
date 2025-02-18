//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IVAUDIO_H
#define IVAUDIO_H
#ifdef _WIN32
#pragma once
#endif

class IAudioStreamEvent
{
public:
	// called by the stream to request more data
	// seek the source to position "offset"
	// -1 indicates previous position
	// copy the data to pBuffer and return the number of bytes copied
	// you may return less than bytesRequested if the end of the stream
	// is encountered.
	virtual int StreamRequestData( void *pBuffer, int bytesRequested, int offset ) = 0;
};


class IAudioStream
{
public:
	virtual ~IAudioStream() {}

	// Decode another bufferSize output bytes from the stream
	// returns number of bytes decoded
	virtual int	Decode( void *pBuffer, unsigned int bufferSize ) = 0;

	// output sampling bits (8/16)
	virtual int GetOutputBits() = 0;
	// output sampling rate in Hz
	virtual int GetOutputRate() = 0;
	// output channels (1=mono,2=stereo)
	virtual int GetOutputChannels() = 0;

	// seek
	virtual unsigned int GetPosition() = 0;

	// NOTE: BUGBUG: Only supports seeking forward currently!
	virtual void SetPosition( unsigned int position ) = 0;

	// reset?
};


#define VAUDIO_INTERFACE_VERSION		"VAudio002"
class IVAudio
{
public:
	virtual ~IVAudio() {}

	virtual IAudioStream	*CreateMP3StreamDecoder( IAudioStreamEvent *pEventHandler ) = 0;
	virtual void			DestroyMP3StreamDecoder( IAudioStream *pDecoder ) = 0;
};


#endif // IVAUDIO_H
