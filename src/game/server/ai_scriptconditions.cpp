//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "IEffects.h"
#include "collisionutils.h"

#include "ai_basenpc.h"
#include "ai_scriptconditions.h"
#include "saverestore_utlvector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_ACTOR_AS_ACTIVATOR	( 1 << 0 )

ConVar debugscriptconditions( "ai_debugscriptconditions", "0" );

#define ScrCondDbgMsg( msg ) \
	do \
{ \
	if ( debugscriptconditions.GetBool() ) \
{ \
	DevMsg msg; \
} \
} \
	while (0)


//=============================================================================
//
// CAI_ScriptConditions
//
//=============================================================================

LINK_ENTITY_TO_CLASS(ai_script_conditions, CAI_ScriptConditions);

BEGIN_DATADESC( CAI_ScriptConditions )

	DEFINE_THINKFUNC( EvaluationThink ),

	DEFINE_OUTPUT( m_OnConditionsSatisfied, "OnConditionsSatisfied" ),
	DEFINE_OUTPUT( m_OnConditionsTimeout, "OnConditionsTimeout" ),
	DEFINE_OUTPUT( m_NoValidActors, "NoValidActors" ),

	//---------------------------------

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	//---------------------------------

	// Inputs
	DEFINE_KEYFIELD(m_fDisabled, 					FIELD_BOOLEAN, 	"StartDisabled" 			),

	DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
	DEFINE_KEYFIELD(m_Actor,						FIELD_STRING,	"Actor"						),

	DEFINE_KEYFIELD(m_flRequiredTime, 				FIELD_FLOAT, 	"RequiredTime" 				),

#ifndef HL2_EPISODIC
	DEFINE_FIELD( m_hActor, FIELD_EHANDLE ),
	DEFINE_EMBEDDED(m_Timer ),
	DEFINE_EMBEDDED(m_Timeout ),
#endif

	DEFINE_KEYFIELD(m_fMinState, 					FIELD_INTEGER,	"MinimumState" 				),
	DEFINE_KEYFIELD(m_fMaxState, 					FIELD_INTEGER,	"MaximumState" 				),

	DEFINE_KEYFIELD(m_fScriptStatus, 				FIELD_INTEGER,	"ScriptStatus" 				),
	DEFINE_KEYFIELD(m_fActorSeePlayer,				FIELD_INTEGER,	"ActorSeePlayer"			),


	DEFINE_KEYFIELD(m_flPlayerActorProximity,		FIELD_FLOAT, 	"PlayerActorProximity" 		),
	DEFINE_EMBEDDED(m_PlayerActorProxTester),

	DEFINE_KEYFIELD(m_flPlayerActorFOV, 			FIELD_FLOAT, 	"PlayerActorFOV" 			),
	DEFINE_KEYFIELD(m_bPlayerActorFOVTrueCone,		FIELD_BOOLEAN,	"PlayerActorFOVTrueCone"	),

	DEFINE_KEYFIELD(m_fPlayerActorLOS, 				FIELD_INTEGER, 	"PlayerActorLOS" 			),
	DEFINE_KEYFIELD(m_fActorSeeTarget,				FIELD_INTEGER,	"ActorSeeTarget" 			),

	DEFINE_KEYFIELD(m_flActorTargetProximity,		FIELD_FLOAT, 	"ActorTargetProximity" 		),
	DEFINE_EMBEDDED(m_ActorTargetProxTester),

	DEFINE_KEYFIELD(m_flPlayerTargetProximity, 		FIELD_FLOAT, 	"PlayerTargetProximity"		),
	DEFINE_EMBEDDED(m_PlayerTargetProxTester),

	DEFINE_KEYFIELD(m_flPlayerTargetFOV, 			FIELD_FLOAT,	"PlayerTargetFOV"			),
	DEFINE_KEYFIELD(m_bPlayerTargetFOVTrueCone,		FIELD_BOOLEAN,	"PlayerTargetFOVTrueCone"	),

	DEFINE_KEYFIELD(m_fPlayerTargetLOS, 			FIELD_INTEGER,	"PlayerTargetLOS"			),
	DEFINE_KEYFIELD(m_fPlayerBlockingActor,			FIELD_INTEGER,  "PlayerBlockingActor"		),

	DEFINE_KEYFIELD(m_flMinTimeout, 				FIELD_FLOAT,	"MinTimeout"				),
	DEFINE_KEYFIELD(m_flMaxTimeout, 				FIELD_FLOAT,	"MaxTimeout"				),

	DEFINE_KEYFIELD(m_fActorInPVS,					FIELD_INTEGER,  "ActorInPVS"		),

	DEFINE_KEYFIELD(m_fActorInVehicle,				FIELD_INTEGER,	 "ActorInVehicle" ),
	DEFINE_KEYFIELD(m_fPlayerInVehicle,				FIELD_INTEGER,	 "PlayerInVehicle" ),

	DEFINE_UTLVECTOR( m_ElementList,				FIELD_EMBEDDED ),
	DEFINE_FIELD( m_bLeaveAsleep,					FIELD_BOOLEAN ),

END_DATADESC()

BEGIN_SIMPLE_DATADESC( CAI_ProxTester )
	DEFINE_FIELD( m_distSq, FIELD_FLOAT ),
	DEFINE_FIELD( m_fInside, FIELD_BOOLEAN ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( CAI_ScriptConditionsElement )
	DEFINE_FIELD( m_hActor, FIELD_EHANDLE ),
	DEFINE_EMBEDDED(m_Timer ),
	DEFINE_EMBEDDED(m_Timeout ),
END_DATADESC()


//-----------------------------------------------------------------------------

#define EVALUATOR( name ) { &CAI_ScriptConditions::Eval##name, #name }

CAI_ScriptConditions::EvaluatorInfo_t CAI_ScriptConditions::gm_Evaluators[] =
{
	EVALUATOR( ActorSeePlayer ),
		EVALUATOR( State ),
		EVALUATOR( PlayerActorProximity ),
		EVALUATOR( PlayerTargetProximity ),
		EVALUATOR( ActorTargetProximity ),
		EVALUATOR( PlayerBlockingActor ),
		EVALUATOR( PlayerActorLook ),
		EVALUATOR( PlayerTargetLook ),
		EVALUATOR( ActorSeeTarget),
		EVALUATOR( PlayerActorLOS ),
		EVALUATOR( PlayerTargetLOS ),

#ifdef HL2_EPISODIC
		EVALUATOR( ActorInPVS ),
		EVALUATOR( PlayerInVehicle ),
		EVALUATOR( ActorInVehicle ),
#endif

};

void CAI_ScriptConditions::OnRestore( void )
{
	BaseClass::OnRestore();

#ifndef HL2_EPISODIC
	//Old HL2 save game! Fix up to new system.
	if ( m_hActor )
	{
		CAI_ScriptConditionsElement conditionactor;

		conditionactor.SetActor( m_hActor );
		conditionactor.SetTimeOut( m_Timeout );
		conditionactor.SetTimer( m_Timer );
 
		m_ElementList.AddToTail( conditionactor );

		m_hActor = NULL;
	}

	if ( m_ElementList.Count() == 0 && m_Actor == NULL_STRING && m_fDisabled == false )
	{
		AddNewElement( NULL );
	}
#endif
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalState( const EvalArgs_t &args )
{
	if ( !args.pActor )
		return true;

	CAI_BaseNPC *pNpc = args.pActor->MyNPCPointer();

	// !!!LATER - fix this code, we shouldn't need the table anymore
	// now that we've placed the NPC state defs in a logical order (sjb)
	static int stateVals[] = 
	{
		-1, // NPC_STATE_NONE
			0, // NPC_STATE_IDLE
			1, // NPC_STATE_ALERT
			2, // NPC_STATE_COMBAT
			-1, // NPC_STATE_SCRIPT
			-1, // NPC_STATE_PLAYDEAD
			-1, // NPC_STATE_PRONE
			-1, // NPC_STATE_DEAD
	};

	int valState = stateVals[pNpc->m_NPCState];

	if ( valState < 0 )
	{
		if ( pNpc->m_NPCState == NPC_STATE_SCRIPT && m_fScriptStatus >= TRS_TRUE )
			return true;

		return false;
	}

	const int valLow  = stateVals[m_fMinState];
	const int valHigh = stateVals[m_fMaxState];

	if ( valLow > valHigh )
	{
		DevMsg( "Script condition warning: Invalid setting for Maximum/Minimum state\n" );
		Disable();
		return false;
	}

	return ( valState >= valLow && valState <= valHigh );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalActorSeePlayer( const EvalArgs_t &args )
{
	if( m_fActorSeePlayer == TRS_NONE )
	{
		// Don't care, so don't do any work.
		return true;
	}

	if ( !args.pActor )
		return true;

	bool fCanSeePlayer = args.pActor->MyNPCPointer()->HasCondition( COND_SEE_PLAYER );
	return ( (int)m_fActorSeePlayer == (int)fCanSeePlayer );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalActorSeeTarget( const EvalArgs_t &args )
{
	if( m_fActorSeeTarget == TRS_NONE )
	{
		// Don't care, so don't do any work.
		return true;
	}

	if ( args.pTarget )
	{
		if ( !args.pActor )
			return true;

		CAI_BaseNPC *pNPCActor = args.pActor->MyNPCPointer();

#ifdef HL2_EPISODIC
		// This is the code we want to have written for HL2, but HL2 shipped without the QuerySeeEntity() call. This #ifdef really wants to be
		// something like #ifndef HL2_RETAIL, since this change does want to be in any products that are built henceforth. (sjb)
		bool fSee = pNPCActor->FInViewCone( args.pTarget ) && pNPCActor->FVisible( args.pTarget ) && pNPCActor->QuerySeeEntity( args.pTarget );
#else
		bool fSee = pNPCActor->FInViewCone( args.pTarget ) && pNPCActor->FVisible( args.pTarget );
#endif//HL2_EPISODIC

		if( fSee )
		{
			if( m_fActorSeeTarget == TRS_TRUE )
			{
				return true;
			}

			return false;
		}
		else
		{
			if( m_fActorSeeTarget == TRS_FALSE )
			{
				return true;
			}

			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerActorProximity( const EvalArgs_t &args )
{
	return ( !args.pActor || m_PlayerActorProxTester.Check( args.pPlayer, args.pActor ) );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerTargetProximity( const EvalArgs_t &args )
{
	return ( !args.pTarget || 
		m_PlayerTargetProxTester.Check( args.pPlayer, args.pTarget ) );
}


//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalActorTargetProximity( const EvalArgs_t &args )
{
	return ( !args.pTarget || !args.pActor ||
		m_ActorTargetProxTester.Check( args.pActor, args.pTarget ) );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerActorLook( const EvalArgs_t &args )
{
	return ( !args.pActor || 
		IsInFOV( args.pPlayer, args.pActor, m_flPlayerActorFOV, m_bPlayerActorFOVTrueCone ) );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerTargetLook( const EvalArgs_t &args )
{
	return ( !args.pTarget || IsInFOV( args.pPlayer, args.pTarget, m_flPlayerTargetFOV, m_bPlayerTargetFOVTrueCone ) );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerActorLOS( const EvalArgs_t &args )
{
	if( m_fPlayerActorLOS == TRS_NONE )
	{
		// Don't execute expensive code if we don't care.
		return true;
	}

	return ( !args.pActor || PlayerHasLineOfSight( args.pPlayer, args.pActor, m_fPlayerActorLOS == TRS_FALSE ) );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerTargetLOS( const EvalArgs_t &args )
{
	if( m_fPlayerTargetLOS == TRS_NONE )
	{
		// Don't execute expensive code if we don't care.
		return true;
	}

	return ( !args.pTarget || PlayerHasLineOfSight( args.pPlayer, args.pTarget, m_fPlayerTargetLOS == TRS_FALSE ) );
}

bool CAI_ScriptConditions::EvalActorInPVS( const EvalArgs_t &args )
{
	if( m_fActorInPVS == TRS_NONE )
	{
		// Don't execute expensive code if we don't care.
		return true;
	}

	return ( !args.pActor || ActorInPlayersPVS( args.pActor, m_fActorInPVS == TRS_FALSE ) );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerBlockingActor( const EvalArgs_t &args )
{
	if ( m_fPlayerBlockingActor == TRS_NONE )
		return true;

#if 0
	CAI_BaseNPC *pNpc = args.pActor->MyNPCPointer();

	const float testDist = 30.0;

	Vector origin = args.pActor->WorldSpaceCenter();
	Vector delta  = UTIL_YawToVector( args.pActor->GetAngles().y ) * testDist;

	Vector vecAbsMins, vecAbsMaxs;
	args.pActor->CollisionProp()->WorldSpaceAABB( &vecAbsMins, &vecAbsMaxs );
	bool intersect = IsBoxIntersectingRay( vecAbsMins, vecAbsMaxs, origin, delta );
#endif

	if ( m_fPlayerBlockingActor == TRS_FALSE )
		return true;

	return false; // for now, never say player is blocking
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerInVehicle( const EvalArgs_t &args )
{
	// We don't care
	if ( m_fPlayerInVehicle == TRS_NONE )
		return true;

	// Need a player to test
	if ( args.pPlayer == NULL )
		return false;

	// Desired states must match
	return args.pPlayer->IsInAVehicle() == ( m_fPlayerInVehicle == TRS_TRUE );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalActorInVehicle( const EvalArgs_t &args )
{
	// We don't care
	if ( m_fActorInVehicle == TRS_NONE )
		return true;

	if ( !args.pActor )
		return true;

	// Must be able to be in a vehicle at all
	CBaseCombatCharacter *pBCC = args.pActor->MyCombatCharacterPointer();
	if ( pBCC == NULL )
		return false;

	// Desired states must match
	return pBCC->IsInAVehicle() == ( m_fActorInVehicle == TRS_TRUE );
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::Spawn()
{
	Assert( ( m_fMinState == NPC_STATE_IDLE || m_fMinState == NPC_STATE_COMBAT || m_fMinState == NPC_STATE_ALERT ) &&
		( m_fMaxState == NPC_STATE_IDLE || m_fMaxState == NPC_STATE_COMBAT || m_fMaxState == NPC_STATE_ALERT ) );

	m_PlayerActorProxTester.Init( m_flPlayerActorProximity );
	m_PlayerTargetProxTester.Init( m_flPlayerTargetProximity );
	m_ActorTargetProxTester.Init( m_flActorTargetProximity );

	m_bLeaveAsleep = m_fDisabled;
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::Activate()
{
	BaseClass::Activate();

	// When we spawn, m_fDisabled is initial state as given by worldcraft.
	// following that, we keep it updated and it reflects current state.
	if( !m_fDisabled )
		Enable();

#ifdef HL2_EPISODIC
	gEntList.AddListenerEntity( this );
#endif
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::UpdateOnRemove( void )
{
	gEntList.RemoveListenerEntity( this );
	BaseClass::UpdateOnRemove();

	m_ElementList.Purge();
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::EvaluationThink()
{
	if ( m_fDisabled == true )
		return;

	int iActorsDone = 0;

#ifdef HL2_DLL
	if( AI_GetSinglePlayer()->GetFlags() & FL_NOTARGET )
	{
		ScrCondDbgMsg( ("%s WARNING: Player is NOTARGET. This will affect all LOS conditiosn involving the player!\n", GetDebugName()) );
	}
#endif


	for ( int i = 0; i < m_ElementList.Count(); )
	{
		CAI_ScriptConditionsElement *pConditionElement = &m_ElementList[i];

		if ( pConditionElement == NULL )
		{
			i++;
			continue;
		}

		CBaseEntity *pActor = pConditionElement->GetActor();
		CBaseEntity *pActivator = this;

#ifdef HL2_EPISODIC
		if ( pActor && HasSpawnFlags( SF_ACTOR_AS_ACTIVATOR ) )
		{
			pActivator = pActor;
		}
#endif

		AssertMsg( !m_fDisabled, ("Violated invariant between CAI_ScriptConditions disabled state and think func setting") );

		if ( m_Actor != NULL_STRING && !pActor )
		{
			if ( m_ElementList.Count() == 1 )
			{
				DevMsg( "Warning: Active AI script conditions associated with an non-existant or destroyed NPC\n" );
				m_NoValidActors.FireOutput( this, this, 0 );
			}

			iActorsDone++;
			m_ElementList.Remove( i );
			continue;
		}

		i++;

		if( m_flMinTimeout > 0 && pConditionElement->GetTimeOut()->Expired() )
		{
			ScrCondDbgMsg( ( "%s firing output OnConditionsTimeout (%f seconds)\n", STRING( GetEntityName() ), pConditionElement->GetTimeOut()->GetInterval() ) );

			iActorsDone++;
			m_OnConditionsTimeout.FireOutput( pActivator, this );
			continue;
		}

		bool      result = true;
		const int nEvaluators = sizeof( gm_Evaluators ) / sizeof( gm_Evaluators[0] );

		EvalArgs_t args =
		{
			pActor,
			GetPlayer(),
			m_hTarget.Get()
		};

		for ( int i = 0; i < nEvaluators; ++i )
		{
			if ( !(this->*gm_Evaluators[i].pfnEvaluator)( args ) )
			{
				pConditionElement->GetTimer()->Reset();
				result = false;

				ScrCondDbgMsg( ( "%s failed on: %s\n", GetDebugName(), gm_Evaluators[ i ].pszName ) );

				break;
			}
		}

		if ( result )
		{
			ScrCondDbgMsg( ( "%s waiting... %f\n", GetDebugName(), pConditionElement->GetTimer()->GetRemaining() ) );
		}

		if ( result && pConditionElement->GetTimer()->Expired() )
		{
			ScrCondDbgMsg( ( "%s firing output OnConditionsSatisfied\n", GetDebugName() ) );

			// Default behavior for now, provide worldcraft option later.
			iActorsDone++;
			m_OnConditionsSatisfied.FireOutput( pActivator, this );
		}
	}

	//All done!
	if ( iActorsDone == m_ElementList.Count() )
	{
		Disable();
		m_ElementList.Purge();
	}

	SetThinkTime();
}

//-----------------------------------------------------------------------------

int CAI_ScriptConditions::AddNewElement( CBaseEntity *pActor )
{
	CAI_ScriptConditionsElement conditionelement;
	conditionelement.SetActor( pActor );

	if( m_flMaxTimeout > 0 )
	{
		conditionelement.GetTimeOut()->Set( random->RandomFloat( m_flMinTimeout, m_flMaxTimeout ), false );
	}
	else
	{
		conditionelement.GetTimeOut()->Set( m_flMinTimeout, false );
	}

	conditionelement.GetTimer()->Set( m_flRequiredTime );

	if ( m_flRequiredTime > 0 )
	{
		conditionelement.GetTimer()->Reset();
	}

	return m_ElementList.AddToTail( conditionelement );
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::Enable( void )
{
	m_hTarget = gEntList.FindEntityByName( NULL, m_target );

	CBaseEntity *pActor = gEntList.FindEntityByName( NULL, m_Actor );
	if ( m_ElementList.Count() == 0 )
	{
		if ( m_Actor != NULL_STRING && pActor == NULL )
		{
			DevMsg( "Warning: Spawning AI script conditions (%s) associated with an non-existant NPC\n", GetDebugName() );
			m_NoValidActors.FireOutput( this, this, 0 );
			Disable();
			return;
		}

		if ( pActor && pActor->MyNPCPointer() == NULL )
		{
			Warning( "Script condition warning: warning actor is not an NPC\n" );
			Disable();
			return;
		}
	}

	while( pActor != NULL )
	{
		if( !ActorInList(pActor) )
		{
			AddNewElement( pActor );
		}

		pActor = gEntList.FindEntityByName( pActor, m_Actor );
	}

	//If we are hitting this it means we are using a Target->Player condition
	if ( m_Actor == NULL_STRING )
	{
		if( !ActorInList(pActor) )
		{
			AddNewElement( NULL );
		}
	}

	m_fDisabled = false;

	SetThink( &CAI_ScriptConditions::EvaluationThink );
	SetThinkTime();
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::Disable( void )
{
	SetThink( NULL );

	m_fDisabled = true;
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::InputEnable( inputdata_t &inputdata )
{
	m_bLeaveAsleep = false;
	Enable();
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::InputDisable( inputdata_t &inputdata )
{
	m_bLeaveAsleep = true;
	Disable();
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::IsInFOV( CBaseEntity *pViewer, CBaseEntity *pViewed, float fov, bool bTrueCone )
{
	CBaseCombatCharacter *pCombatantViewer = (pViewer) ? pViewer->MyCombatCharacterPointer() : NULL;

	if ( fov < 360 && pCombatantViewer /*&& pViewed*/ )
	{
		Vector vLookDir;
		Vector vActorDir;

		// Halve the fov. As expressed here, fov is the full size of the viewcone.
		float flFovDotResult;
		flFovDotResult = cos( DEG2RAD( fov / 2 ) );
		float fDotPr = 1;

		if( bTrueCone )
		{
			// 3D Check
			vLookDir = pCombatantViewer->EyeDirection3D( );
			vActorDir = pViewed->EyePosition() - pViewer->EyePosition();
			vActorDir.NormalizeInPlace();
			fDotPr = vLookDir.Dot(vActorDir);
		}
		else
		{
			// 2D Check
			vLookDir = pCombatantViewer->EyeDirection2D( );
			vActorDir = pViewed->EyePosition() - pViewer->EyePosition();
			vActorDir.z = 0.0;
			vActorDir.AsVector2D().NormalizeInPlace();
			fDotPr = vLookDir.AsVector2D().Dot(vActorDir.AsVector2D());
		}

		if ( fDotPr < flFovDotResult )
		{
			if( fov < 0 )
			{
				// Designer has requested that the player
				// NOT be looking at this place.
				return true;
			}

			return false;
		}
	}

	if( fov < 0 )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::PlayerHasLineOfSight( CBaseEntity *pViewer, CBaseEntity *pViewed, bool fNot )
{
	CBaseCombatCharacter *pCombatantViewer = pViewer->MyCombatCharacterPointer();

	if( pCombatantViewer )
	{
		// We always trace towards the player, so we handle players-in-vehicles
		if ( pViewed->FVisible( pCombatantViewer ) )
		{
			// Line of sight exists.
			if( fNot )
			{
				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			// No line of sight.
			if( fNot )
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::ActorInPlayersPVS( CBaseEntity *pActor, bool bNot )
{
	if ( pActor == NULL )
		return true;

	bool bInPVS = !!UTIL_FindClientInPVS( pActor->edict());

	if ( bInPVS )
	{
		if( bNot )
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		if( bNot )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::ActorInList( CBaseEntity *pActor )
{
	for ( int i = 0; i < m_ElementList.Count(); i++ )
	{
		if ( m_ElementList[i].GetActor() == pActor )
			 return true;
	}

	return false;
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::OnEntitySpawned( CBaseEntity *pEntity )
{
	if( m_fDisabled && m_bLeaveAsleep )
	{
		// Don't add elements if we're not currently running and don't want to automatically wake up.
		// Any spawning NPC's we miss during this time will be found and added when manually Enabled().
		return;
	}

	if ( pEntity->MyNPCPointer() == NULL )
		 return;

	if ( pEntity->NameMatches( m_Actor ) )
	{
		if ( ActorInList( pEntity ) == false )
		{
			AddNewElement( pEntity );

			if ( m_fDisabled == true && m_bLeaveAsleep == false )
			{
				Enable();
			}
		}
	}
}

//=============================================================================
