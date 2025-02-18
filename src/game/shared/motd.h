//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: motd: Handles a list of message of the day entries
//
//=============================================================================

#ifndef MOTD_H
#define MOTD_H
#ifdef _WIN32
#pragma once
#endif

#include "KeyValues.h"
#include "language.h"

//-----------------------------------------------------------------------------
// CMOTDEntryDefinition
//-----------------------------------------------------------------------------
class CMOTDEntryDefinition
{
public:
	CMOTDEntryDefinition( void );
	~CMOTDEntryDefinition( void ) { }

	bool	BInitFromKV( KeyValues *pKVMOTD, CUtlVector<CUtlString> *pVecErrors = NULL );

	const char	*GetName( void ) { return m_pKVMOTD ? m_pKVMOTD->GetName() : "Unknown Entry"; }
	const char	*GetTitle( ELanguage eLang );
	const char	*GetText( ELanguage eLang );
	const char	*GetURL( void ) { return m_pKVMOTD ? m_pKVMOTD->GetString("url","") : NULL; }
	const char	*GetImage( void ) { return m_pKVMOTD ? m_pKVMOTD->GetString("image","") : NULL; }
	const char	*GetPostTimeStr( void ) { return m_pKVMOTD ? m_pKVMOTD->GetString("time") : NULL; }

	const int	GetHeaderType( void ) { return m_pKVMOTD ? m_pKVMOTD->GetInt("header_type") : 0; }
	const char	*GetHeaderTitle( ELanguage eLang );
	const char	*GetHeaderIcon( void ) { return m_pKVMOTD ? m_pKVMOTD->GetString("header_icon","") : NULL; }

	// Post time is the time displayed on the client screen for this post
	const RTime32		GetPostTime( void ) const { return m_PostTime; }

	// Change time is the time at which we last changed this post. If we change wording on
	// a post, we need to know that the change should be sent to clients when they log on afterwards.
	const RTime32		GetChangedTime( void ) const { return m_ChangedTime; }

	const int	 GetNameInt ( void ) const { return m_pKVMOTD ? V_atoi( m_pKVMOTD->GetName() ) : -1; }

private:
	KeyValues	*m_pKVMOTD;
	RTime32		m_PostTime;
	RTime32		m_ChangedTime;
};



//-----------------------------------------------------------------------------
// CMOTDMgr
//-----------------------------------------------------------------------------
class CMOTDManager
{
public:
	// MOTD handling
	bool BInitMOTDEntries( KeyValues *pKVMOTDEntries, CUtlVector<CUtlString> *pVecErrors );
	int  GetNumMOTDAfter( RTime32 iTime );
	CMOTDEntryDefinition *GetNextMOTDAfter( RTime32 iTime );
	int  GetNumMOTDs( void ) { return m_vecMOTDEntries.Count(); }
	CMOTDEntryDefinition *GetMOTDByIndex( int iIndex );

	void PurgeUnusedMOTDEntries( KeyValues *pKVMOTDEntries );

private:
	// Contains the list of MOTD entries.
	CUtlVector< CMOTDEntryDefinition >	m_vecMOTDEntries;

};


#endif // MOTD_H
