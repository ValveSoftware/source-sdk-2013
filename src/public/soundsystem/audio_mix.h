//===== Copyright © Valve Corporation, All rights reserved. =================//
//
// Purpose: audio mix data structures
//
//===========================================================================//

#ifndef AUDIO_MIX_H
#define AUDIO_MIX_H

#ifdef _WIN32
#pragma once
#endif

#include "strtools.h"		// V_memset
#include "utlstring.h"
#include "utlstringtoken.h"
#include "soundsystem/lowlevel.h"

struct dspglobalvars_t
{
	float m_flMixMin;			// min dsp mix at close range
	float m_flMixMax;			// max dsp mix at long range
	float m_fldbMixDrop;		// reduce mix_min/max by n% if sndlvl of new sound less than dbMin
	float m_flDistanceMin;		// range at which sounds are mixed at flMixMin
	float m_flDistanceMax;		// range at which sounds are mixed at flMixMax
	uint16	m_ndbMin;				// if sndlvl of a new sound is < dbMin, reduce mix_min/max by m_fldbMixDrop					
	bool m_bIsOff;
};

class CAudioProcessor
{
public:
	virtual ~CAudioProcessor() {}
	CAudioProcessor( const char *pDebugName, int nChannelCount );
	void SetDebugName( const char *pName );

	virtual void Process( CAudioMixBuffer *pInput, CAudioMixBuffer *pOutput, int nChannelCount, dspglobalvars_t *pGlobals );
	virtual void ProcessSingleChannel( const CAudioMixBuffer &input, CAudioMixBuffer *pOutput, int nChannelIndex ) = 0;

	// Parameter set can modify internal or global state (or both)
	virtual bool SetControlParameter( CUtlStringToken name, float flValue );
	virtual float GetControlParameter( CUtlStringToken name, float flDefaultValue = 0.0f ) = 0;
	virtual bool SetNameParameter( CUtlStringToken name, uint32 nNameValue ) = 0;
	virtual uint32 GetNameParameter( CUtlStringToken name, uint32 nDefaultValue ) = 0;

	virtual bool ShouldProcess();
	float GetPrevMix( float flMix );
	void ApplyMonoProcessor( CAudioMixBuffer *pInput, CAudioMixBuffer *pOutput, int nOutputChannelCount, float flMix );
	void ApplyStereoProcessor( CAudioMixBuffer *pInput, CAudioMixBuffer *pOutput, int nOutputChannelCount, float flMix );
	void ApplyNChannelProcessor( CAudioMixBuffer *pInput, CAudioMixBuffer *pOutput, int nChannelCount, float flMix );

	CUtlString m_debugName;
	uint32 m_nNameHashCode;
	float m_flXFade;
	float m_flXFadePrev;
	float m_flMix;
	int m_nChannels;

	bool m_bEnabled;
};

struct audio_buffer_input_t
{
	const short *m_pSamples;	// pointer to the samples themselves (in 8, 16, or 32-bit format)
	uint m_nSampleCount;			// number of whole samples *not bytes* *not multiplied by channel count*  (e.g. a one-second 16-bit 44.1KHz file would be 44100 samples)
};

// UNDONE: deprecate this and move to a 8-int, 16-int, 32-float, N channel with extract specific channel design
enum vaudio_sampleformats_t
{
	SAMPLE_INT16_MONO = 0,		// default
	SAMPLE_INT8_MONO,			// increase to 16-bit and convert to float
	SAMPLE_INT16_STEREO_L,		// stereo wave, extract left channel
	SAMPLE_INT16_STEREO_R,		// stereo wave, extract right channel
	SAMPLE_INT8_STEREO_L,		// stereo wave, extract left channel
	SAMPLE_INT8_STEREO_R,		// stereo wave, extract right channel
	SAMPLE_FLOAT32_MONO,		// no reformat needed
};

struct audio_source_input_t
{
	const audio_buffer_input_t *m_pPackets;
	uint m_nSamplingRate;
	uint16 m_nPacketCount;
	uint16 m_nSampleFormat;

	void InitPackets( const audio_buffer_input_t *pPacketsIn, int nPacketCountIn, int nSamplingRate, int nBitsPerSample, int nChannelsPerSample )
	{
		V_memset( this, 0, sizeof(*this) );
		Assert( nPacketCountIn >= 0 && nPacketCountIn <= UINT16_MAX );
		m_nPacketCount = (uint16)nPacketCountIn;
		m_pPackets = pPacketsIn;
		m_nSamplingRate = nSamplingRate;
		switch( nBitsPerSample )
		{
		case 16:
			m_nSampleFormat = (uint16)( (nChannelsPerSample == 1) ? SAMPLE_INT16_MONO : SAMPLE_INT16_STEREO_L );
			break;
		case 8:
			m_nSampleFormat = (uint16)( (nChannelsPerSample == 1) ? SAMPLE_INT8_MONO : SAMPLE_INT8_STEREO_L );
			break;
		}
	}
};

struct audio_source_indexstate_t
{
	uint m_nPacketIndex;
	uint m_nBufferSampleOffset;
	uint m_nSampleFracOffset;
	inline void Clear()
	{
		m_nPacketIndex = 0;
		m_nBufferSampleOffset = 0;
		m_nSampleFracOffset = 0;
	}
};

class CAudioMixState
{
	audio_source_input_t *m_pChannelsIn;
	audio_source_indexstate_t *m_pChannelsOut;
	uint32 m_nInputStride;
	uint32 m_nPlaybackStride;
	uint32 m_nChannelCount;

	dspglobalvars_t *m_pGlobals;

public:
	inline void Init( audio_source_input_t *pInput, uint32 nInputStride, audio_source_indexstate_t *pPlayback, uint32 nPlaybackStride, uint32 nCount )
	{
		m_pChannelsIn = pInput;
		m_pChannelsOut = pPlayback;
		m_nInputStride = nInputStride;
		m_nPlaybackStride = nPlaybackStride;
		m_nChannelCount = nCount;
	}
	void SetDSPGlobals( dspglobalvars_t *pGlobals )
	{
		m_pGlobals = pGlobals;
	}

	CAudioMixState( audio_source_input_t *pInput, uint32 nInputStride, audio_source_indexstate_t *pPlayback, uint32 nPlaybackStride, uint32 nCount )
		: m_pChannelsIn(pInput), m_pChannelsOut(pPlayback), m_nInputStride(nInputStride), m_nPlaybackStride(nPlaybackStride), m_nChannelCount(nCount)
	{
	}
	CAudioMixState()
	{
		m_pChannelsIn = 0;
		m_pChannelsOut = 0;
		m_nChannelCount = 0;
		m_pGlobals = nullptr;
	}

	inline void Clear()
	{
		m_pChannelsIn = NULL;
		m_pChannelsOut = NULL;
		m_nChannelCount = 0;
	}

	inline audio_source_input_t *GetInput( int nIndex ) const
	{
		return (audio_source_input_t *)( (byte *)m_pChannelsIn + nIndex * m_nInputStride );
	}

	inline audio_source_indexstate_t *GetOutput( int nIndex ) const
	{
		return (audio_source_indexstate_t *)( (byte *)m_pChannelsOut + nIndex * m_nPlaybackStride );
	}
	bool IsChannelFinished( int nChannel ) const
	{
		return ( GetOutput( nChannel )->m_nPacketIndex >= GetInput( nChannel )->m_nPacketCount ) ? true : false;
	}

	dspglobalvars_t *DSPGlobals() const { return m_pGlobals;  }

};

enum mix_command_id_t
{
	AUDIO_MIX_CLEAR = 0,
	AUDIO_MIX_EXTRACT_SOURCE,
	AUDIO_MIX_ADVANCE_SOURCE,
	AUDIO_MIX_ACCUMULATE,
	AUDIO_MIX_ACCUMULATE_RAMP,
	AUDIO_MIX_MULTIPLY,
	AUDIO_MIX_PROCESS,
	AUDIO_MIX_SUM,
	AUDIO_MIX_SWAP, // swap two buffers
	AUDIO_MIX_MEASURE_DEBUG_LEVEL,
	AUDIO_MIX_OUTPUT_LEVEL,
};

struct audio_mix_command_t
{
	uint16 m_nCommandId;
	uint16 m_nOutput;
	uint16 m_nInput0;
	uint16 m_nInput1;
	float m_flParam0;
	float m_flParam1;
	void Init( mix_command_id_t cmd, uint16 nOut )
	{
		m_nCommandId = (uint16)cmd;
		m_nOutput = nOut;
		m_nInput0 = 0;
		m_nInput1 = 0;
		m_flParam0 = 0.0f;
		m_flParam1 = 0.0f;
	}

	void Init( mix_command_id_t cmd, uint16 nOut, uint16 nIn0, float flScale )
	{
		m_nCommandId = (uint16)cmd;
		m_nOutput = nOut;
		m_nInput0 = nIn0;
		m_nInput1 = 0;
		m_flParam0 = flScale;
		m_flParam1 = 0.0f;
	}

	void Init( mix_command_id_t cmd, uint16 nOut, uint16 nIn0, uint16 nIn1, float flScale0, float flScale1 )
	{
		m_nCommandId = (uint16)cmd;
		m_nOutput = nOut;
		m_nInput0 = nIn0;
		m_nInput1 = nIn1;
		m_flParam0 = flScale0;
		m_flParam1 = flScale1;
	}
};

class CAudioMixCommandList
{
public:
	inline void ClearBuffer( uint16 nTarget )
	{
		audio_mix_command_t cmd;
		cmd.Init( AUDIO_MIX_CLEAR, nTarget );
		m_commands.AddToTail( cmd );
	}
	
	void ClearMultichannel( uint16 nTarget, int nCount );
	void ScaleMultichannel( uint16 nTarget, uint16 nInput, int nCount, float flVolume );

	inline void ExtractSourceToBuffer( uint16 nTarget, uint16 nChannel, float flVolume, float flPitch )
	{
		audio_mix_command_t cmd;
		cmd.Init( AUDIO_MIX_EXTRACT_SOURCE, nTarget, nChannel, 0, flVolume, flPitch );
		m_commands.AddToTail( cmd );
	}
	inline void AdvanceSource( uint16 nChannel, float flPitch )
	{
		audio_mix_command_t cmd;
		cmd.Init( AUDIO_MIX_ADVANCE_SOURCE, 0, nChannel, 0, 0, flPitch );
		m_commands.AddToTail( cmd );
	}

	inline void ProcessBuffer( uint16 nOutput, uint16 nInput, uint16 nProcessor, int nChannelCount )
	{
		audio_mix_command_t cmd;
		cmd.Init( AUDIO_MIX_PROCESS, nOutput, nInput, nProcessor, float(nChannelCount), 0.0f );
		m_commands.AddToTail( cmd );
	}

	inline uint16 ProcessBuffer( uint16 nOutput, uint16 nInput, int nChannelCount, CAudioProcessor *pProc )
	{
		uint16 nProcessor = (uint16)m_processors.AddToTail( pProc );
		ProcessBuffer( nOutput, nInput, nProcessor, nChannelCount );
		return nProcessor;
	}

	inline void ScaleBuffer( uint16 nOutput, uint16 nInput, float flVolume )
	{
		audio_mix_command_t cmd;
		cmd.Init( AUDIO_MIX_MULTIPLY, nOutput, nInput, flVolume );
		m_commands.AddToTail( cmd );
	}

	inline void AccumulateToBuffer( uint16 nOutput, uint16 nInput, float flVolume )
	{
		// if the volume is zero this will have no effect, so it is safe to skip it
		if ( flVolume == 0.0f )
			return;

		audio_mix_command_t cmd;
		cmd.Init( AUDIO_MIX_ACCUMULATE, nOutput, nInput, flVolume );
		m_commands.AddToTail( cmd );
	}

	inline void AccumulateToBufferVolumeRamp( uint16 nOutput, uint16 nInput, float flVolumeStart, float flVolumeEnd )
	{
		// Too small of a volume change to ramp?  Just output without the ramp
		// 1e-3f is small enough that we might do it during a normal ramp 
		// (2e-3f is the slope of a full scale fade at 512 samples per batch)
		if ( fabs( flVolumeEnd-flVolumeStart) < 1e-3f )
		{
			AccumulateToBuffer( nOutput, nInput, flVolumeEnd );
			return;
		}

		audio_mix_command_t cmd;
		cmd.Init( AUDIO_MIX_ACCUMULATE_RAMP, nOutput, nInput, 0, flVolumeStart, flVolumeEnd );
		m_commands.AddToTail( cmd );
	}

	inline void Mix2x1( uint16 nOutput, uint16 nInput0, uint16 nInput1, float flVolume0, float flVolume1 )
	{
		audio_mix_command_t cmd;
		cmd.Init( AUDIO_MIX_SUM, nOutput, nInput0, nInput1, flVolume0, flVolume1 );
		m_commands.AddToTail( cmd );
	}

	inline void SwapBuffers( uint16 nInput0, uint16 nInput1 )
	{
		audio_mix_command_t cmd;
		cmd.Init( AUDIO_MIX_SWAP, nInput0, nInput1, 1.0f );
		m_commands.AddToTail( cmd );
	}

	inline void ReadOutputLevel( uint16 nLevelOutput, uint16 nInput0, uint16 nInputChannelCount )
	{
		audio_mix_command_t cmd;
		cmd.Init( AUDIO_MIX_OUTPUT_LEVEL, nLevelOutput, nInput0, nInputChannelCount, 1.0f, 1.0f );
		m_commands.AddToTail( cmd );
	}

	inline void DebugReadLevel( uint16 nDebugOutput0, uint16 nInput0, uint16 nInputChannelCount )
	{
		audio_mix_command_t cmd;
		cmd.Init( AUDIO_MIX_MEASURE_DEBUG_LEVEL, nDebugOutput0, nInput0, nInputChannelCount, 1.0f, 1.0f );
		m_commands.AddToTail( cmd );
	}

	inline uint16 AddProcessor( CAudioProcessor *pProc )
	{
		return (uint16)m_processors.AddToTail( pProc );
	}

	inline void Clear()
	{
		m_commands.RemoveAll();
		m_processors.RemoveAll();
	}
	void AccumulateMultichannel( uint16 nOutput, int nOutputChannels, uint16 nInput, int nInputChannels, float flInputVolume );

	CUtlVectorFixedGrowable<audio_mix_command_t, 256> m_commands;
	CUtlVectorFixedGrowable<CAudioProcessor *, 8> m_processors;
};

// This describes the state for each iteration of mixing
// it is the list of all low-level audio operations that need to take place
// in order to produce one buffer of mixed output
class CAudioMixDescription : public CAudioMixCommandList
{
public:
	inline void Init( int nChannels )
	{
		m_nMixBuffersInUse = 0;
		m_nMixBufferMax = 0;
		m_nDebugOutputCount = 0;
		m_nOutputLevelCount = 0;
		Clear();
#if USE_VOICE_LAYERS
		for ( int i = 0; i < NUM_VOICE_LAYERS; i++ )
		{
			m_flLayerVolume[i] = 1.0f;
		}
#endif
	}

	// Add a new mix buffer
	inline uint16 AllocMixBuffer( uint nCount = 1 ) 
	{ 
		int nOut = m_nMixBuffersInUse; 
		m_nMixBuffersInUse += nCount; 
		m_nMixBufferMax = MAX( m_nMixBufferMax, m_nMixBuffersInUse );
		return (uint16)nOut;
	}
	inline void FreeMixBuffer( uint16 nStart, uint nCount = 1 )
	{
		// we only support freeing from the end of the stack
		Assert( nStart + nCount == m_nMixBuffersInUse );

		if ( nStart + nCount == m_nMixBuffersInUse )
		{
			m_nMixBuffersInUse -= nCount;
		}
	}

	inline int AllocDebugOutputs( int nOutputs ) 
	{ 
		int nRet = m_nDebugOutputCount;
		m_nDebugOutputCount += nOutputs; 
		return nRet;
	}
	
	inline int AllocOutputLevels( int nOutputs )
	{
		int nRet = m_nOutputLevelCount;
		m_nOutputLevelCount += nOutputs;
		return nRet;
	}

	uint m_nMixBufferMax;
	uint m_nMixBuffersInUse;
	uint m_nDebugOutputCount;
	uint m_nOutputLevelCount;
#if USE_VOICE_LAYERS
	float m_flLayerVolume[NUM_VOICE_LAYERS];
#endif
};

struct mix_debug_outputs_t
{
	uint32 m_nChannelCount;
	float m_flLevel;
	float m_flChannelLevels[8];
};

// NOTE: This object is large (>64KB) declaring one on the stack may crash some platforms
class CAudioMixResults
{
public:
	CUtlVectorFixedGrowable<mix_debug_outputs_t,8> m_debugOutputs;
	CUtlVectorFixedGrowable<float, 16> m_flOutputLevels;
	CUtlVectorFixedGrowable<CAudioMixBuffer, 32> m_pOutput;
};

extern void ProcessAudioMix( CAudioMixResults *pResults, const CAudioMixState &mixState, CAudioMixDescription &mixSetup );
#endif // AUDIO_MIX_H
