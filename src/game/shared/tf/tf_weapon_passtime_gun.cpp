//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_weapon_passtime_gun.h"
#include "passtime_convars.h"
#include "in_buttons.h"
#include "tf_gamerules.h"
#ifdef GAME_DLL
#include "tf_passtime_ball.h"
#include "tf_passtime_logic.h"
#include "tf_player.h"
#include "tf_playerclass.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#else // !GAME_DLL
#include "c_tf_passtime_logic.h"
#include "c_tf_passtime_ball.h"
#include "tf_hud_passtime_reticle.h"
#include "tf_viewmodel.h"
#include "c_tf_player.h"
#include "prediction.h"
#endif
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( PasstimeGun, DT_PasstimeGun )

//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE( CPasstimeGun, DT_PasstimeGun )
#ifdef GAME_DLL
	SendPropInt( SENDINFO( m_eThrowState ) ),
	SendPropFloat( SENDINFO( m_fChargeBeginTime ) )
#else
	RecvPropInt( RECVINFO( m_eThrowState ) ),
	RecvPropFloat( RECVINFO( m_fChargeBeginTime ) )
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
BEGIN_PREDICTION_DATA( CPasstimeGun )
END_PREDICTION_DATA() // this has to be here because the client's precache code uses it to get the classname of this entity...

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_weapon_passtime_gun, CPasstimeGun );
PRECACHE_WEAPON_REGISTER( tf_weapon_passtime_gun );

//-----------------------------------------------------------------------------
namespace
{
	static char const * const kChargeSound = "Passtime.GunCharge";
	static char const * const kTargetHightlightSound = "Passtime.TargetLock";
	static char const * const kShootOkSound = "Passtime.Throw";
	static char const * const kPassOkSound = "Passtime.Throw";
	static char const * const kHalloweenBallModel = "models/passtime/ball/passtime_ball_halloween.mdl";
}

//-----------------------------------------------------------------------------
CPasstimeGun::CPasstimeGun()
	: m_flTargetResetTime( 0 )
	, m_attack( IN_ATTACK )
	, m_attack2( IN_ATTACK2 )
{
	m_eThrowState = THROWSTATE_DISABLED;
#ifdef CLIENT_DLL
	m_pBounceReticle = 0;
#endif
}

//-----------------------------------------------------------------------------
void CPasstimeGun::Spawn()
{
	m_iClip1 = -1;
	m_flTargetResetTime = 0;
	BaseClass::Spawn();

#ifdef CLIENT_DLL
	SetNextClientThink( CLIENT_THINK_ALWAYS );
	if( !m_pBounceReticle )
		m_pBounceReticle = new C_PasstimeBounceReticle();
#endif
}

//-----------------------------------------------------------------------------
CPasstimeGun::~CPasstimeGun()
{
#ifdef CLIENT_DLL
	delete m_pBounceReticle;
#endif
}

//-----------------------------------------------------------------------------
void CPasstimeGun::Equip( CBaseCombatCharacter *pOwner )
{
	// NOTE: This is not called on the client.

	// IsMarkedForDeletion can happen if the gun deletes itself in Spawn
	if ( IsMarkedForDeletion() )
	{
		return;
	}

	BaseClass::Equip( pOwner );
}

//-----------------------------------------------------------------------------
void CPasstimeGun::Precache()
{
	PrecacheScriptSound( kTargetHightlightSound );
	PrecacheScriptSound( kShootOkSound );
	PrecacheScriptSound( kPassOkSound );
	PrecacheScriptSound( kChargeSound );
	m_iAttachmentIndex = PrecacheModel( tf_passtime_ball_model.GetString() );
	if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
	{
		m_iHalloweenAttachmentIndex = PrecacheModel( kHalloweenBallModel );
	}
	else
	{
		m_iHalloweenAttachmentIndex = -1;
	}

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
bool CPasstimeGun::CanHolster() const
{
	return !GetTFPlayerOwner()->m_Shared.HasPasstimeBall();
}

//-----------------------------------------------------------------------------
bool CPasstimeGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	// WeaponReset will always be called too
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
void CPasstimeGun::WeaponReset()
{
	// this can happen when the weapon is holstered or not
	BaseClass::WeaponReset();

	if ( (m_eThrowState != THROWSTATE_DISABLED) && (m_eThrowState != THROWSTATE_IDLE) )
	{
		m_eThrowState = THROWSTATE_CANCELLED;
		m_attack2.LatchUp();
		m_attack.LatchUp();
	}

#ifdef CLIENT_DLL
	if ( m_pBounceReticle )
		m_pBounceReticle->Hide();
#endif

	CTFPlayer* pOwner = GetTFPlayerOwner();
	if ( pOwner )
		pOwner->m_Shared.SetPasstimePassTarget( 0 );

}

//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CPasstimeGun::UpdateAttachmentModels()
{
	BaseClass::UpdateAttachmentModels();

	auto *pTFPlayer = GetTFPlayerOwner();
	if ( !pTFPlayer )
		return;

	if ( !pTFPlayer->IsLocalPlayer() )
		return;

	if ( !pTFPlayer->GetViewModel() )
		return;

	auto *pViewmodelBall = GetViewmodelAttachment();
	if ( !pViewmodelBall )
		return;

	auto iActiveIndex = pViewmodelBall->GetModelIndex();
	if ( m_iHalloweenAttachmentIndex != -1 ) 
	{
		if ( iActiveIndex != m_iHalloweenAttachmentIndex )
		{
			pViewmodelBall->SetModelIndex( m_iHalloweenAttachmentIndex );
			m_bAttachmentDirty = true;
		}
	}
	else if ( iActiveIndex != m_iAttachmentIndex )
	{
		pViewmodelBall->SetModelIndex( m_iAttachmentIndex );
		m_bAttachmentDirty = true;
	}
}
#endif

//-----------------------------------------------------------------------------
bool CPasstimeGun::CanCharge() // const
{
	return tf_passtime_experiment_instapass.GetBool()
		&& tf_passtime_experiment_instapass_charge.GetBool();
}

//-----------------------------------------------------------------------------
float CPasstimeGun::GetChargeBeginTime() 
{
	return m_fChargeBeginTime;
}

//-----------------------------------------------------------------------------
float CPasstimeGun::GetChargeMaxTime() 
{ 
	return (tf_passtime_experiment_instapass.GetBool() && tf_passtime_experiment_instapass_charge.GetBool())
		? 3.0f
		: 0.0f;
}

//-----------------------------------------------------------------------------
float CPasstimeGun::GetCurrentCharge()
{ 
	if ( (m_eThrowState == THROWSTATE_CHARGING) || (m_eThrowState == THROWSTATE_CHARGED) )
		return clamp((gpGlobals->curtime - GetChargeBeginTime()) / GetChargeMaxTime(), 0.0f, 1.0f);
	return 0;
}



//-----------------------------------------------------------------------------
void CPasstimeGun::UpdateOnRemove()
{
#ifdef CLIENT_DLL
	delete m_pBounceReticle;
	m_pBounceReticle = 0;
#endif
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
bool CPasstimeGun::VisibleInWeaponSelection() 
{
	return false;
}

static acttable_t s_acttablePasstime[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_PASSTIME,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_PASSTIME,				false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_PASSTIME,		false },

	// the previous are the only actual unique ones
	
	// following is a copy from tf_weaponbase.cpp
	//acttable_t s_acttableMeleeAllclass[] = 
	//{
		//{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_MELEE_ALLCLASS,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_MELEE_ALLCLASS,			false },
		//{ ACT_MP_RUN,				ACT_MP_RUN_MELEE_ALLCLASS,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_MELEE_ALLCLASS,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_MELEE_ALLCLASS,			false },
		//{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_MELEE_ALLCLASS,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_MELEE_ALLCLASS,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_MELEE_ALLCLASS,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_MELEE_ALLCLASS,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_MELEE_ALLCLASS,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_MELEE_ALLCLASS,				false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_MELEE,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_MELEE_ALLCLASS,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_MELEE_ALLCLASS,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS,	false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_ATTACK_STAND_MELEE_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY,false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_MELEE_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_MELEE_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_MELEE_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_MELEE_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_MELEE_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_MELEE_GRENADE2_ATTACK,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_MELEE,	false },
};


//-----------------------------------------------------------------------------
acttable_t* CPasstimeGun::ActivityList(int &iActivityCount)
{
	iActivityCount = ARRAYSIZE(s_acttablePasstime);
	return GetTFPlayerOwner() 
		? s_acttablePasstime 
		: BaseClass::ActivityList(iActivityCount);
}

//-----------------------------------------------------------------------------
void CPasstimeGun::AttackInputState::Update( int held, int pressed, int released )
{
	if ( eButtonState == BUTTONSTATE_DISABLED )
	{
		return;
	}

	// this exists so i don't have to do lots of confusing "if button pressed and my 
	// charge timer is < curtime and some other stuff then do this thing unless some
	// other variable says do something else".
	// note: can go directly from RELEASED to PRESSED without visiting UP along the way

	const bool bPressed = (pressed & iButton) == iButton;
	const bool bReleased = (released & iButton) == iButton;
	const bool bHeld = (held & iButton) == iButton;

	// if it's latched up, just keep reporting UP until the player releases the button
	if ( bLatchedUp )
	{
		if ( !bReleased )
		{
			eButtonState = BUTTONSTATE_UP;
			return;
		}
		else 
		{
			bLatchedUp = false;
		}
	}

	if ( bPressed )
	{
		eButtonState = BUTTONSTATE_PRESSED;
	}
	else if ( bReleased )
	{
		eButtonState = BUTTONSTATE_RELEASED;
	}
	else if ( bHeld ) 
	{
		eButtonState = BUTTONSTATE_DOWN;
	}
	else 
	{
		eButtonState = BUTTONSTATE_UP;
	}
}

//-----------------------------------------------------------------------------
void CPasstimeGun::AttackInputState::LatchUp()
{
	// can't use input->ClearButton here because we need this to apply on the server
	bLatchedUp = true;
	if ( eButtonState != BUTTONSTATE_UP )
		eButtonState = BUTTONSTATE_RELEASED;
}

//-----------------------------------------------------------------------------
void CPasstimeGun::AttackInputState::UnlatchUp()
{
	bLatchedUp = false;
}

//-----------------------------------------------------------------------------
bool CPasstimeGun::SendWeaponAnim( int actBase )
{
	switch ( actBase )
	{
	case ACT_VM_IDLE: 
		actBase = ACT_BALL_VM_IDLE;
		break;
	}

	return BaseClass::SendWeaponAnim( actBase );
}

//-----------------------------------------------------------------------------
void CPasstimeGun::ItemPostFrame()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
	{
		return;
	}

	bool bCanAttack2Cancel = !tf_passtime_experiment_autopass.GetBool();

#ifdef GAME_DLL

	//
	// Update pass target
	//
	if ( pOwner->m_Shared.HasPasstimeBall() )
	{
		VMatrix mWorldToView( SetupMatrixIdentity() );
		Vector vecEyePos;
		{
			Vector vecEyeDir;
			pOwner->EyePositionAndVectors( &vecEyePos, &vecEyeDir, 0, 0 );
			const QAngle &angEye = pOwner->EyeAngles();
			const VMatrix mTemp( SetupMatrixOrgAngles( vecEyePos, angEye ) );
			MatrixInverseTR( mTemp, mWorldToView );
		}

		//
		// If the current target is behind me, forget it immediately
		//
		auto *pCurrentTarget = ToTFPlayer( pOwner->m_Shared.GetPasstimePassTarget() );
		if ( pCurrentTarget )
		{
			Vector vLocalCurrentTarget( 0, 0, 0 ); // current target in local space
			Vector3DMultiplyPosition( mWorldToView, pCurrentTarget->WorldSpaceCenter(), vLocalCurrentTarget );
			if ( vLocalCurrentTarget.x < 0 ) // behind me
			{
				// clear the target
				pOwner->m_Shared.SetPasstimePassTarget( 0 );
				m_flTargetResetTime = 0;
			}
		}

		//
		// Look for a pass target
		//
		auto bAutoPassing = tf_passtime_experiment_autopass.GetBool() && m_attack2.Is( EButtonState::BUTTONSTATE_DOWN );
		auto flBestTargetDist = bAutoPassing ? FLT_MAX : 0.1f;
		CTFPlayer *pNewTarget = nullptr;
		auto flMaxPassDistSqr = g_pPasstimeLogic->GetMaxPassRange();
		flMaxPassDistSqr *= flMaxPassDistSqr;
		if ( !bCanAttack2Cancel || !m_attack2.Is( BUTTONSTATE_DOWN ) ) // right click prevents pass lock
		{
			//
			// Find a valid pass target that's close to the center of the screen
			// When autopassing is happening, it's just world distance instead of viewspace distance
			//
			for( int i = 1; i <= MAX_PLAYERS; i++ )
			{
				auto *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
				if ( pPlayer == pOwner )
					continue; // skip self

				if ( !BValidPassTarget( pOwner, pPlayer ) )
					continue;

				// Check world distance
				const auto &vTargetPos = pPlayer->WorldSpaceCenter();
				auto flThisTargetDist = vTargetPos.DistToSqr(vecEyePos);
				if ( flThisTargetDist > flMaxPassDistSqr ) 
					continue;

				// Check viewspace distance from crosshair when not autopassing
				if ( !bAutoPassing )
				{
					Vector vLocalTarget;
					Vector3DMultiplyPosition( mWorldToView, vTargetPos, vLocalTarget );

					if ( vLocalTarget.x < 0 )
						continue; // behind me

					flThisTargetDist = Vector( -vLocalTarget.y / vLocalTarget.x, -vLocalTarget.z / vLocalTarget.x, 0 ).Length(); // not aspect-correct
				}

				// check if closer than best
				if ( flThisTargetDist >= flBestTargetDist )
					continue; // too far 

				// pretend that people who are asking for the ball are closer, so they get priority
				// do this after the distance check 
				if ( pPlayer->m_Shared.AskForBallTime() > gpGlobals->curtime )
					flThisTargetDist /= 50.0f;

				// check for line of sight
				trace_t tr;
				UTIL_TraceLine( vecEyePos,	vTargetPos, MASK_PLAYERSOLID, pOwner, COLLISION_GROUP_PROJECTILE, &tr );
				if ( tr.m_pEnt != pPlayer )
					continue; // obstructed

				// success - new target
				flBestTargetDist = flThisTargetDist;
				pNewTarget = pPlayer;
			}
		}

		//
		// Replace the current pass target with a better one
		//
		if ( pNewTarget )
		{
			// Always bump the target reset time when the target is valid.
			// When the target isn't under the cursor anymore, the reset time will try to 
			// keep the lock for a short amount of time.
			m_flTargetResetTime = gpGlobals->curtime + tf_passtime_mode_homing_lock_sec.GetFloat();

			if ( pNewTarget != pCurrentTarget )
			{
				pOwner->m_Shared.SetPasstimePassTarget( pNewTarget );
				pCurrentTarget = pNewTarget;

				// play the lock-on sound for the player
				CRecipientFilter filter;
				filter.AddRecipient( pOwner );
				EmitSound( filter, pOwner->entindex(), kTargetHightlightSound );

				// now play it for the target
				filter.RemoveAllRecipients();
				filter.AddRecipient( pCurrentTarget );
				EmitSound( filter, pCurrentTarget->entindex(), kTargetHightlightSound );
			}
		}
		//
		// See if the current pass target is still valid
		//
		else if ( pCurrentTarget 
			&& (!BValidPassTarget( pOwner, pCurrentTarget ) 
				|| (bCanAttack2Cancel && m_attack2.Is( BUTTONSTATE_DOWN )) // right click prevents pass lock
				|| ((m_flTargetResetTime > 0 ) && (m_flTargetResetTime < gpGlobals->curtime))
				|| (pCurrentTarget->WorldSpaceCenter().DistToSqr( vecEyePos ) >= flMaxPassDistSqr) )
			&& !m_attack.Is( BUTTONSTATE_DOWN ) ) // left click prevents pass unlock
		{
			pOwner->m_Shared.SetPasstimePassTarget( 0 );
			m_flTargetResetTime = 0;
		}

		// autopass
		if ( tf_passtime_experiment_autopass.GetBool() 
			&& m_attack2.Is( EButtonState::BUTTONSTATE_DOWN ) 
			&& pOwner->m_Shared.GetPasstimePassTarget() )
		{
			// NOTE: change state after calling Throw
			Throw( pOwner );
			m_eThrowState = THROWSTATE_THROWN;
			m_attack2.LatchUp();
			m_attack.LatchUp();
		}
	}
	else
	{
		//
		// Not carrying the ball
		//
		pOwner->m_Shared.SetPasstimePassTarget( 0 );
		m_flTargetResetTime = 0;
	}
#endif

	//
	// Update throw state
	// Client and server both run this code; client predicts everything ideally, but there are some 
	// sketchy bits in here that probably don't predict right.
	//
	if ( pOwner->m_Shared.HasPasstimeBall() )
	{
		if ( (m_eThrowState == THROWSTATE_DISABLED) || (m_flNextPrimaryAttack > gpGlobals->curtime) || !CanAttack() )
		{
			// disable the attack input so the state will be correct when 
			// throwstate changes to not disabled
			m_attack.Disable();
			m_attack2.Disable();
		}
		else 
		{
			// update input
			m_attack.Enable();
			m_attack.Update( pOwner->m_nButtons, pOwner->m_afButtonPressed, pOwner->m_afButtonReleased );
			m_attack2.Enable();
			m_attack2.Update( pOwner->m_nButtons, pOwner->m_afButtonPressed, pOwner->m_afButtonReleased );

			if ( bCanAttack2Cancel && m_attack2.Is( BUTTONSTATE_PRESSED ) )
			{
				// check for cancelling attack by pressing attack2
				if ( (m_eThrowState == THROWSTATE_CHARGING) || (m_eThrowState == THROWSTATE_CHARGED) )
				{
#ifdef GAME_DLL
					++CTF_GameStats.m_passtimeStats.summary.nTotalThrowCancels;
#endif
					m_eThrowState = THROWSTATE_CANCELLED;
					m_attack2.LatchUp();
					m_attack.LatchUp();
				}
				else 
				{
					pOwner->DoClassSpecialSkill();
				}
			}

			switch( m_eThrowState )
			{
			case THROWSTATE_IDLE:
			{
				if ( m_attack.Is( BUTTONSTATE_PRESSED ) )
				{
					// note: should transition to CHARGING even if it will immediately finish charging
					m_eThrowState = THROWSTATE_CHARGING;
					m_fChargeBeginTime = gpGlobals->curtime;
					if ( GetChargeMaxTime() != 0 )
					{
						EmitSound( kChargeSound );
					}
					SendWeaponAnim( ACT_BALL_VM_THROW_START );
					m_flThrowLoopStartTime = gpGlobals->curtime + SequenceDuration();
				}
				else
				{
					m_fChargeBeginTime = 0;
					WeaponIdle();
				}
				break;
			}

			case THROWSTATE_CHARGING: 
			{
				if ( m_attack.Is( BUTTONSTATE_RELEASED ) )
				{
					// NOTE: change state after calling Throw
					Throw( pOwner );
					m_eThrowState = THROWSTATE_THROWN;
					break;
				}

				if ( m_flThrowLoopStartTime < gpGlobals->curtime )
				{
					m_flThrowLoopStartTime = FLT_MAX;
					SendWeaponAnim( ACT_BALL_VM_THROW_LOOP );
				}

				if ( (m_fChargeBeginTime <= 0) || (GetCurrentCharge() >= 1) )
				{
					m_eThrowState = THROWSTATE_CHARGED;
				}
				break;
			}

			case THROWSTATE_CHARGED:
			{
				if ( m_attack.Is( BUTTONSTATE_RELEASED ) )
				{
					// NOTE: change state after calling Throw
					Throw( pOwner );
					m_eThrowState = THROWSTATE_THROWN;
				}

				if ( m_flThrowLoopStartTime < gpGlobals->curtime )
				{
					m_flThrowLoopStartTime = FLT_MAX;
					SendWeaponAnim( ACT_BALL_VM_THROW_LOOP );
				}
				break;
			}

			case THROWSTATE_CANCELLED:
			{
				m_eThrowState = THROWSTATE_IDLE;
				SendWeaponAnim( ACT_BALL_VM_THROW_END );
				m_flThrowLoopStartTime = FLT_MAX;
				StopSound( kChargeSound );
#ifdef GAME_DLL
				CPASAttenuationFilter filter( pOwner );
				pOwner->EmitSound( filter, pOwner->entindex(), kShootOkSound );
#endif
				break;
			}

			case THROWSTATE_THROWN: 
			{
				// This means you got the ball between throwing it and holstering the gun.
				// Just do what Deploy does, roughly.
				m_eThrowState = THROWSTATE_IDLE;
				m_attack2.LatchUp();
				m_attack.LatchUp();
				break; 
			}

			case THROWSTATE_DISABLED: // should never get here
			default:
				Warning( "Invalid EThrowState value" );
			};
		}
	}

	//
	// If the player doesn't have the ball, switch to the previous
	// weapon at an appropriate time
	// if the ball was thrown, wait a bit for animation to look better
	//
	if ( !pOwner->m_Shared.HasPasstimeBall()
		&& ((m_eThrowState != THROWSTATE_THROWN) || (m_flNextPrimaryAttack <= gpGlobals->curtime)) )
	{
		// Setting m_eThrowState here fixes players getting stuck in the throw 
		// anim when they lose the ball while charging to throw. See GetChargeBeginTime
		// and CTFPlayerAnimState::CheckPasstimeThrowAnimation to see why.
		m_eThrowState = THROWSTATE_IDLE; 

		if ( !m_hStoredLastWpn || !pOwner->Weapon_Switch( m_hStoredLastWpn ) )
		{
			pOwner->SwitchToNextBestWeapon( this );
		}
	}

	// this SetWeaponVisible should go away once we have real animations. if you remove this, 
	// update the EF_NODRAW hack in CTFWeaponBase::OnDataChanged too
	SetWeaponVisible( pOwner->m_Shared.HasPasstimeBall() );

#ifdef CLIENT_DLL
	if ( m_attack.Is( BUTTONSTATE_DOWN ) )
	{
		pOwner->SetFiredWeapon( true ); // not sure what this does, exactly, but it seems important
	}
#endif
}

//-----------------------------------------------------------------------------
#ifdef GAME_DLL
static const char* IncomingSoundForClass( const CTFPlayerClass* pClass, char (&pszSound)[64] )
{
	// note: this will probably need to be replaced with response rules
	pszSound[0] = 0;
	switch ( pClass->GetClassIndex() )
	{
	case TF_CLASS_SCOUT: 
		V_sprintf_safe( pszSound, "Scout.Incoming0%i", RandomInt(1,3) );
		return pszSound;

    case TF_CLASS_SNIPER:
		V_sprintf_safe( pszSound, "Sniper.Incoming0%i", RandomInt(1,4) );
		return pszSound;

    case TF_CLASS_SOLDIER:
		V_sprintf_safe( pszSound, "Soldier.Incoming01" );
		return pszSound;

	case TF_CLASS_DEMOMAN:
		V_sprintf_safe( pszSound, "Demoman.Incoming0%i", RandomInt(1,3) );
		return pszSound;

	case TF_CLASS_MEDIC:
		V_sprintf_safe( pszSound, "Medic.Incoming0%i", RandomInt(1,3) );
		return pszSound;

	case TF_CLASS_HEAVYWEAPONS:
		V_sprintf_safe( pszSound, "Heavy.Incoming0%i", RandomInt(1,3) );
		return pszSound;

	case TF_CLASS_PYRO:
		V_sprintf_safe( pszSound, "Pyro.Incoming01" );
		return pszSound;

	case TF_CLASS_SPY:
		V_sprintf_safe( pszSound, "Spy.Incoming0%i", RandomInt(1,3) );
		return pszSound;

	case TF_CLASS_ENGINEER:		
		V_sprintf_safe( pszSound, "Engineer.Incoming0%i", RandomInt(1,3) );
		return pszSound;
	};

	return pszSound;
}
#endif

//-----------------------------------------------------------------------------
void CPasstimeGun::Throw( CTFPlayer *pOwner )
{
	StopSound( kChargeSound );
	pOwner->SetAnimation( PLAYER_ATTACK1 );
	pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	SendWeaponAnim( ACT_BALL_VM_THROW_END );
	m_flThrowLoopStartTime = FLT_MAX;

	m_flLastFireTime = gpGlobals->curtime;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration(); // this prevents weapon switch until anim finishes
	m_flNextSecondaryAttack = m_flNextPrimaryAttack;

#ifdef GAME_DLL
	pOwner->NoteWeaponFired(); // not sure what this does, exactly, but it seems important 
	CTFPlayer *pPassTarget = pOwner->m_Shared.GetPasstimePassTarget();
	const LaunchParams& launch = CalcLaunch( pOwner, pPassTarget != 0 );
	g_pPasstimeLogic->LaunchBall( pOwner, launch.startPos, launch.startVel );

	CPASAttenuationFilter pasFilter( pOwner );
	pOwner->EmitSound( pasFilter, pOwner->entindex(), kShootOkSound );
	if ( pPassTarget )
	{
		++CTF_GameStats.m_passtimeStats.summary.nTotalPassesStarted;
		m_ballController.SetTargetSpeed( tf_passtime_mode_homing_speed.GetFloat() );
		auto isCharged = (m_fChargeBeginTime > 0) && (GetCurrentCharge() >= 1);
		m_ballController.StartHoming( g_pPasstimeLogic->GetBall(), pPassTarget, isCharged );
		if ( CTFPlayer *pPlayerPassTarget = ToTFPlayer( pPassTarget ) )
		{
			char pszSound[64];
			IncomingSoundForClass( pOwner->GetPlayerClass(), pszSound );
			{
				// for the thrower
				CRecipientFilter filter;
				filter.MakeReliable();
				filter.AddRecipient( pOwner );
				filter.AddRecipientsByTeam( TFTeamMgr()->GetTeam( TEAM_SPECTATOR ) );
				pOwner->EmitSound( filter, pOwner->entindex(), pszSound );
			}
			{
				// for the catcher
				CRecipientFilter filter;
				filter.MakeReliable();
				filter.AddRecipient( pPlayerPassTarget );
				pPlayerPassTarget->EmitSound( filter, pPlayerPassTarget->entindex(), pszSound );
			}
		}
	}
	else
	{
		++CTF_GameStats.m_passtimeStats.summary.nTotalTosses;
	}
#else
	pOwner->m_Shared.SetHasPasstimeBall( 0 ); // predict throwing
#endif

	pOwner->m_Shared.SetPasstimePassTarget( 0 );
}

//-----------------------------------------------------------------------------
void CPasstimeGun::ItemHolsterFrame()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && pOwner->m_Shared.HasPasstimeBall() )
	{
		m_hStoredLastWpn = GetTFPlayerOwner()->GetActiveWeapon();
		pOwner->Weapon_Switch( this );
	}
}

//-----------------------------------------------------------------------------
const char *CPasstimeGun::GetWorldModel() const
{
	if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
	{
		return kHalloweenBallModel;
	}
	return tf_passtime_ball_model.GetString();
}

//-----------------------------------------------------------------------------
bool CPasstimeGun::Deploy()
{
	// This is not called on the client because the client can't predict it.
	if ( !BaseClass::Deploy() )
	{
		return false;
	}
	
	m_eThrowState = THROWSTATE_IDLE;
	m_attack2.UnlatchUp();
	m_attack.UnlatchUp();
	return true;
}


//-----------------------------------------------------------------------------
bool CPasstimeGun::CanDeploy()
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	return pOwner && pOwner->m_Shared.HasPasstimeBall() && BaseClass::CanDeploy();
}

//-----------------------------------------------------------------------------
// static
bool CPasstimeGun::BValidPassTarget( CTFPlayer *pSource, CTFPlayer *pTarget, HudNotification_t *pReason )
{
	if ( pReason ) *pReason = (HudNotification_t) 0;

	if ( !pTarget || (pTarget == pSource) )
	{
		return false;
	}

	bool bTargetDisguised = pTarget->m_Shared.InCond( TF_COND_DISGUISED );
	int iTargetTeam = pTarget->GetTeamNumber();
	int iSourceTeam = pSource ? pSource->GetTeamNumber() : iTargetTeam;
	bool bSameTeam = iTargetTeam == iSourceTeam;
	bool bTargetableEnemySpy = !bSameTeam && bTargetDisguised && (pTarget->m_Shared.GetDisguiseTeam() == iSourceTeam);

	if ( !bSameTeam && !bTargetableEnemySpy )
	{
		// can't pass to enemies
		return false; 
	}
	else if ( bSameTeam && bTargetDisguised )
	{
		// can't pass to disguised friendly spies
		if ( bTargetDisguised && pReason )
		{
			*pReason = HUD_NOTIFY_PASSTIME_NO_DISGUISE;
		}
		return false;
	}

#ifdef CLIENT_DLL
	return g_pPasstimeLogic->BCanPlayerPickUpBall( pTarget );
#else
	return g_pPasstimeLogic->BCanPlayerPickUpBall( pTarget, pReason );
#endif
}

//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CPasstimeGun::UpdateThrowArch()
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
	{
		return;
	}

	if ( !m_pBounceReticle )
	{
		m_pBounceReticle = new C_PasstimeBounceReticle();
	}

	if ( pOwner->m_Shared.GetPasstimePassTarget() )
	{
		m_pBounceReticle->Hide();
		return;
	}

	const LaunchParams& launchParams = CalcLaunch( pOwner, false );

	// Simple euler integration.
	// This seems to approximate what havok does reasonably accurately as long as there's no impact.
	Vector vecPos = launchParams.startPos;
	Vector vecVel = launchParams.startVel;

	const int iNumSuperSamples = 8;
	const float flDt = 1.0f / 16.0f / iNumSuperSamples;
	const Vector vecGravity_dt = flDt * Vector( 0, 0, -800 ); 
	const float flDamping_dt = flDt * tf_passtime_ball_damping_scale.GetFloat();

	Vector vecStart, vecEnd;
	trace_t tr;
	CTraceFilterSimple traceFilter( pOwner, COLLISION_GROUP_NONE );
	const int iMaxTraces = 100; // is this insane?
	for ( int iPoint = 0; iPoint < iMaxTraces; ++iPoint )
	{
		vecStart = vecPos;
		for ( int iSuperSample = 0; iSuperSample < iNumSuperSamples; ++iSuperSample )
		{
			vecVel += vecGravity_dt;
			vecVel -= vecVel * flDamping_dt;
			vecPos += vecVel * flDt;
		}
		vecEnd = vecPos;

		UTIL_TraceHull( vecStart, vecEnd, 
			-launchParams.traceHullSize, launchParams.traceHullSize, 
			MASK_PLAYERSOLID, &traceFilter, &tr );

		if ( tr.DidHit() )
		{	
			m_pBounceReticle->Show( tr.endpos, tr.plane.normal );
			break;

			// commented out code trying to guess bounce
			//vecVel = Lerp( tr.fraction, oldVel, vecVel ); // what vecVel was at point of impact, very roughly
			//vecPos = tr.endpos + tr.plane.normal; // move away from wall a bit
			//float speed = vecVel.NormalizeInPlace();
			//vecVel = -2 * vecVel.Dot( tr.plane.normal ) * tr.plane.normal + vecVel;
			//vecVel *= speed;
		}
	}

	if ( !tr.DidHit() )
	{
		m_pBounceReticle->Hide();
	}
}
#endif

//-----------------------------------------------------------------------------
//static 
CPasstimeGun::LaunchParams 
CPasstimeGun::LaunchParams::Default( CTFPlayer *pPlayer )
{
	LaunchParams p;
	pPlayer->EyePositionAndVectors( &p.eyePos, &p.viewFwd, &p.viewRight, &p.viewUp );
	const float size = tf_passtime_ball_sphere_radius.GetFloat() / 3.0f;
	p.traceHullSize = Vector( size, size, size );
	p.traceHullDistance = 8;
	p.startPos = pPlayer->Weapon_ShootPosition();
	p.startDir = p.viewFwd;
	p.startVel = p.startDir;
	return p;
}

//-----------------------------------------------------------------------------
static ConVar *s_pThrowSpeedConvars[TF_LAST_NORMAL_CLASS] = {
	nullptr, // TF_CLASS_UNDEFINED
	&tf_passtime_throwspeed_scout,
	&tf_passtime_throwspeed_sniper,
	&tf_passtime_throwspeed_soldier,
	&tf_passtime_throwspeed_demoman,
	&tf_passtime_throwspeed_medic,
	&tf_passtime_throwspeed_heavy,
	&tf_passtime_throwspeed_pyro,
	&tf_passtime_throwspeed_spy,
	&tf_passtime_throwspeed_engineer,
};

//-----------------------------------------------------------------------------
static ConVar *s_pThrowArcConvars[TF_LAST_NORMAL_CLASS] = {
	nullptr, // TF_CLASS_UNDEFINED
	&tf_passtime_throwarc_scout,
	&tf_passtime_throwarc_sniper,
	&tf_passtime_throwarc_soldier,
	&tf_passtime_throwarc_demoman,
	&tf_passtime_throwarc_medic,
	&tf_passtime_throwarc_heavy,
	&tf_passtime_throwarc_pyro,
	&tf_passtime_throwarc_spy,
	&tf_passtime_throwarc_engineer,
};

//-----------------------------------------------------------------------------
static void GetThrowParams( CTFPlayer *pPlayer, float *speed, float *arc )
{
	Assert( pPlayer && speed && arc );
	if ( !pPlayer ) return;

	auto iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( iClass <= TF_CLASS_UNDEFINED || iClass >= TF_LAST_NORMAL_CLASS ) 
	{
		if ( speed ) *speed = 1000.0f;
		if ( arc ) *arc = 0.3f;
	}
	else 
	{
		if ( speed ) *speed = s_pThrowSpeedConvars[iClass]->GetFloat();
		if ( arc ) *arc = s_pThrowArcConvars[iClass]->GetFloat();
	}
}

//-----------------------------------------------------------------------------
// static
CPasstimeGun::LaunchParams CPasstimeGun::CalcLaunch( CTFPlayer *pPlayer, bool bHoming )
{
	auto params = LaunchParams::Default( pPlayer );
	params.startPos = params.eyePos;

	if ( !bHoming )
	{
		float speed = 0.0f, arc = 0.0f;
		GetThrowParams( pPlayer, &speed, &arc );
		params.startVel = VectorLerp( params.startDir, Vector(0,0,1), arc );
		params.startVel.NormalizeInPlace();
		params.startVel *= speed;
	}
	else if ( !tf_passtime_experiment_autopass.GetBool() )
	{
		params.startVel = params.startDir * tf_passtime_mode_homing_speed.GetFloat();
	}
	else
	{
		params.startVel = Vector(0,0,0);
	}

	// mix in some amount of forward velocity
	auto fwdspeed = tf_passtime_throwspeed_velocity_scale.GetFloat() 
		* params.viewFwd.Dot( pPlayer->GetAbsVelocity() );
	VectorMAInline( params.startVel, fwdspeed, params.viewFwd, params.startVel );

	return params;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
void CPasstimeGun::ClientThink()
{
	if ( !IsActiveByLocalPlayer() && !IsLocalPlayerSpectator() )
	{
		if ( m_pBounceReticle )
		{
			m_pBounceReticle->Hide();
		}
		return;
	}

	// doing this in ItemPostFrame makes the position jittery for some reason, 
	// and doing it in ClientThink works better. Not entirely sure why, but I 
	// assume it's something to do with order of operations, or possibly prediction.
	if ( !IsLocalPlayerSpectator() && ((m_eThrowState == THROWSTATE_CHARGING) || (m_eThrowState == THROWSTATE_CHARGED)) )
	{
		UpdateThrowArch();
	}
	else if ( (IsLocalPlayerSpectator() || (m_eThrowState != THROWSTATE_THROWN)) && m_pBounceReticle )
	{
		m_pBounceReticle->Hide();
	}
}

#endif
