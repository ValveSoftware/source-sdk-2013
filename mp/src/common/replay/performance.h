//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYPERFORMANCE_H
#define REPLAYPERFORMANCE_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "shared_defs.h"
#include "qlimits.h"

//----------------------------------------------------------------------------------------

class CReplay;
class KeyValues;

//----------------------------------------------------------------------------------------

class CReplayPerformance
{
public:
	CReplayPerformance( CReplay *pReplay );

	inline bool HasInTick() const { return m_nTickIn >= 0; }
	inline bool HasOutTick() const { return m_nTickOut >= 0; }

	inline int	GetTickIn() const { return m_nTickIn; }
	inline int	GetTickOut() const { return m_nTickOut; }

	void		Copy( const CReplayPerformance *pSrc );
	void		CopyTicks( const CReplayPerformance *pSrc );

	void		SetFilename( const char *pFilename );
	const char	*GetFullPerformanceFilename();

	void		AutoNameIfHasNoTitle( const char *pMapName );
	void		SetTitle( const wchar_t *pTitle );
	
	// NOTE: Doesn't copy exactly - gets a valid filename for the returned performance.
	CReplayPerformance *MakeCopy() const;

	void		Read( KeyValues *pIn );
	void		Write( KeyValues *pOut );

	// NOTE: Any changes made here should be reflected in the copy constructor
	// (which is called from MakeCopy()).
	wchar_t		m_wszTitle[MAX_TAKE_TITLE_LENGTH];
	char		m_szBaseFilename[ MAX_OSPATH ];
	CReplay		*m_pReplay;
	int			m_nTickIn;
	int			m_nTickOut;

private:
	CReplayPerformance( const CReplayPerformance *pPerformance );
};

//----------------------------------------------------------------------------------------

#endif // REPLAYPERFORMANCE_H