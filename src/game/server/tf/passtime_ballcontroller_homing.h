//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PASSTIME_BALLCONTROLLER_HOMING_H
#define PASSTIME_BALLCONTROLLER_HOMING_H
#ifdef _WIN32
#pragma once
#endif

#include "passtime_ballcontroller.h"

//-----------------------------------------------------------------------------
class CPasstimeBallControllerHoming : public CPasstimeBallController
{
public:
	CPasstimeBallControllerHoming();
	~CPasstimeBallControllerHoming();
	void SetTargetSpeed( float f );
	void SetMaxBounces( int i ) { m_iMaxBounces = i; }
	void StartHoming( CPasstimeBall *pBall, CTFPlayer *pTarget, bool isCharged );

private:
	CHandle<CTFPlayer> m_hTarget;
	CHandle<CPasstimeBall> m_hBall;
	float m_fTargetSpeed;
	bool m_bIsHoming;
	float m_fHomingStrength;
	int m_iMaxBounces;

	void StopHoming();
	virtual bool IsActive() const OVERRIDE;
	virtual bool Apply( CPasstimeBall *ball ) OVERRIDE;
	virtual void OnBallCollision( CPasstimeBall *ball, int index, gamevcollisionevent_t *ev ) OVERRIDE;
	virtual void OnBallPickedUp( CPasstimeBall *ball, CTFPlayer *catcher ) OVERRIDE;
	virtual void OnBallDamaged( CPasstimeBall *ball ) OVERRIDE;
	virtual void OnBallSpawned( CPasstimeBall *ball ) OVERRIDE;
	virtual void OnDisabled() OVERRIDE;
};

#endif // PASSTIME_BALLCONTROLLER_HOMING_H
