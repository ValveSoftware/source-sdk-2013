//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_MELEE_ATTACK_H
#define HL2MP_BOT_MELEE_ATTACK_H

#include "Path/NextBotChasePath.h"

class CHL2MPBotMeleeAttack : public Action< CHL2MPBot >
{
public:
	CHL2MPBotMeleeAttack( float giveUpRange = -1.0f );

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "MeleeAttack"; };

private:
	float m_giveUpRange;			// if non-negative and if threat is farther than this, give up our melee attack
	ChasePath m_path;
};

#endif // HL2MP_BOT_MELEE_ATTACK_H
