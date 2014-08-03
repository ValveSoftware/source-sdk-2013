//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef XWVFILE_H
#define XWVFILE_H
#ifdef _WIN32
#pragma once
#endif

#pragma pack(1)

#define XWV_ID		(('X'<<24)|('W'<<16)|('V'<<8)|(' '<<0))
#define XWV_VERSION 4

enum xwvSampleRate_t
{
	XWV_RATE_11025 = 0,
	XWV_RATE_22050 = 1,
	XWV_RATE_44100 = 2,
};

enum xwvFormat_t
{
	XWV_FORMAT_PCM = 0,
	XWV_FORMAT_XMA = 1,
	XWV_FORMAT_ADPCM = 2,
};

// generated in big-endian
struct xwvHeader_t
{
	unsigned int	id;
	unsigned int	version;
	unsigned int	headerSize;			// header only
	unsigned int	staticDataSize;		// follows header
	unsigned int	dataOffset;			// start of samples, possibly sector aligned
	unsigned int	dataSize;			// length of samples in bytes
	unsigned int	numDecodedSamples;	// for duration calcs
	int				loopStart;			// -1 = no loop, offset of loop in samples
	unsigned short	loopBlock;			// the xma block where the loop starts 
	unsigned short	numLeadingSamples;	// number of leading samples in the loop block to discard
	unsigned short	numTrailingSamples;	// number of trailing samples at the final block to discard
	unsigned short	vdatSize;			// follows seek table
	byte			format;
	byte			bitsPerSample;
	byte			sampleRate;
	byte			channels;
	byte			quality;
	byte			bHasSeekTable;		// indicates presence, follows header
	byte			padding[2];			// created as 0

	inline unsigned int GetPreloadSize() { return headerSize + staticDataSize; }

	inline int GetBitsPerSample() const { return bitsPerSample; }

	int GetSampleRate() const
	{
		int rates[] = {11025, 22050, 44100};
		int rate = sampleRate;
		return rates[rate]; 
	}
	
	inline int GetChannels() const { return channels; }

	void SetSampleRate( int sampleRateIn )
	{
		byte rate = ( sampleRateIn == 11025 ) ? XWV_RATE_11025 : ( sampleRateIn==22050 )? XWV_RATE_22050 : XWV_RATE_44100;
		sampleRate = rate;
	}

	inline void SetChannels( int channelsIn ) { channels = channelsIn; }

	inline int GetSeekTableSize()
	{
		// seek table is indexed by packets
		return bHasSeekTable ? ( dataSize / 2048 ) * sizeof( int ) : 0;
	}
};

#pragma pack()

#endif // XWVFILE_H
