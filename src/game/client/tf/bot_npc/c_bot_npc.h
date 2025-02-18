//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_BOT_NPC_H
#define C_BOT_NPC_H

#include "c_ai_basenpc.h"


//--------------------------------------------------------------------------------------------------------
/**
 * The client-side implementation of Bot NPC
 */
class C_BotNPC : public C_NextBotCombatCharacter
{
public:
	DECLARE_CLASS( C_BotNPC, C_NextBotCombatCharacter );
	DECLARE_CLIENTCLASS();

	C_BotNPC();
	virtual ~C_BotNPC();

public:	
	virtual void Spawn( void );
	virtual bool IsNextBot() { return true; }

	virtual Vector GetObserverCamOrigin( void );	// Return the origin for player observers tracking this target

	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	virtual void ClientThink();

private:
	C_BotNPC( const C_BotNPC & );				// not defined, not accessible

	CNetworkHandle( C_BaseEntity, m_laserTarget );
	HPARTICLEFFECT	m_laserBeamEffect;

	CNetworkVar( bool, m_isNuking );
	HPARTICLEFFECT	m_nukeEffect;
};


#endif // C_BOT_NPC_H
