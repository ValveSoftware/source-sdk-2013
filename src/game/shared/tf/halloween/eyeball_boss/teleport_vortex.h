//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Teleport vortex for the Eyeball Boss
//
//=============================================================================//
#ifndef TELEPORT_VORTEX_H
#define TELEPORT_VORTEX_H

#ifdef _WIN32
#pragma once
#endif

// Client specific.
#ifdef CLIENT_DLL
	#define CTeleportVortex C_TeleportVortex

	#include "c_baseanimating.h"
#else
	#include "baseanimating.h"
#endif

enum EVortexState
{
	VORTEXSTATE_INACTIVE,
	VORTEXSTATE_ACTIVE_EYEBALL_MOVED,
	VORTEXSTATE_ACTIVE_EYEBALL_DIED,
};

//=============================================================================
//
// Teleport vortex for the Eyeball Boss
//
class CTeleportVortex : public CBaseAnimating
{
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS(); 
	DECLARE_CLASS( CTeleportVortex, CBaseAnimating );

public:
					CTeleportVortex();
	virtual			~CTeleportVortex();

	virtual void	Spawn( void );
	virtual void	Precache( void );

#ifdef CLIENT_DLL
	void			PlayBookAnimation( const char *pAnimName );

	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual void	ClientThink();
	virtual void	BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed );

	void			DestroyParticleEffect();
#else
	void			SetupVortex( bool bIsDeathVortex, bool bSplitTeam = false );
	void			VortexThink();

	virtual bool	KeyValue( const char *szKeyName, const char *szValue );
	virtual int		UpdateTransmitState( void );
	virtual void	StartTouch( CBaseEntity *pOther );
	virtual void	Touch( CBaseEntity *pOther ); 

	void			SetAdvantageTeam( inputdata_t &inputdata );
#endif

protected:
	float			GetRampTime();
	bool			ShouldDoBookRampIn();
	bool			ShouldDoBookRampOut();

	CountdownTimer	m_lifeTimer;

	CNetworkVar( int, m_iState );
	
#ifdef CLIENT_DLL
	CNewParticleEffect	*m_pVortexEffect;
	float			m_flScale;
	int				m_iOldState;		// This seems like it should not be necessary, but I'm getting OnDataChanged() called when m_iState has not actually changed, and updateType will be DATA_UPDATE_DATATABLE_CHANGED multiple times in a row.  Presumably this is because the position is being set explicitly on the server.
#else
	int				m_iAutoSetupVortex;
	int				m_nSoundCounter;	// Count the number of sounds played
	CFmtStr			m_pszWhere;
	bool			m_bSplitTeam;
#endif
};


#ifndef CLIENT_DLL

void SendPlayerToTheUnderworld( CTFPlayer *teleportingPlayer, const char *where );

#endif


#endif // TELEPORT_VORTEX_H


