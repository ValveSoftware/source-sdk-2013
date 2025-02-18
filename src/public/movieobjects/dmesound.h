//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing a sound
//
//=============================================================================

#ifndef DMESOUND_H
#define DMESOUND_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"


//-----------------------------------------------------------------------------
// A class representing a camera
//-----------------------------------------------------------------------------
class CDmeSound : public CDmElement
{
	DEFINE_ELEMENT( CDmeSound, CDmElement );

public:
	CDmaString m_SoundName;
	CDmaString m_GameSoundName;	// Only used if it's a gamesound

	// Return false if it can't find the sound full path
	bool ComputeSoundFullPath( char *pBuf, int nBufLen );
};

class CDmeGameSound : public CDmeSound
{
	DEFINE_ELEMENT( CDmeGameSound, CDmeSound );

public:

	CDmElement *FindOrAddPhonemeExtractionSettings();

	CDmaVar< float >	m_Volume;
	CDmaVar< int >		m_Level;
	CDmaVar< int >		m_Pitch;

	CDmaVar< bool >		m_IsStatic;
	CDmaVar< int >		m_Channel;
	CDmaVar< int >		m_Flags;

//	CDmaElement			m_Source;
//	CDmaVar< bool >		m_FollowSource;
	CDmaVar< Vector >	m_Origin;
	CDmaVar< Vector >	m_Direction;
};

#endif // DMESOUND_H
