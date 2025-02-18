//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_wait_for_players.h
// Michael Booth, November 2010

#ifndef BOSS_ALPHA_WAIT_FOR_PLAYER_H
#define BOSS_ALPHA_WAIT_FOR_PLAYER_H

#ifdef TF_RAID_MODE

class CBossAlphaWaitForPlayers : public Action< CBossAlpha >
{
public:
	virtual ActionResult< CBossAlpha >	OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction );
	virtual ActionResult< CBossAlpha >	Update( CBossAlpha *me, float interval );
	virtual void					OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction );

	virtual EventDesiredResult< CBossAlpha > OnInjured( CBossAlpha *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CBossAlpha > OnContact( CBossAlpha *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual const char *GetName( void ) const	{ return "WaitForPlayers"; }		// return name of this action
};

#endif // TF_RAID_MODE

#endif // BOSS_ALPHA_WAIT_FOR_PLAYER_H
