//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_default.h"
#include "scriptedtarget.h"
#include "entitylist.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Interactions
//=========================================================
int	g_interactionScriptedTarget		= 0;

LINK_ENTITY_TO_CLASS( scripted_target, CScriptedTarget );

BEGIN_DATADESC( CScriptedTarget )

	DEFINE_FIELD( m_vLastPosition,	FIELD_POSITION_VECTOR ),

	DEFINE_KEYFIELD( m_iDisabled,		FIELD_INTEGER,	"StartDisabled" ),
	DEFINE_KEYFIELD( m_iszEntity,		FIELD_STRING,	"m_iszEntity" ),
	DEFINE_KEYFIELD( m_flRadius,			FIELD_FLOAT,	"m_flRadius" ),

	DEFINE_KEYFIELD( m_nMoveSpeed,		FIELD_INTEGER,	"MoveSpeed" ),
	DEFINE_KEYFIELD( m_flPauseDuration,	FIELD_FLOAT,	"PauseDuration" ),
	DEFINE_FIELD( m_flPauseDoneTime,	FIELD_TIME ),
	DEFINE_KEYFIELD( m_flEffectDuration,	FIELD_FLOAT,	"EffectDuration" ),

	// Function Pointers
	DEFINE_THINKFUNC( ScriptThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	// Outputs
	DEFINE_OUTPUT(m_AtTarget,			"AtTarget" ),
	DEFINE_OUTPUT(m_LeaveTarget,		"LeaveTarget" ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CScriptedTarget::InputEnable( inputdata_t &inputdata )
{
	TurnOn();
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CScriptedTarget::InputDisable( inputdata_t &inputdata )
{
	TurnOff();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptedTarget::TurnOn( void )
{
	m_vLastPosition = GetAbsOrigin();
	SetThink( &CScriptedTarget::ScriptThink );
	m_iDisabled		= false;
	SetNextThink( gpGlobals->curtime );
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CScriptedTarget::TurnOff( void )
{
	SetThink( NULL );
	m_iDisabled		= true;

	// If I have a target entity, free him
	if (GetTarget())
	{
		CAI_BaseNPC* pNPC	= GetTarget()->MyNPCPointer();
		pNPC->DispatchInteraction( g_interactionScriptedTarget, NULL, NULL );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CScriptedTarget::Spawn( void )
{
	if (g_interactionScriptedTarget == 0)
	{
		g_interactionScriptedTarget			= CBaseCombatCharacter::GetInteractionID();
	}

	SetSolid( SOLID_NONE );

	m_vLastPosition = GetAbsOrigin();

	if (!m_iDisabled )
	{
		TurnOn();
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
CScriptedTarget* CScriptedTarget::NextScriptedTarget(void)
{
	// ----------------------------------------------------------------------
	// If I just hit my target, set how long I'm supposed to pause here
	// ----------------------------------------------------------------------
	if (m_flPauseDoneTime == 0)
	{
		m_flPauseDoneTime = gpGlobals->curtime + m_flPauseDuration;
		m_AtTarget.FireOutput( GetTarget(), this );
	}

	// -------------------------------------------------------------
	// If I'm done pausing move on to next burn target
	// -------------------------------------------------------------
	if (gpGlobals->curtime >= m_flPauseDoneTime)
	{
		m_flPauseDoneTime = 0;

		// ----------------------------------------------------------
		//  Fire output that current Scripted target has been reached
		// ----------------------------------------------------------
		m_LeaveTarget.FireOutput( GetTarget(), this );

		// ------------------------------------------------------------
		//  Get next target.  
		// ------------------------------------------------------------
		CScriptedTarget* pNextTarget = ((CScriptedTarget*)GetNextTarget());

		// --------------------------------------------
		//	Fire output if last one has been reached
		// --------------------------------------------
		if (!pNextTarget)
		{
			TurnOff();
			SetTarget( NULL );
		}
		// ------------------------------------------------
		//	Otherwise, turn myself off, the next target on
		//  and pass on my target entity
		// ------------------------------------------------
		else
		{
			// ----------------------------------------------------
			//  Make sure there is a LOS between these two targets
			// ----------------------------------------------------
			trace_t tr;
			UTIL_TraceLine(GetAbsOrigin(), pNextTarget->GetAbsOrigin(), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);	
			if (tr.fraction != 1.0)
			{
				Warning( "WARNING: Scripted Target from (%s) to (%s) is occluded!\n",GetDebugName(),pNextTarget->GetDebugName() );
			}

			pNextTarget->TurnOn();
			pNextTarget->SetTarget( GetTarget() );

			SetTarget( NULL );
			TurnOff();
		}
		// --------------------------------------------
		//	Return new target
		// --------------------------------------------
		return pNextTarget;
	}
	// -------------------------------------------------------------
	//  Otherwise keep the same scripted target until pause is done
	// -------------------------------------------------------------
	else
	{
		return this;
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
CBaseEntity* CScriptedTarget::FindEntity( void )
{
	// ---------------------------------------------------
	//	First try to find the entity by name
	// ---------------------------------------------------
	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, m_iszEntity );
	if (pEntity && pEntity->GetFlags() & FL_NPC)
	{
		CAI_BaseNPC* pNPC	= pEntity->MyNPCPointer();
		if (pNPC->DispatchInteraction( g_interactionScriptedTarget, NULL, this ))
		{
			return pEntity;
		}
	}

	// ---------------------------------------------------
	//	If that fails, assume we were given a classname
	//  and find nearest entity in radius of that class
	// ---------------------------------------------------
	float			flNearestDist	= MAX_COORD_RANGE;
	CBaseEntity*	pNearestEnt		= NULL;
	CBaseEntity*	pTestEnt		= NULL;

	for ( CEntitySphereQuery sphere( GetAbsOrigin(), m_flRadius ); ( pTestEnt = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if (pTestEnt->GetFlags() & FL_NPC)
		{
			if (FClassnameIs( pTestEnt, STRING(m_iszEntity)))
			{
				float flTestDist = (pTestEnt->GetAbsOrigin() - GetAbsOrigin()).Length();
				if (flTestDist < flNearestDist)
				{
					flNearestDist	= flTestDist;
					pNearestEnt		= pTestEnt;
				}
			}
		}
	}
	
	// UNDONE: If nearest fails, try next nearest
	if (pNearestEnt)
	{
		CAI_BaseNPC* pNPC	= pNearestEnt->MyNPCPointer();
		if (pNPC->DispatchInteraction( g_interactionScriptedTarget, NULL, this ))
		{
			return pNearestEnt;
		}
	}

	return NULL;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CScriptedTarget::ScriptThink( void )
{
	// --------------------------------------------
	//  If I don't have target entity look for one
	// --------------------------------------------
	if (GetTarget() == NULL)
	{
		m_flPauseDoneTime		= 0;
		SetTarget( FindEntity() );
	}
	SetNextThink( gpGlobals->curtime + 0.1f );
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CScriptedTarget::DrawDebugTextOverlays(void) 
{
	// Skip AIClass debug overlays
	int text_offset = CBaseEntity::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// --------------
		// Print State
		// --------------
		char tempstr[512];
		if (m_iDisabled) 
		{
			Q_strncpy(tempstr,"State: Off",sizeof(tempstr));
		}
		else
		{
			Q_strncpy(tempstr,"State: On",sizeof(tempstr));
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;

		// -----------------
		// Print Next Entity
		// -----------------
		CBaseEntity *pTarget = GetNextTarget();
		if (pTarget) 
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Next: %s",pTarget->GetDebugName() );
		}
		else
		{
			Q_strncpy(tempstr,"Next: -NONE-",sizeof(tempstr));
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;

		// --------------
		// Print Target
		// --------------
		if (GetTarget()!=NULL) 
		{
			Q_snprintf(tempstr,sizeof(tempstr),"User: %s",GetTarget()->GetDebugName() );
		}
		else if (m_iDisabled)
		{
			Q_strncpy(tempstr,"User: -NONE-",sizeof(tempstr));
		}
		else 
		{
			Q_strncpy(tempstr,"User: -LOOKING-",sizeof(tempstr));
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


//-----------------------------------------------------------------------------
// Purpose: Override base class to add display of paths
//-----------------------------------------------------------------------------
void CScriptedTarget::DrawDebugGeometryOverlays(void) 
{
	// ----------------------------------------------
	// Draw line to next target is bbox is selected
	// ----------------------------------------------
	if (m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_ABSBOX_BIT))
	{
		if (m_iDisabled)
		{
			NDebugOverlay::Box(GetAbsOrigin(), Vector(-5,-5,-5), Vector(5,5,5), 200,100,100, 0 ,0);
		}
		else
		{
			NDebugOverlay::Cross3D(m_vLastPosition,	Vector(-8,-8,-8),Vector(8,8,8),255,0,0,true,0.1);
			NDebugOverlay::Box(GetAbsOrigin(), Vector(-5,-5,-5), Vector(5,5,5), 255,0,0, 0 ,0);
			NDebugOverlay::Line(GetAbsOrigin(),m_vLastPosition,255,0,0,true,0.0);
		}

		CBaseEntity *pTarget = GetNextTarget();
		if (pTarget)
		{
			NDebugOverlay::Line(GetAbsOrigin(),pTarget->GetAbsOrigin(),200,100,100,true,0.0);
		}
		if (GetTarget() != NULL)
		{
			NDebugOverlay::Line(GetAbsOrigin(),GetTarget()->EyePosition(),0,255,0,true,0.0);
		}

	}
	CBaseEntity::DrawDebugGeometryOverlays();
}

