//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"
#include "tf_playeranimstate.h"
#include "base_playeranimstate.h"
#include "datacache/imdlcache.h"
#include "tf_gamerules.h"
#include "in_buttons.h"
#include "debugoverlay_shared.h"
#include "tf_weapon_passtime_gun.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_func_capture_zone.h"
#include "tf_gcmessages.h"

#define CCaptureZone C_CaptureZone
#else
#include "tf_player.h"
#include "func_capture_zone.h"
#endif

#define TF_RUN_SPEED			320.0f
#define TF_WALK_SPEED			75.0f
#define TF_CROUCHWALK_SPEED		110.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : CMultiPlayerAnimState*
//-----------------------------------------------------------------------------
CTFPlayerAnimState* CreateTFPlayerAnimState( CTFPlayer *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	// Setup the movement data.
	MultiPlayerMovementData_t movementData;
	movementData.m_flBodyYawRate = 720.0f;
	movementData.m_flRunSpeed = TF_RUN_SPEED;
	movementData.m_flWalkSpeed = TF_WALK_SPEED;
	movementData.m_flSprintSpeed = -1.0f;

	// Create animation state for this player.
	CTFPlayerAnimState *pRet = new CTFPlayerAnimState( pPlayer, movementData );

	// Specific TF player initialization.
	pRet->InitTF( pPlayer );

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFPlayerAnimState::CTFPlayerAnimState()
{
	m_pTFPlayer = NULL;

	// Don't initialize TF specific variables here. Init them in InitTF()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			&movementData - 
//-----------------------------------------------------------------------------
CTFPlayerAnimState::CTFPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData )
: CMultiPlayerAnimState( pPlayer, movementData )
{
	m_pTFPlayer = NULL;

	// Don't initialize TF specific variables here. Init them in InitTF()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFPlayerAnimState::~CTFPlayerAnimState()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initialize Team Fortress specific animation state.
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::InitTF( CTFPlayer *pPlayer )
{
	m_pTFPlayer = pPlayer;
	m_bInAirWalk = false;
	m_flHoldDeployedPoseUntilTime = 0.0f;
	m_flTauntMoveX = 0.f;
	m_flTauntMoveY = 0.f;
	m_vecSmoothedUp = Vector( 0.f, 0.f, 1.f );
	m_flVehicleLeanVel = 0.f;
	m_flVehicleLeanPos = 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::ClearAnimationState( void )
{
	m_bInAirWalk = false;

	BaseClass::ClearAnimationState();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : actDesired - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CTFPlayerAnimState::TranslateActivity( Activity actDesired )
{
	Activity translateActivity = BaseClass::TranslateActivity( actDesired );

	translateActivity = ActivityOverride( translateActivity, NULL );

	CBaseCombatWeapon *pWeapon = GetTFPlayer()->GetActiveWeapon();
	if ( pWeapon )
	{
		translateActivity = pWeapon->ActivityOverride( translateActivity, NULL );

		CEconItemView *pEconItemView = pWeapon->GetAttributeContainer()->GetItem();
		if ( pEconItemView )
		{
			translateActivity = pEconItemView->GetStaticData()->GetActivityOverride( GetTFPlayer()->GetTeamNumber(), translateActivity );
		}
	}

	CTFPlayer *pPlayer = GetTFPlayer();
	if ( pPlayer->m_Shared.InCond( TF_COND_COMPETITIVE_WINNER ) )
	{
		if ( translateActivity == ACT_MP_STAND_PRIMARY || 
		   ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) && ( translateActivity == ACT_MP_STAND_MELEE ) ) || 
		   ( pPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) && ( translateActivity == ACT_MP_STAND_SECONDARY ) ) )
		{
			translateActivity = ACT_MP_COMPETITIVE_WINNERSTATE;
		}
	}

	return translateActivity;
}


static acttable_t s_acttableKartState[] = 
{
	{ ACT_MP_STAND_IDLE,					ACT_KART_IDLE,			false },
	{ ACT_MP_RUN,							ACT_KART_IDLE,			false },
	{ ACT_MP_WALK,							ACT_KART_IDLE,			false },
	{ ACT_MP_AIRWALK,						ACT_KART_IDLE,			false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_KART_ACTION_SHOOT,	false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_KART_ACTION_SHOOT,	false },
	{ ACT_MP_JUMP_START,					ACT_KART_JUMP_START,	false },
	{ ACT_MP_JUMP_FLOAT,					ACT_KART_JUMP_FLOAT,	false },
	{ ACT_MP_JUMP_LAND,						ACT_KART_JUMP_LAND,		false },
};

static acttable_t s_acttableLoserState[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_LOSERSTATE,			false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_LOSERSTATE,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_LOSERSTATE,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_LOSERSTATE,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_LOSERSTATE,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_LOSERSTATE,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_LOSERSTATE,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_LOSERSTATE,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_LOSERSTATE,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_LOSERSTATE,		false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_LOSERSTATE,				false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_LOSERSTATE,false },
};

static acttable_t s_acttableCompetitiveLoserState[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_COMPETITIVE_LOSERSTATE, false },
};

static acttable_t s_acttableBuildingDeployed[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_BUILDING_DEPLOYED,			false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_BUILDING_DEPLOYED,		false },
	{ ACT_MP_RUN,				ACT_MP_RUN_BUILDING_DEPLOYED,			false },
	{ ACT_MP_WALK,				ACT_MP_WALK_BUILDING_DEPLOYED,			false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_BUILDING_DEPLOYED,		false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_BUILDING_DEPLOYED,	false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_BUILDING_DEPLOYED,			false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_BUILDING_DEPLOYED,	false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_BUILDING_DEPLOYED,	false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_BUILDING_DEPLOYED,		false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_BUILDING_DEPLOYED,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_BUILDING_DEPLOYED,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_BUILDING_DEPLOYED,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_BUILDING_DEPLOYED,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_BUILDING_DEPLOYED,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_BUILDING,		false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_BUILDING,		false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_BUILDING,		false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_BUILDING,				false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_BUILDING,				false },
};

Activity CTFPlayerAnimState::ActivityOverride( Activity baseAct, bool *pRequired )
{
	acttable_t *pTable = NULL;
	int iActivityCount = 0;

	CTFPlayer *pPlayer = GetTFPlayer();

	// Override if we're in a kart
	if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		pTable = s_acttableKartState;
		iActivityCount = ARRAYSIZE( s_acttableKartState );
	}
	else
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_COMPETITIVE_LOSER ) )
		{
			iActivityCount = ARRAYSIZE( s_acttableCompetitiveLoserState );
			pTable = s_acttableCompetitiveLoserState;
		}
		else if ( pPlayer->m_Shared.IsLoser() )
		{
			iActivityCount = ARRAYSIZE( s_acttableLoserState );
			pTable = s_acttableLoserState;
		}
		else if ( pPlayer->m_Shared.IsCarryingObject() )
		{
			iActivityCount = ARRAYSIZE( s_acttableBuildingDeployed );
			pTable = s_acttableBuildingDeployed;
		}
	}

	for ( int i = 0; i < iActivityCount; i++ )
	{
		const acttable_t& act = pTable[i];
		if ( baseAct == act.baseAct )
		{
			if (pRequired)
			{
				*pRequired = act.required;
			}
			return (Activity)act.weaponAct;
		}
	}

	return baseAct;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::ShouldUpdateAnimState( void )
{
	CTFPlayer *pTFPlayer = GetTFPlayer();
	if ( pTFPlayer )
	{
		// Stop animating if we have a custom player model that doesn't use the normal class animations
		if ( pTFPlayer->GetPlayerClass()->HasCustomModel() && !pTFPlayer->GetPlayerClass()->CustomModelUsesClassAnimations() )
			return false;
	}

	return BaseClass::ShouldUpdateAnimState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::GetOuterAbsVelocity( Vector& vel )
{
#ifdef CLIENT_DLL
	if ( IsItemTestingBot() )
	{
		switch ( TFGameRules()->ItemTesting_GetBotAnim() )
		{
		default:
		case TI_BOTANIM_IDLE:
		case TI_BOTANIM_CROUCH:
		case TI_BOTANIM_JUMP:
			break;

		case TI_BOTANIM_CROUCH_WALK:
		case TI_BOTANIM_RUN:
			{
				QAngle angles( 0, 0, 0 );
				angles[YAW] = m_angRender[YAW];
				Vector vForward, vRight, vUp;
				AngleVectors( angles, &vForward, &vRight, &vUp );
				vel = vForward * GetCurrentMaxGroundSpeed();
			}
			break;
		}
		
		return;
	}
#endif

	BaseClass::GetOuterAbsVelocity( vel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::Update( float eyeYaw, float eyePitch )
{
	// Profile the animation update.
	VPROF( "CMultiPlayerAnimState::Update" );

	// Get the TF player.
	CTFPlayer *pTFPlayer = GetTFPlayer();
	if ( !pTFPlayer )
		return;

	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = pTFPlayer->GetModelPtr();
	if ( !pStudioHdr )
		return;

	if ( pTFPlayer->GetPlayerClass()->HasCustomModel() )
	{
		if ( !pTFPlayer->GetPlayerClass()->CustomModelUsesClassAnimations() )
		{
			if ( pTFPlayer->GetPlayerClass()->CustomModelRotates() )
			{
				if ( pTFPlayer->GetPlayerClass()->CustomModelRotationSet() )
				{
					QAngle angRot = pTFPlayer->GetPlayerClass()->GetCustomModelRotation();
					m_angRender = angRot;
				}
				else
				{
					m_angRender = vec3_angle;
					m_angRender[YAW] = AngleNormalize( eyeYaw );
				}
			}

			// Restart our animation whenever we change models
			if ( pTFPlayer->GetPlayerClass()->CustomModelHasChanged() )
			{
				RestartMainSequence();
			}

			ClearAnimationState();
			return;
		}
	}

	// Check to see if we should be updating the animation state - dead, ragdolled?
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	// Store the eye angles.
	m_flEyeYaw = AngleNormalize( eyeYaw );
	m_flEyePitch = AngleNormalize( eyePitch );

	// Compute the player sequences.
	ComputeSequences( pStudioHdr );

	CTFPlayer *pTauntPartner = pTFPlayer->GetTauntPartner();

	Vector vPositionToFace = ( pTauntPartner ? pTauntPartner->GetAbsOrigin() : vec3_origin );
	bool bInTaunt = pTFPlayer->m_Shared.InCond( TF_COND_TAUNTING );
	bool bInKart = pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART );
	bool bIsImmobilized = bInTaunt || pTFPlayer->m_Shared.IsControlStunned();

	if ( SetupPoseParameters( pStudioHdr ) )
	{
		// check if new item affect player pose params
		CTFWeaponBase *pWeapon = static_cast< CTFWeaponBase* >( m_pPlayer->GetActiveWeapon() );
		if ( m_hActiveWeapon != pWeapon )
		{
			m_hActiveWeapon = pWeapon;

			m_PlayerPoseParams.RemoveAll();
			if ( pWeapon )
			{
				int nPoseParams = 0;
				poseparamtable_t *pPoseParamList = pWeapon->GetPlayerPoseParamList( nPoseParams );
				if ( pPoseParamList )
				{
					m_PlayerPoseParams.EnsureCount( nPoseParams );
					for ( int i=0; i<nPoseParams; ++i )
					{
						m_PlayerPoseParams[i] = CachedPoseParam_t( GetBasePlayer()->LookupPoseParameter( pStudioHdr, pPoseParamList[i].strName ), pPoseParamList[i].flValue );
					}
				}
			}
		}

		for ( int i=0; i<m_PlayerPoseParams.Count(); ++i )
		{
			m_pTFPlayer->SetPoseParameter( pStudioHdr, m_PlayerPoseParams[i].first, m_PlayerPoseParams[i].second );
		}

		if ( !bIsImmobilized )
		{
			// Pose parameter - what direction are the player's legs running in.
			ComputePoseParam_MoveYaw( pStudioHdr );
		}

		if ( bInTaunt )
		{
			// If you are forcing aim yaw, your code is almost definitely broken if you don't include a delay between 
			// teleporting and forcing yaw. This is due to an unfortunate interaction between the command lookback window,
			// and the fact that m_flEyeYaw is never propogated from the server to the client.
			// TODO: Fix this after Halloween 2014.
			m_bForceAimYaw = true;
			m_flEyeYaw = pTFPlayer->GetTauntYaw();

			Taunt_ComputePoseParam_MoveX( pStudioHdr );
			Taunt_ComputePoseParam_MoveY( pStudioHdr );
		}
		else if ( bInKart )
		{
			// If you are forcing aim yaw, your code is almost definitely broken if you don't include a delay between 
			// teleporting and forcing yaw. This is due to an unfortunate interaction between the command lookback window,
			// and the fact that m_flEyeYaw is never propogated from the server to the client.
			// TODO: Fix this after Halloween 2014.
			m_bForceAimYaw = true; // This makes it so our "legs" dont lag behind our eyes when standing still.
			Vehicle_ComputePoseParam_MoveYaw( pStudioHdr );
			Vehicle_ComputePoseParam_AccelLean( pStudioHdr );

			// Trace down a bit for the ground
			trace_t tr;
			//UTIL_TraceLine( pTFPlayer->GetAbsOrigin(), pTFPlayer->GetAbsOrigin() - Vector(0,0,20), MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
			UTIL_TraceLine( pTFPlayer->GetAbsOrigin(), pTFPlayer->GetAbsOrigin() - Vector(0,0,64), MASK_SOLID, pTFPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

			// Use the ground normal if we hit, else abs up
			Vector vSurfaceNormal = tr.DidHit() ? tr.plane.normal : Vector( 0.f, 0.f, 1.f );

			// Have smoothed up approach the surface normal
			m_vecSmoothedUp[ 0 ] = Approach( vSurfaceNormal[ 0 ], m_vecSmoothedUp[ 0 ], 0.2f * gpGlobals->frametime );
			m_vecSmoothedUp[ 1 ] = Approach( vSurfaceNormal[ 1 ], m_vecSmoothedUp[ 1 ], 0.2f * gpGlobals->frametime );
			m_vecSmoothedUp[ 2 ] = Approach( vSurfaceNormal[ 2 ], m_vecSmoothedUp[ 2 ], 0.2f * gpGlobals->frametime );

			// Get player's forward
			Vector vOldForward;
			QAngle vTauntAngles = pTFPlayer->GetAbsAngles();
			vTauntAngles[ YAW ] = pTFPlayer->GetTauntYaw();
			AngleVectors( vTauntAngles, &vOldForward, NULL, NULL );

			// Construct basis
			Vector vRight = vOldForward.Cross( m_vecSmoothedUp );
			Vector vForward = m_vecSmoothedUp.Cross( vRight );
			// Set angles
			VectorAngles( vForward, m_vecSmoothedUp, m_angRender );

#if 0
			if ( tr.DidHit() )
			{
#ifdef GAME_DLL
				NDebugOverlay::Line( tr.endpos, tr.endpos + tr.plane.normal * 100, 255, 0, 0, true, 1.f );
				NDebugOverlay::Line( pTFPlayer->GetAbsOrigin(), pTFPlayer->GetAbsOrigin() + m_vecSmoothedUp * 100, 255, 0, 0, true, 1.f );
				NDebugOverlay::Line( pTFPlayer->GetAbsOrigin(), pTFPlayer->GetAbsOrigin() + vForward * 100, 255, 0, 0, true, 1.f );
#else
				NDebugOverlay::Line( tr.endpos, tr.endpos + tr.plane.normal * 100, 0, 0, 255, true, 1.f );
				NDebugOverlay::Line( pTFPlayer->GetAbsOrigin(), pTFPlayer->GetAbsOrigin() + m_vecSmoothedUp * 100, 0, 0, 255, true, 1.f );
				NDebugOverlay::Line( pTFPlayer->GetAbsOrigin(), pTFPlayer->GetAbsOrigin() + vForward * 100, 0, 0, 255, true, 1.f );
#endif
			}
#endif
		}
		else if ( TFGameRules()->PlayersAreOnMatchSummaryStage() )
		{
			m_bForceAimYaw = true;
			m_flEyeYaw = pTFPlayer->GetTauntYaw();
		}
		
		if ( !bIsImmobilized || bInTaunt || bInKart )
		{
			// Pose parameter - Torso aiming (up/down).
			ComputePoseParam_AimPitch( pStudioHdr );

			// Pose parameter - Torso aiming (rotation).
			ComputePoseParam_AimYaw( pStudioHdr );
		}
	}

#ifdef CLIENT_DLL 
	if ( C_BasePlayer::ShouldDrawLocalPlayer() )
	{
		GetBasePlayer()->SetPlaybackRate( 1.0f );
	}

	if ( IsItemTestingBot() )
	{
		GetBasePlayer()->SetPlaybackRate( TFGameRules()->ItemTesting_GetBotAnimSpeed() );
	}

#endif
}

//-----------------------------------------------------------------------------
// Updates animation state if we are throwing the passtime ball
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::CheckPasstimeThrowAnimation()
{
	CTFPlayer *pPlayer = GetTFPlayer();
	if ( !pPlayer )
	{
		return;
	}

	// FIXME: there must be a better way of doing this...
	CPasstimeGun *pGun = dynamic_cast< CPasstimeGun * >( pPlayer->GetEntityForLoadoutSlot( LOADOUT_POSITION_UTILITY ) );
	if ( !pGun )
	{
		return;
	}

	if ( pGun->GetCurrentCharge() > 0 ) 
	{
		if ( pPlayer->m_Shared.m_iPasstimeThrowAnimState == PASSTIME_THROW_ANIM_NONE )
		{
			int iSeq = pPlayer->SelectWeightedSequence( ACT_MP_PASSTIME_THROW_BEGIN );
			pPlayer->m_Shared.m_flPasstimeThrowAnimStateTime = gpGlobals->curtime + pPlayer->SequenceDuration( iSeq );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_PASSTIME_THROW_BEGIN );
			pPlayer->m_Shared.m_iPasstimeThrowAnimState = PASSTIME_THROW_ANIM_LOOP;
		}
		else if ( pPlayer->m_Shared.m_iPasstimeThrowAnimState == PASSTIME_THROW_ANIM_LOOP )
		{
			if ( gpGlobals->curtime > pPlayer->m_Shared.m_flPasstimeThrowAnimStateTime )
			{
				int iSeq = pPlayer->SelectWeightedSequence( ACT_MP_PASSTIME_THROW_MIDDLE );
				pPlayer->m_Shared.m_flPasstimeThrowAnimStateTime = gpGlobals->curtime + pPlayer->SequenceDuration( iSeq );
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_PASSTIME_THROW_MIDDLE );
			}
		}
	}
	else // not charging
	{
		 if ( pPlayer->m_Shared.m_iPasstimeThrowAnimState == PASSTIME_THROW_ANIM_LOOP )
		 {
			 pPlayer->DoAnimationEvent( PLAYERANIMEVENT_PASSTIME_THROW_END );
			 int iSeq = pPlayer->SelectWeightedSequence( ACT_MP_PASSTIME_THROW_END );
			 pPlayer->m_Shared.m_flPasstimeThrowAnimStateTime = gpGlobals->curtime + pPlayer->SequenceDuration( iSeq );
			 pPlayer->m_Shared.m_iPasstimeThrowAnimState = PASSTIME_THROW_ANIM_END;
		 }
		 else if ( pPlayer->m_Shared.m_iPasstimeThrowAnimState == PASSTIME_THROW_ANIM_END )
		 {
			 if ( gpGlobals->curtime > pPlayer->m_Shared.m_flPasstimeThrowAnimStateTime )
			 {
				 pPlayer->m_Shared.m_iPasstimeThrowAnimState = PASSTIME_THROW_ANIM_NONE;
			 }
		 }
	}
}



extern bool IsInPrediction();

//-----------------------------------------------------------------------------
// Purpose: Updates animation state if player's looking at CYOAPDA
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::CheckCYOAPDAAnimtion()
{
	CTFPlayer *pPlayer = GetTFPlayer();
	if ( !pPlayer )
		return;

	if ( IsInPrediction() )
		return;

	// do not play anims if in kart
	if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return;

	if ( pPlayer->IsTaunting() )
		return;

	bool isViewingCYOAPDA = pPlayer->IsViewingCYOAPDA();

	CEconItemView *pItem = NULL;

#ifdef GAME_DLL
	if ( pPlayer->Inventory() )
	{
		pItem = pPlayer->GetEquippedItemForLoadoutSlot( LOADOUT_POSITION_ACTION );
	}
#else
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		pItem = TFInventoryManager()->GetItemInLoadoutForClass( pPlayer->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_ACTION, &steamID );
	}
#endif
	item_definition_index_t contractTrackerDefIndex = 5869;
	if ( pItem && pItem->GetItemDefIndex() != contractTrackerDefIndex )
	{
		// If we don't have the contracker equipped, we can't be looking at it.
		// We may have been looking at it and only just now removed it,
		// so we still need to check the animation state and maybe animate out.
		//
		// Old code used to return here which would cause the client to stay
		// locked in the look sequence, but also able to move and shoot.
		isViewingCYOAPDA = false;
	}

	TFCYOAPDAAnimState_t state = pPlayer->m_Shared.m_iCYOAPDAAnimState;
	if ( isViewingCYOAPDA
		 )
	{
		if ( state == CYOA_PDA_ANIM_NONE )
		{
			int iSeq = pPlayer->SelectWeightedSequence( ACT_MP_CYOA_PDA_INTRO );
			pPlayer->m_Shared.m_flCYOAPDAAnimStateTime = gpGlobals->curtime + pPlayer->SequenceDuration( iSeq );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CYOAPDA_BEGIN );
			pPlayer->m_Shared.m_iCYOAPDAAnimState = CYOA_PDA_ANIM_IDLE;
		}
		else if ( state == CYOA_PDA_ANIM_IDLE && gpGlobals->curtime > pPlayer->m_Shared.m_flCYOAPDAAnimStateTime )
		{
			int iSeq = pPlayer->SelectWeightedSequence( ACT_MP_CYOA_PDA_IDLE );
			pPlayer->m_Shared.m_flCYOAPDAAnimStateTime = gpGlobals->curtime + pPlayer->SequenceDuration( iSeq );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CYOAPDA_MIDDLE );
			pPlayer->m_Shared.m_iCYOAPDAAnimState = CYOA_PDA_ANIM_IDLE;
		}
	}
	else
	{
		if ( state == CYOA_PDA_ANIM_IDLE && gpGlobals->curtime > pPlayer->m_Shared.m_flCYOAPDAAnimStateTime )
		{
			int iSeq = pPlayer->SelectWeightedSequence( ACT_MP_CYOA_PDA_OUTRO );
			pPlayer->m_Shared.m_flCYOAPDAAnimStateTime = gpGlobals->curtime + pPlayer->SequenceDuration( iSeq );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CYOAPDA_END );
			pPlayer->m_Shared.m_iCYOAPDAAnimState = CYOA_PDA_ANIM_OUTRO;
		}
		else if ( state == CYOA_PDA_ANIM_OUTRO && gpGlobals->curtime > pPlayer->m_Shared.m_flCYOAPDAAnimStateTime )
		{
			pPlayer->m_Shared.m_iCYOAPDAAnimState = CYOA_PDA_ANIM_NONE;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates animation state if we're stunned.
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::CheckStunAnimation()
{
	CTFPlayer *pPlayer = GetTFPlayer();
	if ( !pPlayer )
		return;

	// do not play stun anims if in kart
	if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return;

	// State machine to determine the correct stun activity.
	if ( !pPlayer->m_Shared.IsControlStunned() && 
		 (pPlayer->m_Shared.m_iStunAnimState == STUN_ANIM_LOOP) )
	{
		// Clean up if the condition went away before we finished.
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_STUN_END );
		pPlayer->m_Shared.m_iStunAnimState = STUN_ANIM_NONE;
	}
	else if ( pPlayer->m_Shared.IsControlStunned() &&
		      (pPlayer->m_Shared.m_iStunAnimState == STUN_ANIM_NONE) &&
		      (gpGlobals->curtime < pPlayer->m_Shared.GetStunExpireTime()) )
	{
		// Play the start up animation.
		int iSeq = pPlayer->SelectWeightedSequence( ACT_MP_STUN_BEGIN );
		pPlayer->m_Shared.m_flStunMid = gpGlobals->curtime + pPlayer->SequenceDuration( iSeq );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_STUN_BEGIN );
		pPlayer->m_Shared.m_iStunAnimState = STUN_ANIM_LOOP;
	}
	else if ( pPlayer->m_Shared.m_iStunAnimState == STUN_ANIM_LOOP )
	{
		// We are playing the looping part of the stun animation cycle.
		if ( gpGlobals->curtime > pPlayer->m_Shared.m_flStunFade )
		{
			// Gameplay is telling us to fade out. Time for the end anim.
			int iSeq = pPlayer->SelectWeightedSequence( ACT_MP_STUN_END );
			pPlayer->m_Shared.SetStunExpireTime( gpGlobals->curtime + pPlayer->SequenceDuration( iSeq ) );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_STUN_END );
			pPlayer->m_Shared.m_iStunAnimState = STUN_ANIM_END;
		}
		else if ( gpGlobals->curtime > pPlayer->m_Shared.m_flStunMid )
		{
			// Loop again.
			int iSeq = pPlayer->SelectWeightedSequence( ACT_MP_STUN_MIDDLE );
			pPlayer->m_Shared.m_flStunMid = gpGlobals->curtime + pPlayer->SequenceDuration( iSeq );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_STUN_MIDDLE );
		}
	}
	else if ( pPlayer->m_Shared.m_iStunAnimState == STUN_ANIM_END )
	{
		if ( gpGlobals->curtime > pPlayer->m_Shared.GetStunExpireTime() )
		{
			// The animation loop is over.
			pPlayer->m_Shared.m_iStunAnimState = STUN_ANIM_NONE;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CTFPlayerAnimState::CalcMainActivity()
{
	CheckStunAnimation();
	CheckPasstimeThrowAnimation();
	CheckCYOAPDAAnimtion();

#ifdef CLIENT_DLL
	bool bIsAiming = m_pTFPlayer->m_Shared.IsAiming();

	if ( IsItemTestingBot() )
	{
		switch ( TFGameRules()->ItemTesting_GetBotAnim() )
		{
		default:
		case TI_BOTANIM_JUMP:
			break;

		case TI_BOTANIM_IDLE:
			if ( bIsAiming ) 
				return ACT_MP_DEPLOYED_IDLE;
			return ACT_MP_STAND_IDLE;

		case TI_BOTANIM_CROUCH:
			if ( bIsAiming ) 
				return ACT_MP_CROUCH_DEPLOYED_IDLE;
			return ACT_MP_CROUCH_IDLE;

		case TI_BOTANIM_CROUCH_WALK:
			if ( bIsAiming ) 
				return ACT_MP_CROUCH_DEPLOYED;
			return ACT_MP_CROUCHWALK;

		case TI_BOTANIM_RUN:
			if ( bIsAiming ) 
				return ACT_MP_DEPLOYED;
			return ACT_MP_RUN;
		}
	}
#endif

	return BaseClass::CalcMainActivity();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr )
{
	if ( IsItemTestingBot() )
	{
		if ( TFGameRules()->ItemTesting_GetBotViewScan() )
		{
			static float flDeltaYaw = 0.4f;
			static float flCurrentYaw = 0.0f;
			static float flDeltaPitch = 0.4f;
			static float flCurrentPitch = 0.0f;

			// Pan left & right
			flCurrentYaw = flCurrentYaw + ( flDeltaYaw * TFGameRules()->ItemTesting_GetBotAnimSpeed() );
			if ( fabs(flCurrentYaw) >= 45 )
			{
				flDeltaYaw *= -1;
			}
			flCurrentYaw = AngleNormalize( flCurrentYaw );
			GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimYaw, -flCurrentYaw );

			// Pan up & down
			flCurrentPitch = AngleNormalize( flCurrentPitch + ( flDeltaPitch * TFGameRules()->ItemTesting_GetBotAnimSpeed() ) );
			if ( fabs(flCurrentPitch) >= 150 )
			{
				flDeltaPitch *= -1;
			}
			flCurrentPitch = AngleNormalize( flCurrentPitch );
			flCurrentPitch = clamp(flCurrentPitch, -45.f, 90.f );
			GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimPitch, -flCurrentPitch );

			return;
		}

		// Rotating on the spot?
		if ( TFGameRules()->ItemTesting_GetBotTurntable() )
		{
			m_flGoalFeetYaw = m_flEyeYaw;
			m_flCurrentFeetYaw = m_flGoalFeetYaw;
			m_angRender[YAW] = m_flCurrentFeetYaw;
			float flAimYaw = m_flEyeYaw - m_flCurrentFeetYaw;
			flAimYaw = AngleNormalize( flAimYaw );
			GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimYaw, -flAimYaw );
			return;
		}
	}

	BaseClass::ComputePoseParam_AimYaw( pStudioHdr );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::Taunt_ComputePoseParam_MoveX( CStudioHdr *pStudioHdr )
{
	CTFPlayer *pTFPlayer = GetTFPlayer();
	if ( pTFPlayer->IsTaunting() && pTFPlayer->CanMoveDuringTaunt() )
	{
		int iMove = 0;
		iMove += pTFPlayer->m_nButtons & IN_FORWARD ? 1 : 0;
		iMove += pTFPlayer->m_nButtons & IN_BACK ? -1 : 0;

		float fl_move_x = 1.f;
		if ( pTFPlayer->GetTauntMoveAcceleration() > 0.f )
		{
			fl_move_x = Sign( iMove ) * ( gpGlobals->frametime / pTFPlayer->GetTauntMoveAcceleration() );
		}

		// turning?
		if ( iMove != 0.f )
		{
			m_flTauntMoveX = clamp( m_flTauntMoveX + fl_move_x, -1.f, 1.f );
		}
		else if ( m_flTauntMoveX != 0.f )
		{
			// smooth the value back to 0
			if ( m_flTauntMoveX < 0.f )
			{
				m_flTauntMoveX = clamp( m_flTauntMoveX + fabs( fl_move_x ), -1.f, 0.f );
			}
			if ( m_flTauntMoveX > 0.f )
			{
				m_flTauntMoveX = clamp( m_flTauntMoveX - fabs( fl_move_x ), 0.f, 1.f );
			}
		}
	}
	else
	{
		m_flTauntMoveX = 0.f;
	}

	pTFPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, Sign( m_flTauntMoveX ) * SimpleSpline( fabs( m_flTauntMoveX ) ) );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::Taunt_ComputePoseParam_MoveY( CStudioHdr *pStudioHdr )
{
	CTFPlayer *pTFPlayer = GetTFPlayer();
	if ( pTFPlayer->IsTaunting() && pTFPlayer->CanMoveDuringTaunt() )
	{
		float flTauntYawDiff = pTFPlayer->GetTauntYaw() - pTFPlayer->GetPrevTauntYaw();
		float fl_move_y = 1.f;
		if ( pTFPlayer->GetTauntTurnAccelerationTime() > 0.f )
		{
			fl_move_y = Sign( flTauntYawDiff ) * ( gpGlobals->frametime / pTFPlayer->GetTauntTurnAccelerationTime() );
		}

		// turning?
		if ( flTauntYawDiff != 0.f )
		{
			m_flTauntMoveY = clamp( m_flTauntMoveY + fl_move_y, -1.f, 1.f );
		}
		else if ( m_flTauntMoveY != 0.f )
		{
			// smooth the value back to 0
			if ( m_flTauntMoveY < 0.f )
			{
				m_flTauntMoveY = clamp( m_flTauntMoveY + fabs( fl_move_y ), -1.f, 0.f );
			}
			if ( m_flTauntMoveY > 0.f )
			{
				m_flTauntMoveY = clamp( m_flTauntMoveY - fabs( fl_move_y ), 0.f, 1.f );
			}
		}
	}
	else
	{
		m_flTauntMoveY = 0.f;
	}
	pTFPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, Sign( m_flTauntMoveY ) * SimpleSpline( fabs( m_flTauntMoveY ) ) );
}

extern ConVar tf_halloween_kart_slow_turn_accel_speed;
void CTFPlayerAnimState::Vehicle_ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr )
{
	float flValue = -m_pTFPlayer->m_Shared.GetVehicleTurnPoseAmount() / tf_halloween_kart_slow_turn_accel_speed.GetFloat();
	if ( m_pTFPlayer->GetTauntMoveSpeed() < 0.f )
	{
		flValue = -flValue;
	}

	flValue *= 0.5f;

#ifdef DEBUG
	#ifdef CLIENT_DLL
		engine->Con_NPrintf( 10, "CLIENT Pose: %3.2f", flValue );
	#else
		engine->Con_NPrintf( 11, "SERVER Pose: %3.2f", flValue );
	#endif
#endif

	m_pTFPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, flValue );
}

extern ConVar tf_halloween_kart_dash_speed;
extern ConVar tf_halloween_kart_brake_speed;

void CTFPlayerAnimState::Vehicle_ComputePoseParam_AccelLean( CStudioHdr *pStudioHdr )
{
	m_pTFPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, m_flVehicleLeanPos );
}

void CTFPlayerAnimState::Vehicle_LeanAccel( float flInAccel )
{
	// Accelerate our lean vel
	float flDiff = flInAccel - m_flVehicleLeanPos;
	float flAccel = 0.1f * flDiff - 1.5f * m_flVehicleLeanVel;
	m_flVehicleLeanVel += flAccel * gpGlobals->frametime;

	// Move our lean pos by our lean vel
	m_flVehicleLeanPos += m_flVehicleLeanVel * gpGlobals->frametime;
	// Decay it a bit
	m_flVehicleLeanPos -= m_flVehicleLeanPos * 0.1f;
	m_flVehicleLeanPos = clamp( m_flVehicleLeanPos, -1.f, 1.f );

#ifdef DEBUG
	#ifdef CLIENT_DLL
		engine->Con_NPrintf( 16, "CLIENT Acc: %.2f Vel: %.2f Pose: %.2f", flAccel, m_flVehicleLeanVel, m_flVehicleLeanPos );
	#else
		engine->Con_NPrintf( 17, "SERVER Acc: %.2f Vel: %.2f Pose: %.2f", flAccel, m_flVehicleLeanVel, m_flVehicleLeanPos );
	#endif
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::RestartGesture( int iGestureSlot, Activity iGestureActivity, bool bAutoKill )
{
	Activity translatedActivity = TranslateActivity( iGestureActivity );
	
	BaseClass::RestartGesture( iGestureSlot, translatedActivity, bAutoKill );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	bool bInDuck = ( m_pTFPlayer->GetFlags() & FL_DUCKING ) ? true : false;
	if ( bInDuck && SelectWeightedSequence( TranslateActivity( ACT_MP_CROUCHWALK ) ) < 0 )
	{
		bInDuck = false;
	}

	Activity iGestureActivity = ACT_INVALID;

	switch( event )
	{
	case PLAYERANIMEVENT_ATTACK_PRIMARY:
		{
			CTFPlayer *pPlayer = GetTFPlayer();
			if ( !pPlayer )
				return;

			CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
			bool bIsMinigun = ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_MINIGUN );
			bool bIsSniperRifle = ( pWpn && WeaponID_IsSniperRifleOrBow( pWpn->GetWeaponID() ) );

			// Heavy weapons primary fire.
			if ( bIsMinigun )
			{
				// Play standing primary fire.
				iGestureActivity = ACT_MP_ATTACK_STAND_PRIMARYFIRE;

				if ( m_bInSwim )
				{
					// Play swimming primary fire.
					iGestureActivity = ACT_MP_ATTACK_SWIM_PRIMARYFIRE;
				}
				else if ( bInDuck )
				{
					// Play crouching primary fire.
					iGestureActivity = ACT_MP_ATTACK_CROUCH_PRIMARYFIRE;
				}

				if ( !IsGestureSlotPlaying( GESTURE_SLOT_ATTACK_AND_RELOAD, TranslateActivity(iGestureActivity) ) )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity );
				}
			}
			else if ( bIsSniperRifle && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
			{
				// Weapon primary fire, zoomed in
				if ( bInDuck )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED );
				}
				else
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED );
				}

				iGestureActivity = ACT_VM_PRIMARYATTACK;

				// Hold our deployed pose for a few seconds
				m_flHoldDeployedPoseUntilTime = gpGlobals->curtime + 2.0;
			}
			else
			{
				Activity baseActivity = bInDuck ? ACT_MP_ATTACK_CROUCH_PRIMARYFIRE : ACT_MP_ATTACK_STAND_PRIMARYFIRE;

				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, baseActivity );
//				iGestureActivity = ACT_VM_PRIMARYATTACK;
			}

			break;
		}

	case PLAYERANIMEVENT_ATTACK_PRIMARY_SUPER:
		{
			Activity baseActivity = bInDuck ? ACT_MP_ATTACK_CROUCH_PRIMARY_SUPER : ACT_MP_ATTACK_STAND_PRIMARY_SUPER;

			if ( m_bInSwim )
				baseActivity = ACT_MP_ATTACK_SWIM_PRIMARY_SUPER;

			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, baseActivity );
//			iGestureActivity = ACT_VM_PRIMARYATTACK;
		}
		break;

	case PLAYERANIMEVENT_VOICE_COMMAND_GESTURE:
		{
			if ( !IsGestureSlotActive( GESTURE_SLOT_ATTACK_AND_RELOAD ) )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, (Activity)nData );
			}
			break;
		}
	case PLAYERANIMEVENT_ATTACK_SECONDARY:
		{
			Activity baseActivity = bInDuck ? ACT_MP_ATTACK_CROUCH_SECONDARYFIRE : ACT_MP_ATTACK_STAND_SECONDARYFIRE;
			if ( GetBasePlayer()->GetWaterLevel() >= WL_Waist )
			{
				baseActivity = ACT_MP_ATTACK_SWIM_SECONDARYFIRE;
			}
			
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, baseActivity );
			iGestureActivity = ACT_VM_SECONDARYATTACK;
			break;
		}
	case PLAYERANIMEVENT_ATTACK_PRE:
		{
			CTFPlayer *pPlayer = GetTFPlayer();
			if ( !pPlayer )
				return;

			CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
			bool bIsMinigun  = ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_MINIGUN );

			bool bAutoKillPreFire = false;
			if ( bIsMinigun )
			{
				bAutoKillPreFire = true;
			}

			if ( m_bInSwim && bIsMinigun )
			{
				// Weapon pre-fire. Used for minigun windup while swimming
				iGestureActivity = ACT_MP_ATTACK_SWIM_PREFIRE;
			}
			else if ( bInDuck ) 
			{
				// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc in crouch.
				iGestureActivity = ACT_MP_ATTACK_CROUCH_PREFIRE;
			}
			else
			{
				// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc.
				iGestureActivity = ACT_MP_ATTACK_STAND_PREFIRE;
			}

			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity, bAutoKillPreFire );

			break;
		}
	case PLAYERANIMEVENT_ATTACK_POST:
		{
			CTFPlayer *pPlayer = GetTFPlayer();
			if ( !pPlayer )
				return;

			CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
			bool bIsMinigun  = ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_MINIGUN );

			if ( m_bInSwim && bIsMinigun )
			{
				// Weapon pre-fire. Used for minigun winddown while swimming
				iGestureActivity = ACT_MP_ATTACK_SWIM_POSTFIRE;
			}
			else if ( bInDuck ) 
			{
				// Weapon post-fire. Used for minigun winddown in crouch.
				iGestureActivity = ACT_MP_ATTACK_CROUCH_POSTFIRE;
			}
			else
			{
				// Weapon post-fire. Used for minigun winddown.
				iGestureActivity = ACT_MP_ATTACK_STAND_POSTFIRE;
			}

			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity );

			break;
		}

	case PLAYERANIMEVENT_RELOAD:
		{
			// Weapon reload.
			if ( m_bInAirWalk )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_AIRWALK );
			}
			else
			{
				BaseClass::DoAnimationEvent( event, nData );
			}
			break;
		}
	case PLAYERANIMEVENT_RELOAD_LOOP:
		{
			// Weapon reload.
			if ( m_bInAirWalk )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_AIRWALK_LOOP );
			}
			else
			{
				BaseClass::DoAnimationEvent( event, nData );
			}
			break;
		}
	case PLAYERANIMEVENT_RELOAD_END:
		{
			// Weapon reload.
			if ( m_bInAirWalk )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_AIRWALK_END );
			}
			else
			{
				BaseClass::DoAnimationEvent( event, nData );
			}
			break;
		}
	case PLAYERANIMEVENT_DOUBLEJUMP:
		{
			CTFPlayer *pPlayer = GetTFPlayer();
			if ( !pPlayer )
				return;

			// Check to see if we are jumping!
			if ( !m_bJumping )
			{
				m_bJumping = true;
				m_bFirstJumpFrame = true;
				m_flJumpStartTime = gpGlobals->curtime;
				RestartMainSequence();
			}

			// Force the air walk off.
			m_bInAirWalk = false;

			// Player the air dash gesture.
			if ( pPlayer->m_Shared.IsLoser() )
			{
				RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_DOUBLEJUMP_LOSERSTATE );
			}
			else
			{
				RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_DOUBLEJUMP );
			}
			break;
		}
	case PLAYERANIMEVENT_DOUBLEJUMP_CROUCH:
//		RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_DOUBLEJUMP_CROUCH );
//		m_aGestureSlots[GESTURE_SLOT_JUMP].m_pAnimLayer->m_flBlendIn = 0.4f;
//		m_aGestureSlots[GESTURE_SLOT_JUMP].m_pAnimLayer->m_flBlendOut = 0.4f;
#ifdef CLIENT_DLL
//		m_aGestureSlots[GESTURE_SLOT_JUMP].m_pAnimLayer->m_bClientBlend = true;
#endif
		break;
	case PLAYERANIMEVENT_STUN_BEGIN:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_STUN_BEGIN, false );
		break;
	case PLAYERANIMEVENT_STUN_MIDDLE:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_STUN_MIDDLE, false );
		break;
	case PLAYERANIMEVENT_STUN_END:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_STUN_END );
		break;
	case PLAYERANIMEVENT_PASSTIME_THROW_BEGIN:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_PASSTIME_THROW_BEGIN, false );
		break;
	case PLAYERANIMEVENT_PASSTIME_THROW_MIDDLE:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_PASSTIME_THROW_MIDDLE, false );
		break;
	case PLAYERANIMEVENT_PASSTIME_THROW_END:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_PASSTIME_THROW_END );
		break;
	case PLAYERANIMEVENT_CYOAPDA_BEGIN:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_CYOA_PDA_INTRO, false );
		break;
	case PLAYERANIMEVENT_CYOAPDA_MIDDLE:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_CYOA_PDA_IDLE, false );
		break;
	case PLAYERANIMEVENT_CYOAPDA_END:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_CYOA_PDA_OUTRO );
		break;
	default:
		{
			BaseClass::DoAnimationEvent( event, nData );
			break;
		}
	}

#ifdef CLIENT_DLL
	// Make the weapon play the animation as well
	if ( iGestureActivity != ACT_INVALID )
	{
		CBaseCombatWeapon *pWeapon = GetTFPlayer()->GetActiveWeapon();
		if ( pWeapon )
		{
			pWeapon->SendWeaponAnim( iGestureActivity );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::HandleSwimming( Activity &idealActivity )
{
	bool bInWater = BaseClass::HandleSwimming( idealActivity );

	if ( bInWater )
	{
		if ( m_pTFPlayer->m_Shared.IsAiming() )
		{
			CTFWeaponBase *pWpn = m_pTFPlayer->GetActiveTFWeapon();
			if ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_MINIGUN )
			{
				idealActivity = ACT_MP_SWIM_DEPLOYED;
			}
			// Check for sniper deployed underwater - should only be when standing on something
			else if ( pWpn && WeaponID_IsSniperRifle( pWpn->GetWeaponID() ) )
			{
				if ( m_pTFPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
				{
					idealActivity = ACT_MP_SWIM_DEPLOYED;
				}
			}
		}
	}

	return bInWater;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::HandleMoving( Activity &idealActivity )
{
	float flSpeed = GetOuterXYSpeed();

	// If we move, cancel the deployed anim hold
	if ( flSpeed > MOVING_MINIMUM_SPEED )
	{
		m_flHoldDeployedPoseUntilTime = 0.0;
	}

	if ( m_pTFPlayer->m_Shared.IsLoser() )
	{
		return BaseClass::HandleMoving( idealActivity );
	}

	if ( m_pTFPlayer->m_Shared.IsAiming() ) 
	{
		if ( flSpeed > MOVING_MINIMUM_SPEED )
		{
			idealActivity = ACT_MP_DEPLOYED;
		}
		else
		{
			idealActivity = ACT_MP_DEPLOYED_IDLE;
		}
	}
	else if ( m_flHoldDeployedPoseUntilTime > gpGlobals->curtime )
	{
		// Unless we move, hold the deployed pose for a number of seconds after being deployed
		idealActivity = ACT_MP_DEPLOYED_IDLE;
	}
	else 
	{
		return BaseClass::HandleMoving( idealActivity );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::HandleDucking( Activity &idealActivity )
{
	bool bInDuck = ( m_pTFPlayer->GetFlags() & FL_DUCKING ) ? true : false;
	if ( bInDuck && SelectWeightedSequence( TranslateActivity( ACT_MP_CROUCHWALK ) ) < 0 && !m_pTFPlayer->m_Shared.IsLoser() )
	{
		bInDuck = false;
	}

	if ( bInDuck )
	{
		if ( GetOuterXYSpeed() < MOVING_MINIMUM_SPEED || m_pTFPlayer->m_Shared.IsLoser() )
		{
			idealActivity = ACT_MP_CROUCH_IDLE;		
			if ( m_pTFPlayer->m_Shared.IsAiming() || m_flHoldDeployedPoseUntilTime > gpGlobals->curtime )
			{
				idealActivity = ACT_MP_CROUCH_DEPLOYED_IDLE;
			}
		}
		else
		{
			if ( m_pTFPlayer->m_Shared.GetAirDash() > 0 )
			{
				idealActivity = ACT_MP_DOUBLEJUMP_CROUCH;
			}
			else
			{
				idealActivity = ACT_MP_CROUCHWALK;		
			}

			if ( m_pTFPlayer->m_Shared.IsAiming() )
			{
				// Don't do this for the heavy! we don't usually let him deployed crouch walk
				bool bIsMinigun = false;

				CTFPlayer *pPlayer = GetTFPlayer();
				if ( pPlayer && pPlayer->GetActiveTFWeapon() )
				{
					bIsMinigun = ( pPlayer->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_MINIGUN );
				}

				if ( !bIsMinigun )
				{
					idealActivity = ACT_MP_CROUCH_DEPLOYED;
				}
			}
		}

		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerAnimState::GetCurrentMaxGroundSpeed()
{
	float flSpeed = BaseClass::GetCurrentMaxGroundSpeed();

	if ( m_pTFPlayer->m_Shared.GetAirDash() > 0 )
	{
		return 1.f;
	}
	else
	{
		return flSpeed;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerAnimState::GetGesturePlaybackRate( void )
{
	if ( IsItemTestingBot() )
		return TFGameRules()->ItemTesting_GetBotAnimSpeed();

	float flPlaybackRate = 1.f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pTFPlayer, flPlaybackRate, mult_gesture_time );
	return flPlaybackRate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::HandleJumping( Activity &idealActivity )
{
	bool bInDuck = ( m_pTFPlayer->GetFlags() & FL_DUCKING ) ? true : false;
	if ( bInDuck && SelectWeightedSequence( TranslateActivity( ACT_MP_CROUCHWALK ) ) < 0 )
	{
		bInDuck = false;
	}

	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	// Don't allow a firing heavy to jump or air walk.
	if ( m_pTFPlayer->GetPlayerClass()->IsClass( TF_CLASS_HEAVYWEAPONS ) && m_pTFPlayer->m_Shared.InCond( TF_COND_AIMING ) )
		return false;
		
	// Handle air walking before handling jumping - air walking supersedes jump
	TFPlayerClassData_t *pData = m_pTFPlayer->GetPlayerClass()->GetData();
	bool bValidAirWalkClass = ( pData && pData->m_bDontDoAirwalk == false );

	if ( bValidAirWalkClass && ( vecVelocity.z > 300.0f || m_bInAirWalk || m_pTFPlayer->GetGrapplingHookTarget() != NULL ) && !bInDuck )
	{
		// Check to see if we were in an airwalk and now we are basically on the ground.
		if ( ( GetBasePlayer()->GetFlags() & FL_ONGROUND ) && m_bInAirWalk )
		{				
			m_bInAirWalk = false;
			RestartMainSequence();
			RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );	
		}
		else if ( GetBasePlayer()->GetWaterLevel() >= WL_Waist )
		{
			// Turn off air walking and reset the animation.
			m_bInAirWalk = false;
			RestartMainSequence();
		}
		else if ( ( GetBasePlayer()->GetFlags() & FL_ONGROUND ) == 0 )
		{
			// In an air walk.
			if ( m_pTFPlayer->m_Local.m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED && m_pTFPlayer->m_Shared.CanFallStomp() )
			{
				idealActivity = ACT_MP_FALLING_STOMP;
			}
			else
			{
				idealActivity = ACT_MP_AIRWALK;
			}
			m_bInAirWalk = true;
		}
	}
	// Jumping.
	else
	{
		if ( m_bJumping )
		{
			// Remove me once all classes are doing the new jump
			TFPlayerClassData_t *pDataJump = m_pTFPlayer->GetPlayerClass()->GetData();
			bool bNewJump = (pDataJump && pDataJump->m_bDontDoNewJump == false );

			if ( m_bFirstJumpFrame )
			{
				m_bFirstJumpFrame = false;
				RestartMainSequence();	// Reset the animation.
			}

			// Reset if we hit water and start swimming.
			if ( GetBasePlayer()->GetWaterLevel() >= WL_Waist )
			{
				m_bJumping = false;
				RestartMainSequence();
			}
			// Don't check if he's on the ground for a sec.. sometimes the client still has the
			// on-ground flag set right when the message comes in.
			else if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
			{
				if ( GetBasePlayer()->GetFlags() & FL_ONGROUND )
				{
					m_bJumping = false;
					RestartMainSequence();

					if ( bNewJump )
					{
						RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );					
					}
				}
			}

			// if we're still jumping
			if ( m_bJumping )
			{
				if ( bNewJump )
				{
					if ( gpGlobals->curtime - m_flJumpStartTime > 0.5 )
					{
						idealActivity = ACT_MP_JUMP_FLOAT;
					}
					else
					{
						idealActivity = ACT_MP_JUMP_START;
					}
				}
				else
				{
					idealActivity = ACT_MP_JUMP;
				}
			}
		}	
	}

	if ( m_bJumping || m_bInAirWalk )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::IsItemTestingBot( void )
{
	if ( TFGameRules()->IsInItemTestingMode() )
	{
		// Clients don't know what's a bot. Assume the first player is the non-bat.
		return ( m_pTFPlayer->entindex() > 1 );
	}
	return false;
}
