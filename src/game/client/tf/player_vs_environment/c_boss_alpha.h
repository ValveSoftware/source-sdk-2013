//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_BOSS_ALPHA_H
#define C_BOSS_ALPHA_H

#include "c_ai_basenpc.h"


//--------------------------------------------------------------------------------------------------------
/**
 * The client-side implementation of Boss Alpha
 */
class C_BossAlpha : public C_NextBotCombatCharacter
{
public:
	DECLARE_CLASS( C_BossAlpha, C_NextBotCombatCharacter );
	DECLARE_CLIENTCLASS();

	C_BossAlpha();
	virtual ~C_BossAlpha();

public:	
	virtual void Spawn( void );
	virtual bool IsNextBot() { return true; }

	virtual Vector GetObserverCamOrigin( void );	// Return the origin for player observers tracking this target

	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	virtual void ClientThink();

private:
	C_BossAlpha( const C_BossAlpha & );				// not defined, not accessible

	CNetworkVar( bool, m_isNuking );
	HPARTICLEFFECT	m_nukeEffect;
};


#endif // C_BOSS_ALPHA_H
