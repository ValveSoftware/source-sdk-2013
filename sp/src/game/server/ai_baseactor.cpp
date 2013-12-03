//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#include "sceneentity.h"
#include "choreoevent.h"
#include "choreoscene.h"
#include "choreoactor.h"
#include "ai_baseactor.h"
#include "ai_navigator.h"
#include "saverestore_utlvector.h"
#include "bone_setup.h"
#include "physics_npc_solver.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar flex_minplayertime( "flex_minplayertime", "5" );
ConVar flex_maxplayertime( "flex_maxplayertime", "7" );
ConVar flex_minawaytime( "flex_minawaytime", "0.5" );
ConVar flex_maxawaytime( "flex_maxawaytime", "1.0" );
ConVar ai_debug_looktargets( "ai_debug_looktargets", "0" );
ConVar ai_debug_expressions( "ai_debug_expressions", "0", FCVAR_NONE, "Show random expression decisions for NPCs." );
static ConVar scene_showfaceto( "scene_showfaceto", "0", FCVAR_ARCHIVE, "When playing back, show the directions of faceto events." );


BEGIN_DATADESC( CAI_BaseActor )

	DEFINE_FIELD( m_fLatchedPositions, FIELD_INTEGER ),
	DEFINE_FIELD( m_latchedEyeOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( m_latchedEyeDirection, FIELD_VECTOR ),
	DEFINE_FIELD( m_latchedHeadDirection, FIELD_VECTOR ),
	DEFINE_FIELD( m_goalHeadDirection, FIELD_VECTOR ),
	DEFINE_FIELD( m_goalHeadInfluence, FIELD_FLOAT ),
	DEFINE_FIELD( m_goalSpineYaw, FIELD_FLOAT ),
	DEFINE_FIELD( m_goalBodyYaw, FIELD_FLOAT ),
	DEFINE_FIELD( m_goalHeadCorrection, FIELD_VECTOR ),
	DEFINE_FIELD( m_flBlinktime, FIELD_TIME ),
	DEFINE_FIELD( m_hLookTarget, FIELD_EHANDLE ),
	DEFINE_UTLVECTOR( m_lookQueue,	FIELD_EMBEDDED ), 
	DEFINE_UTLVECTOR( m_randomLookQueue, FIELD_EMBEDDED ),
	DEFINE_UTLVECTOR( m_syntheticLookQueue,	FIELD_EMBEDDED ), 
	DEFINE_FIELD( m_flNextRandomLookTime, FIELD_TIME ),
	DEFINE_FIELD( m_iszExpressionScene, FIELD_STRING ),
	DEFINE_FIELD( m_hExpressionSceneEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flNextRandomExpressionTime, FIELD_TIME ),
	DEFINE_FIELD( m_iszIdleExpression, FIELD_STRING ),
	DEFINE_FIELD( m_iszAlertExpression, FIELD_STRING ),
	DEFINE_FIELD( m_iszCombatExpression, FIELD_STRING ),
	DEFINE_FIELD( m_iszDeathExpression, FIELD_STRING ),
	//DEFINE_FIELD( m_ParameterBodyTransY, FIELD_INTEGER ),
	//DEFINE_FIELD( m_ParameterBodyTransX, FIELD_INTEGER ),
	//DEFINE_FIELD( m_ParameterBodyLift, FIELD_INTEGER ),
	DEFINE_FIELD( m_ParameterBodyYaw, FIELD_INTEGER ),
	//DEFINE_FIELD( m_ParameterBodyPitch, FIELD_INTEGER ),
	//DEFINE_FIELD( m_ParameterBodyRoll, FIELD_INTEGER ),
	DEFINE_FIELD( m_ParameterSpineYaw, FIELD_INTEGER ),
	//DEFINE_FIELD( m_ParameterSpinePitch, FIELD_INTEGER ),
	//DEFINE_FIELD( m_ParameterSpineRoll, FIELD_INTEGER ),
	DEFINE_FIELD( m_ParameterNeckTrans, FIELD_INTEGER ),
	DEFINE_FIELD( m_ParameterHeadYaw, FIELD_INTEGER ),
	DEFINE_FIELD( m_ParameterHeadPitch, FIELD_INTEGER ),
	DEFINE_FIELD( m_ParameterHeadRoll, FIELD_INTEGER ),
	//DEFINE_FIELD( m_FlexweightMoveRightLeft, FIELD_INTEGER ),
	//DEFINE_FIELD( m_FlexweightMoveForwardBack, FIELD_INTEGER ),
	//DEFINE_FIELD( m_FlexweightMoveUpDown, FIELD_INTEGER ),
	DEFINE_FIELD( m_FlexweightBodyRightLeft, FIELD_INTEGER ),
	//DEFINE_FIELD( m_FlexweightBodyUpDown, FIELD_INTEGER ),
	//DEFINE_FIELD( m_FlexweightBodyTilt, FIELD_INTEGER ),
	DEFINE_FIELD( m_FlexweightChestRightLeft, FIELD_INTEGER ),
	//DEFINE_FIELD( m_FlexweightChestUpDown, FIELD_INTEGER ),
	//DEFINE_FIELD( m_FlexweightChestTilt, FIELD_INTEGER ),
	DEFINE_FIELD( m_FlexweightHeadForwardBack, FIELD_INTEGER ),
	DEFINE_FIELD( m_FlexweightHeadRightLeft, FIELD_INTEGER ),
	DEFINE_FIELD( m_FlexweightHeadUpDown, FIELD_INTEGER ),
	DEFINE_FIELD( m_FlexweightHeadTilt, FIELD_INTEGER ),

	DEFINE_FIELD( m_ParameterGestureHeight, FIELD_INTEGER ),
	DEFINE_FIELD( m_ParameterGestureWidth, FIELD_INTEGER ),
	DEFINE_FIELD( m_FlexweightGestureUpDown, FIELD_INTEGER ),
	DEFINE_FIELD( m_FlexweightGestureRightLeft, FIELD_INTEGER ),
	DEFINE_FIELD( m_flAccumYawDelta, FIELD_FLOAT ),
	DEFINE_FIELD( m_flAccumYawScale, FIELD_FLOAT ),

	DEFINE_ARRAY( m_flextarget, FIELD_FLOAT, 64 ),

	DEFINE_KEYFIELD( m_bDontUseSemaphore, FIELD_BOOLEAN, "DontUseSpeechSemaphore" ),

	DEFINE_KEYFIELD( m_iszExpressionOverride, FIELD_STRING, "ExpressionOverride" ),

	DEFINE_EMBEDDEDBYREF( m_pExpresser ),

	DEFINE_INPUTFUNC( FIELD_STRING,	"SetExpressionOverride",	InputSetExpressionOverride ),

END_DATADESC()


BEGIN_SIMPLE_DATADESC( CAI_InterestTarget_t )
	DEFINE_FIELD( m_eType,		FIELD_INTEGER ),
	DEFINE_FIELD( m_hTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecPosition,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flStartTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flEndTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flRamp,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flInterest,	FIELD_FLOAT ),
END_DATADESC()




//-----------------------------------------------------------------------------
// Purpose: clear out latched state
//-----------------------------------------------------------------------------
void CAI_BaseActor::StudioFrameAdvance ()
{
	// clear out head and eye latched values
	m_fLatchedPositions &= ~(HUMANOID_LATCHED_ALL);

	BaseClass::StudioFrameAdvance();
}


void CAI_BaseActor::Precache()
{
	BaseClass::Precache();

	if ( NULL_STRING != m_iszExpressionOverride )
	{
		PrecacheInstancedScene( STRING( m_iszExpressionOverride ) );
	}

	if ( m_iszIdleExpression != NULL_STRING )
	{
		PrecacheInstancedScene( STRING(m_iszIdleExpression ) );
	}

	if ( m_iszCombatExpression != NULL_STRING )
	{
		PrecacheInstancedScene( STRING(m_iszCombatExpression ) );
	}

	if ( m_iszAlertExpression != NULL_STRING )
	{
		PrecacheInstancedScene( STRING(m_iszAlertExpression) );
	}

	if ( m_iszDeathExpression != NULL_STRING )
	{
		PrecacheInstancedScene( STRING(m_iszDeathExpression) );
	}
}

static char const *g_ServerSideFlexControllers[] = 
{
	"body_rightleft",
	//"body_updown",
	//"body_tilt",
	"chest_rightleft",
	//"chest_updown",
	//"chest_tilt",
	"head_forwardback",
	"head_rightleft",
	"head_updown",
	"head_tilt",

	"gesture_updown",
	"gesture_rightleft"
};

//-----------------------------------------------------------------------------
// Purpose: Static method 
// Input  : *szName - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_BaseActor::IsServerSideFlexController( char const *szName )
{
	int c = ARRAYSIZE( g_ServerSideFlexControllers );
	for ( int i = 0; i < c; ++i )
	{
		if ( !Q_stricmp( szName, g_ServerSideFlexControllers[ i ] ) )
			return true;
	}
	return false;
}

void CAI_BaseActor::SetModel( const char *szModelName )
{
	BaseClass::SetModel( szModelName );

	//Init( m_ParameterBodyTransY, "body_trans_Y" );
	//Init( m_ParameterBodyTransX, "body_trans_X" );
	//Init( m_ParameterBodyLift, "body_lift" );
	Init( m_ParameterBodyYaw, "body_yaw" );
	//Init( m_ParameterBodyPitch, "body_pitch" );
	//Init( m_ParameterBodyRoll, "body_roll" );
	Init( m_ParameterSpineYaw, "spine_yaw" );
	//Init( m_ParameterSpinePitch, "spine_pitch" );
	//Init( m_ParameterSpineRoll, "spine_roll" );
	Init( m_ParameterNeckTrans, "neck_trans" );
	Init( m_ParameterHeadYaw, "head_yaw" );
	Init( m_ParameterHeadPitch, "head_pitch" );
	Init( m_ParameterHeadRoll, "head_roll" );

	//Init( m_FlexweightMoveRightLeft, "move_rightleft" );
	//Init( m_FlexweightMoveForwardBack, "move_forwardback" );
	//Init( m_FlexweightMoveUpDown, "move_updown" );
	Init( m_FlexweightBodyRightLeft, "body_rightleft" );
	//Init( m_FlexweightBodyUpDown, "body_updown" );
	//Init( m_FlexweightBodyTilt, "body_tilt" );
	Init( m_FlexweightChestRightLeft, "chest_rightleft" );
	//Init( m_FlexweightChestUpDown, "chest_updown" );
	//Init( m_FlexweightChestTilt, "chest_tilt" );
	Init( m_FlexweightHeadForwardBack, "head_forwardback" );
	Init( m_FlexweightHeadRightLeft, "head_rightleft" );
	Init( m_FlexweightHeadUpDown, "head_updown" );
	Init( m_FlexweightHeadTilt, "head_tilt" );

	Init( m_ParameterGestureHeight, "gesture_height" );
	Init( m_ParameterGestureWidth, "gesture_width" );
	Init( m_FlexweightGestureUpDown, "gesture_updown" );
	Init( m_FlexweightGestureRightLeft, "gesture_rightleft" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

bool CAI_BaseActor::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	Assert( info );
	Assert( info->m_pScene );
	Assert( info->m_pEvent );

	// FIXME: this code looks duplicated
	switch ( info->m_pEvent->GetType() )
	{
	case CChoreoEvent::FACE: 
		{
			return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
		}
		break;

	case CChoreoEvent::GENERIC:
		{
			if (stricmp( event->GetParameters(), "AI_BLINK") == 0)
			{
				info->m_nType = SCENE_AI_BLINK;
				// blink eyes
				Blink();
				// don't blink for duration, or next random blink time
				float flDuration = (event->GetEndTime() - scene->GetTime());
				m_flBlinktime = gpGlobals->curtime + MAX( flDuration, random->RandomFloat( 1.5, 4.5 ) ); 
			}
			else if (stricmp( event->GetParameters(), "AI_HOLSTER") == 0)
			{
				// FIXME: temp code for test
				info->m_nType = SCENE_AI_HOLSTER;
				info->m_iLayer = HolsterWeapon();
				return true;
			}
			else if (stricmp( event->GetParameters(), "AI_UNHOLSTER") == 0)
			{
				// FIXME: temp code for test
				info->m_nType = SCENE_AI_UNHOLSTER;
				info->m_iLayer = UnholsterWeapon();
				return true;
			}
			else if (stricmp( event->GetParameters(), "AI_AIM") == 0)
			{
				info->m_nType = SCENE_AI_AIM;
				info->m_hTarget = pTarget;
			}
			else if (stricmp( event->GetParameters(), "AI_RANDOMLOOK") == 0)
			{
				info->m_nType = SCENE_AI_RANDOMLOOK;
				info->m_flNext = 0.0;
			}
			else if (stricmp( event->GetParameters(), "AI_RANDOMFACEFLEX") == 0)
			{
				info->m_nType = SCENE_AI_RANDOMFACEFLEX;
				info->m_flNext = 0.0;
				info->InitWeight( this );
			}
			else if (stricmp( event->GetParameters(), "AI_RANDOMHEADFLEX") == 0)
			{
				info->m_nType = SCENE_AI_RANDOMHEADFLEX;
				info->m_flNext = 0.0;
			}	
			else if (stricmp( event->GetParameters(), "AI_IGNORECOLLISION") == 0)
			{
				CBaseEntity *pTarget = FindNamedEntity( event->GetParameters2( ) );

				if (pTarget)
				{
					info->m_nType = SCENE_AI_IGNORECOLLISION;
					info->m_hTarget = pTarget;
					float remaining = event->GetEndTime() - scene->GetTime();
					NPCPhysics_CreateSolver( this, pTarget, true, remaining );
					info->m_flNext = gpGlobals->curtime + remaining;
					return true;
				}
				else
				{
					Warning( "CSceneEntity %s unable to find actor named \"%s\"\n", scene->GetFilename(), event->GetParameters2() );
					return false;
				}
			}
			else if (stricmp( event->GetParameters(), "AI_DISABLEAI") == 0)
			{
				info->m_nType = SCENE_AI_DISABLEAI;
			}
			else
			{
				return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
			}
			return true;
		}
		break;

	default:
		return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
	}
}


bool CAI_BaseActor::ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( info );
	Assert( info->m_pScene );
	Assert( info->m_pEvent );

	// FIXME: this code looks duplicated
	switch ( info->m_pEvent->GetType() )
	{
	case CChoreoEvent::FACE: 
		{
			// make sure target exists
			if (info->m_hTarget == NULL)
				return false;

			bool bInScene = false;
			
			// lockbodyfacing is designed to run on top of both normal AI and on top of
			// scripted_sequences.  By allowing torso turns during post-idles, pre-idles, 
			// act-busy's, scripted_sequences, normal AI movements, etc., it increases 
			// the functionality of those AI features without breaking their assuptions 
			// that the entity won't be made to "turn" by something outside of those 
			// AI's control.
			// lockbody facing is also usefull when npcs are moving and you want them to turn
			// towards something but still walk in the direction of travel.
			if (!event->IsLockBodyFacing())
				bInScene = EnterSceneSequence( scene, event, true );

			// make sure we're still able to play this command
			if (!info->m_bStarted)
			{
				info->m_flInitialYaw = GetLocalAngles().y;
				info->m_flTargetYaw = info->m_flInitialYaw;
				info->m_flFacingYaw = info->m_flInitialYaw;
				if (IsMoving())
				{
					info->m_flWeight = 1.0;
				}
				else
				{
					info->m_flWeight = 0.0;
				}
			}

			// lock in place if aiming at self
			if (info->m_hTarget == this)
			{
				return true;
			}

			if (!bInScene || info->m_bIsMoving != IsMoving())
			{
				info->m_flInitialYaw = GetLocalAngles().y;
			}
			info->m_bIsMoving = IsMoving();

			// Msg("%f : %f - %f\n", scene->GetTime(), event->GetStartTime(), event->GetEndTime() );
			float flTime = clamp( scene->GetTime(), event->GetStartTime(), event->GetEndTime() - 0.1f );
			float intensity = event->GetIntensity( flTime );

			// clamp in-ramp to 0.5 seconds
			float flDuration = scene->GetTime() - event->GetStartTime();
			float flMaxIntensity = flDuration < 0.5f ? SimpleSpline( flDuration / 0.5f ) : 1.0f;
			intensity = clamp( intensity, 0.0f, flMaxIntensity );

			if (bInScene && info->m_bIsMoving)
			{
				info->m_flInitialYaw = GetLocalAngles().y;
			}

			if (!event->IsLockBodyFacing())
			{
				if (!info->m_bIsMoving && bInScene)
				{
					AccumulateIdealYaw( info->m_flFacingYaw, intensity );
				}
			}

			float diff;
			float dir;
			float flSpineYaw;
			float flBodyYaw;
			
			// move upper body to account for missing body yaw
			diff = UTIL_AngleDiff( info->m_flTargetYaw, GetLocalAngles().y );
			if (diff < 0)
			{
				diff = -diff;
				dir = -1;
			}
			else
			{
				dir = 1;
			}
			flSpineYaw = MIN( diff, 30 );
			flBodyYaw = MIN( diff - flSpineYaw, 30 );
			m_goalSpineYaw = m_goalSpineYaw * (1.0 - intensity) + intensity * flSpineYaw * dir;
			m_goalBodyYaw = m_goalBodyYaw * (1.0 - intensity) + intensity * flBodyYaw * dir;

			/*
			NDebugOverlay::YawArrow( GetAbsOrigin(), GetLocalAngles().y, 64, 16, 255, 255, 255, 0, true, 0.1 );
			NDebugOverlay::YawArrow( GetAbsOrigin() + Vector( 0, 0, 8 ), GetLocalAngles().y + m_goalBodyYaw, 64, 16, 255, 128, 128, 0, true, 0.1 );
			NDebugOverlay::YawArrow( GetAbsOrigin() + Vector( 0, 0, 16 ), GetLocalAngles().y + m_goalSpineYaw, 64, 16, 128, 255, 128, 0, true, 0.1 );
			NDebugOverlay::YawArrow( GetAbsOrigin() + Vector( 0, 0, 24 ), info->m_flTargetYaw, 64, 16, 128, 128, 255, 0, true, 0.1 );
			*/

			CAI_BaseNPC *pGoalNpc = info->m_hTarget->MyNPCPointer();

			float goalYaw = GetLocalAngles().y;
			
			if ( pGoalNpc )
			{
				goalYaw = CalcIdealYaw( pGoalNpc->FacingPosition() );
			}
			else
			{
				goalYaw = CalcIdealYaw( info->m_hTarget->EyePosition() );
			}

			if (developer.GetInt() > 0 && scene_showfaceto.GetBool())
			{
				NDebugOverlay::YawArrow( GetAbsOrigin() + Vector( 0, 0, 1 ), goalYaw, 8 + 32 * intensity, 8, 255, 255, 255, 0, true, 0.12 );
			}

			diff = UTIL_AngleDiff( goalYaw, info->m_flInitialYaw ) * intensity;
			dir = 1.0;

			// debounce delta a bit
			info->m_flTargetYaw = UTIL_AngleMod( info->m_flInitialYaw + diff );

			if (diff < 0)
			{
				diff = -diff;
				dir = -1;
			}

			// calc how much to use the spine for turning
			float spineintensity = (1.0 - MAX( 0.0, (intensity - 0.5) / 0.5 ));
			// force spine to full if not in scene or locked
			if (!bInScene || event->IsLockBodyFacing() )
			{
				spineintensity = 1.0;
			}

			flSpineYaw = MIN( diff * spineintensity, 30 );
			flBodyYaw = MIN( diff * spineintensity - flSpineYaw, 30 );
			info->m_flFacingYaw = info->m_flInitialYaw + (diff - flBodyYaw - flSpineYaw) * dir;

			if (!event->IsLockBodyFacing())
			{
				AddFacingTarget( info->m_hTarget, intensity, 0.2 ); // facing targets are lagged by one frame
			}
			return true;
		}
	case CChoreoEvent::GENERIC:
		{
			switch(info->m_nType)
			{
				case SCENE_AI_BLINK:
					{
						// keep eyes not blinking for duration
						float flDuration = (event->GetEndTime() - scene->GetTime());
						m_flBlinktime = MAX( m_flBlinktime, gpGlobals->curtime + flDuration );
					}
					return true;
				case SCENE_AI_HOLSTER:
					{
					}
					return true;
				case SCENE_AI_UNHOLSTER:
					{
					}
					return true;
				case SCENE_AI_AIM:
					{
						if ( info->m_hTarget )
						{
							Vector vecAimTargetLoc = info->m_hTarget->EyePosition();
							Vector vecAimDir = vecAimTargetLoc - EyePosition();

							VectorNormalize( vecAimDir );
							SetAim( vecAimDir);
						}
					}
					return true;
				case SCENE_AI_RANDOMLOOK:
					{
						if (info->m_flNext < gpGlobals->curtime)
						{
							info->m_flNext = gpGlobals->curtime + PickLookTarget( m_syntheticLookQueue ) - 0.4;
							if (m_syntheticLookQueue.Count() > 0)
							{
								float flDuration = (event->GetEndTime() - scene->GetTime());
								int i = m_syntheticLookQueue.Count() - 1;
								m_syntheticLookQueue[i].m_flEndTime = MIN( m_syntheticLookQueue[i].m_flEndTime, gpGlobals->curtime + flDuration );
								m_syntheticLookQueue[i].m_flInterest = 0.1;
							}
						}
					}
					return true;
				case SCENE_AI_RANDOMFACEFLEX:
					return RandomFaceFlex( info, scene, event );
				case SCENE_AI_RANDOMHEADFLEX:
					return true;
				case SCENE_AI_IGNORECOLLISION:
					if (info->m_hTarget && info->m_flNext < gpGlobals->curtime)
					{
						float remaining = event->GetEndTime() - scene->GetTime();
						NPCPhysics_CreateSolver( this, info->m_hTarget, true, remaining );
						info->m_flNext = gpGlobals->curtime + remaining;
					}

					// FIXME: needs to handle scene pause
					return true;
				case SCENE_AI_DISABLEAI:
					if (!(GetState() == NPC_STATE_SCRIPT  || IsCurSchedule( SCHED_SCENE_GENERIC )) )
					{
						EnterSceneSequence( scene, event );
					}
					return true;
				default:
					return false;
			}
		}
		break;
	default:
		return BaseClass::ProcessSceneEvent( info, scene, event );
	}
}


bool CAI_BaseActor::RandomFaceFlex( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event )
{
	if (info->m_flNext < gpGlobals->curtime)
	{
		const flexsettinghdr_t *pSettinghdr = ( const flexsettinghdr_t * )FindSceneFile( event->GetParameters2() );
		if (pSettinghdr == NULL)
		{
			pSettinghdr = ( const flexsettinghdr_t * )FindSceneFile( "random" );
		}
		if ( pSettinghdr )
		{
			info->m_flNext = gpGlobals->curtime + random->RandomFloat( 0.3, 0.5 ) * (30.0 / pSettinghdr->numflexsettings);

			flexsetting_t const *pSetting = NULL;
			pSetting = pSettinghdr->pSetting( random->RandomInt( 0, pSettinghdr->numflexsettings - 1 ) );

			flexweight_t *pWeights = NULL;
			int truecount = pSetting->psetting( (byte *)pSettinghdr, 0, &pWeights );
			if ( !pWeights )
				return false;

			int i;
			for (i = 0; i < truecount; i++, pWeights++)
			{
				// Translate to local flex controller
				// this is translating from the settings's local index to the models local index
				int index = FlexControllerLocalToGlobal( pSettinghdr, pWeights->key );

				// FIXME: this is supposed to blend based on pWeight->influence, but the order is wrong...
				// float value = GetFlexWeight( index ) * (1 - scale * pWeights->influence) + scale * pWeights->weight;

				// Add scaled weighting in to total
				m_flextarget[ index ] = pWeights->weight;
			}
		}
		else
		{
			return false;
		}
	}

	// adjust intensity if this is a background scene and there's other flex animations playing
	float intensity = info->UpdateWeight( this ) * event->GetIntensity( scene->GetTime() );

	// slide it up.
	for (LocalFlexController_t i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
	{
		float weight = GetFlexWeight( i );

		if (weight != m_flextarget[i])
		{
			float delta = (m_flextarget[i] - weight) / random->RandomFloat( 2.0, 4.0 );
			weight = weight + delta * intensity;
		}
		weight = clamp( weight, 0.0f, 1.0f );
		SetFlexWeight( i, weight );
	}

	return true;
}





bool CAI_BaseActor::ClearSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled )
{
	Assert( info );
	Assert( info->m_pScene );
	Assert( info->m_pEvent );

	// FIXME: this code looks duplicated
	switch ( info->m_pEvent->GetType() )
	{
	case CChoreoEvent::FACE: 
		{
			return BaseClass::ClearSceneEvent( info, fastKill, canceled );
		}
		break;
	default:
		return BaseClass::ClearSceneEvent( info, fastKill, canceled );
	}
}



bool CAI_BaseActor::CheckSceneEventCompletion( CSceneEventInfo *info, float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( info );
	Assert( info->m_pScene );
	Assert( info->m_pEvent );

	switch ( event->GetType() )
	{
	case CChoreoEvent::GENERIC:
		{
			switch( info->m_nType)
			{
			case SCENE_AI_HOLSTER:
			case SCENE_AI_UNHOLSTER:
				{
					if (info->m_iLayer == -1)
					{
						return true;
					}
					float preload = event->GetEndTime() - currenttime;
					if (preload < 0)
					{
						return true;
					}
					float t = (1.0 - GetLayerCycle( info->m_iLayer )) * SequenceDuration( GetLayerSequence( info->m_iLayer ) );

					return (t <= preload);
				}
			}
		}
	}

	return BaseClass::CheckSceneEventCompletion( info, currenttime, scene, event );
}



//-----------------------------------------------------------------------------
// Purpose: clear out latched state
//-----------------------------------------------------------------------------
void CAI_BaseActor::SetViewtarget( const Vector &viewtarget )
{
	// clear out eye latch
	m_fLatchedPositions &= ~HUMANOID_LATCHED_EYE;

	BaseClass::SetViewtarget( viewtarget );
}


//-----------------------------------------------------------------------------
// Purpose: Returns true position of the eyeballs
//-----------------------------------------------------------------------------
void CAI_BaseActor::UpdateLatchedValues( ) 
{ 
	if (!(m_fLatchedPositions & HUMANOID_LATCHED_HEAD))
	{
		// set head latch
		m_fLatchedPositions |= HUMANOID_LATCHED_HEAD;

		if (!HasCondition( COND_IN_PVS ) || !GetAttachment( "eyes", m_latchedEyeOrigin, &m_latchedHeadDirection ))
		{
			m_latchedEyeOrigin = BaseClass::EyePosition( );
			AngleVectors( GetLocalAngles(), &m_latchedHeadDirection );
		}
		// clear out eye latch
		m_fLatchedPositions &= ~(HUMANOID_LATCHED_EYE);
		// DevMsg( "eyeball %4f %4f %4f  : %3f %3f %3f\n", origin.x, origin.y, origin.z, angles.x, angles.y, angles.z );
	}

	if (!(m_fLatchedPositions & HUMANOID_LATCHED_EYE))
	{
		m_fLatchedPositions |= HUMANOID_LATCHED_EYE;

		if ( CapabilitiesGet() & bits_CAP_ANIMATEDFACE )
		{
			m_latchedEyeDirection = GetViewtarget() - m_latchedEyeOrigin; 
			VectorNormalize( m_latchedEyeDirection );
		}
		else
		{
			m_latchedEyeDirection = m_latchedHeadDirection;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true position of the eyeballs
//-----------------------------------------------------------------------------
Vector CAI_BaseActor::EyePosition( )
{ 
	UpdateLatchedValues();

	return m_latchedEyeOrigin;
}


#define MIN_LOOK_TARGET_DIST 1.0f
#define MAX_FULL_LOOK_TARGET_DIST 10.0f

//-----------------------------------------------------------------------------
// Purpose: Returns true if target is in legal range of eye movement for the current head position
//-----------------------------------------------------------------------------
bool CAI_BaseActor::ValidEyeTarget(const Vector &lookTargetPos)
{
	Vector vHeadDir = HeadDirection3D( );
	Vector lookTargetDir	= lookTargetPos - EyePosition();
	float flDist = VectorNormalize(lookTargetDir);

	if (flDist < MIN_LOOK_TARGET_DIST)
	{
		return false;
	}

	// Only look if it doesn't crank my eyeballs too far
	float dotPr = DotProduct(lookTargetDir, vHeadDir);
	// DevMsg( "ValidEyeTarget( %4f %4f %4f )  %3f\n", lookTargetPos.x, lookTargetPos.y, lookTargetPos.z, dotPr );

	if (dotPr > 0.259) // +- 75 degrees
	// if (dotPr > 0.86) // +- 30 degrees
	{
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if target is in legal range of possible head movements
//-----------------------------------------------------------------------------
bool CAI_BaseActor::ValidHeadTarget(const Vector &lookTargetPos)
{
	Vector vFacing = BodyDirection3D();
	Vector lookTargetDir = lookTargetPos - EyePosition();
	float flDist = VectorNormalize(lookTargetDir);

	if (flDist < MIN_LOOK_TARGET_DIST)
	{
		return false;
	}

	// Only look if it doesn't crank my head too far
	float dotPr = DotProduct(lookTargetDir, vFacing);
	if (dotPr > 0 && fabs( lookTargetDir.z ) < 0.7) // +- 90 degrees side to side, +- 45 up/down
	{
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Returns how much to try to look at the target
//-----------------------------------------------------------------------------
float CAI_BaseActor::HeadTargetValidity(const Vector &lookTargetPos)
{
	Vector vFacing = BodyDirection3D();

	int iForward = LookupAttachment( "forward" );
	if ( iForward > 0 )
	{
		Vector tmp1;
		GetAttachment( iForward, tmp1, &vFacing, NULL, NULL );
	}

	Vector lookTargetDir = lookTargetPos - EyePosition();
	float flDist = lookTargetDir.Length2D();
	VectorNormalize(lookTargetDir);

	if (flDist <= MIN_LOOK_TARGET_DIST)
	{
		return 0;
	}

	// Only look if it doesn't crank my head too far
	float dotPr = DotProduct(lookTargetDir, vFacing);
	// only look if target is within +-135 degrees
	// scale 1..-0.707 == 1..1,  -.707..-1 == 1..0
	// 	X * b + b = 1 == 1 / (X + 1) = b, 3.4142
	float flInterest = clamp( 3.4142f + 3.4142f * dotPr, 0.f, 1.f );

	// stop looking when point too close 
	if (flDist < MAX_FULL_LOOK_TARGET_DIST)
	{
		flInterest = flInterest * (flDist - MIN_LOOK_TARGET_DIST ) / (MAX_FULL_LOOK_TARGET_DIST - MIN_LOOK_TARGET_DIST);
	}

	return flInterest;
}

//-----------------------------------------------------------------------------
// Purpose: Integrate head turn over time
//-----------------------------------------------------------------------------
void CAI_BaseActor::SetHeadDirection( const Vector &vTargetPos, float flInterval)
{
	Assert(0); // Actors shouldn't be calling this, it doesn't do anything
}

float CAI_BaseActor::ClampWithBias( PoseParameter_t index, float value, float base )
{
	return EdgeLimitPoseParameter( (int)index, value, base );
}


//-----------------------------------------------------------------------------
// Purpose: Accumulate all the wanted yaw changes
//-----------------------------------------------------------------------------

void CAI_BaseActor::AccumulateIdealYaw( float flYaw, float flIntensity )
{
	float diff = AngleDiff( flYaw, GetLocalAngles().y );
	m_flAccumYawDelta += diff * flIntensity;
	m_flAccumYawScale += flIntensity;
}


//-----------------------------------------------------------------------------
// Purpose: do any pending yaw movements
//-----------------------------------------------------------------------------

bool CAI_BaseActor::SetAccumulatedYawAndUpdate( void )
{
	if (m_flAccumYawScale > 0.0)
	{
		float diff = m_flAccumYawDelta / m_flAccumYawScale;
		float facing = GetLocalAngles().y + diff;

		m_flAccumYawDelta = 0.0;
		m_flAccumYawScale = 0.0;

		if (IsCurSchedule( SCHED_SCENE_GENERIC ))
		{
			if (!IsMoving())
			{
				GetMotor()->SetIdealYawAndUpdate( facing );
				return true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: match actors "forward" attachment to point in direction of vHeadTarget
//-----------------------------------------------------------------------------

void CAI_BaseActor::UpdateBodyControl( )
{
	// FIXME: only during idle, or in response to accel/decel
	//Set( m_ParameterBodyTransY, Get( m_FlexweightMoveRightLeft ) );
	//Set( m_ParameterBodyTransX, Get( m_FlexweightMoveForwardBack ) );
	//Set( m_ParameterBodyLift, Get( m_FlexweightMoveUpDown ) );
	Set( m_ParameterBodyYaw, Get( m_FlexweightBodyRightLeft ) + m_goalBodyYaw );
	//Set( m_ParameterBodyPitch, Get( m_FlexweightBodyUpDown ) );
	//Set( m_ParameterBodyRoll, Get( m_FlexweightBodyTilt ) );
	Set( m_ParameterSpineYaw, Get( m_FlexweightChestRightLeft ) + m_goalSpineYaw );
	//Set( m_ParameterSpinePitch, Get( m_FlexweightChestUpDown ) );
	//Set( m_ParameterSpineRoll, Get( m_FlexweightChestTilt ) );
	Set( m_ParameterNeckTrans, Get( m_FlexweightHeadForwardBack ) );
}


static ConVar scene_clamplookat( "scene_clamplookat", "1", FCVAR_NONE, "Clamp head turns to a max of 20 degrees per think." );


void CAI_BaseActor::UpdateHeadControl( const Vector &vHeadTarget, float flHeadInfluence )
{
	float flTarget;
	float flLimit;

	if (!(CapabilitiesGet() & bits_CAP_TURN_HEAD))
	{
		return;
	}

	// calc current animation head bias, movement needs to clamp accumulated with this
	QAngle angBias;
	QAngle vTargetAngles;

	int iEyes = LookupAttachment( "eyes" );
	int iChest = LookupAttachment( "chest" );
	int iForward = LookupAttachment( "forward" );

	matrix3x4_t eyesToWorld;
	matrix3x4_t forwardToWorld, worldToForward;

	if (iEyes <= 0 || iForward <= 0)
	{
		// Head control on model without "eyes" or "forward" attachment
		// Most likely this is a cheaple or a generic_actor set to a model that doesn't support head/eye turning.
		// DevWarning( "%s using model \"%s\" that doesn't support head turning\n", GetClassname(), STRING( GetModelName() ) );
		CapabilitiesRemove( bits_CAP_TURN_HEAD );
		return;
	}

	GetAttachment( iEyes, eyesToWorld );

	GetAttachment( iForward, forwardToWorld );
	MatrixInvert( forwardToWorld, worldToForward );

	// Lookup chest attachment to do compounded range limit checks
	if (iChest > 0)
	{
		matrix3x4_t chestToWorld, worldToChest;
		GetAttachment( iChest, chestToWorld );
		MatrixInvert( chestToWorld, worldToChest );
		matrix3x4_t tmpM;
		ConcatTransforms( worldToChest, eyesToWorld, tmpM );
		MatrixAngles( tmpM, angBias );

		angBias.y -= Get( m_ParameterHeadYaw );
		angBias.x -= Get( m_ParameterHeadPitch );
		angBias.z -= Get( m_ParameterHeadRoll );

		/*
		if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
		{
			// Msg("bias %f %f %f\n", angBias.x, angBias.y, angBias.z );

			Vector tmp1, tmp2;
			
			VectorTransform( Vector( 0, 0, 0), chestToWorld, tmp1 );
			VectorTransform( Vector( 100, 0, 0), chestToWorld, tmp2 );
			NDebugOverlay::Line( tmp1, tmp2, 0,0,255, false, 0.12 );

			VectorTransform( Vector( 0, 0, 0), eyesToWorld, tmp1 );
			VectorTransform( Vector( 100, 0, 0), eyesToWorld, tmp2 );
			NDebugOverlay::Line( tmp1, tmp2, 0,0,255, false, 0.12 );

			// NDebugOverlay::Line( EyePosition(), pEntity->EyePosition(), 0,0,255, false, 0.5);
		}
		*/
	}
	else
	{
		angBias.Init( 0, 0, 0 );
	}

	matrix3x4_t targetXform;
	targetXform = forwardToWorld;
	Vector vTargetDir = vHeadTarget - EyePosition();

	if (scene_clamplookat.GetBool())
	{
		// scale down pitch when the target is behind the head
		Vector vTargetLocal;
		VectorNormalize( vTargetDir );
		VectorIRotate( vTargetDir, forwardToWorld, vTargetLocal );
		vTargetLocal.z *= clamp( vTargetLocal.x, 0.1f, 1.0f );
		VectorNormalize( vTargetLocal );
		VectorRotate( vTargetLocal, forwardToWorld, vTargetDir );

		// clamp local influence when target is behind the head
		flHeadInfluence = flHeadInfluence * clamp( vTargetLocal.x * 2.0f + 2.0f, 0.0f, 1.0f );
	}

	Studio_AlignIKMatrix( targetXform, vTargetDir );

	matrix3x4_t headXform;
	ConcatTransforms( worldToForward, targetXform, headXform );
	MatrixAngles( headXform, vTargetAngles );

	// partially debounce head goal
	float s0 = 1.0 - flHeadInfluence + GetHeadDebounce() * flHeadInfluence;
	float s1 = (1.0 - s0);
	// limit velocity of head turns
	m_goalHeadCorrection.x = UTIL_Approach( m_goalHeadCorrection.x * s0 + vTargetAngles.x * s1, m_goalHeadCorrection.x, 10.0 );
	m_goalHeadCorrection.y = UTIL_Approach( m_goalHeadCorrection.y * s0 + vTargetAngles.y * s1, m_goalHeadCorrection.y, 30.0 );
	m_goalHeadCorrection.z = UTIL_Approach( m_goalHeadCorrection.z * s0 + vTargetAngles.z * s1, m_goalHeadCorrection.z, 10.0 );

	/*
	if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
		// Msg( "yaw %.1f (%f) pitch %.1f (%.1f)\n", m_goalHeadCorrection.y, vTargetAngles.y, vTargetAngles.x, m_goalHeadCorrection.x );
		// Msg( "yaw %.2f (goal %.2f) (influence %.2f) (flex %.2f)\n", flLimit, m_goalHeadCorrection.y, flHeadInfluence, Get( m_FlexweightHeadRightLeft ) );
	}
	*/

	flTarget = m_goalHeadCorrection.y + Get( m_FlexweightHeadRightLeft );
	flLimit = ClampWithBias( m_ParameterHeadYaw, flTarget, angBias.y );
	/*
	if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
		Msg( "yaw  %5.1f : (%5.1f : %5.1f) %5.1f (%5.1f)\n", flLimit, m_goalHeadCorrection.y, Get( m_FlexweightHeadRightLeft ), angBias.y, Get( m_ParameterHeadYaw ) );
	}
	*/
	Set( m_ParameterHeadYaw, flLimit );

	flTarget = m_goalHeadCorrection.x + Get( m_FlexweightHeadUpDown );
	flLimit = ClampWithBias( m_ParameterHeadPitch, flTarget, angBias.x );
	/*
	if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
		Msg( "pitch %5.1f : (%5.1f : %5.1f) %5.1f (%5.1f)\n", flLimit, m_goalHeadCorrection.x, Get( m_FlexweightHeadUpDown ), angBias.x, Get( m_ParameterHeadPitch ) );
	}
	*/
	Set( m_ParameterHeadPitch, flLimit );

	flTarget = m_goalHeadCorrection.z + Get( m_FlexweightHeadTilt );
	flLimit = ClampWithBias( m_ParameterHeadRoll, flTarget, angBias.z );
	/*
	if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
		Msg( "roll  %5.1f : (%5.1f : %5.1f) %5.1f (%5.1f)\n", flLimit, m_goalHeadCorrection.z, Get( m_FlexweightHeadTilt ), angBias.z, Get( m_ParameterHeadRoll ) );
	}
	*/
	Set( m_ParameterHeadRoll, flLimit );
}



//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's eye direction in 2D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseActor::EyeDirection2D( void )
{
	Vector vEyeDirection = EyeDirection3D( );
	vEyeDirection.z = 0;

	vEyeDirection.AsVector2D().NormalizeInPlace();

	return vEyeDirection;
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's eye direction in 2D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseActor::EyeDirection3D( void )
{
	UpdateLatchedValues( );

	return m_latchedEyeDirection;
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's head direction in 2D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseActor::HeadDirection2D( void )
{	
	Vector vHeadDirection = HeadDirection3D();
	vHeadDirection.z = 0;
	vHeadDirection.AsVector2D().NormalizeInPlace();
	return vHeadDirection;
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's head direction in 3D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseActor::HeadDirection3D( )
{	
	UpdateLatchedValues( );

	return m_latchedHeadDirection;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CAI_BaseActor::HasActiveLookTargets( void )
{
	return m_lookQueue.Count() != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Clear any active look targets for the specified entity
//-----------------------------------------------------------------------------
void CAI_BaseActor::ClearLookTarget( CBaseEntity *pTarget )
{
	int iIndex = m_lookQueue.Find( pTarget );
	if ( iIndex != m_lookQueue.InvalidIndex() )
	{
		m_lookQueue.Remove(iIndex);
	}

	iIndex = m_randomLookQueue.Find( pTarget );
	if ( iIndex != m_randomLookQueue.InvalidIndex() )
	{
		m_randomLookQueue.Remove(iIndex);

		// Figure out the new random look time
		m_flNextRandomLookTime = gpGlobals->curtime + 1.0;
		for (int i = 0; i < m_randomLookQueue.Count(); i++)
		{
			if ( m_randomLookQueue[i].m_flEndTime > m_flNextRandomLookTime )
			{
				m_flNextRandomLookTime = m_randomLookQueue[i].m_flEndTime + 0.4;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Look at other NPCs and clients from time to time
//-----------------------------------------------------------------------------
float CAI_BaseActor::PickLookTarget( bool bExcludePlayers, float minTime, float maxTime )
{
	return PickLookTarget( m_randomLookQueue, bExcludePlayers, minTime, maxTime );
}

float CAI_BaseActor::PickLookTarget( CAI_InterestTarget &queue, bool bExcludePlayers, float minTime, float maxTime )
{
	AILookTargetArgs_t args;
	
	args.vTarget			= vec3_invalid;
	args.flDuration			= random->RandomFloat( minTime, maxTime );
	args.flInfluence		= random->RandomFloat( 0.3, 0.5 );
	args.flRamp				= random->RandomFloat( 0.2, 0.4 );
	args.bExcludePlayers	= bExcludePlayers;
	args.pQueue				= &queue;
	
	bool foundLookTarget = true;
	
	if ( !PickTacticalLookTarget( &args ) )
	{
		if ( !PickRandomLookTarget( &args ) )
		{
			foundLookTarget = false;
		}
	}
	
	if ( !foundLookTarget )
	{
		// DevMsg("nothing to see\n" );
		MakeRandomLookTarget( &args, minTime, maxTime );
	}
	
	// See if derived NPCs want to do anything with this look target before I use it
	OnSelectedLookTarget( &args );

	if ( args.hTarget != NULL )
	{
		Assert( args.vTarget == vec3_invalid );
		queue.Add( args.hTarget, args.flInfluence, args.flDuration, args.flRamp );
	}
	else
	{
		Assert( args.vTarget != vec3_invalid );
		queue.Add( args.vTarget, args.flInfluence, args.flDuration, args.flRamp );
	}

	return args.flDuration;
	
}

bool CAI_BaseActor::PickTacticalLookTarget( AILookTargetArgs_t *pArgs )
{
	CBaseEntity *pEnemy = GetEnemy();

	if (pEnemy != NULL)
	{
		if ( ( FVisible( pEnemy ) || random->RandomInt(0, 3) == 0 ) && ValidHeadTarget(pEnemy->EyePosition()))
		{
			// look at enemy closer
			pArgs->hTarget = pEnemy;
			pArgs->flInfluence = random->RandomFloat( 0.7, 1.0 );
			pArgs->flRamp = 0;
			return true;
		}
		else
		{
			// look at something else for a shorter time
			pArgs->flDuration = random->RandomFloat( 0.5, 0.8 );
			// move head faster
			pArgs->flRamp = 0.2;
		}
	}
	return false;
}

bool CAI_BaseActor::PickRandomLookTarget( AILookTargetArgs_t *pArgs )
{
	bool bIsNavigating = ( GetNavigator()->IsGoalActive() && GetNavigator()->IsGoalSet() );
	
	if ( bIsNavigating && random->RandomInt(1, 10) <= 3 )
	{
		Vector navLookPoint;
		Vector delta;
		if ( GetNavigator()->GetPointAlongPath( &navLookPoint, 12 * 12 ) && (delta = navLookPoint - GetAbsOrigin()).Length() > 8.0 * 12.0 )
		{
			if ( random->RandomInt(1, 10) <= 5 )
			{
				pArgs->vTarget = navLookPoint;
				pArgs->flDuration = random->RandomFloat( 0.2, 0.4 );
			}
			else
			{
				pArgs->hTarget = this;
				pArgs->flDuration = random->RandomFloat( 1.0, 2.0 );
			}
			pArgs->flRamp = 0.2;
			return true;
		}
	}

	if ( GetState() == NPC_STATE_COMBAT && random->RandomInt(1, 10) <= 8 )
	{
		// if in combat, look forward 80% of the time?
		pArgs->hTarget = this;
		return true;
	}

	CBaseEntity *pBestEntity = NULL;
	CBaseEntity *pEntity = NULL;
	int iHighestImportance = 0;
	int iConsidered = 0;
	for ( CEntitySphereQuery sphere( GetAbsOrigin(), 30 * 12, 0 ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		if (pEntity == this)
		{
			continue;
		}

		if ( pArgs->bExcludePlayers && pEntity->GetFlags() & FL_CLIENT )
		{
			// Don't look at any players.
			continue;
		}

		if (!pEntity->IsViewable())
		{
			// Don't look at things without a model, or aren't tagged as interesting
			continue;
		}

		if ( pEntity->GetOwnerEntity() && !pEntity->GetOwnerEntity()->IsViewable() )
		{
			// Don't look at things that are associated with non-viewable owners. 
			// Specifically, this prevents NPC's looking at beams or sprites that
			// are part of a viewmodel. (sjb)
			continue;
		}

		// Don't look at any object that is ultimately parented to the player.
		// These objects will almost always be at the player's origin (feet), and it
		// looks bad when an actor looks at the player's feet. (sjb)
		CBaseEntity *pParent = pEntity->GetParent();
		bool bObjectParentedToPlayer = false;
		while( pParent )
		{
			if( pParent->IsPlayer() )
			{
				bObjectParentedToPlayer = true;
				break;
			}

			pParent = pParent->GetParent();
		}

		if( bObjectParentedToPlayer )
			continue;
		
		// skip entities we're already looking at
		if ( pArgs->pQueue->Find( pEntity ) != pArgs->pQueue->InvalidIndex() )
			continue;

		// keep track of number of interesting things
		iConsidered++;

		if ((pEntity->GetFlags() & FL_CLIENT) && (pEntity->IsMoving() || random->RandomInt( 0, 2) == 0))
		{
			if (FVisible( pEntity ) && ValidHeadTarget(pEntity->EyePosition()))
			{
				pArgs->flDuration = random->RandomFloat( 1.0, 4.0 );
				pBestEntity = pEntity;
				break;
			}
		}
	
		Vector delta = (pEntity->EyePosition() - EyePosition());
		VectorNormalize( delta );

		int iImportance;
#if 0
		// consider things in front to be more important than things to the sides
		iImportance = (DotProduct( delta, HeadDirection3D() );
#else
		// No, for now, give all targets random priority (as long as they're in front)
		iImportance = random->RandomInt( 1, 100 );
		
#endif
		// make other npcs, and moving npc's far more important
		if (pEntity->MyNPCPointer())
		{
			iImportance *= 10;
			if (pEntity->IsMoving())
			{
				iImportance *= 10;
			}
		}

		if ( iImportance > iHighestImportance )
		{
			if (FVisible( pEntity ) && ValidHeadTarget(pEntity->EyePosition()))
			{
				iHighestImportance = iImportance;
				pBestEntity	= pEntity; 
				// NDebugOverlay::Line( EyePosition(), pEntity->EyePosition(), 0,0,255, false, 0.5);
			}
		}
	}

	// if there were too few things to look at, don't trust the item
	if (iConsidered < random->RandomInt( 0, 5))
	{
		pBestEntity = NULL;
	}

	if (pBestEntity)
	{
		//Msg("looking at %s\n", pBestEntity->GetClassname() );
		//NDebugOverlay::Line( EyePosition(), pBestEntity->WorldSpaceCenter(), 255, 0, 0, false, 5 );
		pArgs->hTarget = pBestEntity;
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// All attempts to find a target have failed, so just make something up.
//-----------------------------------------------------------------------------
void CAI_BaseActor::MakeRandomLookTarget( AILookTargetArgs_t *pArgs, float minTime, float maxTime )
{
	Vector forward, right, up;
	GetVectors( &forward, &right, &up );

	// DevMsg("random view\n");

	// For now, just look farther afield while driving in the vehicle.  Without this we look around wildly!
#ifdef HL2_EPISODIC
	if ( MyCombatCharacterPointer() && MyCombatCharacterPointer()->IsInAVehicle() )
	{
		pArgs->vTarget = EyePosition() + forward * 2048 + right * random->RandomFloat(-650,650) + up * random->RandomFloat(-32,32);
	}
	else
#endif // HL2_EPISODIC
	{
		pArgs->vTarget = EyePosition() + forward * 128 + right * random->RandomFloat(-32,32) + up * random->RandomFloat(-16,16);
	}

	pArgs->flDuration = random->RandomFloat( minTime, maxTime );
	pArgs->flInfluence = 0.01;
	pArgs->flRamp = random->RandomFloat( 0.8, 2.8 );
}

//-----------------------------------------------------------------------------
// Purpose: Make sure we're looking at what we're shooting at
//-----------------------------------------------------------------------------

void CAI_BaseActor::StartTaskRangeAttack1( const Task_t *pTask )
{
	BaseClass::StartTaskRangeAttack1( pTask );
	if (GetEnemy())
	{
		AddLookTarget( GetEnemy(), 1.0, 0.5, 0.2 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Set direction that the NPC is looking
//-----------------------------------------------------------------------------
void CAI_BaseActor::AddLookTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp )
{
	m_lookQueue.Add( pTarget, flImportance, flDuration, flRamp );
}


void CAI_BaseActor::AddLookTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
{
	m_lookQueue.Add( vecPosition, flImportance, flDuration, flRamp );
}

//-----------------------------------------------------------------------------
// Purpose: Maintain eye, head, body postures, etc.
//-----------------------------------------------------------------------------
void CAI_BaseActor::MaintainLookTargets( float flInterval )
{
	int i;

	if ( m_iszExpressionScene != NULL_STRING && m_hExpressionSceneEnt == NULL )
	{
		InstancedScriptedScene( this, STRING(m_iszExpressionScene), &m_hExpressionSceneEnt, 0.0, true );
	}

	// decay body/spine yaw
	m_goalSpineYaw = m_goalSpineYaw * 0.8;
	m_goalBodyYaw = m_goalBodyYaw * 0.8;
	m_goalHeadCorrection = m_goalHeadCorrection * 0.8;

	// ARRGGHHH, this needs to be moved!!!!
	SetAccumulatedYawAndUpdate( );
	ProcessSceneEvents( );
	MaintainTurnActivity( );
	DoBodyLean( );
	UpdateBodyControl( );
	InvalidateBoneCache();

	// cached versions of the current eye position
	Vector vEyePosition = EyePosition( );

	// FIXME: make this client side and automatic
	// set gesture positions
	Set( m_ParameterGestureHeight, Get( m_FlexweightGestureUpDown ) );
	Set( m_ParameterGestureWidth, Get( m_FlexweightGestureRightLeft ) );

	// initialize goal head direction to be current direction - this frames animation layering/pose parameters -  
	// but with the head controlls removed.
	Vector vHead = HeadDirection3D( );
	float flHeadInfluence = 0.0;

	// NDebugOverlay::Line( vEyePosition, vEyePosition + vHead * 16, 0,0,255, false, 0.1);

	// clean up look targets
	m_lookQueue.Cleanup();

	// clean up random look targets
	m_randomLookQueue.Cleanup();

	// clean up synthetic look targets
	m_syntheticLookQueue.Cleanup();

	// if there's real things to look at, turn off the random targets
	if (m_lookQueue.Count() != 0 || m_syntheticLookQueue.Count() != 0)
	{
		for (i = 0; i < m_randomLookQueue.Count(); i++)
		{
			if (gpGlobals->curtime < m_randomLookQueue[i].m_flEndTime - m_randomLookQueue[i].m_flRamp - 0.2)
			{
				m_randomLookQueue[i].m_flEndTime = gpGlobals->curtime + m_randomLookQueue[i].m_flRamp + 0.2;
			}
		}
		m_flNextRandomLookTime = gpGlobals->curtime + 1.0;
	}
	else if (gpGlobals->curtime >= m_flNextRandomLookTime && GetState() != NPC_STATE_SCRIPT)
	{
		// Look at whatever!
		m_flNextRandomLookTime = gpGlobals->curtime + PickLookTarget( m_randomLookQueue ) - 0.4;
	}

	// don't bother with any of the rest if the player can't see you
	if (!HasCondition( COND_IN_PVS ))
	{
		return;
	}

	// Time to finish the current random expression? Or time to pick a new one?
	if ( m_flNextRandomExpressionTime && gpGlobals->curtime > m_flNextRandomExpressionTime )
	{
		// Random expressions need to be cleared, because they don't loop. So if we
		// pick the same one again, we want to restart it.
		ClearExpression();

		PlayExpressionForState( GetState() );
	}

	CUtlVector<CAI_InterestTarget_t *> active;
	// clean up random look targets
	for (i = 0; i < m_randomLookQueue.Count(); i++)
	{
		active.AddToTail( &m_randomLookQueue[i] );
	}
	for (i = 0; i < m_lookQueue.Count(); i++)
	{
		active.AddToTail( &m_lookQueue[i] );
	}
	for (i = 0; i < m_syntheticLookQueue.Count(); i++)
	{
		active.AddToTail( &m_syntheticLookQueue[i] );
	}
		
	// figure out ideal head yaw
	bool bValidHeadTarget = false;
	bool bExpectedHeadTarget = false;
	for (i = 0; i < active.Count();i++)
	{
		Vector dir;
		float flDist = 100.0f;
		
		bExpectedHeadTarget = true;
		float flInterest = active[i]->Interest( );

		if (active[i]->IsThis( this ))
		{
			int iForward = LookupAttachment( "forward" );
			if ( iForward > 0)
			{
				Vector tmp1;
				GetAttachment( iForward, tmp1, &dir, NULL, NULL );
			}
			else
			{
				dir = HeadDirection3D();
			}
		}
		else
		{
			dir = active[i]->GetPosition() - vEyePosition;
			flDist = VectorNormalize( dir );
			flInterest = flInterest * HeadTargetValidity( active[i]->GetPosition() );
		}
		
		/*
		if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
		{
			DevMsg( "head (%d) %.2f : %s : %.1f %.1f %.1f\n", i, flInterest, active[i]->m_hTarget->GetClassname(), active[i]->GetPosition().x, active[i]->GetPosition().y, active[i]->GetPosition().z );
		}
		*/
		
		if (flInterest > 0.0)
		{
			if (flHeadInfluence == 0.0)
			{
				vHead = dir;
				flHeadInfluence = flInterest;
			}
			else
			{
				flHeadInfluence = flHeadInfluence * (1 - flInterest) + flInterest;
				float w = flInterest / flHeadInfluence;
				vHead = vHead * (1 - w) + dir * w;
			}

			bValidHeadTarget = true;

			// NDebugOverlay::Line( vEyePosition, vEyePosition + dir * 64, 0,255,0, false, 0.1);
		}
		else
		{
			// NDebugOverlay::Line( vEyePosition, active[i]->GetPosition(), 255,0,0, false, 0.1);
		}
	}

	Assert( flHeadInfluence <= 1.0 );

	// turn head toward target
	if (bValidHeadTarget)
	{
		UpdateHeadControl( vEyePosition + vHead * 100, flHeadInfluence );
		m_goalHeadDirection = vHead;
		m_goalHeadInfluence = flHeadInfluence;
	}
	else
	{
		// no target, decay all head control direction
		m_goalHeadDirection = m_goalHeadDirection * 0.8 + vHead * 0.2;

		m_goalHeadInfluence = MAX( m_goalHeadInfluence - 0.2, 0 );

		VectorNormalize( m_goalHeadDirection );
		UpdateHeadControl( vEyePosition + m_goalHeadDirection * 100, m_goalHeadInfluence );
		// NDebugOverlay::Line( vEyePosition, vEyePosition + m_goalHeadDirection * 100, 255,0,0, false, 0.1);
	}

	// DevMsg( "%.1f %.1f ", GetPoseParameter( "head_pitch" ), GetPoseParameter( "head_roll" ) );

	// figure out eye target
	// eyes need to look directly at a target, even if the head doesn't quite aim there yet.
	bool bFoundTarget = false;
	EHANDLE	hTarget = NULL;

	for (i = active.Count() - 1; i >= 0; i--)
	{
		if (active[i]->IsThis( this ))
		{
			// DevMsg( "eyes (%d) %s\n", i, STRING( active[i]->m_hTarget->GetEntityName().ToCStr() ) );
			bFoundTarget = true;
			hTarget = this;
			SetViewtarget( vEyePosition + HeadDirection3D() * 100 );
			// NDebugOverlay::Line( vEyePosition, vEyePosition + HeadDirection3D() * 100, 255,0,0, false, 0.1);
			break;
		}
		else
		{
			// E3 Hack
			if (ValidEyeTarget(active[i]->GetPosition()))
			{
				// DevMsg( "eyes (%d) %s\n", i, STRING( pTarget->GetEntityName().ToCStr() ) );

				bFoundTarget = true;
				hTarget = active[i]->m_hTarget;
				SetViewtarget( active[i]->GetPosition() );
				break;
			}
		}
	}

	// FIXME: add blink when changing targets
	if (m_hLookTarget != hTarget)
	{
		m_flBlinktime -= 0.5;
		m_hLookTarget = hTarget;

		if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) && ai_debug_looktargets.GetInt() == 2 && m_hLookTarget.Get() )
		{
			if ( m_hLookTarget != this )
			{
				Vector vecEyePos = m_hLookTarget->EyePosition();
				NDebugOverlay::Box( vecEyePos, -Vector(5,5,5), Vector(5,5,5), 0, 255, 0, 255, 20 );
				NDebugOverlay::Line( EyePosition(), vecEyePos, 0,255,0, true, 20 );
				NDebugOverlay::Text( vecEyePos, UTIL_VarArgs( "%s (%s)", m_hLookTarget->GetClassname(), m_hLookTarget->GetDebugName() ), false, 20 );
			}
		}

		OnNewLookTarget();
	}

	// this should take into acount where it will try to be....
	if (!bFoundTarget && !ValidEyeTarget( GetViewtarget() ))
	{
		Vector right, up;
		VectorVectors( HeadDirection3D(), right, up );
		// DevMsg("random view\n");
		SetViewtarget( EyePosition() + HeadDirection3D() * 128 + right * random->RandomFloat(-32,32) + up * random->RandomFloat(-16,16) );
	}

	if ( m_hLookTarget != NULL )
	{
		Vector absVel = m_hLookTarget->GetAbsVelocity();
		CBaseEntity *ground = m_hLookTarget->GetGroundEntity();
		if ( ground && ground->GetMoveType() == MOVETYPE_PUSH)
		{
			absVel = absVel + ground->GetAbsVelocity();
		}

#ifdef HL2_EPISODIC
		// Translate our position if riding in a vehicle
		if ( m_hLookTarget->MyCombatCharacterPointer() )
		{
			CBaseCombatCharacter *pBCC = m_hLookTarget->MyCombatCharacterPointer();
			CBaseEntity *pVehicle = pBCC->GetVehicleEntity();
			if ( pVehicle )
			{
				IPhysicsObject *pObj = pVehicle->VPhysicsGetObject();
				if ( pObj )
				{
					Vector vecVelocity;
					pObj->GetVelocity( &vecVelocity, NULL );

					absVel += vecVelocity;
				}
			}
		}
#endif //HL2_EPISODIC

		if ( !VectorCompare( absVel, vec3_origin ) )
		{
			Vector viewTarget = GetViewtarget();

			// Forward one think cycle
			viewTarget += absVel * flInterval;

			SetViewtarget( viewTarget );
		}
	}

	// NDebugOverlay::Triangle( vEyePosition, GetViewtarget(), GetAbsOrigin(), 255, 255, 255, 10, false, 0.1 );

	// DevMsg("pitch %.1f yaw %.1f\n", GetFlexWeight( "eyes_updown" ), GetFlexWeight( "eyes_rightleft" ) );

	// blink
	if (m_flBlinktime < gpGlobals->curtime)
	{
		Blink();
		m_flBlinktime = gpGlobals->curtime + random->RandomFloat( 1.5, 4.5 );
	}

	if ( ai_debug_looktargets.GetInt() == 1 && (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
		NDebugOverlay::Box( GetViewtarget(), -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, 0, 20 );
		NDebugOverlay::Line( EyePosition(),GetViewtarget(), 0,255,0, false, .1 );
	}
}

//-----------------------------------------------------------------------------

void CAI_BaseActor::PlayExpressionForState( NPC_STATE state )
{
	// If we have an override expression, use it above everything else
	if ( m_iszExpressionOverride != NULL_STRING && state != NPC_STATE_DEAD )
	{
		SetExpression( STRING(m_iszExpressionOverride) );
		return;
	}

	// If we have a random expression, use that
	const char *pszExpression = SelectRandomExpressionForState( state );
	if ( pszExpression && *pszExpression )
	{
		float flDuration = SetExpression( pszExpression );
		m_flNextRandomExpressionTime = gpGlobals->curtime + flDuration;
		return;
	}
	else
	{
		// Stop looking for random expressions for this state
		m_flNextRandomExpressionTime = 0;
	}

	// Lastly, use the base expression loops
	switch ( state )
	{
	case NPC_STATE_IDLE:
		if ( m_iszIdleExpression != NULL_STRING )
		{
			SetExpression( STRING(m_iszIdleExpression) );
		}
		break;

	case NPC_STATE_COMBAT:
		if ( m_iszCombatExpression != NULL_STRING )
		{
			SetExpression( STRING(m_iszCombatExpression) );
		}
		break;

	case NPC_STATE_ALERT:
		if ( m_iszAlertExpression != NULL_STRING )
		{
			SetExpression( STRING(m_iszAlertExpression) );
		}
		break;

	case NPC_STATE_PLAYDEAD:
	case NPC_STATE_DEAD:
		if ( m_iszDeathExpression != NULL_STRING )
		{
			SetExpression( STRING(m_iszDeathExpression) );
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return a random expression for the specified state to play over 
//			the state's expression loop.
//-----------------------------------------------------------------------------
const char *CAI_BaseActor::SelectRandomExpressionForState( NPC_STATE state )
{
	return NULL;
}

//-----------------------------------------------------------------------------

void CAI_BaseActor::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	PlayExpressionForState( NewState );

#ifdef HL2_EPISODIC
	// If we've just switched states, ensure we stop any scenes that asked to be stopped
	if ( OldState == NPC_STATE_IDLE )
	{
		RemoveActorFromScriptedScenes( this, true, true );
	}
#endif

	BaseClass::OnStateChange( OldState, NewState );
}

//-----------------------------------------------------------------------------

float CAI_BaseActor::SetExpression( const char *pszExpressionScene )
{
	if ( !pszExpressionScene || !*pszExpressionScene )
	{
		ClearExpression();
		return 0;
	}

	if ( m_iszExpressionScene != NULL_STRING && stricmp( STRING(m_iszExpressionScene), pszExpressionScene ) == 0 )
	{
		return 0;
	}

	if ( m_hExpressionSceneEnt != NULL )
	{
		StopScriptedScene( this, m_hExpressionSceneEnt );
	}

	if ( ai_debug_expressions.GetInt() )
	{
		Msg("%s (%s) set expression to: %s\n", GetClassname(), GetDebugName(), pszExpressionScene );
	}

	m_iszExpressionScene = NULL_STRING;
	if ( pszExpressionScene )
	{
		float flDuration = InstancedScriptedScene( this, pszExpressionScene, &m_hExpressionSceneEnt, 0.0, true );

		if ( m_hExpressionSceneEnt != NULL )
		{
			m_iszExpressionScene = AllocPooledString( pszExpressionScene );
		}

		return flDuration;
	}

	return 0;
}

//-----------------------------------------------------------------------------

void CAI_BaseActor::ClearExpression()
{
	if ( m_hExpressionSceneEnt != NULL )
	{
		StopScriptedScene( this, m_hExpressionSceneEnt );
	}
	m_iszExpressionScene = NULL_STRING;
}

//-----------------------------------------------------------------------------

const char *CAI_BaseActor::GetExpression()
{
	return STRING(m_iszExpressionScene);
}

//-----------------------------------------------------------------------------

void CAI_BaseActor::InputSetExpressionOverride( inputdata_t &inputdata )
{
	bool fHadOverride = ( m_iszExpressionOverride != NULL_STRING );
	m_iszExpressionOverride = inputdata.value.StringID();
	if (  m_iszExpressionOverride != NULL_STRING )
	{
		SetExpression( STRING(m_iszExpressionOverride) );
	}
	else if ( fHadOverride )
	{
		PlayExpressionForState( GetState() );
	}
}

//-----------------------------------------------------------------------------

bool CAI_BaseActor::UseSemaphore( void )
{
	if ( m_bDontUseSemaphore )
		return false;

	return true;
}

//-----------------------------------------------------------------------------

CAI_Expresser *CAI_BaseActor::CreateExpresser()
{
	m_pExpresser = new CAI_Expresser(this);
	return m_pExpresser;
}

//-----------------------------------------------------------------------------

CAI_Expresser *CAI_BaseActor::GetExpresser() 
{ 
	return m_pExpresser; 
}
	
//-----------------------------------------------------------------------------

bool CAI_BaseActor::CreateComponents()
{
	if ( !BaseClass::CreateComponents() )
		return false;

	m_pExpresser = CreateExpresser();
	if ( !m_pExpresser)
		return false;

	m_pExpresser->Connect(this);

	return true;
}

//-----------------------------------------------------------------------------
