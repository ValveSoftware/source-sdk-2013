//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Alyx, the female sidekick and love interest that's taking the world by storm!
//
//			Try the new Alyx Brite toothpaste!
//			Alyx lederhosen!
//
//			FIXME: need a better comment block
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ai_basehumanoid.h"
#include "npc_alyx.h"
#include "ai_senses.h"
#include "soundent.h"
#include "props.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( npc_alyx, CNPC_Alyx );

BEGIN_DATADESC( CNPC_Alyx )

	DEFINE_FIELD( m_hEmpTool, FIELD_EHANDLE ),

END_DATADESC()

int AE_ALYX_EMPTOOL_ATTACHMENT;
int AE_ALYX_EMPTOOL_SEQUENCE;

//=========================================================
// Classify - indicates this NPC's place in the 
// relationship table.
//=========================================================
Class_T	CNPC_Alyx::Classify ( void )
{
	return	CLASS_PLAYER_ALLY_VITAL;
}


//=========================================================
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_Alyx::HandleAnimEvent( animevent_t *pEvent )
{
	if (pEvent->event == AE_ALYX_EMPTOOL_ATTACHMENT)
	{
		if (!m_hEmpTool)
		{
			// Old savegame?
			CreateEmpTool();
			if (!m_hEmpTool)
				return;
		}

		int iAttachment = LookupAttachment( pEvent->options );
		m_hEmpTool->SetParent(this, iAttachment);
		m_hEmpTool->SetLocalOrigin( Vector( 0, 0, 0 ) );
		m_hEmpTool->SetLocalAngles( QAngle( 0, 0, 0 ) );

		return;
	}
	else if (pEvent->event == AE_ALYX_EMPTOOL_SEQUENCE)
	{
		if (!m_hEmpTool)
			return;

		CDynamicProp *pEmpTool = dynamic_cast<CDynamicProp *>(m_hEmpTool.Get());

		if (!pEmpTool)
			return;

		int iSequence = pEmpTool->LookupSequence( pEvent->options );
		if (iSequence != ACT_INVALID)
		{
			pEmpTool->PropSetSequence( iSequence );
		}

		return;
	}

	switch( pEvent->event )
	{
	case 1:
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// 
//=========================================================
bool CNPC_Alyx::CreateBehaviors()
{
	return BaseClass::CreateBehaviors();
}


//=========================================================
// Spawn
//=========================================================
void CNPC_Alyx::Spawn()
{
	BaseClass::Spawn();

	// If Alyx has a parent, she's currently inside a pod. Prevent her from moving.
	if ( GetMoveParent() )
	{
		SetMoveType( MOVETYPE_NONE );
		CapabilitiesClear();

		CapabilitiesAdd( bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD );
		CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );
	}
	else
	{
		SetupAlyxWithoutParent();
		CreateEmpTool( );
	}

	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	m_iHealth			= 80;

	NPCInit();
}

//=========================================================
// Precache - precaches all resources this NPC needs
//=========================================================
void CNPC_Alyx::Precache()
{
	BaseClass::Precache();
	PrecacheScriptSound( "npc_alyx.die" );
	PrecacheModel( STRING( GetModelName() ) );
	PrecacheModel( "models/alyx_emptool_prop.mdl" );
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::SelectModel()
{
	// Alyx is allowed to use multiple models, because she appears in the pod.
	// She defaults to her normal model.
	const char *szModel = STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		SetModelName( AllocPooledString("models/alyx.mdl") );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::SetupAlyxWithoutParent( void )
{
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_DOORS_GROUP | bits_CAP_TURN_HEAD | bits_CAP_DUCK | bits_CAP_SQUAD );
	CapabilitiesAdd( bits_CAP_USE_WEAPONS );
	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );
	CapabilitiesAdd( bits_CAP_AIM_GUN );
	CapabilitiesAdd( bits_CAP_MOVE_SHOOT );
	CapabilitiesAdd( bits_CAP_USE_SHOT_REGULATOR );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CNPC_Alyx::CreateEmpTool( void )
{
	m_hEmpTool = (CBaseAnimating*)CreateEntityByName( "prop_dynamic" );
	if ( m_hEmpTool )
	{
		m_hEmpTool->SetModel( "models/alyx_emptool_prop.mdl" );
		m_hEmpTool->SetName( AllocPooledString("Alyx_Emptool") );
		int iAttachment = LookupAttachment( "Emp_Holster" );
		m_hEmpTool->SetParent(this, iAttachment);
		m_hEmpTool->SetOwnerEntity(this);
		m_hEmpTool->SetSolid( SOLID_NONE );
		m_hEmpTool->SetLocalOrigin( Vector( 0, 0, 0 ) );
		m_hEmpTool->SetLocalAngles( QAngle( 0, 0, 0 ) );
		m_hEmpTool->Spawn();
	}
}




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	// Figure out if Alyx has just been removed from her parent
	if ( GetMoveType() == MOVETYPE_NONE && !GetMoveParent() )
	{
		SetupAlyxWithoutParent();
		SetupVPhysicsHull();
	}

	if ( HasCondition( COND_TALKER_PLAYER_DEAD ) )
	{
		SpeakIfAllowed( TLK_PLDEAD );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Activity CNPC_Alyx::NPC_TranslateActivity( Activity activity )
{
	activity = BaseClass::NPC_TranslateActivity( activity );
	if ( activity == ACT_IDLE && (m_NPCState == NPC_STATE_COMBAT || m_NPCState == NPC_STATE_ALERT) )
	{
		if (gpGlobals->curtime - m_flLastAttackTime < 3 || gpGlobals->curtime - GetEnemyLastTimeSeen() < 8)
		{
			activity = ACT_IDLE_ANGRY;
		}
	}

	return activity;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void CNPC_Alyx::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );

	// FIXME: hack until some way of removing decals after healing
	m_fNoDamageDecal = true;
}

//-----------------------------------------------------------------------------

void CNPC_Alyx::DeathSound( const CTakeDamageInfo &info )
{
	// Sentences don't play on dead NPCs
	SentenceStop();

	EmitSound( "npc_alyx.die" );
}

//=========================================================
// AI Schedules Specific to this NPC
//=========================================================

AI_BEGIN_CUSTOM_NPC( npc_alyx, CNPC_Alyx )

	DECLARE_ANIMEVENT( AE_ALYX_EMPTOOL_ATTACHMENT )
	DECLARE_ANIMEVENT( AE_ALYX_EMPTOOL_SEQUENCE )

AI_END_CUSTOM_NPC()

