//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_wander.h
// Wanderering/idle enemies for Squad Co-op mode
// Michael Booth, October 2009

#ifndef TF_BOT_WANDER_H
#define TF_BOT_WANDER_H

#ifdef TF_RAID_MODE

//-----------------------------------------------------------------------------
class CTFBotWander : public Action< CTFBot >
{
public:
	CTFBotWander( void );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual EventDesiredResult< CTFBot > OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result = NULL );
	virtual EventDesiredResult< CTFBot > OnInjured( CTFBot *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CTFBot > OnOtherKilled( CTFBot *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info );

	virtual EventDesiredResult< CTFBot > OnCommandAttack( CTFBot *me, CBaseEntity *victim );

	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?
	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;							// is it time to retreat?

	virtual const char *GetName( void ) const	{ return "Wander"; };

private:
	CountdownTimer m_visionTimer;
	CountdownTimer m_vocalizeTimer;
};


inline QueryResultType CTFBotWander::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


inline QueryResultType CTFBotWander::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}


#endif TF_RAID_MODE

#endif // TF_BOT_WANDER_H
