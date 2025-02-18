//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef TF_TANK_BOSS_BODY_H
#define TF_TANK_BOSS_BODY_H

#include "animation.h"
#include "NextBotBodyInterface.h"

class INextBot;


//----------------------------------------------------------------------------------------------------------------
/**
 * The interface for control and information about the bot's body state (posture, animation state, etc)
 */
class CTFTankBossBody : public IBody
{
public:
	CTFTankBossBody( INextBot *bot );
	virtual ~CTFTankBossBody() { }

	virtual void Update( void );
	virtual unsigned int GetSolidMask( void ) const;					// return the bot's collision mask (hack until we get a general hull trace abstraction here or in the locomotion interface)

	bool StartSequence( const char *name );
	void SetSkin( int nSkin );
};



#endif // TF_TANK_BOSS_BODY_H
