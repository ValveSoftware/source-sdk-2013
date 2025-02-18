//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Teleporter
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJ_TELEPORTER_H
#define TF_OBJ_TELEPORTER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_obj.h"
#include "GameEventListener.h"

class CTFPlayer;

enum 
{
	TTYPE_NONE=0,
	TTYPE_ENTRANCE,
	TTYPE_EXIT,
};

#define TELEPORTER_MAX_HEALTH	150

// ------------------------------------------------------------------------ //
// Base Teleporter object
// ------------------------------------------------------------------------ //
class CObjectTeleporter : public CBaseObject, public CGameEventListener
{
	DECLARE_CLASS( CObjectTeleporter, CBaseObject );

public:
	DECLARE_SERVERCLASS();

	CObjectTeleporter();

	virtual void	Spawn();
	virtual void	UpdateOnRemove();
	virtual void	FirstSpawn( void );
	virtual void	Precache();
	virtual bool	StartBuilding( CBaseEntity *pBuilder );
	virtual void	SetStartBuildingModel( void );
	virtual void	OnGoActive( void );
	virtual int		DrawDebugTextOverlays(void) ;
	virtual bool	IsPlacementPosValid( void );
	virtual void	SetModel( const char *pModel );
	virtual void	InitializeMapPlacedObject( void );

	virtual void	FinishedBuilding( void );

	void SetState( int state );
	virtual void	DeterminePlaybackRate( void );

	void RecieveTeleportingPlayer( CTFPlayer* pTeleportingPlayer );
	void TeleporterThink( void );
	void TeleporterTouch( CBaseEntity *pOther );
	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );

	virtual void TeleporterSend( CTFPlayer *pPlayer );
	virtual void TeleporterReceive( CTFPlayer *pPlayer, float flDelay );

	virtual void StartUpgrading( void );
	virtual void FinishUpgrading( void );

	CObjectTeleporter *GetMatchingTeleporter( void );
	CObjectTeleporter *FindMatch( void );	// Find the teleport partner to this object

	bool IsReady( void );					// is this teleporter connected and functional? (ie: not sapped, disabled, upgrading, unconnected, etc)
	bool IsMatchingTeleporterReady( void );
	bool IsSendingPlayer( CTFPlayer *player );	// returns true if we are in the process of teleporting the given player

	int GetState( void ) { return m_iState; }	// state of the object ( building, charging, ready etc )

	void SetTeleportingPlayer( CTFPlayer *pPlayer )
	{
		m_hTeleportingPlayer = pPlayer;
	}

	// Wrench hits
	virtual int		Command_Repair( CTFPlayer *pActivator, float flAmount, float flRepairMod, float flRepairToMetalRatio = 3.f, bool bSendEvent = true ) OVERRIDE;
	void			AddHealth( int nHealthToAdd )
	{
		SetHealth( MIN( GetMaxHealth(), GetHealth() + nHealthToAdd ) );
	}
	virtual bool	InputWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector hitLoc );

	// Upgrading
	virtual bool	IsUpgrading( void ) const { return ( m_iState == TELEPORTER_STATE_UPGRADING ); }
	virtual bool	CheckUpgradeOnHit( CTFPlayer *pPlayer );
	void			CopyUpgradeStateToMatch( CObjectTeleporter *pMatch, bool bFrom );

	virtual void	Explode( void );

	bool			PlayerCanBeTeleported( CTFPlayer *pPlayer );

	void			SetTeleporterType( int iVal ) { m_iTeleportType = iVal; }
	int				GetTeleporterType( void ) { return m_iTeleportType; }
	bool			IsEntrance( void ) { return m_iTeleportType == TTYPE_ENTRANCE; }
	bool			IsExit( void ) { return m_iTeleportType == TTYPE_EXIT; }

	virtual void	MakeCarriedObject( CTFPlayer *pCarrier );

	virtual void	SetObjectMode( int iVal );

	virtual int		GetBaseHealth( void ) { return TELEPORTER_MAX_HEALTH; }

	virtual int		GetUpgradeMetalRequired();

	void SetTeleportWhere( const CUtlStringList& teleportWhereName )
	{
		// deep copy strings
		for ( int i=0; i<teleportWhereName.Count(); ++i )
		{
			m_teleportWhereName.CopyAndAddToTail( teleportWhereName[i] );
		}
	}

	const CUtlStringList& GetTeleportWhere() { return m_teleportWhereName; }

	virtual void	InputEnable( inputdata_t &inputdata ) OVERRIDE;
	virtual void	InputDisable( inputdata_t &inputdata ) OVERRIDE;


	CTFPlayer *GetTeleportingPlayer( void ){ return m_hTeleportingPlayer.Get(); }

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

protected:
	CNetworkVar( int, m_iState );
	CNetworkVar( float, m_flRechargeTime );
	CNetworkVar( float, m_flCurrentRechargeDuration );
	CNetworkVar( int, m_iTimesUsed );
	CNetworkVar( float, m_flYawToExit );
	CNetworkVar( bool, m_bMatchBuilding );

	CHandle<CObjectTeleporter> m_hMatchingTeleporter;

	float m_flLastStateChangeTime;

	float m_flMyNextThink;	// replace me

	CHandle<CTFPlayer> m_hReservedForPlayer;
	float			   m_flReserveAfterTouchUntil;

	CHandle<CTFPlayer> m_hTeleportingPlayer;

	float m_flNextEnemyTouchHint;

	// Direction Arrow, shows roughly what direction the exit is from the entrance
	void ShowDirectionArrow( bool bShow );

	bool m_bShowDirectionArrow;
	int m_iDirectionBodygroup;
	int m_iBlurBodygroup;
	int m_iTeleportType;

	string_t m_iszMatchingMapPlacedTeleporter;

private:
	DECLARE_DATADESC();

	void UpdateMaxHealth( int nHealth, bool bForce = false );
	void SpawnBread( const CTFPlayer* pTeleportingPlayer );

	CUtlStringList m_teleportWhereName;
};

#endif // TF_OBJ_TELEPORTER_H
