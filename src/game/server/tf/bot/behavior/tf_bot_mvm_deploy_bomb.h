//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_mvm_deploy_bomb.h
// Set us up the bomb!

#ifndef TF_BOT_MVM_DEPLOY_BOMB_H
#define TF_BOT_MVM_DEPLOY_BOMB_H

//-----------------------------------------------------------------------------
class CTFBotMvMDeployBomb : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual void					OnEnd( CTFBot *me, Action< CTFBot > *nextAction );

	EventDesiredResult< CTFBot >	OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result );
	QueryResultType					ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;

	virtual const char *GetName( void ) const	{ return "MvMDeployBomb"; };

private:
	CountdownTimer m_timer;
	Vector m_anchorPos;
};


#endif // TF_BOT_MVM_DEPLOY_BOMB_H
