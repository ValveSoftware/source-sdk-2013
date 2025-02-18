//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef C_MERASMUS_H
#define C_MERASMUS_H

#include "c_ai_basenpc.h"


//--------------------------------------------------------------------------------------------------------
/**
 * The client-side implementation of the Dark Knight
 */
class C_Merasmus : public C_NextBotCombatCharacter
{
public:
	DECLARE_CLASS( C_Merasmus, C_NextBotCombatCharacter );
	DECLARE_CLIENTCLASS();

	C_Merasmus();
	virtual ~C_Merasmus();

public:	
	virtual void Spawn( void );
	virtual bool IsNextBot() { return true; }

	virtual Vector GetObserverCamOrigin( void );	// Return the origin for player observers tracking this target

	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );

	virtual int GetSkin();

private:
	C_Merasmus( const C_Merasmus & );				// not defined, not accessible

	HPARTICLEFFECT		m_ghostEffect;
	HPARTICLEFFECT		m_aoeEffect;
	HPARTICLEFFECT		m_stunEffect;

	bool m_bWasRevealed;
	CNetworkVar( bool, m_bRevealed );
	CNetworkVar( bool, m_bIsDoingAOEAttack );
	CNetworkVar( bool, m_bStunned );
};

#endif // C_MERASMUS_H
