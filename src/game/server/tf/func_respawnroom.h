//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef FUNC_RESPAWNROOM_H
#define FUNC_RESPAWNROOM_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

class CFuncRespawnRoomVisualizer;

// This class is to get around the fact that DEFINE_FUNCTION doesn't like multiple inheritance
class CFuncRespawnRoomShim : public CBaseTrigger
{
	virtual void RespawnRoomTouch( CBaseEntity *pOther ) = 0;
public:
	void	Touch( CBaseEntity *pOther ) { return RespawnRoomTouch( pOther ) ; }
};

//-----------------------------------------------------------------------------
// Purpose: Defines an area considered inside a respawn room
//-----------------------------------------------------------------------------
DECLARE_AUTO_LIST( IFuncRespawnRoomAutoList );


class CFuncRespawnRoom : public CFuncRespawnRoomShim, public IFuncRespawnRoomAutoList
{
	DECLARE_CLASS( CFuncRespawnRoom, CFuncRespawnRoomShim );

public:

	CFuncRespawnRoom();

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual void Spawn( void );
	virtual void Activate( void );
	virtual void ChangeTeam( int iTeamNum ) OVERRIDE;

	virtual void RespawnRoomTouch( CBaseEntity *pOther ) OVERRIDE;
	virtual void StartTouch(CBaseEntity *pOther) OVERRIDE;
	virtual void EndTouch(CBaseEntity *pOther) OVERRIDE;

	// Inputs
	void	InputSetActive( inputdata_t &inputdata );
	void	InputSetInactive( inputdata_t &inputdata );
	void	InputToggleActive( inputdata_t &inputdata );
	void	InputRoundActivate( inputdata_t &inputdata );

	void	SetActive( bool bActive );
	bool	GetActive() const;

	void	AddVisualizer( CFuncRespawnRoomVisualizer *pViz );
	
private:
	bool	m_bActive;
	int		m_iOriginalTeam;

	CUtlVector< CHandle<CFuncRespawnRoomVisualizer> >	m_hVisualizers;
};

//-----------------------------------------------------------------------------
// Is a given point contained within a respawn room?
//-----------------------------------------------------------------------------
bool PointInRespawnRoom( const CBaseEntity *pEntity, const Vector &vecOrigin, bool bTouching_SameTeamOnly = false );

bool PointsCrossRespawnRoomVisualizer( const Vector& vecStart, const Vector &vecEnd, int nTeamToIgnore = TEAM_UNASSIGNED );
#endif // FUNC_RESPAWNROOM_H
