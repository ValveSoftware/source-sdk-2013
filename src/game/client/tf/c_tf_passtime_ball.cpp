//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_tf_passtime_ball.h"
#include "passtime_convars.h"
#include "tf_shareddefs.h"
#include "c_tf_player.h"
#include "c_playerresource.h"
#include "c_tf_player.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_PasstimeBall, DT_PasstimeBall, CPasstimeBall )
	RecvPropInt(RECVINFO(m_iCollisionCount)),
	RecvPropEHandle(RECVINFO(m_hHomingTarget)),
	RecvPropEHandle(RECVINFO(m_hCarrier)),
	RecvPropEHandle(RECVINFO(m_hPrevCarrier)),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( passtime_ball, C_PasstimeBall );
PRECACHE_REGISTER( passtime_ball );

C_TFPlayer *C_PasstimeBall::GetCarrier() { return m_hCarrier.Get(); }
C_TFPlayer *C_PasstimeBall::GetPrevCarrier() { return m_hPrevCarrier.Get(); }

//-----------------------------------------------------------------------------
C_PasstimeBall::C_PasstimeBall()
{
	UseClientSideAnimation();
	m_fDrawTime = 0.0f;
}

//-----------------------------------------------------------------------------
C_PasstimeBall::~C_PasstimeBall()
{
}

//-----------------------------------------------------------------------------
unsigned int C_PasstimeBall::PhysicsSolidMaskForEntity() const 
{ 
	return MASK_PLAYERSOLID; // must match server
}
 

//-----------------------------------------------------------------------------
bool C_PasstimeBall::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	// note: returning false for COLLISION_GROUP_PLAYER_MOVEMENT means the ball won't
	// stop player movement. the only real visible effect when this function doesn't
	// return false for COLLISION_GROUP_PLAYER_MOVEMENT is that the ball is unable 
	// to impart physics forces on the ball when the ball is blocked, since the player
	// will set velocity to zero due to being "stuck" on the ball, even though the
	// ball won't actually prevent the player from moving through it.
	return (collisionGroup != COLLISION_GROUP_PLAYER_MOVEMENT);
	//	&& (contentsMask & MASK_SHOT_HULL);
	//return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

//-----------------------------------------------------------------------------
void C_PasstimeBall::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	
	if ( updateType == DATA_UPDATE_CREATED )
	{
		return;
	}

	if ( TFGameRules()->IsPasstimeMode() )
	{
		bool bIsVisible = !(GetEffects() & EF_NODRAW);
		if ( bIsVisible && !m_bWasVisible )
		{
			float nextValidTime = gpGlobals->curtime + 0.1f;
			m_fDrawTime = nextValidTime;
		}
		m_bWasVisible = bIsVisible;
	}
}

//-----------------------------------------------------------------------------
int C_PasstimeBall::DrawModel( int flags )
{
	if( gpGlobals->curtime < m_fDrawTime )
	{
		return 0;
	}

	return BaseClass::DrawModel( flags );
}
