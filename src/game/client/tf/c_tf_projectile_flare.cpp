//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_projectile_flare.h"
#include "tf_weapon_flaregun.h"
#include "c_tf_player.h"
#include "particles_new.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Flare, DT_TFProjectile_Flare )

BEGIN_NETWORK_TABLE( C_TFProjectile_Flare, DT_TFProjectile_Flare )
	RecvPropBool( RECVINFO( m_bCritical ) ),
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFProjectile_Flare::C_TFProjectile_Flare( void )
{
	pEffect = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFProjectile_Flare::~C_TFProjectile_Flare( void )
{
	if ( pEffect )
	{
		ParticleProp()->StopEmission( pEffect );
		pEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Flare::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateTrails();		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GetFlareTrailParticleName( int iTeamNumber, bool bCritical, int nType )
{
	if ( nType == FLAREGUN_GRORDBORT )
	{
		return "drg_manmelter_projectile";
	}
	else if ( nType == FLAREGUN_SCORCHSHOT )
	{
		if ( iTeamNumber == TF_TEAM_BLUE )
		{
			return ( bCritical ? "scorchshot_trail_crit_blue" : "scorchshot_trail_blue" );
		}
		else
		{
			return ( bCritical ? "scorchshot_trail_crit_red" : "scorchshot_trail_red" );
		}
	}
	else
	{
		if ( iTeamNumber == TF_TEAM_BLUE )
		{
			return ( bCritical ? "flaregun_trail_crit_blue" : "flaregun_trail_blue" );
		}
		else
		{
			return ( bCritical ? "flaregun_trail_crit_red" : "flaregun_trail_red" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Flare::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	if ( pEffect )
	{
		ParticleProp()->StopEmission( pEffect );
		pEffect = NULL;
	}

	int nType = 0;

	C_TFFlareGun *pFlareGun = dynamic_cast< C_TFFlareGun* >( GetLauncher() );
	if ( pFlareGun )
	{
		nType = pFlareGun->GetFlareGunType();
	}

	pEffect = ParticleProp()->Create( GetFlareTrailParticleName( GetTeamNumber(), m_bCritical, nType ), PATTACH_ABSORIGIN_FOLLOW );
}
