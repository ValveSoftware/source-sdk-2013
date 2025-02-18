//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_spotlight.h"
#include "ai_basenpc.h"
#include "spotlightend.h"
#include "beam_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Parameters for how the scanner relates to citizens.
//-----------------------------------------------------------------------------
#define	SPOTLIGHT_WIDTH					96


//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
BEGIN_SIMPLE_DATADESC( CAI_Spotlight )

	// Robin: Don't save, recreated after restore/transition.
	//DEFINE_FIELD( m_hSpotlight,				FIELD_EHANDLE ),
	//DEFINE_FIELD( m_hSpotlightTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_vSpotlightTargetPos,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vSpotlightDir,			FIELD_VECTOR ),
	DEFINE_FIELD( m_flSpotlightCurLength,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flSpotlightMaxLength,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flConstraintAngle,		FIELD_FLOAT ),
	DEFINE_FIELD( m_nHaloSprite,			FIELD_MODELINDEX ),
	DEFINE_FIELD( m_nSpotlightAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_nFlags,					FIELD_INTEGER ),
	DEFINE_FIELD( m_vAngularVelocity,		FIELD_QUATERNION ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_Spotlight::CAI_Spotlight()
{
#ifdef _DEBUG
	m_vSpotlightTargetPos.Init();
	m_vSpotlightDir.Init();
#endif
}


CAI_Spotlight::~CAI_Spotlight()
{
	SpotlightDestroy();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_Spotlight::Precache(void)
{
	// Sprites
	m_nHaloSprite = GetOuter()->PrecacheModel("sprites/light_glow03.vmt");

	GetOuter()->PrecacheModel( "sprites/glow_test02.vmt" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_Spotlight::Init( CAI_BaseNPC *pOuter, int nFlags, float flConstraintAngle, float flMaxLength )
{
	SetOuter( pOuter );
	m_nFlags = nFlags;
	m_flConstraintAngle = flConstraintAngle; 
	m_flSpotlightMaxLength = flMaxLength;

	// Check for user error
	if (m_flSpotlightMaxLength <= 0)
	{
		DevMsg("ERROR: Invalid spotlight length <= 0, setting to 500\n");
		m_flSpotlightMaxLength = 500;
	}

	Precache();

	m_vSpotlightTargetPos	= vec3_origin;
	m_hSpotlight			= NULL;
	m_hSpotlightTarget		= NULL;

	AngleVectors( GetAbsAngles(), &m_vSpotlightDir );
	m_vAngularVelocity.Init( 0, 0, 0, 1 );
	m_flSpotlightCurLength	= m_flSpotlightMaxLength;
}


//------------------------------------------------------------------------------
// Computes the spotlight endpoint
//------------------------------------------------------------------------------
void CAI_Spotlight::ComputeEndpoint( const Vector &vecStartPoint, Vector *pEndPoint )
{
	// Create the endpoint
	trace_t tr;
	AI_TraceLine( vecStartPoint, vecStartPoint + m_vSpotlightDir * 2 * m_flSpotlightMaxLength, MASK_OPAQUE, GetOuter(), COLLISION_GROUP_NONE, &tr );
	*pEndPoint = tr.endpos;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CAI_Spotlight::SpotlightCreate( int nAttachment, const Vector &vecInitialDir )
{
	// Make sure we don't already have one
	if ( m_hSpotlight != NULL )
		return;

	m_vSpotlightDir	= vecInitialDir;
	VectorNormalize( m_vSpotlightDir );
	m_nSpotlightAttachment = nAttachment;

	CreateSpotlightEntities();
}

//-----------------------------------------------------------------------------
// Purpose: Create the beam & spotlight end for this spotlight.
//			Will be re-called after restore/transition
//-----------------------------------------------------------------------------
void CAI_Spotlight::CreateSpotlightEntities( void )
{
	m_vAngularVelocity.Init( 0, 0, 0, 1 );

	// Create the endpoint
	// Get the initial position...
	Vector vecStartPoint;
	if ( m_nSpotlightAttachment == 0 ) 
	{
		vecStartPoint = GetOuter()->GetAbsOrigin();
	}
	else
	{
		GetOuter()->GetAttachment( m_nSpotlightAttachment, vecStartPoint );
	}

	Vector vecEndPoint;
	ComputeEndpoint( vecStartPoint, &vecEndPoint );

	m_hSpotlightTarget = (CSpotlightEnd*)CreateEntityByName( "spotlight_end" );
	m_hSpotlightTarget->Spawn();
	m_hSpotlightTarget->SetAbsOrigin( vecEndPoint );
	m_hSpotlightTarget->SetOwnerEntity( GetOuter() );
	m_hSpotlightTarget->SetRenderColor( 255, 255, 255 );
	m_hSpotlightTarget->m_Radius = m_flSpotlightMaxLength;
	if ( FBitSet (m_nFlags, AI_SPOTLIGHT_NO_DLIGHTS) )
	{
		m_hSpotlightTarget->m_flLightScale = 0.0;
	}
	else
	{
		m_hSpotlightTarget->m_flLightScale = SPOTLIGHT_WIDTH;
	}

	// Create the beam
	m_hSpotlight = CBeam::BeamCreate( "sprites/glow_test02.vmt", SPOTLIGHT_WIDTH );
	// Set the temporary spawnflag on the beam so it doesn't save (we'll recreate it on restore)
	m_hSpotlight->AddSpawnFlags( SF_BEAM_TEMPORARY );
	m_hSpotlight->SetColor( 255, 255, 255 ); 
	m_hSpotlight->SetHaloTexture( m_nHaloSprite );
	m_hSpotlight->SetHaloScale( 32 );
	m_hSpotlight->SetEndWidth( m_hSpotlight->GetWidth() );
	m_hSpotlight->SetBeamFlags( (FBEAM_SHADEOUT|FBEAM_NOTILE) );
	m_hSpotlight->SetBrightness( 32 );
	m_hSpotlight->SetNoise( 0 );
	m_hSpotlight->EntsInit( GetOuter(), m_hSpotlightTarget );
	m_hSpotlight->SetStartAttachment( m_nSpotlightAttachment );
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CAI_Spotlight::SpotlightDestroy(void)
{
	if ( m_hSpotlight )
	{
		UTIL_Remove(m_hSpotlight);
		m_hSpotlight = NULL;
		
		UTIL_Remove(m_hSpotlightTarget);
		m_hSpotlightTarget = NULL;
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CAI_Spotlight::SetSpotlightTargetPos( const Vector &vSpotlightTargetPos )
{
	m_vSpotlightTargetPos = vSpotlightTargetPos;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CAI_Spotlight::SetSpotlightTargetDirection( const Vector &vSpotlightTargetDir )
{
	if ( !m_hSpotlight )
	{
		CreateSpotlightEntities();
	}

	VectorMA( m_hSpotlight->GetAbsStartPos(), 1000.0f, vSpotlightTargetDir, m_vSpotlightTargetPos );
}


//------------------------------------------------------------------------------
// Constrain to cone
//------------------------------------------------------------------------------
bool CAI_Spotlight::ConstrainToCone( Vector *pDirection )
{
	Vector vecOrigin, vecForward;
	if ( m_nSpotlightAttachment == 0 ) 
	{
		QAngle vecAngles;
		vecAngles = GetOuter()->GetAbsAngles();
		AngleVectors( vecAngles, &vecForward );
	}
	else
	{
		GetOuter()->GetAttachment( m_nSpotlightAttachment, vecOrigin, &vecForward );
	}


	if ( m_flConstraintAngle == 0.0f )
	{
		*pDirection = vecForward;
		return true;
	}

	float flDot = DotProduct( vecForward, *pDirection );
	if ( flDot >= cos( DEG2RAD( m_flConstraintAngle ) ) )
		return false;

	Vector vecAxis;
	CrossProduct( *pDirection, vecForward, vecAxis );
	VectorNormalize( vecAxis );

	Quaternion q;
	AxisAngleQuaternion( vecAxis, -m_flConstraintAngle, q );

	Vector vecResult;
	matrix3x4_t rot;
	QuaternionMatrix( q, rot );
	VectorRotate( vecForward, rot, vecResult );
	VectorNormalize( vecResult );

	*pDirection = vecResult;

	return true;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
#define QUAT_BLEND_FACTOR	0.4f

void CAI_Spotlight::UpdateSpotlightDirection( void )
{
	if ( !m_hSpotlight )
	{
		CreateSpotlightEntities();
	}

	// Compute the current beam direction
	Vector vTargetDir;
	VectorSubtract( m_vSpotlightTargetPos, m_hSpotlight->GetAbsStartPos(), vTargetDir ); 
	VectorNormalize(vTargetDir);
	ConstrainToCone( &vTargetDir );

	// Compute the amount to rotate
	float flDot = DotProduct( vTargetDir, m_vSpotlightDir );
	flDot = clamp( flDot, -1.0f, 1.0f );
	float flAngle = AngleNormalize( RAD2DEG( acos( flDot ) ) );
	float flClampedAngle = clamp( flAngle, 0.0f, 45.0f );
	float flBeamTurnRate = SimpleSplineRemapVal( flClampedAngle, 0.0f, 45.0f, 10.0f, 45.0f );
	if ( fabs(flAngle) > flBeamTurnRate * gpGlobals->frametime )
	{
		flAngle = flBeamTurnRate * gpGlobals->frametime;
	}

	// Compute the rotation axis
	Vector vecRotationAxis;
	CrossProduct( m_vSpotlightDir, vTargetDir, vecRotationAxis );
	if ( VectorNormalize( vecRotationAxis ) < 1e-3 )
	{
		vecRotationAxis.Init( 0, 0, 1 );
	}

	// Compute the actual rotation amount, using quat slerp blending
	Quaternion desiredQuat, resultQuat;
	AxisAngleQuaternion( vecRotationAxis, flAngle, desiredQuat );
	QuaternionSlerp( m_vAngularVelocity, desiredQuat, QUAT_BLEND_FACTOR, resultQuat );
	m_vAngularVelocity = resultQuat;

	// If we're really close, and we're not moving very quickly, slam.
	float flActualRotation = AngleNormalize( RAD2DEG(2 * acos(m_vAngularVelocity.w)) );
	if (( flActualRotation < 1e-3 ) && (flAngle < 1e-3 ))
	{
		m_vSpotlightDir = vTargetDir;
		m_vAngularVelocity.Init( 0, 0, 0, 1 );
		return;
	}

	// Update the desired direction
	matrix3x4_t rot;
	Vector vecNewDir;
	QuaternionMatrix( m_vAngularVelocity, rot );
	VectorRotate( m_vSpotlightDir, rot, vecNewDir );
	m_vSpotlightDir = vecNewDir;
	VectorNormalize(m_vSpotlightDir);
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CAI_Spotlight::UpdateSpotlightEndpoint( void )
{
	if ( !m_hSpotlight )
	{
		CreateSpotlightEntities();
	}

	Vector vecStartPoint, vecEndPoint;
	vecStartPoint = m_hSpotlight->GetAbsStartPos();
	ComputeEndpoint( vecStartPoint, &vecEndPoint );

	// If I'm not facing the spotlight turn it off 
	Vector vecSpotDir;
	VectorSubtract( vecEndPoint, vecStartPoint, vecSpotDir );
	float flBeamLength = VectorNormalize(vecSpotDir);
	
	m_hSpotlightTarget->SetAbsOrigin( vecEndPoint );
	m_hSpotlightTarget->SetAbsVelocity( vec3_origin );
	m_hSpotlightTarget->m_vSpotlightOrg = vecStartPoint;
	m_hSpotlightTarget->m_vSpotlightDir = vecSpotDir;

	// Avoid sudden change in where beam fades out when cross disconinuities
	m_flSpotlightCurLength = Lerp( 0.20f, m_flSpotlightCurLength, flBeamLength );

	// Fade out spotlight end if past max length.  
	if (m_flSpotlightCurLength > 2*m_flSpotlightMaxLength)
	{
		m_hSpotlightTarget->SetRenderColorA( 0 );
		m_hSpotlight->SetFadeLength(m_flSpotlightMaxLength);
	}
	else if (m_flSpotlightCurLength > m_flSpotlightMaxLength)		
	{
		m_hSpotlightTarget->SetRenderColorA( (1-((m_flSpotlightCurLength-m_flSpotlightMaxLength)/m_flSpotlightMaxLength)) );
		m_hSpotlight->SetFadeLength(m_flSpotlightMaxLength);
	}
	else
	{
		m_hSpotlightTarget->SetRenderColorA( 1.0 );
		m_hSpotlight->SetFadeLength(m_flSpotlightCurLength);
	}

	// Adjust end width to keep beam width constant
	float flNewWidth = SPOTLIGHT_WIDTH * ( flBeamLength / m_flSpotlightMaxLength );
	
	flNewWidth = MIN( 100, flNewWidth );

	m_hSpotlight->SetWidth(flNewWidth);
	m_hSpotlight->SetEndWidth(flNewWidth);

	// Adjust width of light on the end.  
	if ( FBitSet (m_nFlags, AI_SPOTLIGHT_NO_DLIGHTS) )
	{
		m_hSpotlightTarget->m_flLightScale = 0.0;
	}
	else
	{
		m_hSpotlightTarget->m_flLightScale = flNewWidth;
	}
}


//------------------------------------------------------------------------------
// Purpose: Update the direction and position of my spotlight (if it's active)
//------------------------------------------------------------------------------
void CAI_Spotlight::Update(void)
{
	if ( !m_hSpotlight )
	{
		CreateSpotlightEntities();
	}

	// Update the beam direction
	UpdateSpotlightDirection();
	UpdateSpotlightEndpoint();
}

