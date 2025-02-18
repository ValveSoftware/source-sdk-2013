//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PASSTIME_BALLCONTROLLER_PLAYERSEEK_H
#define PASSTIME_BALLCONTROLLER_PLAYERSEEK_H
#ifdef _WIN32
#pragma once
#endif

#include "passtime_ballcontroller.h"

class CBaseEntity;

//-----------------------------------------------------------------------------
class CPasstimeBallControllerPlayerSeek : public CPasstimeBallController
{
public:
	CPasstimeBallControllerPlayerSeek();
private:
	virtual bool IsActive() const OVERRIDE { return true; }
	virtual bool Apply( CPasstimeBall *ball ) OVERRIDE;
	virtual void OnDisabled() OVERRIDE { SetIsEnabled( true ); } // never actually disable
	virtual void OnBallSpawned( CPasstimeBall *ball ) OVERRIDE;

	CTFPlayer *FindTarget( CTFPlayer *pIgnorePlayer, const Vector& ballOrigin ) const;
	bool Seek( CPasstimeBall *ball, CTFPlayer *pTarget ) const;

	float m_fEnableTime;
};

#endif // PASSTIME_BALLCONTROLLER_PLAYERSEEK_H  
