//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYTIME_H
#define REPLAYTIME_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

class KeyValues;

//----------------------------------------------------------------------------------------

#include "vgui/ILocalize.h"

//----------------------------------------------------------------------------------------

class CReplayTime
{
public:
	CReplayTime();

	void InitDateAndTimeToNow();

	void Read( KeyValues *pIn );
	void Write( KeyValues *pOut );

	// Modifiers:
	void SetDate( int nDay, int nMonth, int nYear );
	void SetTime( int nHour, int nMin, int nSec );
	inline void SetRawDate( int nRawDate )	{ m_fDate = nRawDate; }
	inline void SetRawTime( int nRawTime )	{ m_fTime = nRawTime; }

	// Accessors:
	void GetTime( int &nHour, int &nMin, int &nSec ) const;
	void GetDate( int &nDay, int &nMonth, int &nYear ) const;

	static const char *FormatTimeString( int nSecs );
	static const char *FormatPreciseTimeString( float flSecs );
	static const wchar_t *GetLocalizedMonth( vgui::ILocalize *pLocalize, int nMonth );
	static const wchar_t *GetLocalizedDay( vgui::ILocalize *pLocalize, int nDay );
	static const wchar_t *GetLocalizedYear( vgui::ILocalize *pLocalize, int nYear );
	static const wchar_t *GetLocalizedTime( vgui::ILocalize *pLocalize, int nHour, int nMin, int nSec );
	static const wchar_t *GetLocalizedDate( vgui::ILocalize *pLocalize, int nDay, int nMonth, int nYear,
		int *pHour = NULL, int *pMin = NULL, int *pSec = NULL, bool bForceFullFormat = false );	// bForceFullFormat true will keep from returning "today" or "yesterday"
	static const wchar_t *GetLocalizedDate( vgui::ILocalize *pLocalize, const CReplayTime &t, bool bForceFullFormat = false );

	int				m_fDate;		// Representation of a date (bitfield)
	int				m_fTime;		// Representation of time (bitfield)
};

//----------------------------------------------------------------------------------------

#endif // REPLAYTIME_H