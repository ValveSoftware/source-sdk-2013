//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_escort.cpp
// Move near an entity and protect it
// Michael Booth, April 2011

#ifndef TF_BOT_ESCORT_H
#define TF_BOT_ESCORT_H

#include "Path/NextBotChasePath.h"

class CTFBotEscort : public Action< CTFBot >
{
public:
	CTFBotEscort( CBaseEntity *who );
	virtual ~CTFBotEscort() { }

	void SetWho( CBaseEntity *who );
	CBaseEntity *GetWho( void ) const;

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToSuccess( CTFBot *me, const Path *path );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;							// is it time to retreat?

	virtual EventDesiredResult< CTFBot > OnCommandApproach( CTFBot *me, const Vector &pos, float range );

	virtual const char *GetName( void ) const	{ return "Escort"; }

private:
	CHandle< CBaseEntity > m_who;
	PathFollower m_pathToWho;
	CountdownTimer m_vocalizeTimer;
	CountdownTimer m_repathTimer;
};

#endif // TF_BOT_ESCORT_H
