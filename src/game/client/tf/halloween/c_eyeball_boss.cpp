//========= Copyright Valve Corporation, All rights reserved. ============//
// c_eyeball_boss.cpp

#include "cbase.h"
#include "NextBot/C_NextBot.h"
#include "c_eyeball_boss.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef NextBot



//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_EyeballBoss, DT_EyeballBoss, CEyeballBoss )

	RecvPropVector( RECVINFO( m_lookAtSpot ) ),
	RecvPropInt( RECVINFO( m_attitude ) ),
	
END_RECV_TABLE()


//-----------------------------------------------------------------------------
C_EyeballBoss::C_EyeballBoss()
{
	m_ghostEffect = NULL;
	m_auraEffect = NULL;
	m_attitude = EYEBALL_CALM;
	m_priorAttitude = m_attitude;
}


//-----------------------------------------------------------------------------
C_EyeballBoss::~C_EyeballBoss()
{
	if ( m_ghostEffect )
	{
		ParticleProp()->StopEmission( m_ghostEffect );
		m_ghostEffect = NULL;
	}

	if ( m_auraEffect )
	{
		ParticleProp()->StopEmission( m_auraEffect );
		m_auraEffect = NULL;
	}
}


//-----------------------------------------------------------------------------
void C_EyeballBoss::Spawn( void )
{
	BaseClass::Spawn();

	m_leftRightPoseParameter = -1;
	m_upDownPoseParameter = -1;

	m_myAngles = vec3_angle;

	m_attitude = EYEBALL_CALM;
	m_priorAttitude = m_attitude;

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


//-----------------------------------------------------------------------------
void C_EyeballBoss::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_priorAttitude = m_attitude;
}


//-----------------------------------------------------------------------------
void C_EyeballBoss::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		const char *pszMaterial = NULL;
		const char *pszAuraEffect = "eyeboss_aura_calm";
		switch ( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			{
				pszAuraEffect = "eyeboss_team_red";
				//pszMaterial = "models/effects/invulnfx_red.vmt";
			}
			break;
		case TF_TEAM_BLUE:
			{
				pszAuraEffect = "eyeboss_team_blue";
				//pszMaterial = "models/effects/invulnfx_blue.vmt";
			}
			break;
		default:
			{
				if ( !m_ghostEffect )
				{
					m_ghostEffect = ParticleProp()->Create( "ghost_pumpkin", PATTACH_ABSORIGIN_FOLLOW );
				}
			}
			break;
		}

		if ( !m_auraEffect )
		{
			m_auraEffect = ParticleProp()->Create( pszAuraEffect, PATTACH_ABSORIGIN_FOLLOW );
		}

		if ( pszMaterial )
		{
			m_InvulnerableMaterial.Init( pszMaterial, TEXTURE_GROUP_CLIENT_EFFECTS );
		}
		else
		{
			m_InvulnerableMaterial.Shutdown();
		}
	}
	else if ( GetTeamNumber() == TF_TEAM_HALLOWEEN )
	{
		// update eyeball aura
		if ( m_attitude != m_priorAttitude )
		{
			// kill the old aura
			if ( m_auraEffect )
			{
				ParticleProp()->StopEmission( m_auraEffect );
			}

			switch( m_attitude )
			{
			case EYEBALL_CALM:
				m_auraEffect = ParticleProp()->Create( "eyeboss_aura_calm", PATTACH_ABSORIGIN_FOLLOW );
				break;

			case EYEBALL_GRUMPY:
				m_auraEffect = ParticleProp()->Create( "eyeboss_aura_grumpy", PATTACH_ABSORIGIN_FOLLOW );
				break;

			case EYEBALL_ANGRY:
				m_auraEffect = ParticleProp()->Create( "eyeboss_aura_angry", PATTACH_ABSORIGIN_FOLLOW );
				break;
			}

			m_priorAttitude = m_attitude;
		}
	}
}

//-----------------------------------------------------------------------------
void C_EyeballBoss::ClientThink( void )
{
	// update eyeball aim
	if ( m_leftRightPoseParameter < 0 )
	{
		m_leftRightPoseParameter = LookupPoseParameter( "left_right" );
	}

	if ( m_upDownPoseParameter < 0 )
	{
		m_upDownPoseParameter = LookupPoseParameter( "up_down" );
	}


	Vector myForward, myRight, myUp;
	AngleVectors( m_myAngles, &myForward, &myRight, &myUp );


	const float myApproachRate = 3.0f; // 1.0f;

	Vector toTarget = m_lookAtSpot - WorldSpaceCenter();
	toTarget.NormalizeInPlace();

	myForward += toTarget * myApproachRate * gpGlobals->frametime;
	myForward.NormalizeInPlace();

	QAngle myNewAngles;
	VectorAngles( myForward, myNewAngles );

	SetAbsAngles( myNewAngles );
	m_myAngles = myNewAngles;



	// set pose parameters to aim pupil directly at target
	float toTargetRight = DotProduct( myRight, toTarget );
	float toTargetUp = DotProduct( myUp, toTarget );

	if ( m_leftRightPoseParameter >= 0 )
	{
		int angle = -50 * toTargetRight;

		SetPoseParameter( m_leftRightPoseParameter, angle );
	}

	if ( m_upDownPoseParameter >= 0 )
	{
		int angle = -50 * toTargetUp;

		SetPoseParameter( m_upDownPoseParameter, angle );
	}
}


//-----------------------------------------------------------------------------
void C_EyeballBoss::SetDormant( bool bDormant )
{
	BaseClass::SetDormant( bDormant );
}


//-----------------------------------------------------------------------------
void C_EyeballBoss::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
}


//-----------------------------------------------------------------------------
QAngle const &C_EyeballBoss::GetRenderAngles( void )
{
	return m_myAngles;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_EyeballBoss::InternalDrawModel( int flags )
{
	bool bUseInvulnMaterial = ( GetTeamNumber() == TF_TEAM_RED ) || ( GetTeamNumber() == TF_TEAM_BLUE );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( m_InvulnerableMaterial );
	}

	int ret = BaseClass::InternalDrawModel( flags );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}