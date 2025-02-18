//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TF_PASSTIME_BALL_H
#define C_TF_PASSTIME_BALL_H
#ifdef _WIN32
#pragma once
#endif

#include "predictable_entity.h"
#include "util_shared.h"
#include "c_baseanimating.h"
#include "../shared/SpriteTrail.h"

class C_TFPlayer;

//-----------------------------------------------------------------------------
class C_PasstimeBall : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_PasstimeBall, C_BaseAnimating );
	DECLARE_NETWORKCLASS();
	C_PasstimeBall();
	virtual ~C_PasstimeBall();
	virtual void OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual int DrawModel( int flags ) OVERRIDE;
	virtual unsigned int PhysicsSolidMaskForEntity() const OVERRIDE;
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const OVERRIDE;
	
	virtual void AddDecal( const Vector& rayStart, const Vector& rayEnd,
		const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, 
		trace_t& tr, int maxLODToDecal ) OVERRIDE { }// no decals ever

	int GetCollisionCount() const { return m_iCollisionCount; }
	C_TFPlayer *GetHomingTarget() const { return m_hHomingTarget; }
	C_TFPlayer *GetCarrier();
	C_TFPlayer *GetPrevCarrier();
	
private:
	bool m_bWasVisible;
	float m_fDrawTime;
	CNetworkVar( int, m_iCollisionCount );
	CNetworkHandle( C_TFPlayer, m_hHomingTarget );
	CNetworkHandle( C_TFPlayer, m_hCarrier );
	CNetworkHandle( C_TFPlayer, m_hPrevCarrier );
};

#endif // C_TF_PASSTIME_BALL_H  
