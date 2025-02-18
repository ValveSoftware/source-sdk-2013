//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PASSTIME_BALLCONTROLLER_H
#define PASSTIME_BALLCONTROLLER_H
#ifdef _WIN32
#pragma once
#endif

#include "util_shared.h"

class CPasstimeBall;
class CTFPlayer;
struct gamevcollisionevent_t;

//-----------------------------------------------------------------------------
class CPasstimeBallController : public TAutoList< CPasstimeBallController >
{
public:
	// ApplyTo returns the number of controllers that were applied.
	// BallCollision happens from vphysics callbacks.
	// BallPickedUp happens after player gets ball, but before ball 
	// is removed from world.
	static int ApplyTo( CPasstimeBall *pBall );
	static int DisableOn( const CPasstimeBall *pBall );
	static void BallCollision( CPasstimeBall *pBall, int iCollisionIndex, gamevcollisionevent_t *pEvent );
	static void BallPickedUp( CPasstimeBall *pBall, CTFPlayer *pCatcher );
	static void BallDamaged( CPasstimeBall *pBall );
	static void BallSpawned( CPasstimeBall *pBall );
	
	explicit CPasstimeBallController( int priority );
	virtual ~CPasstimeBallController() {}
	void SetIsEnabled( bool is );
	bool IsEnabled() const;
	int GetPriority() const;

protected:
	virtual bool Apply( CPasstimeBall *pBall ) = 0;
	virtual bool IsActive() const = 0;
	virtual void OnBallCollision( CPasstimeBall *pBall, int iCollisionIndex, gamevcollisionevent_t *pEvent ) {}
	virtual void OnBallPickedUp( CPasstimeBall *pBall, CTFPlayer *pCatcher ) {}
	virtual void OnBallDamaged( CPasstimeBall *pBall ) {}
	virtual void OnBallSpawned( CPasstimeBall *pBall ) {}
	virtual void OnEnabled() {}
	virtual void OnDisabled() {}

private:
	bool m_bEnabled;
	int m_iPriority; // higher priority comes first, must be > INT_MIN

	// noncopyable
	CPasstimeBallController( const CPasstimeBallController & ) = delete;
	CPasstimeBallController( CPasstimeBallController && ) = delete;
	CPasstimeBallController &operator=( const CPasstimeBallController & ) = delete;
	CPasstimeBallController &operator=( CPasstimeBallController && ) = delete;
};

#endif // PASSTIME_BALLCONTROLLER_H  


