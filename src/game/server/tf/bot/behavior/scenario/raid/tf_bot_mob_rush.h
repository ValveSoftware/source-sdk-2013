//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_mob_rush.h
// A member of a rushing mob of melee attackers
// Michael Booth, October 2009

#ifndef TF_BOT_MOB_RUSH_H
#define TF_BOT_MOB_RUSH_H

#ifdef TF_RAID_MODE

#include "Path/NextBotChasePath.h"


//-----------------------------------------------------------------------------
class CTFBotMobRush : public Action< CTFBot >
{
public:
	CTFBotMobRush( CTFPlayer *victim, float reactionTime = 0.0f );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual EventDesiredResult< CTFBot > OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result = NULL );
	virtual EventDesiredResult< CTFBot > OnInjured( CTFBot *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CTFBot > OnOtherKilled( CTFBot *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );

	QueryResultType	ShouldRetreat( const INextBot *me ) const;

	virtual const char *GetName( void ) const	{ return "MobRush"; };

private:
	CHandle< CTFPlayer > m_victim;
	CountdownTimer m_reactionTimer;
	CountdownTimer m_tauntTimer;
	CountdownTimer m_vocalizeTimer;
	ChasePath m_path;
};

#endif // TF_RAID_MODE

#endif // TF_BOT_MOB_RUSH_H
