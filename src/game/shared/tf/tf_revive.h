//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Revive
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef TF_REVIVE_H
#define TF_REVIVE_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

#ifdef CLIENT_DLL
#define CTFReviveMarker C_TFReviveMarker
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CTFReviveMarker : public CBaseAnimating
{
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS(); 
	DECLARE_CLASS( CTFReviveMarker, CBaseAnimating );

public:
	CTFReviveMarker();
	
	virtual void Precache() OVERRIDE;
	virtual void Spawn( void ) OVERRIDE;
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const OVERRIDE;
	virtual bool IsCombatItem( void ) const { return true; }

#ifdef GAME_DLL
	static CTFReviveMarker *Create( CTFPlayer *pOwner );
	virtual int	UpdateTransmitState( void ) OVERRIDE;
	virtual int ShouldTransmit( const CCheckTransmitInfo *pInfo ) OVERRIDE;
	void ReviveThink( void );
#else
	virtual void OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual int GetHealth( void ) const OVERRIDE { return m_iHealth; }
	virtual int GetMaxHealth( void ) const OVERRIDE { return m_iMaxHealth; }
	virtual bool IsVisibleToTargetID( void ) const OVERRIDE { return true; }
#endif // GAME_DLL

	void SetOwner( CTFPlayer *pPlayer );
	void SetReviver( CTFPlayer *pPlayer ) { m_pReviver = pPlayer; }
	CTFPlayer *GetOwner( void ) const { return m_hOwner; }
	CTFPlayer *GetReviver( void ) const { return m_pReviver; }
#ifdef GAME_DLL
	void AddMarkerHealth( float flAmount );
	bool IsReviveInProgress( void );
	// Fully healed.  Ask player if they'd like to be revived at their marker.
	bool ReviveOwner( void );
	void PromptOwner( void );
	bool HasOwnerBeenPrompted( void ) { return m_bOwnerPromptedToRevive; }
	void SetOwnerHasBeenPrompted( bool bValue ) { m_bOwnerPromptedToRevive = bValue; }
#endif // GAME_DLL

private:
	CNetworkHandle( CTFPlayer, m_hOwner );
	CTFPlayer *m_pReviver;
	CNetworkVar( uint16, m_nRevives );
#ifdef GAME_DLL
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iHealth );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iMaxHealth );
	float m_flHealAccumulator;
	bool m_bOwnerPromptedToRevive;
	float m_flLastHealTime;
	bool m_bOnGround;
#else
	int	m_iHealth;
	int m_iMaxHealth;
	bool m_bCalledForMedic;
#endif // CLIENT_DLL
};

#endif // TF_REVIVE_H
