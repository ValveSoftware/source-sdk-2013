//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//
#if !defined( RENAMED_RECVTABLE_COMPAT_H )
#define RENAMED_RECVTABLE_COMPAT_H
#ifdef _WIN32
#pragma once
#endif

class CRenamedRecvTableInfo;

extern CRenamedRecvTableInfo *g_pRenamedRecvTableInfoHead;

//-----------------------------------------------------------------------------
// Purpose: Used by NOTE_RENAMED_RECVTABLE() macro.
//-----------------------------------------------------------------------------
class CRenamedRecvTableInfo
{
public:
	CRenamedRecvTableInfo( const char *pOldName, const char *pNewName )
	:	m_pOldName( pOldName ),
		m_pNewName( pNewName )
	{
		m_pNext						= g_pRenamedRecvTableInfoHead;
		g_pRenamedRecvTableInfoHead = this;
	}

public:
	const char				*m_pOldName;
	const char				*m_pNewName;
	CRenamedRecvTableInfo	*m_pNext;
};

//-----------------------------------------------------------------------------
// Purpose: To keep from breaking older demos, use this macro to allow the
// engine to find the new datatable from the old name.
//-----------------------------------------------------------------------------
#define NOTE_RENAMED_RECVTABLE( oldname_, newname_ ) \
	static CRenamedRecvTableInfo g_##oldname_##Register( \
		#oldname_, \
		#newname_ \
	);


#endif // RENAMED_RECVTABLE_COMPAT_H
