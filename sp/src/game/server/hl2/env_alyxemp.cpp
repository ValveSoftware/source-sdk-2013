//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Alyx's EMP effect
//
//=============================================================================//

#include "cbase.h"
#include "env_alyxemp_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	EMP_BEAM_SPRITE	"effects/laser1.vmt"


LINK_ENTITY_TO_CLASS( env_alyxemp, CAlyxEmpEffect );

BEGIN_DATADESC( CAlyxEmpEffect )
	
	DEFINE_KEYFIELD( m_nType,			FIELD_INTEGER,	"Type" ),
	DEFINE_KEYFIELD( m_strTargetName,	FIELD_STRING,	"EndTargetName" ),

	DEFINE_FIELD( m_nState,			FIELD_INTEGER ),
	DEFINE_FIELD( m_flDuration,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flStartTime,	FIELD_TIME ),
	DEFINE_FIELD( m_hTargetEnt,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_hBeam,			FIELD_EHANDLE ),

	DEFINE_FIELD( m_iState,			FIELD_INTEGER ),
	DEFINE_FIELD( m_bAutomated,		FIELD_BOOLEAN ),

	DEFINE_THINKFUNC( AutomaticThink ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "StartCharge", InputStartCharge ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartDischarge", InputStartDischarge ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Stop", InputStop ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTargetEnt", InputSetTargetEnt ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CAlyxEmpEffect, DT_AlyxEmpEffect )
	SendPropInt( SENDINFO(m_nState), 8, SPROP_UNSIGNED),
	SendPropFloat( SENDINFO(m_flDuration), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flStartTime), 0, SPROP_NOSCALE),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::Spawn( void )
{
	Precache();

	// No model but we still need to force this!
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	// No shadows
	AddEffects( EF_NOSHADOW | EF_NORECEIVESHADOW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::Activate( void )
{
	// Start out with a target entity
	SetTargetEntity( STRING(m_strTargetName) );
	
	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szEntityName - 
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::SetTargetEntity( const char *szEntityName )
{
	// Find and store off our target entity
	CBaseEntity *pTargetEnt = NULL;
	if ( szEntityName && szEntityName[0] )
	{
		pTargetEnt = gEntList.FindEntityByName( NULL, szEntityName );

		if ( pTargetEnt == NULL )
		{
			Assert(0);
			DevMsg( "Unable to find env_alyxemp (%s) target %s!\n", GetEntityName().ToCStr(), szEntityName );
		}
	}

	SetTargetEntity( pTargetEnt );
}

//-----------------------------------------------------------------------------
// Passing NULL is ok!
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::SetTargetEntity( CBaseEntity *pTarget )
{
	m_hTargetEnt.Set( pTarget );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::ActivateAutomatic( CBaseEntity *pAlyx, CBaseEntity *pTarget )
{
	Assert( pAlyx->GetBaseAnimating() != NULL );

	SetParent( pAlyx, pAlyx->GetBaseAnimating()->LookupAttachment("LeftHand") );
	SetLocalOrigin( vec3_origin );

	m_iState = ALYXEMP_STATE_OFF;
	SetTargetEntity( pTarget );
	SetThink( &CAlyxEmpEffect::AutomaticThink );
	SetNextThink( gpGlobals->curtime );

	m_bAutomated = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::AutomaticThink()
{
	bool bSetNextThink = true;

	switch( m_iState )
	{
	case ALYXEMP_STATE_OFF:
		StartCharge( 0.05f );
		break;

	case ALYXEMP_STATE_CHARGING:
		StartDischarge();
		break;

	case ALYXEMP_STATE_DISCHARGING:
		Stop( 1.0f );
		bSetNextThink = false;
		break;
	}

	m_iState++;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::Precache( void )
{
	PrecacheModel( EMP_BEAM_SPRITE );

	PrecacheScriptSound( "AlyxEmp.Charge" );
	PrecacheScriptSound( "AlyxEmp.Discharge" );
	PrecacheScriptSound( "AlyxEmp.Stop" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::InputStartCharge( inputdata_t &inputdata )
{
	StartCharge( inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::StartCharge( float flDuration )
{
	EmitSound( "AlyxEmp.Charge" );

	m_nState = (int)ALYXEMP_STATE_CHARGING;
	m_flDuration = flDuration;
	m_flStartTime = gpGlobals->curtime;

	if( m_bAutomated )
	{
		SetNextThink( gpGlobals->curtime + m_flDuration );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::InputStartDischarge( inputdata_t &inputdata )
{
	StartDischarge();
}

void CAlyxEmpEffect::StartDischarge()
{
	EmitSound( "AlyxEmp.Discharge" );

	m_nState = (int)ALYXEMP_STATE_DISCHARGING;
	m_flStartTime = gpGlobals->curtime;

	// Beam effects on the target entity!
	if ( !m_hBeam && m_hTargetEnt )
	{
		// Check to store off our view model index
		m_hBeam = CBeam::BeamCreate( EMP_BEAM_SPRITE, 8 );

		if ( m_hBeam != NULL )
		{
			m_hBeam->PointEntInit( m_hTargetEnt->GetAbsOrigin(), this );
			m_hBeam->SetStartEntity( m_hTargetEnt );
			m_hBeam->SetWidth( 4 );
			m_hBeam->SetEndWidth( 8 );
			m_hBeam->SetBrightness( 255 );
			m_hBeam->SetColor( 255, 255, 255 );
			m_hBeam->LiveForTime( 999.0f );
			m_hBeam->RelinkBeam();
			m_hBeam->SetNoise( 16 );
		}

		// End hit
		Vector shotDir = ( GetAbsOrigin() - m_hTargetEnt->GetAbsOrigin() );
		VectorNormalize( shotDir );

		CPVSFilter filter( m_hTargetEnt->GetAbsOrigin() );
		te->GaussExplosion( filter, 0.0f, m_hTargetEnt->GetAbsOrigin() - ( shotDir * 4.0f ), RandomVector(-1.0f, 1.0f), 0 );
	}

	if( m_bAutomated )
	{
		SetNextThink( gpGlobals->curtime + 0.5f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::InputStop( inputdata_t &inputdata )
{
	float flDuration = inputdata.value.Float();

	Stop( flDuration );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::Stop( float flDuration )
{
	EmitSound( "AlyxEmp.Stop" );

	m_nState = (int)ALYXEMP_STATE_OFF;
	m_flDuration = flDuration;
	m_flStartTime = gpGlobals->curtime;

	if ( m_hBeam != NULL )
	{
		UTIL_Remove( m_hBeam );
		m_hBeam = NULL;
	}

	if( m_bAutomated )
	{
		SetThink( &CAlyxEmpEffect::SUB_Remove );
		SetNextThink( gpGlobals->curtime + flDuration + 1.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAlyxEmpEffect::InputSetTargetEnt( inputdata_t &inputdata )
{
	SetTargetEntity( inputdata.value.String() );
}
