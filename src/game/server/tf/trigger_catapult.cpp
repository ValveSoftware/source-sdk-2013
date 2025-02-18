//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "trigger_catapult.h"

#include "vcollide_parse.h"
#include "props.h"
#include "movevars_shared.h"
#include "tf_player.h"
#include "saverestore_utlvector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *CTriggerCatapult::s_szPlayerPassesTriggerFiltersThinkContext = "CTriggerCatapult::PlayerPassesTriggerFiltersThink";

extern ConVar catapult_physics_drag_boost;


BEGIN_DATADESC( CTriggerCatapult )

	DEFINE_THINKFUNC( LaunchThink ),
	DEFINE_THINKFUNC( PlayerPassesTriggerFiltersThink ),

	DEFINE_KEYFIELD( m_flPlayerVelocity,	FIELD_FLOAT,	"playerSpeed" ),
	DEFINE_KEYFIELD( m_flPhysicsVelocity,	FIELD_FLOAT,	"physicsSpeed" ),
	DEFINE_KEYFIELD( m_vecLaunchAngles,		FIELD_VECTOR,	"launchDirection" ),
	DEFINE_KEYFIELD( m_strLaunchTarget,		FIELD_STRING,	"launchTarget" ),
	DEFINE_KEYFIELD( m_bUseThresholdCheck,		FIELD_BOOLEAN,	"useThresholdCheck" ),
	DEFINE_KEYFIELD( m_bUseExactVelocity,	FIELD_BOOLEAN,	"useExactVelocity" ),
	DEFINE_KEYFIELD( m_flLowerThreshold,	FIELD_FLOAT,	"lowerThreshold" ),
	DEFINE_KEYFIELD( m_flUpperThreshold,	FIELD_FLOAT,	"upperThreshold" ),
	DEFINE_KEYFIELD( m_ExactVelocityChoice,	FIELD_INTEGER,	"exactVelocityChoiceType" ),
	DEFINE_KEYFIELD( m_bOnlyVelocityCheck, FIELD_BOOLEAN, "onlyVelocityCheck" ),
	DEFINE_KEYFIELD( m_bApplyAngularImpulse, FIELD_BOOLEAN, "applyAngularImpulse" ),

	DEFINE_KEYFIELD( m_flEntryAngleTolerance,	FIELD_FLOAT,	"EntryAngleTolerance" ),
	DEFINE_KEYFIELD( m_flAirControlSupressionTime,	FIELD_FLOAT,	"AirCtrlSupressionTime" ),
	DEFINE_KEYFIELD( m_bDirectionSuppressAirControl, FIELD_BOOLEAN, "DirectionSuppressAirControl" ),

	DEFINE_FIELD( m_hLaunchTarget, FIELD_EHANDLE ),
	DEFINE_ARRAY( m_flRefireDelay, FIELD_TIME, MAX_PLAYERS_ARRAY_SAFE ),

	DEFINE_UTLVECTOR( m_hAbortedLaunchees, FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPlayerSpeed", InputSetPlayerSpeed ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPhysicsSpeed", InputSetPhysicsSpeed ),
	DEFINE_INPUTFUNC( FIELD_STRING,"SetLaunchTarget", InputSetLaunchTarget ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetExactVelocityChoiceType", InputSetExactVelocityChoiceType ),

	DEFINE_OUTPUT( m_OnCatapulted, "OnCatapulted" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_catapult, CTriggerCatapult );

//IMPLEMENT_SERVERCLASS_ST( CTriggerCatapult, DT_TriggerCatapult )
//	SendPropArray3( SENDINFO_ARRAY3(m_flRefireDelay), SendPropFloat(SENDINFO_ARRAY(m_flRefireDelay)) ),
//	SendPropFloat( SENDINFO( m_flPlayerVelocity ) ),
//	SendPropFloat( SENDINFO( m_flPhysicsVelocity ) ),
//	SendPropQAngles( SENDINFO( m_vecLaunchAngles ) ),
//	//SendPropStringT( SENDINFO( m_strLaunchTarget ) ),
//	SendPropInt( SENDINFO( m_ExactVelocityChoice ) ),
//	SendPropBool( SENDINFO( m_bUseExactVelocity ) ),
//	SendPropBool( SENDINFO( m_bUseThresholdCheck ) ),
//	SendPropBool( SENDINFO( m_bOnlyVelocityCheck ) ),
//	SendPropFloat( SENDINFO( m_flLowerThreshold ) ),
//	SendPropFloat( SENDINFO( m_flUpperThreshold ) ),
//	SendPropFloat( SENDINFO( m_flAirControlSupressionTime ) ),
//	SendPropBool( SENDINFO( m_bApplyAngularImpulse ) ),
//	SendPropFloat( SENDINFO( m_flEntryAngleTolerance ) ),
//	SendPropEHandle( SENDINFO( m_hLaunchTarget ) ),
//	SendPropBool( SENDINFO( m_bPlayersPassTriggerFilters ) ),
//	SendPropBool( SENDINFO( m_bDirectionSuppressAirControl ) ),
//END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTriggerCatapult::CTriggerCatapult( void )
{
	//Defaulting to true;
	m_bApplyAngularImpulse = true;
	m_flAirControlSupressionTime = -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCatapult::DrawDebugGeometryOverlays( void )
{
	BaseClass::DrawDebugGeometryOverlays();
	CBaseEntity *pLaunchTarget = m_hLaunchTarget;
	if ( pLaunchTarget )
	{
		// Help us visualize the target
		Vector vecSourcePos = GetAbsOrigin();
		Vector vecTargetPos = pLaunchTarget->GetAbsOrigin();

		float flSpeed = m_flPlayerVelocity;
		float flGravity = sv_gravity.GetFloat();

		Vector vecVelocity = (vecTargetPos - vecSourcePos);

		// This is a hack to get around air resistance with weighted cubes -- this is not intended for all objects!
		// float flDragCoefficient = (pVictim->IsPlayer()) ? 1.0f : ( 1.6f );
		float flDragCoefficient = 0.0f;

		// throw at a constant time
		float time = vecVelocity.Length( ) / flSpeed;
		vecVelocity = vecVelocity * (1.0 / time) * flDragCoefficient;

		// adjust upward toss to compensate for gravity loss
		vecVelocity.z += flGravity * time * 0.5;

		Vector vecApex = vecSourcePos + (vecTargetPos - vecSourcePos) * 0.5;
		vecApex.z += 0.5 * flGravity * (time * 0.5) * (time * 0.5);

		// Visualize it!
		if( !m_bUseExactVelocity )
		{
			NDebugOverlay::Box( vecSourcePos, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, 8.0f, 0.05f );
			NDebugOverlay::Box( vecTargetPos, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, 8.0f, 0.05f );
			NDebugOverlay::Box( vecApex, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, 8.0f, 0.05f );
			NDebugOverlay::Line( vecSourcePos, vecApex, 0, 255, 0, false, 0.05f );
			NDebugOverlay::Line( vecApex, vecTargetPos, 0, 255, 0, false, 0.05f );
		}
		else
		{
			Vector lastPos = vecSourcePos;
			vecVelocity = (vecTargetPos - vecSourcePos);
			vecVelocity = CalculateLaunchVectorPreserve( vecVelocity, this, pLaunchTarget, true );
			for( int i = 0; i < 20; i++ )
			{
				float flTime = 0.2f*(i+1);
				
				vecApex = vecSourcePos + vecVelocity*flTime;
				vecApex.z -= 0.5 * flGravity * (flTime) * (flTime);
				NDebugOverlay::Box( vecApex, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, 8.0f, 0.05f );
				NDebugOverlay::Line( vecApex, lastPos, 0, 255, 0, false, 0.05f );
				lastPos = vecApex;
			}
		}

		// Physics!
		flSpeed = m_flPhysicsVelocity;
		vecVelocity = (vecTargetPos - vecSourcePos);

		// This is a hack to get around air resistance with weighted cubes -- this is not intended for all objects!
		flDragCoefficient = catapult_physics_drag_boost.GetFloat();

		// throw at a constant time
		time = vecVelocity.Length( ) / flSpeed;
		vecVelocity = vecVelocity * (1.0 / time) * flDragCoefficient;

		// adjust upward toss to compensate for gravity loss
		vecVelocity.z += flGravity * time * 0.5;

		vecApex = vecSourcePos + (vecTargetPos - vecSourcePos) * 0.5;
		vecApex.z += 0.5 * flGravity * (time * 0.5) * (time * 0.5);

		// Visualize it!
		if( !m_bUseExactVelocity )
		{
			NDebugOverlay::Box( vecApex, -Vector(2,2,2), Vector(2,2,2), 255, 255, 0, 8.0f, 0.05f );
			NDebugOverlay::Line( vecSourcePos, vecApex, 255, 255, 0, false, 0.05f );
			NDebugOverlay::Line( vecApex, vecTargetPos, 255, 255, 0, false, 0.05f );
		}
		else
		{
			Vector lastPos = vecSourcePos;
			vecVelocity = (vecTargetPos - vecSourcePos);
			vecVelocity = CalculateLaunchVectorPreserve( vecVelocity, this, pLaunchTarget );
			for( int i = 0; i < 20; i++ )
			{
				float flTime = 0.2f*(i+1);
				
				vecApex = vecSourcePos + vecVelocity*flTime;
				vecApex.z -= 0.5 * flGravity * (flTime) * (flTime);
				NDebugOverlay::Box( vecApex, -Vector(2,2,2), Vector(2,2,2), 255, 255, 0, 8.0f, 0.05f );
				NDebugOverlay::Line( vecApex, lastPos, 255, 255, 0, false, 0.05f );
				lastPos = vecApex;
			}
		}
	}
	else
	{
		// Meh
	}
}
//---------------------------------------------------------
//---------------------------------------------------------
int CTriggerCatapult::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();
	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr), "Launch target: %s", m_strLaunchTarget.ToCStr() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		if ( m_bUseThresholdCheck )
		{		
			Q_snprintf(tempstr,sizeof(tempstr), "Lower threshold velocity: %.2f", m_flLowerThreshold );
			EntityText(text_offset,tempstr,0);
			text_offset++;

			Q_snprintf(tempstr,sizeof(tempstr), "Upper threshold velocity: %.2f", m_flUpperThreshold );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}

		Q_snprintf(tempstr,sizeof(tempstr), "Player velocity: %.2f", m_flPlayerVelocity );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr), "Physics velocity: %.2f", m_flPhysicsVelocity );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		// Get the target
		CBaseEntity *pLaunchTarget = m_hLaunchTarget;
		
		// See if we're attempting to hit a target
		if ( pLaunchTarget )
		{
			Vector vecSourcePos = GetAbsOrigin();
			float flGravity = sv_gravity.GetFloat();
			
			{
				Vector vecTargetPos = pLaunchTarget->GetAbsOrigin();
				vecTargetPos.z -= 32.0f;  
							
				Vector vecVelocity = (vecTargetPos - vecSourcePos);

				// throw at a constant time
				float time = vecVelocity.Length( ) / m_flPlayerVelocity;
				vecVelocity = vecVelocity * (1.0 / time);

				// adjust upward toss to compensate for gravity loss
				vecVelocity.z += flGravity * time * 0.5;

				Q_snprintf(tempstr,sizeof(tempstr), "Adjusted Player velocity: %.2f", vecVelocity.Length() );
				EntityText(text_offset,tempstr,0);
				text_offset++;
			}

			{
				Vector vecTargetPos = pLaunchTarget->GetAbsOrigin();
				Vector vecVelocity = (vecTargetPos - vecSourcePos );

				// throw at a constant time
				float time = vecVelocity.Length( ) / m_flPhysicsVelocity;
				vecVelocity = vecVelocity * (1.0 / time);

				// adjust upward toss to compensate for gravity loss
				vecVelocity.z += flGravity * time * 0.5;

				Q_snprintf(tempstr,sizeof(tempstr), "Adjusted Physics velocity: %.2f", vecVelocity.Length() );
				EntityText(text_offset,tempstr,0);
				text_offset++;
			}
		}				
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCatapult::Spawn( void )
{
	BaseClass::Spawn();

	// Don't let the camera shoot through us!
	InitTrigger();

	for ( int i = 0; i < MAX_PLAYERS_ARRAY_SAFE; ++i )
	{
		m_flRefireDelay[i] = 0.0f;
	}

	m_flLowerThreshold = clamp( m_flLowerThreshold, 0.0f, 1.0f );
	m_flUpperThreshold = clamp( m_flUpperThreshold, 0.0f, 1.0f );

	SetTransmitState( FL_EDICT_PVSCHECK );

	m_hLaunchTarget = gEntList.FindEntityByName( NULL, m_strLaunchTarget );

	SetContextThink( &CTriggerCatapult::PlayerPassesTriggerFiltersThink, gpGlobals->curtime + 1.0f, s_szPlayerPassesTriggerFiltersThinkContext );
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCatapult::InputSetPlayerSpeed( inputdata_t &in )
{
	m_flPlayerVelocity = in.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCatapult::InputSetPhysicsSpeed( inputdata_t &in )
{
	m_flPhysicsVelocity = in.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCatapult::InputSetLaunchTarget( inputdata_t &in )
{
	m_strLaunchTarget = in.value.StringID();

	m_hLaunchTarget = gEntList.FindEntityByName( NULL, m_strLaunchTarget );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCatapult::InputSetExactVelocityChoiceType( inputdata_t &in )
{
	m_ExactVelocityChoice = in.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCatapult::LaunchThink( void )
{
	for ( int i = 0; i < m_hAbortedLaunchees.Count(); i++ )
	{
		CBaseEntity *pOther = m_hAbortedLaunchees[i].Get();
		bool bShouldRemove = true;

		if ( pOther )
		{
			if ( pOther->IsPlayer() )
			{
				// Time to get launched and stay in the list in case we're stuck under something again.
				bShouldRemove = false;
				StartTouch( pOther );
			}
			else if ( pOther->VPhysicsGetObject() )
			{
				if( ( pOther->VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD ) )
				{
					// the sphere should stay in the list.
					bShouldRemove = false;
				}
				else
				{
					// Time to get launched!
					StartTouch( pOther );
				}
			}
		}

		if ( bShouldRemove )
		{
			m_hAbortedLaunchees.Remove( i );
			i--;
		}
	}

	// see if we are still holding something in the catapult
	if( m_hAbortedLaunchees.Count() )
	{
		SetThink( &CTriggerCatapult::LaunchThink );
		SetNextThink( gpGlobals->curtime + 0.05f );
	}
	else
	{
		SetThink( NULL );
	}
}

//Think once a second, looking for a living player. Once one is found, evaluate if they pass our trigger filters and network that down to the client
void CTriggerCatapult::PlayerPassesTriggerFiltersThink( void )
{
	for( int i = 1; i != gpGlobals->maxClients; ++i )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if( pPlayer && pPlayer->IsAlive() )
		{
			m_bPlayersPassTriggerFilters = PassesTriggerFilters( pPlayer );
			SetContextThink( NULL, TICK_NEVER_THINK, s_szPlayerPassesTriggerFiltersThinkContext ); //never test again
			return;
		}
	}

	SetContextThink( &CTriggerCatapult::PlayerPassesTriggerFiltersThink, gpGlobals->curtime + 1.0f, s_szPlayerPassesTriggerFiltersThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCatapult::EndTouch( CBaseEntity *pOther )
{
	m_hAbortedLaunchees.FindAndFastRemove( pOther );
}
