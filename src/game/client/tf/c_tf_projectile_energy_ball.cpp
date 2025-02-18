//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_projectile_energy_ball.h"
#include "particles_new.h"
#include "SpriteTrail.h"
#include "c_tf_player.h"
#include "collisionutils.h"
#include "util_shared.h"
#include "tf_weapon_rocketlauncher.h"

//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_EnergyBall, DT_TFProjectile_EnergyBall )

BEGIN_NETWORK_TABLE( C_TFProjectile_EnergyBall, DT_TFProjectile_EnergyBall )
	RecvPropBool( RECVINFO( m_bChargedShot ) ),
	RecvPropVector( RECVINFO( m_vColor1 ) ),
	RecvPropVector( RECVINFO( m_vColor2 ) )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFProjectile_EnergyBall::C_TFProjectile_EnergyBall( void )
{
	pEffect = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFProjectile_EnergyBall::~C_TFProjectile_EnergyBall( void )
{
	if ( pEffect )
	{
		ParticleProp()->StopEmission( pEffect );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_EnergyBall::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	if ( pEffect )
	{
		ParticleProp()->StopEmission( pEffect );
		pEffect = NULL;
	}

	bool bDeflected = m_iCachedDeflect != GetDeflected();

	if ( pEffect == NULL )
	{
		ParticleProp()->Init( this );
		pEffect = ParticleProp()->Create( GetTrailParticleName(), PATTACH_ABSORIGIN_FOLLOW, 0 );

		if ( pEffect )
		{
			if ( bDeflected )
			{
				if ( GetTeamNumber() == TF_TEAM_BLUE )
				{
					pEffect->SetControlPoint( CUSTOM_COLOR_CP1, TF_PARTICLE_WEAPON_BLUE_1 );
					pEffect->SetControlPoint( CUSTOM_COLOR_CP2, TF_PARTICLE_WEAPON_BLUE_2 );
				}
				else
				{
					pEffect->SetControlPoint( CUSTOM_COLOR_CP1, TF_PARTICLE_WEAPON_RED_1 );
					pEffect->SetControlPoint( CUSTOM_COLOR_CP2, TF_PARTICLE_WEAPON_RED_2 );
				}
			}
			else
			{
				pEffect->SetControlPoint( CUSTOM_COLOR_CP1, m_vColor1 );
				pEffect->SetControlPoint( CUSTOM_COLOR_CP2, m_vColor2 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFProjectile_EnergyBall::GetTrailParticleName( void )
{
	if ( m_bChargedShot )
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_rockettrail_charged" : "drg_cow_rockettrail_charged_blue";
	else
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_rockettrail_normal" : "drg_cow_rockettrail_normal_blue";
}