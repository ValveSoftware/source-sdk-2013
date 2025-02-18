//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#if !defined( MOUTHINFO_H )
#define MOUTHINFO_H
#ifdef _WIN32
#pragma once
#endif

class CAudioSource;

#pragma pack(push,4)
class CVoiceData
{
public:
	CVoiceData( void )
	{
		m_flElapsed = 0.0f;
		m_pAudioSource = NULL;
		m_bIgnorePhonemes = false;
	}

	void SetElapsedTime( float t )
	{
		m_flElapsed = t;
	}

	float GetElapsedTime() const
	{
		return m_flElapsed;
	}

	void SetSource( CAudioSource *source, bool bIgnorePhonemes )
	{
		m_pAudioSource = source;
		m_bIgnorePhonemes = bIgnorePhonemes;
	}

	bool ShouldIgnorePhonemes() const
	{
		return m_bIgnorePhonemes;
	}

	CAudioSource	*GetSource()
	{
		return m_pAudioSource;
	}

private:
	float					m_flElapsed;
	CAudioSource 			*m_pAudioSource;
	bool					m_bIgnorePhonemes;
};

#define UNKNOWN_VOICE_SOURCE -1

//-----------------------------------------------------------------------------
// Purpose: Describes position of mouth for lip syncing
//-----------------------------------------------------------------------------
class CMouthInfo
{
public:
	// 0 = mouth closed, 255 = mouth agape
	byte					mouthopen;		
	// counter for running average
	byte					sndcount;		
	// running average
	int						sndavg;			

public:
							CMouthInfo( void ) { m_nVoiceSources = 0; m_needsEnvelope = false; }
	virtual					~CMouthInfo( void ) { ClearVoiceSources(); }

	int						GetNumVoiceSources( void );
	CVoiceData				*GetVoiceSource( int number );

	void					ClearVoiceSources( void );
	int						GetIndexForSource( CAudioSource *source );
	bool					IsSourceReferenced( CAudioSource *source );

	CVoiceData				*AddSource( CAudioSource *source, bool bIgnorePhonemes );

	void					RemoveSource( CAudioSource *source );
	void					RemoveSourceByIndex( int index );

	bool					IsActive( void );
	bool					NeedsEnvelope() { return m_needsEnvelope != 0; }
	void					ActivateEnvelope() { m_needsEnvelope = true; }

private:
	enum
	{
		MAX_VOICE_DATA = 4
	};

	short					m_nVoiceSources;
	short					m_needsEnvelope; 
	CVoiceData				m_VoiceSources[ MAX_VOICE_DATA ];
};
#pragma pack(pop)


inline bool CMouthInfo::IsActive( void )
{
	return ( GetNumVoiceSources() > 0 ) ? true : false;
}

inline int CMouthInfo::GetNumVoiceSources( void )
{
	return m_nVoiceSources;
}

inline CVoiceData *CMouthInfo::GetVoiceSource( int number )
{
	if ( number < 0 || number >= m_nVoiceSources )
		return NULL;

	return &m_VoiceSources[ number ];
}

inline void CMouthInfo::ClearVoiceSources( void )
{
	m_nVoiceSources = 0;
}

inline int CMouthInfo::GetIndexForSource( CAudioSource *source )
{
	for ( int i = 0; i < m_nVoiceSources; i++ )
	{
		CVoiceData *v = &m_VoiceSources[ i ];
		if ( !v )
			continue;

		if ( v->GetSource() == source )
			return i;
	}

	return UNKNOWN_VOICE_SOURCE;
}

inline bool CMouthInfo::IsSourceReferenced( CAudioSource *source )
{
	if ( GetIndexForSource( source ) != UNKNOWN_VOICE_SOURCE )
		return true;

	return false;
}

inline void CMouthInfo::RemoveSource( CAudioSource *source )
{
	int idx = GetIndexForSource( source );
	if ( idx == UNKNOWN_VOICE_SOURCE )
		return;

	RemoveSourceByIndex( idx );
}

inline void CMouthInfo::RemoveSourceByIndex( int index )
{
	if ( index < 0 || index >= m_nVoiceSources )
		return;

	m_VoiceSources[ index ] = m_VoiceSources[ --m_nVoiceSources ];
}

inline CVoiceData *CMouthInfo::AddSource( CAudioSource *source, bool bIgnorePhonemes )
{
	int idx = GetIndexForSource( source );
	if ( idx == UNKNOWN_VOICE_SOURCE )
	{
		if ( m_nVoiceSources < MAX_VOICE_DATA )
		{
			idx = m_nVoiceSources++;
		}
		else
		{
			// No room!
			return NULL;
		}
	}

	CVoiceData *data = &m_VoiceSources[ idx ];
	data->SetSource( source, bIgnorePhonemes );
	data->SetElapsedTime( 0.0f );
	return data;
}

#endif // MOUTHINFO_H