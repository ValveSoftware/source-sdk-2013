//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_BOT_NPC_MINION_H
#define C_BOT_NPC_MINION_H

#include "c_ai_basenpc.h"


//--------------------------------------------------------------------------------------------------------
/**
 * The client-side implementation of Bot NPC Minion
 */
class C_BotNPCMinion : public C_NextBotCombatCharacter
{
public:
	DECLARE_CLASS( C_BotNPCMinion, C_NextBotCombatCharacter );
	DECLARE_CLIENTCLASS();

	C_BotNPCMinion();
	virtual ~C_BotNPCMinion();

public:	
	virtual void Spawn( void );
	virtual bool IsNextBot() { return true; }

	virtual Vector GetObserverCamOrigin( void );	// Return the origin for player observers tracking this target

	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	virtual void ClientThink();

private:
	C_BotNPCMinion( const C_BotNPCMinion & );				// not defined, not accessible

	CNetworkHandle( C_BaseEntity, m_stunTarget );
	HPARTICLEFFECT	m_stunEffect;
	HPARTICLEFFECT	m_stunBeamEffect;
	HPARTICLEFFECT	m_scanEffect;
};


#endif // C_BOT_NPC_MINION_H
