//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side CTFTeam class
//
// $NoKeywords: $
//=============================================================================

#ifndef C_TF_TEAM_H
#define C_TF_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "c_team.h"
#include "shareddefs.h"
#include "c_baseobject.h"

class C_BaseEntity;
class C_BaseObject;
class CBaseTechnology;
class C_TFPlayer;

//-----------------------------------------------------------------------------
// Purpose: TF's Team manager
//-----------------------------------------------------------------------------
class C_TFTeam : public C_Team
{
	DECLARE_CLASS( C_TFTeam, C_Team );
	DECLARE_CLIENTCLASS();

public:

					C_TFTeam();
	virtual			~C_TFTeam();

	int				GetFlagCaptures( void ) { return m_nFlagCaptures; }
	int				GetRole( void ) { return m_iRole; }
	char			*Get_Name( void );

	int				GetNumObjects( int iObjectType = -1 );
	CBaseObject		*GetObject( int num );

	CUtlVector< CHandle<C_BaseObject> > m_aObjects;

	C_BasePlayer	*GetTeamLeader( void );
	void			UpdateTeamName( void );
	const wchar_t *Get_Localized_Name( void ){ return m_wzTeamname; };

	virtual void OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;

	bool IsUsingCustomTeamName( void ) { return m_bUsingCustomTeamName; }

	// IClientThinkable override
	virtual	void	ClientThink();

private:

	int		m_nFlagCaptures;
	int		m_iRole;

	CNetworkHandle( C_BasePlayer, m_hLeader );
	wchar_t	m_wzTeamname[ MAX_TEAM_NAME_LENGTH ];
	bool m_bUsingCustomTeamName;
};

C_TFTeam *GetGlobalTFTeam( int iTeamNumber );

#endif // C_TF_TEAM_H
