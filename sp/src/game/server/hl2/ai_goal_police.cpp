//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_goal_police.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ai_goal_police

// Used by police to define a region they should keep a target outside of

LINK_ENTITY_TO_CLASS( ai_goal_police, CAI_PoliceGoal );

BEGIN_DATADESC( CAI_PoliceGoal )

	DEFINE_KEYFIELD( m_flRadius,	FIELD_FLOAT,	"PoliceRadius" ),
	DEFINE_KEYFIELD( m_iszTarget,	FIELD_STRING,	"PoliceTarget" ),
	
	DEFINE_FIELD( m_bOverrideKnockOut, FIELD_BOOLEAN ),
	// m_hTarget

	DEFINE_INPUTFUNC( FIELD_VOID, "EnableKnockOut", InputEnableKnockOut ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableKnockOut", InputDisableKnockOut ),

	DEFINE_OUTPUT( m_OnKnockOut,		"OnKnockOut" ),
	DEFINE_OUTPUT( m_OnFirstWarning,	"OnFirstWarning" ),
	DEFINE_OUTPUT( m_OnSecondWarning,	"OnSecondWarning" ),
	DEFINE_OUTPUT( m_OnLastWarning,		"OnLastWarning" ),
	DEFINE_OUTPUT( m_OnSupressingTarget,"OnSupressingTarget" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_PoliceGoal::CAI_PoliceGoal( void )
{
	m_hTarget = NULL;
	m_bOverrideKnockOut = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CAI_PoliceGoal::GetRadius( void )
{
	return m_flRadius;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CAI_PoliceGoal::GetTarget( void )
{
	if ( m_hTarget == NULL )
	{
		CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_iszTarget );

		if ( pTarget == NULL )
		{
			DevMsg( "Unable to find ai_goal_police target: %s\n", STRING(m_iszTarget) );
			return NULL;
		}

		m_hTarget = pTarget;
	}

	return m_hTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &targetPos - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PoliceGoal::ShouldKnockOutTarget( const Vector &targetPos, bool bTargetVisible )
{
	if ( m_bOverrideKnockOut )
		return true;

	// Must be flagged to do it
	if ( HasSpawnFlags( SF_POLICE_GOAL_KNOCKOUT_BEHIND ) == false )
		return false;

	// If the target's not visible, we don't care about him
	if ( !bTargetVisible )
		return false;

	Vector targetDir = targetPos - GetAbsOrigin();
	VectorNormalize( targetDir );

	Vector	facingDir;
	AngleVectors( GetAbsAngles(), &facingDir );

	// See if it's behind us
	if ( DotProduct( facingDir, targetDir ) < 0 )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CAI_PoliceGoal::KnockOutTarget( CBaseEntity *pTarget )
{
	m_OnKnockOut.FireOutput( pTarget, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PoliceGoal::ShouldRemainAtPost( void )
{
	return HasSpawnFlags( SF_POLICE_GOAL_DO_NOT_LEAVE_POST );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : level - 
//-----------------------------------------------------------------------------
void CAI_PoliceGoal::FireWarningLevelOutput( int level )
{
	switch( level )
	{
	case 1:
		m_OnFirstWarning.FireOutput( this, this );
		break;

	case 2:
		m_OnSecondWarning.FireOutput( this, this );
		break;

	case 3:
		m_OnLastWarning.FireOutput( this, this );
		break;

	default:
		m_OnSupressingTarget.FireOutput( this, this );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CAI_PoliceGoal::InputEnableKnockOut( inputdata_t &data )
{
	m_bOverrideKnockOut = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CAI_PoliceGoal::InputDisableKnockOut( inputdata_t &data )
{
	m_bOverrideKnockOut = false;
}

