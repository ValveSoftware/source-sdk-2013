//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_fetch_flag.h
// Go get the flag!
// Michael Booth, May 2011

#ifndef TF_BOT_FETCH_FLAG_H
#define TF_BOT_FETCH_FLAG_H

#include "Path/NextBotPathFollow.h"


//-----------------------------------------------------------------------------
class CTFBotFetchFlag : public Action< CTFBot >
{
public:
	#define TEMPORARY_FLAG_FETCH true
	CTFBotFetchFlag( bool isTemporary = false );
	virtual ~CTFBotFetchFlag() { }

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual QueryResultType ShouldHurry( const INextBot *me ) const;
	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;

	virtual const char *GetName( void ) const	{ return "FetchFlag"; };

private:
	bool m_isTemporary;
	PathFollower m_path;
	CountdownTimer m_repathTimer;
};


#endif // TF_BOT_FETCH_FLAG_H
