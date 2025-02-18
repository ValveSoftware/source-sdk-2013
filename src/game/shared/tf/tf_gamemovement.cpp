//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "gamemovement.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "in_buttons.h"
#include "movevars_shared.h"
#include "collisionutils.h"
#include "debugoverlay_shared.h"
#include "baseobject_shared.h"
#include "particle_parse.h"
#include "baseobject_shared.h"
#include "coordsize.h"
#include "tf_weapon_medigun.h"
#include "tf_wearable_weapons.h"
#include "takedamageinfo.h"
#include "tf_weapon_buff_item.h"
#include "halloween/tf_weapon_spellbook.h"
#include "tf_logic_player_destruction.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
	#include "c_world.h"
	#include "c_team.h"
	#include "prediction.h"

	#define CTeam C_Team

#else
	#include "tf_player.h"
	#include "team.h"
	#include "bot/tf_bot.h"
	#include "tf_fx.h"
#endif


ConVar	tf_duck_debug_spew( "tf_duck_debug_spew", "0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar	tf_showspeed( "tf_showspeed", "0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar	tf_avoidteammates( "tf_avoidteammates", "1", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Controls how teammates interact when colliding.\n  0: Teammates block each other\n  1: Teammates pass through each other, but push each other away (default)" );
ConVar	tf_avoidteammates_pushaway( "tf_avoidteammates_pushaway", "1", FCVAR_REPLICATED, "Whether or not teammates push each other away when occupying the same space" );
ConVar  tf_solidobjects( "tf_solidobjects", "1", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar	tf_clamp_back_speed( "tf_clamp_back_speed", "0.9", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar  tf_clamp_back_speed_min( "tf_clamp_back_speed_min", "100", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar  tf_clamp_airducks( "tf_clamp_airducks", "1", FCVAR_REPLICATED );
ConVar  tf_resolve_stuck_players( "tf_resolve_stuck_players", "1", FCVAR_REPLICATED );
ConVar  tf_scout_hype_mod( "tf_scout_hype_mod", "55", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar	tf_max_charge_speed( "tf_max_charge_speed", "750", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_CHEAT  | FCVAR_DEVELOPMENTONLY );
ConVar  tf_parachute_gravity( "tf_parachute_gravity", "0.2f", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Gravity while parachute is deployed" );
ConVar  tf_parachute_maxspeed_xy( "tf_parachute_maxspeed_xy", "300.0f", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Max XY Speed while Parachute is deployed" );
ConVar  tf_parachute_maxspeed_z( "tf_parachute_maxspeed_z", "-100.0f", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Max Z Speed while Parachute is deployed" );
ConVar  tf_parachute_maxspeed_onfire_z( "tf_parachute_maxspeed_onfire_z", "10.0f", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Max Z Speed when on Fire and Parachute is deployed" );
ConVar  tf_parachute_aircontrol( "tf_parachute_aircontrol", "2.5f", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Multiplier for how much air control players have when Parachute is deployed" );
ConVar	tf_parachute_deploy_toggle_allowed( "tf_parachute_deploy_toggle_allowed", "0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

ConVar  tf_halloween_kart_aircontrol( "tf_halloween_kart_aircontrol", "1.2f", FCVAR_CHEAT | FCVAR_REPLICATED, "Multiplier for how much air control players have when in Kart Mode" );
ConVar	tf_ghost_up_speed( "tf_ghost_up_speed", "300.f", FCVAR_CHEAT | FCVAR_REPLICATED, "Speed that ghost go upward while holding jump key" );
ConVar	tf_ghost_xy_speed( "tf_ghost_xy_speed", "300.f", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar	tf_grapplinghook_move_speed( "tf_grapplinghook_move_speed", "750", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar	tf_grapplinghook_use_acceleration( "tf_grapplinghook_use_acceleration", "0", FCVAR_REPLICATED, "Use full acceleration calculation for grappling hook movement" );
ConVar	tf_grapplinghook_acceleration( "tf_grapplinghook_acceleration", "3500", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar	tf_grapplinghook_dampening( "tf_grapplinghook_dampening", "500", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar	tf_grapplinghook_follow_distance( "tf_grapplinghook_follow_distance", "64", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar	tf_grapplinghook_jump_up_speed( "tf_grapplinghook_jump_up_speed", "375", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar	tf_grapplinghook_prevent_fall_damage( "tf_grapplinghook_prevent_fall_damage", "0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar	tf_grapplinghook_medic_latch_speed_scale( "tf_grapplinghook_medic_latch_speed_scale", "0.65", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar tf_movement_aircurrent_friction_mult( "tf_movement_aircurrent_friction_mult", "0.75", FCVAR_REPLICATED | FCVAR_CHEAT,
                                             "Friction multiplier when sliding against surfaces while trapped in an air current" );
ConVar tf_movement_aircurrent_aircontrol_mult( "tf_movement_aircurrent_aircontrol_mult", "0.25", FCVAR_REPLICATED | FCVAR_CHEAT,
                                               "Multiplier on air control when player is in an air current (such as airblast)" );
ConVar tf_movement_lost_footing_restick( "tf_movement_lost_footing_restick", "50.0", FCVAR_REPLICATED | FCVAR_CHEAT,
                                         "Early escape the lost footing condition if the player is moving slower than this across the ground" );
ConVar tf_movement_lost_footing_friction( "tf_movement_lost_footing_friction", "0.1", FCVAR_REPLICATED | FCVAR_CHEAT,
                                          "Ground friction for players who have lost their footing" );

extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;
extern ConVar cl_sidespeed;
extern ConVar mp_tournament_readymode_countdown;

#define TF_MAX_SPEED   (400 * 1.3)	// 400 is Scout max speed, and we allow up to 3% movement bonus.

#define TF_WATERJUMP_FORWARD	30
#define TF_WATERJUMP_UP			300
#define TF_TIME_TO_DUCK			0.3f
#define TF_AIRDUCKED_COUNT		2
//ConVar	tf_waterjump_up( "tf_waterjump_up", "300", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
//ConVar	tf_waterjump_forward( "tf_waterjump_forward", "30", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

#define	NUM_CROUCH_HINTS	3

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFGameMovement : public CGameMovement
{
public:
	DECLARE_CLASS( CTFGameMovement, CGameMovement );

	CTFGameMovement(); 

	virtual void PlayerMove();
	virtual unsigned int PlayerSolidMask( bool brushOnly = false );
	virtual void ProcessMovement( CBasePlayer *pBasePlayer, CMoveData *pMove );
	virtual bool CanAccelerate();
	virtual bool CheckJumpButton();
	virtual int CheckStuck( void );
	virtual bool CheckWater( void );
	virtual void WaterMove( void );
	virtual void FullWalkMove();
	virtual void WalkMove( void );
	virtual void AirMove( void );
	virtual void FullTossMove( void );
	virtual void CategorizePosition( void );
	virtual void CheckFalling( void );
	virtual void Duck( void );
	virtual Vector GetPlayerViewOffset( bool ducked ) const;
	bool			GrapplingHookMove( void );
	bool			ChargeMove( void );
	bool			StunMove( void );
	bool			TauntMove( void );
	void			VehicleMove( void );
	bool			HighMaxSpeedMove( void );
	virtual float	GetAirSpeedCap( void );

	virtual void	TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm );
	virtual CBaseHandle	TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm );
	virtual void	StepMove( Vector &vecDestination, trace_t &trace );
	virtual bool	GameHasLadders() const;
	virtual void SetGroundEntity( trace_t *pm );
	virtual void PlayerRoughLandingEffects( float fvol );

	virtual void HandleDuckingSpeedCrop( void );
protected:

	virtual void CheckWaterJump( void );
	void		 FullWalkMoveUnderwater();

private:

	bool		CheckWaterJumpButton( void );
	void		AirDash( void );
	void		PreventBunnyJumping();
	void		ToggleParachute( void );
	void		CheckKartWallBumping();

	// Ducking.
#if 0
	// New duck tests!
	void			HandleDuck( int nButtonsPressed );
	void			HandleUnDuck( int nButtonsReleased );
	void			TestDuck();
#endif
	void DuckOverrides();
	void OnDuck( int nButtonsPressed );
	void OnUnDuck( int nButtonsReleased );


private:

	Vector		m_vecWaterPoint;
	CTFPlayer  *m_pTFPlayer;
	bool		m_isPassingThroughEnemies;
};


// Expose our interface.
static CTFGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = ( IGameMovement * )&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameMovement, IGameMovement,INTERFACENAME_GAMEMOVEMENT, g_GameMovement );


// ---------------------------------------------------------------------------------------- //
// CTFGameMovement.
// ---------------------------------------------------------------------------------------- //

CTFGameMovement::CTFGameMovement()
{
	m_pTFPlayer = NULL;
	m_isPassingThroughEnemies = false;

}

//----------------------------------------------------------------------------------------
// Purpose: moves the player
//----------------------------------------------------------------------------------------
void CTFGameMovement::PlayerMove()
{
	// If we are in the lost footing condition, we are allowed to violate our normal max speed on the ground (that is,
	// you can stumble at faster than run speed)
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_LOST_FOOTING ) )
		{ mv->m_flClientMaxSpeed = mv->m_flMaxSpeed; }

	// call base class to do movement
	BaseClass::PlayerMove();

	// handle player's interaction with water
	int nNewWaterLevel = m_pTFPlayer->GetWaterLevel();
	if ( m_nOldWaterLevel != nNewWaterLevel )
	{
		if ( WL_NotInWater == m_nOldWaterLevel )
		{
			// The player has just entered the water.  Determine if we should play a splash sound.
			bool bPlaySplash = false;

			Vector vecVelocity = m_pTFPlayer->GetAbsVelocity();
			if ( vecVelocity.z <= -200.0f )
			{
				// If the player has significant downward velocity, play a splash regardless of water depth.  (e.g. Jumping hard into a puddle)
				bPlaySplash = true;
			}
			else
			{
				// Look at the water depth below the player.  If it's significantly deep, play a splash to accompany the sinking that's about to happen.
				Vector vecStart = m_pTFPlayer->GetAbsOrigin();
				Vector vecEnd = vecStart;
				vecEnd.z -= 20;	// roughly thigh deep
				trace_t tr;
				// see if we hit anything solid a little bit below the player
				UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID,m_pTFPlayer, COLLISION_GROUP_NONE, &tr );
				if ( tr.fraction >= 1.0f ) 
				{
					// some amount of water below the player, play a splash
					bPlaySplash = true;
				}
			}

			if ( bPlaySplash )
			{
				m_pTFPlayer->EmitSound( "Physics.WaterSplash" );
			}
		} 
	}

	// Remove our shield charge if we slow down a bunch.
	float flSpeed = VectorLength( mv->m_vecVelocity );
	if ( flSpeed < 300.0f )
	{
		m_pTFPlayer->m_Shared.EndCharge();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CTFGameMovement::GetPlayerViewOffset( bool ducked ) const
{
	return ( ( ducked ) ? ( VEC_DUCK_VIEW_SCALED( m_pTFPlayer ) ) : ( m_pTFPlayer->GetClassEyeHeight() ) );
}

//-----------------------------------------------------------------------------
// Purpose: Allow bots etc to use slightly different solid masks
//-----------------------------------------------------------------------------
unsigned int CTFGameMovement::PlayerSolidMask( bool brushOnly )
{
	unsigned int uMask = 0;

	// Ghost players dont collide with anything but the world
	if ( m_pTFPlayer && m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
	{
		return MASK_PLAYERSOLID_BRUSHONLY;
	}

	if ( m_pTFPlayer && !m_isPassingThroughEnemies )
	{
		switch( m_pTFPlayer->GetTeamNumber() )
		{
		case TF_TEAM_RED:
			uMask = CONTENTS_BLUETEAM;
			break;

		case TF_TEAM_BLUE:
			uMask = CONTENTS_REDTEAM;
			break;
		}
	}

	return ( uMask | BaseClass::PlayerSolidMask( brushOnly ) );
}

//-----------------------------------------------------------------------------
// Purpose: Overridden to allow players to run faster than the maxspeed
//-----------------------------------------------------------------------------
void CTFGameMovement::ProcessMovement( CBasePlayer *pBasePlayer, CMoveData *pMove )
{
	// Verify data.
	Assert( pBasePlayer );
	Assert( pMove );
	if ( !pBasePlayer || !pMove )
		return;

	// Reset point contents for water check.
	ResetGetPointContentsCache();

	// Cropping movement speed scales mv->m_fForwardSpeed etc. globally
	// Once we crop, we don't want to recursively crop again, so we set the crop
	// flag globally here once per usercmd cycle.
	m_iSpeedCropped = SPEED_CROPPED_RESET;

	// Get the current TF player.
	m_pTFPlayer = ToTFPlayer( pBasePlayer );
	player = m_pTFPlayer;
	mv = pMove;

	// The max speed is currently set to the scout - if this changes we need to change this!
	mv->m_flMaxSpeed = TF_MAX_SPEED;

	// Handle charging demomens
	ChargeMove();

	// Handle player stun.
	StunMove();

	// Handle player taunt move
	TauntMove();

	// Handle grappling hook move
	GrapplingHookMove();

	// Handle scouts that can move really fast with buffs
	HighMaxSpeedMove();

	// Run the command.
	PlayerMove();


	FinishMove();

#if defined(GAME_DLL)
	m_pTFPlayer->m_bTakenBlastDamageSinceLastMovement = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameMovement::GrapplingHookMove()
{
	CBaseEntity *pHookTarget = m_pTFPlayer->GetGrapplingHookTarget();

	if ( !pHookTarget )
		return false;

	// check if player can be moved
	if ( !m_pTFPlayer->CanPlayerMove() || m_pTFPlayer->m_Shared.IsControlStunned() )
	{
		mv->m_flForwardMove = 0.f;
		mv->m_flSideMove = 0.f;
		mv->m_flUpMove = 0.f;
		mv->m_nButtons = 0;
		m_pTFPlayer->m_nButtons = mv->m_nButtons;
		return false;
	}

	m_pTFPlayer->SetGroundEntity( NULL );

	Vector vDesiredMove = pHookTarget->WorldSpaceCenter() - m_pTFPlayer->WorldSpaceCenter();

	CTFPlayer *pPlayerToCheck = m_pTFPlayer;
	if ( pHookTarget->IsPlayer() )
	{
		CTFPlayer *pHookedPlayer = ToTFPlayer( pHookTarget );
		// If our target is grappling, adjust aim to behind them
		CBaseEntity *pHookedPlayerTarget = pHookedPlayer->GetGrapplingHookTarget();
		if ( pHookedPlayerTarget )
		{
			Vector vTargetGrapple = pHookedPlayerTarget->WorldSpaceCenter() - pHookedPlayer->WorldSpaceCenter();
			vTargetGrapple.NormalizeInPlace();
			vDesiredMove += vTargetGrapple * ( -1 * tf_grapplinghook_follow_distance.GetFloat() );
		}
		else
		{
			// Otherwise, aim short of their center.
			vDesiredMove += vDesiredMove.Normalized() * ( -1 * tf_grapplinghook_follow_distance.GetFloat() );
		}
	}

	mv->m_flMaxSpeed = tf_grapplinghook_move_speed.GetFloat();

	ETFFlagType ignoreTypes[] = { TF_FLAGTYPE_PLAYER_DESTRUCTION };
	bool bHasTheFlag = pPlayerToCheck->HasTheFlag( ignoreTypes, ARRAYSIZE( ignoreTypes ) );
	bool bIsTeamLeader = false;

	if ( TFGameRules() && ( TFGameRules()->GetGameType() == TF_GAMETYPE_PD ) )
	{
		CTFPlayerDestructionLogic *pPlayerDestructionLogic = CTFPlayerDestructionLogic::GetPlayerDestructionLogic();
		if ( pPlayerDestructionLogic && ( pPlayerDestructionLogic->GetTeamLeader( pPlayerToCheck->GetTeamNumber() ) == pPlayerToCheck ) )
		{
			bIsTeamLeader = true;
		}
	}

	// If we're grappling along with an ally, use their rune to avoid falling behind or passing them
	if ( pPlayerToCheck->m_Shared.GetCarryingRuneType() == RUNE_AGILITY && !bHasTheFlag )
	{
		mv->m_flMaxSpeed = 950.f;
	}
	// Heavies get a grapple speed reduction across the board, even if they have Agility
	if ( pPlayerToCheck->GetPlayerClass()->GetClassIndex() == TF_CLASS_HEAVYWEAPONS && !bHasTheFlag )
	{
		mv->m_flMaxSpeed *= 0.7f;
	}
	// Grapple movement speed penalty if player is carrying the flag. Scout and Agility get smaller penalties
	else if ( bHasTheFlag || bIsTeamLeader )
	{
		if ( pPlayerToCheck->GetPlayerClass()->GetClassIndex() == TF_CLASS_SCOUT )
		{
			if ( pPlayerToCheck->m_Shared.GetCarryingRuneType() == RUNE_NONE || pPlayerToCheck->m_Shared.GetCarryingRuneType() == RUNE_AGILITY )
			{
				mv->m_flMaxSpeed *= 0.80f;
			}
			else
				mv->m_flMaxSpeed *= 0.65f;
		}
		else if ( pPlayerToCheck->m_Shared.GetCarryingRuneType() == RUNE_NONE || pPlayerToCheck->m_Shared.GetCarryingRuneType() == RUNE_AGILITY )
		{
			mv->m_flMaxSpeed *= 0.65f;
		}
		else
		{
			mv->m_flMaxSpeed *= 0.50f;
		}
	}
	// Pyros that are hooked into enemy players travel slower because of their advantage in close quarters
	else if ( pPlayerToCheck->GetPlayerClass()->GetClassIndex() == TF_CLASS_PYRO && pPlayerToCheck->m_Shared.InCond( TF_COND_GRAPPLED_TO_PLAYER ) )
	{
		mv->m_flMaxSpeed *= 0.7f;
	}

	if ( tf_grapplinghook_use_acceleration.GetBool() )
	{
		// Use acceleration with dampening
		float flSpeed = mv->m_vecVelocity.Length();
		if ( flSpeed > 0.f ) {
			float flDampen = Min( tf_grapplinghook_dampening.GetFloat() * gpGlobals->frametime, flSpeed );
			mv->m_vecVelocity *= ( flSpeed - flDampen ) / flSpeed;
		}

		mv->m_vecVelocity += vDesiredMove.Normalized() * ( tf_grapplinghook_acceleration.GetFloat() * gpGlobals->frametime );

		flSpeed = mv->m_vecVelocity.Length();
		if ( flSpeed > mv->m_flMaxSpeed )
		{
			mv->m_vecVelocity *= mv->m_flMaxSpeed / flSpeed;
		}
	}
	else
	{
		// Simple velocity calculation
		float vDist = vDesiredMove.Length();
		if ( vDist > mv->m_flMaxSpeed * gpGlobals->frametime )
		{
			mv->m_vecVelocity = vDesiredMove * ( mv->m_flMaxSpeed / vDist );
		}
		else
		{
			mv->m_vecVelocity = vDesiredMove / gpGlobals->frametime;
		}
	}

	// slow down when player is close to the hook target to prevent yoyo effect 
	float flDistSqrToTarget = m_pTFPlayer->GetAbsOrigin().DistToSqr( pHookTarget->GetAbsOrigin() );
	if ( flDistSqrToTarget < 10000 )
	{
		// remap the speed between 80-100 unit distance
		mv->m_vecVelocity = mv->m_vecVelocity.Normalized() * RemapValClamped( flDistSqrToTarget, 6400, 10000, 0.f, mv->m_flMaxSpeed );
	}

	mv->m_flForwardMove = 0.f;
	mv->m_flSideMove = 0.f;
	mv->m_flUpMove = 0.f;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameMovement::ChargeMove()
{
	if ( !m_pTFPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		// Check for Quick Fix Medic healing a charging player
		if ( !m_pTFPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
			return false;

		CTFWeaponBase *pTFWeapon = m_pTFPlayer->GetActiveTFWeapon();
		if ( !pTFWeapon )
			return false;

		if ( pTFWeapon->GetWeaponID() != TF_WEAPON_MEDIGUN )
			return false;

		CWeaponMedigun *pMedigun = static_cast< CWeaponMedigun* >( pTFWeapon );
		if ( !pMedigun || pMedigun->GetMedigunType() != MEDIGUN_QUICKFIX )
			return false;

		CTFPlayer *pHealTarget = ToTFPlayer( pMedigun->GetHealTarget() );
		if ( !pHealTarget || !pHealTarget->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
			return false;
	}

	mv->m_flMaxSpeed = tf_max_charge_speed.GetFloat();

	int oldbuttons = mv->m_nButtons;

	// Handle demoman shield charge.
	mv->m_flForwardMove = tf_max_charge_speed.GetFloat();
	mv->m_flSideMove = 0.0f;
	mv->m_flUpMove = 0.0f;
	if ( mv->m_nButtons & IN_ATTACK2 )
	{
		// Allow the player to continue to hold alt-fire.
		mv->m_nButtons = IN_ATTACK2;
	}
	else
	{
		mv->m_nButtons = 0;
	}

	if ( oldbuttons & IN_ATTACK )
	{
		mv->m_nButtons |= IN_ATTACK;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameMovement::StunMove()
{
	// Handle control stun.
	if ( m_pTFPlayer->m_Shared.IsControlStunned() 
		|| m_pTFPlayer->m_Shared.IsLoserStateStunned() )
	{
		bool bAttackButtonDown = ( mv->m_nButtons & IN_ATTACK2 || mv->m_nButtons & IN_ATTACK );

		// Can't fire or select weapons.
		mv->m_nButtons = 0;

		if ( bAttackButtonDown && m_pTFPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && m_pTFPlayer->GetActiveTFWeapon() && ( m_pTFPlayer->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_MINIGUN ) )
		{
			// Heavies can still spin their gun.
			mv->m_nButtons = IN_ATTACK2;
		}

		if ( m_pTFPlayer->m_Shared.IsControlStunned() )
		{
			mv->m_flForwardMove = 0.0f;
			mv->m_flSideMove = 0.0f;
			mv->m_flUpMove = 0.0f;
		}

		m_pTFPlayer->m_nButtons = mv->m_nButtons;
	}

	// Handle movement stuns
	float flStunAmount = m_pTFPlayer->m_Shared.GetAmountStunned( TF_STUN_MOVEMENT );
	// Lerp to the desired amount
	if ( flStunAmount )
	{
		if ( m_pTFPlayer->m_Shared.m_flStunLerpTarget != flStunAmount )
		{
			m_pTFPlayer->m_Shared.m_flLastMovementStunChange = gpGlobals->curtime;
			m_pTFPlayer->m_Shared.m_flStunLerpTarget = flStunAmount;
			m_pTFPlayer->m_Shared.m_bStunNeedsFadeOut = true;
		}

		mv->m_flForwardMove *= 1.f - flStunAmount;
		mv->m_flSideMove *= 1.f - flStunAmount;
		if ( m_pTFPlayer->m_Shared.GetStunFlags() & TF_STUN_MOVEMENT_FORWARD_ONLY )
		{
			mv->m_flForwardMove = 0.f;
		}

		return true;
	}
	else if ( m_pTFPlayer->m_Shared.m_flLastMovementStunChange )
	{
		// Lerp out to normal speed
		if ( m_pTFPlayer->m_Shared.m_bStunNeedsFadeOut )
		{
			m_pTFPlayer->m_Shared.m_flLastMovementStunChange = gpGlobals->curtime;
			m_pTFPlayer->m_Shared.m_bStunNeedsFadeOut = false;
		}

		float flCurStun = RemapValClamped( (gpGlobals->curtime - m_pTFPlayer->m_Shared.m_flLastMovementStunChange), 0.2, 0.0, 0.0, 1.0 );
		if ( flCurStun )
		{
			float flRemap = m_pTFPlayer->m_Shared.m_flStunLerpTarget * flCurStun;
			mv->m_flForwardMove *= (1.0 - flRemap);
			mv->m_flSideMove *= (1.0 - flRemap);
			if ( m_pTFPlayer->m_Shared.GetStunFlags() & TF_STUN_MOVEMENT_FORWARD_ONLY )
			{
				mv->m_flForwardMove = 0.f;
			}
		}
		else
		{
			m_pTFPlayer->m_Shared.m_flStunLerpTarget = 0.f;
			m_pTFPlayer->m_Shared.m_flLastMovementStunChange = 0;
		}

		return true;
	}

	// No one can move when in a final countdown transition or with the ConTracker open.
	// Do this here to avoid the inevitable hack that prevents players 
	// from receiving a flag or condition by stalling thinks, etc.
	if ( m_pTFPlayer->IsViewingCYOAPDA() || ( TFGameRules() && TFGameRules()->BInMatchStartCountdown() ) )
	{
		mv->m_flForwardMove = 0.f;
		mv->m_flSideMove = 0.f;
		mv->m_flUpMove = 0.f;
		mv->m_nButtons = 0;
		m_pTFPlayer->m_nButtons = mv->m_nButtons;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameMovement::TauntMove( void )
{
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		VehicleMove();
	}
	else if ( m_pTFPlayer->m_Shared.InCond( TF_COND_TAUNTING ) && m_pTFPlayer->CanMoveDuringTaunt() )
	{
		m_pTFPlayer->SetTauntYaw( mv->m_vecViewAngles[YAW] );

		bool bForceMoveForward = m_pTFPlayer->IsTauntForceMovingForward();
		float flMaxMoveSpeed = m_pTFPlayer->GetTauntMoveSpeed();
		float flAcceleration = m_pTFPlayer->GetTauntMoveAcceleration();

		float flMoveDir = 0.f;
		if ( !bForceMoveForward )
		{
			// Grab analog inputs, normalized to [0,1], to allow controller to also drive taunt movement.
			if ( mv->m_flForwardMove > 0 && cl_forwardspeed.GetFloat() > 0 )
			{
				flMoveDir += mv->m_flForwardMove / cl_forwardspeed.GetFloat();
			}
			else if ( mv->m_flForwardMove < 0 && cl_backspeed.GetFloat() > 0 )
			{
				flMoveDir += mv->m_flForwardMove / cl_backspeed.GetFloat();
			}

			// No need to read buttons explicitly anymore, since that input is already included in m_flForwardMove
			/*	if ( mv->m_nButtons & IN_FORWARD )
				flMoveDir += 1.f;
			if ( mv->m_nButtons & IN_BACK )
				flMoveDir += -1.f;	  */

			// Clamp to [0,1], just in case.
			if ( flMoveDir > 1.0f )
			{
				flMoveDir = 1.0f;
			}
			else if ( flMoveDir < -1.0f )
			{
				flMoveDir = -1.0f;
			}
		}
		else
		{
			flMoveDir = 1.f;
		}

		bool bMoving = flMoveDir != 0.f;
		float flSign = bMoving ? 1.f : -1.f;
		if ( flAcceleration > 0.f )
		{
			m_pTFPlayer->SetCurrentTauntMoveSpeed( clamp( m_pTFPlayer->GetCurrentTauntMoveSpeed() + flSign * ( gpGlobals->frametime / flAcceleration ) * flMaxMoveSpeed, 0.f, flMaxMoveSpeed ) );
		}
		else
		{
			m_pTFPlayer->SetCurrentTauntMoveSpeed( flMaxMoveSpeed );
		}

		// don't allow taunt to move if the player cannot move
		if ( !m_pTFPlayer->CanPlayerMove() )
		{
			flMaxMoveSpeed = 0.f;
		}
			
		float flSmoothMoveSpeed = 0.f;
		if ( flMaxMoveSpeed > 0.f )
		{
			flSmoothMoveSpeed = SimpleSpline( m_pTFPlayer->GetCurrentTauntMoveSpeed() / flMaxMoveSpeed ) * flMaxMoveSpeed;
		}

		mv->m_flMaxSpeed = flMaxMoveSpeed;
		mv->m_flForwardMove = flMoveDir * flSmoothMoveSpeed;
		mv->m_flClientMaxSpeed = flMaxMoveSpeed;

		return true;
	}
	else
	{
		m_pTFPlayer->SetCurrentTauntMoveSpeed( 0.f );
	}

	return false;
}

ConVar tf_halloween_kart_dash_speed( "tf_halloween_kart_dash_speed", "1000", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_dash_accel( "tf_halloween_kart_dash_accel", "750", FCVAR_CHEAT | FCVAR_REPLICATED );

ConVar tf_halloween_kart_normal_speed( "tf_halloween_kart_normal_speed", "650", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_normal_accel( "tf_halloween_kart_normal_accel", "300", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_slowmoving_accel( "tf_halloween_kart_slowmoving_accel", "500", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_slowmoving_threshold( "tf_halloween_kart_slowmoving_threshold", "300", FCVAR_CHEAT | FCVAR_REPLICATED );

ConVar tf_halloween_kart_reverse_speed( "tf_halloween_kart_reverse_speed", "-50", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_brake_speed( "tf_halloween_kart_brake_speed", "0", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_brake_accel( "tf_halloween_kart_brake_accel", "500", FCVAR_CHEAT | FCVAR_REPLICATED );

ConVar tf_halloween_kart_idle_speed( "tf_halloween_kart_idle_speed", "0", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_coast_accel( "tf_halloween_kart_coast_accel", "300", FCVAR_CHEAT | FCVAR_REPLICATED );

ConVar tf_halloween_kart_bombhead_scale( "tf_halloween_kart_bombhead_scale", "1.5f", FCVAR_CHEAT | FCVAR_REPLICATED );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::VehicleMove( void )
{
	// Reset Flags
	m_pTFPlayer->m_iKartState = 0;
	m_pTFPlayer->SetTauntYaw( mv->m_vecViewAngles[YAW] );

	float flMaxMoveSpeed = tf_halloween_kart_normal_speed.GetFloat();

	float flTargetSpeed = tf_halloween_kart_idle_speed.GetFloat();
	// Just standard accell by default
	float flAcceleration = tf_halloween_kart_coast_accel.GetFloat();

	bool bInput = false;

	// Hitting the gas
	if ( mv->m_flForwardMove > 0.0f )
	{
		// Grab normalized analog input (no need to check key input explicitly, since it's already baked into m_flForwardMove
		float flNormalizedForwardInput = cl_forwardspeed.GetFloat() > 0.0f ? mv->m_flForwardMove / cl_forwardspeed.GetFloat() : 0.0f;
		if ( flNormalizedForwardInput > 1.0f )
		{
			flNormalizedForwardInput = 1.0f;
		}

		// Target normal speed
		flTargetSpeed = tf_halloween_kart_normal_speed.GetFloat();
		// Use normal accell speed if it's faster than our current speed
		if ( flTargetSpeed > m_pTFPlayer->GetCurrentTauntMoveSpeed() )
		{
			if ( m_pTFPlayer->GetCurrentTauntMoveSpeed() < tf_halloween_kart_slowmoving_threshold.GetFloat() )
			{
				flAcceleration = tf_halloween_kart_slowmoving_accel.GetFloat() * flNormalizedForwardInput;
			}
			else
			{
				flAcceleration = tf_halloween_kart_normal_accel.GetFloat() * flNormalizedForwardInput;
			}
		}

		bInput = true;
		m_pTFPlayer->m_iKartState |= CTFPlayerShared::kKartState_Driving;
	}
	else if ( mv->m_flForwardMove < 0.0f )	// Hitting the brakes
	{
		// Grab normalized analog input (no need to check key input explicitly, since it's already baked into m_flForwardMove. And flip the sign, since we're going backwards.
		float flNormalizedForwardInput = cl_backspeed.GetFloat() > 0.0f ? mv->m_flForwardMove / cl_backspeed.GetFloat() : 0.0f;
		if ( flNormalizedForwardInput < -1.0f )
		{
			flNormalizedForwardInput = 1.0f;
		}
		else
		{
			flNormalizedForwardInput = -flNormalizedForwardInput;
		}

		// slowing down
		if ( m_pTFPlayer->GetCurrentTauntMoveSpeed() > 0 )
		{
			// Target brake speed
			flTargetSpeed = tf_halloween_kart_brake_speed.GetFloat();
			// Use brake accell speed if it's slower than our current speed
			if ( flTargetSpeed < m_pTFPlayer->GetCurrentTauntMoveSpeed() )
			{
				flAcceleration = tf_halloween_kart_brake_accel.GetFloat() * flNormalizedForwardInput;
			}
			m_pTFPlayer->m_iKartState |= CTFPlayerShared::kKartState_Braking;
		}
		// if we are already stopped, look for new input to start going backwards
		else 
		{
			// check for new input, else do nothing
			if ( mv->m_flOldForwardMove >= 0.0f  || m_pTFPlayer->GetCurrentTauntMoveSpeed() < 0 || m_pTFPlayer->GetVehicleReverseTime() < gpGlobals->curtime )
			{
				// going backwards, keep going backwards
				flTargetSpeed = tf_halloween_kart_reverse_speed.GetFloat();
				// Use brake accell speed if it's slower than our current speed
				if ( flTargetSpeed < m_pTFPlayer->GetCurrentTauntMoveSpeed() )
				{
					flAcceleration = tf_halloween_kart_brake_accel.GetFloat() * flNormalizedForwardInput;
				}
				m_pTFPlayer->m_iKartState |= CTFPlayerShared::kKartState_Reversing;
			}
			else
			{
				// Stall for 1 second then start reversing
				if ( m_pTFPlayer->GetVehicleReverseTime() == FLT_MAX )
				{
					m_pTFPlayer->SetVehicleReverseTime( gpGlobals->curtime + 0.6f );
				}
				m_pTFPlayer->m_iKartState |= CTFPlayerShared::kKartState_Stopped;
			}
		}

		bInput = true;
	}
	
	if ( m_pTFPlayer->GetCurrentTauntMoveSpeed() > 0 )
	{
		m_pTFPlayer->SetVehicleReverseTime( FLT_MAX );
	}

	// braking?
	if ( bInput && Sign( m_pTFPlayer->GetCurrentTauntMoveSpeed() ) != Sign( flTargetSpeed ) )
	{
		flAcceleration = tf_halloween_kart_brake_accel.GetFloat();
	}

	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) )
	{
		flMaxMoveSpeed *= tf_halloween_kart_bombhead_scale.GetFloat();
		flAcceleration *= tf_halloween_kart_bombhead_scale.GetFloat();
	}

	float flTargetMoveSpeed = Approach( flTargetSpeed, m_pTFPlayer->GetCurrentTauntMoveSpeed(), flAcceleration * gpGlobals->frametime );
	float flSmoothMoveSpeed = Bias( fabs( m_pTFPlayer->GetCurrentTauntMoveSpeed() ) / flMaxMoveSpeed, 0.7f ) * flMaxMoveSpeed * Sign( flTargetMoveSpeed );

	// Boost slams the accelerator
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART_DASH ) )
	{
		flTargetSpeed = tf_halloween_kart_dash_speed.GetFloat();
		flMaxMoveSpeed = tf_halloween_kart_dash_speed.GetFloat();
		flTargetMoveSpeed = flTargetSpeed;
		flSmoothMoveSpeed = flTargetSpeed;
		flAcceleration = tf_halloween_kart_dash_accel.GetFloat();
	}

	m_pTFPlayer->SetCurrentTauntMoveSpeed( flTargetMoveSpeed );
	float flLeanAccel = flTargetSpeed > flSmoothMoveSpeed ? flAcceleration : flTargetSpeed < flSmoothMoveSpeed ? -flAcceleration : 0.f;
	flLeanAccel = Sign( m_pTFPlayer->GetCurrentTauntMoveSpeed() ) != Sign( flTargetSpeed ) ? -flLeanAccel : flLeanAccel;
	m_pTFPlayer->m_PlayerAnimState->Vehicle_LeanAccel( flLeanAccel );

#ifdef DEBUG
	engine->Con_NPrintf( 0, "Speed:  %3.2f", m_pTFPlayer->GetCurrentTauntMoveSpeed() );
	engine->Con_NPrintf( 1, "Target: %3.2f", flTargetSpeed );
	engine->Con_NPrintf( 2, "Accell: %3.2f", flAcceleration );
#endif
			
	mv->m_flMaxSpeed = flMaxMoveSpeed;
	mv->m_flForwardMove = flSmoothMoveSpeed;
	mv->m_flClientMaxSpeed = flMaxMoveSpeed;
	mv->m_flSideMove = 0.f; // No sideways movement
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameMovement::HighMaxSpeedMove()
{
	if ( fabsf( mv->m_flForwardMove ) < player->MaxSpeed() )
	{
		if ( AlmostEqual( mv->m_flForwardMove, cl_forwardspeed.GetFloat() ) )
		{
			mv->m_flForwardMove = player->MaxSpeed();
		}
		else if ( AlmostEqual( mv->m_flForwardMove, -cl_backspeed.GetFloat() ) )
		{
			mv->m_flForwardMove = -player->MaxSpeed();
		}
	}

	if ( fabsf( mv->m_flSideMove ) < player->MaxSpeed() )
	{
		if ( AlmostEqual( mv->m_flSideMove, cl_sidespeed.GetFloat() ) )
		{
			mv->m_flSideMove = player->MaxSpeed();
		}
		else if ( AlmostEqual( mv->m_flSideMove, -cl_sidespeed.GetFloat() ) )
		{
			mv->m_flSideMove = -player->MaxSpeed();
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameMovement::CanAccelerate()
{
	// Only allow the player to accelerate when in certain states.
	int nCurrentState = m_pTFPlayer->m_Shared.GetState();
	if ( nCurrentState == TF_STATE_ACTIVE )
	{
		return player->GetWaterJumpTime() == 0;
	}
	else if ( player->IsObserver() )
	{
		return true;
	}
	else
	{	
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if we are in water.  If so the jump button acts like a
// swim upward key.
//-----------------------------------------------------------------------------
bool CTFGameMovement::CheckWaterJumpButton( void )
{
	// See if we are water jumping.  If so, decrement count and return.
	if ( player->m_flWaterJumpTime )
	{
		player->m_flWaterJumpTime -= gpGlobals->frametime;
		if (player->m_flWaterJumpTime < 0)
		{
			player->m_flWaterJumpTime = 0;
		}

		return false;
	}

	// In water above our waist.
	if ( player->GetWaterLevel() >= 2 || m_pTFPlayer->m_Shared.InCond( TF_COND_SWIMMING_NO_EFFECTS ) )
	{	
		// Swimming, not jumping.
		SetGroundEntity( NULL );

		int iCannotSwim = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pTFPlayer, iCannotSwim, cannot_swim );
		if ( iCannotSwim )
		{
			return false;
		}

		// We move up a certain amount.
		if ( player->GetWaterType() == CONTENTS_WATER || m_pTFPlayer->m_Shared.InCond( TF_COND_SWIMMING_NO_EFFECTS ) )
		{
			mv->m_vecVelocity[2] = 100;
		}
		else if ( player->GetWaterType() == CONTENTS_SLIME )
		{
			mv->m_vecVelocity[2] = 80;
		}

		// Play swimming sound.
		if ( player->m_flSwimSoundTime <= 0 && !m_pTFPlayer->m_Shared.InCond( TF_COND_SWIMMING_NO_EFFECTS ) )
		{
			// Don't play sound again for 1 second.
			player->m_flSwimSoundTime = 1000;
			PlaySwimSound();
		}

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::AirDash( void )
{
	// Apply approx. the jump velocity added to an air dash.
	Assert( GetCurrentGravity() == 800.0f );

	float flJumpMod = 1.f;
	// Passive version
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pTFPlayer, flJumpMod, mod_jump_height );
	// Weapon-restricted version
	CTFWeaponBase *pWpn = m_pTFPlayer->GetActiveTFWeapon();
	if ( pWpn )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWpn, flJumpMod, mod_jump_height_from_weapon );
	}

	// Lose hype on airdash
	int iHypeResetsOnJump = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pTFPlayer, iHypeResetsOnJump, hype_resets_on_jump );
	if ( iHypeResetsOnJump != 0 )
	{
		// Loose x hype on jump
		float flHype = m_pTFPlayer->m_Shared.GetScoutHypeMeter();
		m_pTFPlayer->m_Shared.SetScoutHypeMeter( flHype - iHypeResetsOnJump );
		m_pTFPlayer->TeamFortress_SetSpeed();
	}

	if ( m_pTFPlayer->m_Shared.GetCarryingRuneType() == RUNE_AGILITY )
	{
		flJumpMod *= 1.8f;
	}

  	float flDashZ = 268.3281572999747f * flJumpMod;

	// Get the wish direction.
	Vector vecForward, vecRight;
	AngleVectors( mv->m_vecViewAngles, &vecForward, &vecRight, NULL );
	vecForward.z = 0.0f;
	vecRight.z = 0.0f;		
	VectorNormalize( vecForward );
	VectorNormalize( vecRight );

	// Copy movement amounts
	float flForwardMove = mv->m_flForwardMove;
	float flSideMove = mv->m_flSideMove;

	// Find the direction,velocity in the x,y plane.
	Vector vecWishDirection( ( ( vecForward.x * flForwardMove ) + ( vecRight.x * flSideMove ) ),
		                     ( ( vecForward.y * flForwardMove ) + ( vecRight.y * flSideMove ) ), 
		                     0.0f );
	
	// Update the velocity on the scout.
	mv->m_vecVelocity = vecWishDirection;
	mv->m_vecVelocity.z += flDashZ;

	int iAirDash = m_pTFPlayer->m_Shared.GetAirDash();
	if ( iAirDash == 0 )
	{
#if defined(GAME_DLL)
		// Our first air jump.
		m_pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_DOUBLE_JUMP, "started_jumping:1" );
#else
		IGameEvent *event = gameeventmanager->CreateEvent( "air_dash" );
		if ( event )
		{
			event->SetInt( "player", m_pTFPlayer->GetUserID() );
			gameeventmanager->FireEventClientSide( event );
		}
#endif
	}

	m_pTFPlayer->m_Shared.SetAirDash( iAirDash+1 );

	// Play the gesture.
 	m_pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_DOUBLEJUMP );
#ifdef GAME_DLL
	// Pitch shift a sound for all airdashes greater then 1
	if ( iAirDash > 0 )
	{
		EmitSound_t params;
		params.m_pSoundName = "General.banana_slip";
		params.m_flSoundTime = 0;
		params.m_pflSoundDuration = 0;
		//params.m_bWarnOnDirectWaveReference = true;
		CPASFilter filter( m_pTFPlayer->GetAbsOrigin( ) );
		params.m_flVolume = 0.1f;
		params.m_SoundLevel = SNDLVL_25dB;
		params.m_nPitch = RemapVal( iAirDash, 1.0f, 5.0f, 100.f, 120.f );
		params.m_nFlags |= ( SND_CHANGE_PITCH | SND_CHANGE_VOL );
		m_pTFPlayer->StopSound( "General.banana_slip" );
		m_pTFPlayer->EmitSound( filter, m_pTFPlayer->entindex( ), params );
	}
#endif // GAME_DLL
}

// Only allow bunny jumping up to 1.2x server / player maxspeed setting
#define BUNNYJUMP_MAX_SPEED_FACTOR 1.2f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::PreventBunnyJumping()
{
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return;

	// Speed at which bunny jumping is limited
	float maxscaledspeed = BUNNYJUMP_MAX_SPEED_FACTOR * player->m_flMaxspeed;
	if ( maxscaledspeed <= 0.0f )
		return;

	// Current player speed
	float spd = mv->m_vecVelocity.Length();
	if ( spd <= maxscaledspeed )
		return;

	// Apply this cropping fraction to velocity
	float fraction = ( maxscaledspeed / spd );


	mv->m_vecVelocity *= fraction;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::ToggleParachute()
{
	if ( ( m_pTFPlayer->GetFlags() & FL_ONGROUND ) )
	{
		m_pTFPlayer->m_Shared.RemoveCond( TF_COND_PARACHUTE_DEPLOYED );
		return;
	}

	if ( mv->m_nOldButtons & IN_JUMP )
		return;

	// Can not add if in kart (Kart code does it for spell) but players can manually undeploy
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		if ( m_pTFPlayer->m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE ) )
		{
			m_pTFPlayer->m_Shared.RemoveCond( TF_COND_PARACHUTE_ACTIVE );
		}
		return;
	}

	// Check for Parachute and deploy / undeploy
	int iParachute = 0;
	// Passive version
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pTFPlayer, iParachute, parachute_attribute );
	if ( iParachute )
	{
		// Toggle between the conditions
		if ( m_pTFPlayer->m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE ) )
		{
			m_pTFPlayer->m_Shared.RemoveCond( TF_COND_PARACHUTE_ACTIVE );
		}
		else
		{
			int iParachuteDisabled = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pTFPlayer, iParachuteDisabled, parachute_disabled );
			if ( !iParachuteDisabled && ( tf_parachute_deploy_toggle_allowed.GetBool() || !m_pTFPlayer->m_Shared.InCond( TF_COND_PARACHUTE_DEPLOYED ) ) )
			{
				m_pTFPlayer->m_Shared.AddCond( TF_COND_PARACHUTE_ACTIVE );
				m_pTFPlayer->m_Shared.AddCond( TF_COND_PARACHUTE_DEPLOYED );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameMovement::CheckJumpButton()
{
	// Are we dead?  Then we cannot jump.
	if ( player->pl.deadflag )
		return false;

	// Check to see if we are in water.
	if ( !CheckWaterJumpButton() )
		return false;

	if ( m_pTFPlayer->GetGrapplingHookTarget() && m_pTFPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_HEAVYWEAPONS )
	{
		float flStartZ = mv->m_vecVelocity[2];
		mv->m_vecVelocity[2] += tf_grapplinghook_jump_up_speed.GetFloat();

		// Powered up flag carriers get a jump height penalty except Agility
		if ( m_pTFPlayer->m_Shared.GetCarryingRuneType() != RUNE_AGILITY && m_pTFPlayer->m_Shared.GetCarryingRuneType() != RUNE_NONE && m_pTFPlayer->HasTheFlag() )
		{
			mv->m_vecVelocity[2] *= 0.80f;
		}

		if ( mv->m_vecVelocity[2] > GetAirSpeedCap() )
			mv->m_vecVelocity[2] = GetAirSpeedCap();

		// Apply gravity.
		FinishGravity();

		mv->m_outJumpVel.z = mv->m_vecVelocity[2] - flStartZ;
		mv->m_outStepHeight += 0.15f;
		mv->m_nOldButtons |= IN_JUMP;

		return true;
	}

	// holding jump key will make ghost fly
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
	{
		float flStartZ = mv->m_vecVelocity[2];
		mv->m_vecVelocity[2] = tf_ghost_up_speed.GetFloat();

		// Apply gravity.
		FinishGravity();

		mv->m_outJumpVel.z = mv->m_vecVelocity[2] - flStartZ;
		mv->m_outStepHeight += 0.15f;
		mv->m_nOldButtons |= IN_JUMP;
		return true;
	}

	// Can I jump?
	if ( !m_pTFPlayer->CanJump() )
		return false;

	// Check to see if the player is a scout.
	bool bScout = m_pTFPlayer->GetPlayerClass()->IsClass( TF_CLASS_SCOUT );
	bool bAirDash = false;
	bool bOnGround = ( player->GetGroundEntity() != NULL );

	ToggleParachute();

	// Cannot jump will ducked.
	if ( player->GetFlags() & FL_DUCKING )
	{
		// Let a scout do it.
		bool bAllow = ( bScout && !bOnGround );

		if ( !bAllow )
			return false;
	}

	// Cannot jump while in the unduck transition.
	if ( ( player->m_Local.m_bDucking && (  player->GetFlags() & FL_DUCKING ) ) || ( player->m_Local.m_flDuckJumpTime > 0.0f ) )
		return false;

	// Cannot jump again until the jump button has been released.
	if ( mv->m_nOldButtons & IN_JUMP )
		return false;

	// In air, so ignore jumps 
	// (unless you are a scout or ghost or parachute
	if ( !bOnGround )
	{
		if ( m_pTFPlayer->CanAirDash() )
		{
			bAirDash = true;
		}
		else
		{
			mv->m_nOldButtons |= IN_JUMP;
			return false;
		}
	}

	// Check for an air dash.
	if ( bAirDash )
	{
 		AirDash();
		// Reset air duck for Scouts on AirDash.
		m_pTFPlayer->m_Shared.SetAirDucked( 0 );
		return true;
	}

	PreventBunnyJumping();

	// Start jump animation and player sound (specific TF animation and flags).
	m_pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_JUMP );
	player->PlayStepSound( (Vector &)mv->GetAbsOrigin(), player->m_pSurfaceData, 1.0, true );
	m_pTFPlayer->m_Shared.SetJumping( true );

	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		m_pTFPlayer->EmitSound( "BumperCar.Jump" );
	}

	// Set the player as in the air.
	SetGroundEntity( NULL );

	// Check the surface the player is standing on to see if it impacts jumping.
	float flGroundFactor = 1.0f;
	if ( player->m_pSurfaceData )
	{
		flGroundFactor = player->m_pSurfaceData->game.jumpFactor; 
	}

	// fMul = sqrt( 2.0 * gravity * jump_height (21.0units) ) * GroundFactor
	Assert( GetCurrentGravity() == 800.0f );

	float flJumpMod = 1.f;
	//if ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	//{
	//	flJumpMod *= 1.3f;
	//}

	// Passive version
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pTFPlayer, flJumpMod, mod_jump_height );
	// Weapon-restricted version
	CTFWeaponBase *pWpn = m_pTFPlayer->GetActiveTFWeapon();
	if ( pWpn )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWpn, flJumpMod, mod_jump_height_from_weapon );
	}
/*
#ifdef STAGING_ONLY
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_SPACE_GRAVITY ) )
	{
		flJumpMod *= tf_space_gravity_jump_multipler.GetFloat();
	}
	
#endif // STAGING_ONLY
*/
	if ( m_pTFPlayer->m_Shared.GetCarryingRuneType() == RUNE_AGILITY )
	{
		flJumpMod *= 1.8f;
	}
	
	float flMul = ( 289.0f * flJumpMod ) * flGroundFactor;

	// Save the current z velocity.
	float flStartZ = mv->m_vecVelocity[2];

	// Acclerate upward
	if ( (  player->m_Local.m_bDucking ) || (  player->GetFlags() & FL_DUCKING ) )
	{
		// If we are ducking...
		// d = 0.5 * g * t^2		- distance traveled with linear accel
		// t = sqrt(2.0 * 45 / g)	- how long to fall 45 units
		// v = g * t				- velocity at the end (just invert it to jump up that high)
		// v = g * sqrt(2.0 * 45 / g )
		// v^2 = g * g * 2.0 * 45 / g
		// v = sqrt( g * 2.0 * 45 )
		mv->m_vecVelocity[2] = flMul;  // 2 * gravity * jump_height * ground_factor
	}
	else
	{
		mv->m_vecVelocity[2] += flMul;  // 2 * gravity * jump_height * ground_factor
	}

	// Apply gravity.
	FinishGravity();

	// Save the output data for the physics system to react to if need be.
	mv->m_outJumpVel.z += mv->m_vecVelocity[2] - flStartZ;
	mv->m_outStepHeight += 0.15f;

	// Flag that we jumped and don't jump again until it is released.
	mv->m_nOldButtons |= IN_JUMP;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameMovement::CheckStuck( void )
{
	// assume we are not stuck in a player
	m_isPassingThroughEnemies = false;

	if ( tf_resolve_stuck_players.GetBool() )
	{
		const Vector &originalPos = mv->GetAbsOrigin();
		trace_t traceresult;

		TracePlayerBBox( originalPos, originalPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, traceresult );

#ifdef GAME_DLL
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && m_pTFPlayer && m_pTFPlayer->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
		{
			if ( traceresult.startsolid )
			{
				if ( m_pTFPlayer->m_playerMovementStuckTimer.HasStarted() && m_pTFPlayer->m_playerMovementStuckTimer.IsElapsed() )
				{
					DevMsg( "%3.2f: A robot is interpenetrating a solid - killed!\n", gpGlobals->curtime );

					UTIL_LogPrintf( "\"%s<%i><%s><%s>\" startsolid killed (position \"%3.2f %3.2f %3.2f\")\n",
						m_pTFPlayer->GetPlayerName(),
						m_pTFPlayer->GetUserID(),
						m_pTFPlayer->GetNetworkIDString(),
						m_pTFPlayer->GetTeam()->GetName(),
						m_pTFPlayer->GetAbsOrigin().x, m_pTFPlayer->GetAbsOrigin().y, m_pTFPlayer->GetAbsOrigin().z );

					m_pTFPlayer->TakeDamage( CTakeDamageInfo( m_pTFPlayer, m_pTFPlayer, vec3_origin, m_pTFPlayer->WorldSpaceCenter(), 999999.9f, DMG_CRUSH ) );
				}
				else
				{
					if ( traceresult.m_pEnt )
					{
						Warning( "Robot's getting stuck with %s\n", traceresult.m_pEnt->GetClassname() );
					}
				}
			}
			else
			{
				// Bot is *not* stuck right now. Continually restart timer, so if we become stuck it will count down and expire.
				const float stuckTooLongTime = 10.0f;
				m_pTFPlayer->m_playerMovementStuckTimer.Start( stuckTooLongTime );
			}
		}
#endif

		if ( traceresult.startsolid && traceresult.DidHitNonWorldEntity() )
		{
			if ( traceresult.m_pEnt->IsPlayer() )
			{
				// We are stuck in an enemy player. Don't collide with enemies until we are no longer penetrating them.
				m_isPassingThroughEnemies = true;

				// verify position is now clear
				TracePlayerBBox( originalPos, originalPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, traceresult );

				if ( !traceresult.DidHit() )
				{
					// no longer stuck
					DevMsg( "%3.2f: Resolved stuck player/player\n", gpGlobals->curtime );

					return 0;
				}
			}
			else if ( fabs( traceresult.m_pEnt->GetAbsVelocity().z ) > 0.7071f && FClassnameIs( traceresult.m_pEnt, "func_tracktrain" ) )
			{
				// we're stuck in a vertically moving tracktrain, assume flat surface normal and move us out
				SetGroundEntity( &traceresult );

				// we're stuck in a vertically moving tracktrain, snap on top of it
				const float maxAdjust = 80.0f;
				const float step = 10.0f;
				Vector tryPos;
				for( float shift = step; shift < maxAdjust; shift += step )
				{
					tryPos = mv->GetAbsOrigin();
					tryPos.z += shift;

					TracePlayerBBox( tryPos, tryPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, traceresult );
					if ( !traceresult.DidHit() || ( traceresult.m_pEnt && traceresult.m_pEnt->IsPlayer() ) )
					{
						// no longer stuck
						mv->SetAbsOrigin( tryPos );

						DevMsg( "%3.2f: Forced stuck player to top of func_tracktrain\n", gpGlobals->curtime );

						return 0;
					}
				}
			}
		}
	}

	return BaseClass::CheckStuck();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameMovement::CheckWater( void )
{
	Vector vecPlayerMin = GetPlayerMins();
	Vector vecPlayerMax = GetPlayerMaxs();

	Vector vecPoint( ( mv->GetAbsOrigin().x + ( vecPlayerMin.x + vecPlayerMax.x ) * 0.5f ),
				     ( mv->GetAbsOrigin().y + ( vecPlayerMin.y + vecPlayerMax.y ) * 0.5f ),
				     ( mv->GetAbsOrigin().z + vecPlayerMin.z + 1 ) );


	// Assume that we are not in water at all.
	int wl = WL_NotInWater;
	int wt = CONTENTS_EMPTY;

	// Check to see if our feet are underwater.
	int nContents = GetPointContentsCached( vecPoint, 0 );	
	if ( nContents & MASK_WATER )
	{
		// Clear our jump flag, because we have landed in water.
		m_pTFPlayer->m_Shared.SetJumping( false );

		// Set water type and level.
		wt = nContents;
		wl = WL_Feet;

		float flWaistZ = mv->GetAbsOrigin().z + ( vecPlayerMin.z + vecPlayerMax.z ) * 0.5f + 12.0f;

		// Now check eyes
		vecPoint.z = mv->GetAbsOrigin().z + player->GetViewOffset()[2];
		nContents = GetPointContentsCached( vecPoint, 1 );
		if ( nContents & MASK_WATER )
		{
			// In over our eyes
			wl = WL_Eyes;  
			VectorCopy( vecPoint, m_vecWaterPoint );
			m_vecWaterPoint.z = flWaistZ;
		}
		else
		{
			// Now check a point that is at the player hull midpoint (waist) and see if that is underwater.
			vecPoint.z = flWaistZ;
			nContents = GetPointContentsCached( vecPoint, 2 );
			if ( nContents & MASK_WATER )
			{
				// Set the water level at our waist.
				wl = WL_Waist;
				VectorCopy( vecPoint, m_vecWaterPoint );
			}
		}
	}

	// force player to be under water
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_SWIMMING_CURSE ) )
	{
		wl = WL_Eyes;
	}

	player->SetWaterLevel( wl );
	player->SetWaterType( wt );

	// If we just transitioned from not in water to water, record the time for splashes, etc.
	if ( ( WL_NotInWater == m_nOldWaterLevel ) && ( wl > WL_NotInWater ) )
	{
		m_flWaterEntryTime = gpGlobals->curtime;
	}
#ifdef GAME_DLL
	else if ( ( WL_NotInWater == wl ) && ( m_nOldWaterLevel > WL_NotInWater ) )
	{
		m_pTFPlayer->SetWaterExitTime( gpGlobals->curtime );
	}
#endif

	if ( m_nOldWaterLevel != wl )
	{
		m_pTFPlayer->TeamFortress_SetSpeed();
	}

	return ( wl > WL_Feet );
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::WaterMove( void )
{
	float	wishspeed;
	Vector	wishdir;
	Vector	start, dest;
	Vector  temp;
	trace_t	pm;
	float speed, newspeed, addspeed, accelspeed;

	// Determine movement angles.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( mv->m_vecViewAngles, &vecForward, &vecRight, &vecUp );

	// Calculate the desired direction and speed.
	Vector vecWishVelocity;
	for ( int iAxis = 0 ; iAxis < 3; ++iAxis )
	{
		vecWishVelocity[iAxis] = ( vecForward[iAxis] * mv->m_flForwardMove ) + ( vecRight[iAxis] * mv->m_flSideMove );
	}

	// if you can't swim just sink instead
	int iCannotSwim = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pTFPlayer, iCannotSwim, cannot_swim );
	if ( iCannotSwim )
	{
		vecWishVelocity[0] *= 0.1;
		vecWishVelocity[1] *= 0.1;
		vecWishVelocity[2] = -60;
	}
	// Check for upward velocity (JUMP).
	else if ( mv->m_nButtons & IN_JUMP )
	{
		if ( player->GetWaterLevel() == WL_Eyes )
		{
			vecWishVelocity[2] += mv->m_flClientMaxSpeed;
		}
	}
	// Sinking if not moving.
	else if ( !mv->m_flForwardMove && !mv->m_flSideMove && !mv->m_flUpMove )
	{
		vecWishVelocity[2] -= 60;
	}
	// Move up based on view angle.
	else
	{
		vecWishVelocity[2] += mv->m_flUpMove;
	}

	// Copy it over and determine speed
	VectorCopy( vecWishVelocity, wishdir );
	wishspeed = VectorNormalize( wishdir );

	// Cap speed.
	if (wishspeed > mv->m_flMaxSpeed)
	{
		VectorScale( vecWishVelocity, mv->m_flMaxSpeed/wishspeed, vecWishVelocity );
		wishspeed = mv->m_flMaxSpeed;
	}

	// Slow us down a bit.
	int iSwimmingMastery = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pTFPlayer, iSwimmingMastery, swimming_mastery );
	if ( iSwimmingMastery == 0 )
	{
		wishspeed *= 0.8;
	}
	
	// Water friction
	VectorCopy( mv->m_vecVelocity, temp );
	speed = VectorNormalize( temp );
	if ( speed )
	{
		newspeed = speed - gpGlobals->frametime * speed * sv_friction.GetFloat() * player->m_surfaceFriction;
		if ( newspeed < 0.1f )
		{
			newspeed = 0;
		}

		VectorScale (mv->m_vecVelocity, newspeed/speed, mv->m_vecVelocity);
	}
	else
	{
		newspeed = 0;
	}

	// water acceleration
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
	{
		VectorNormalize(vecWishVelocity);
		accelspeed = sv_accelerate.GetFloat() * wishspeed * gpGlobals->frametime * player->m_surfaceFriction;
		for ( int i = 0; i < 3; i++)
		{
			float deltaSpeed = accelspeed * vecWishVelocity[i];
			mv->m_vecVelocity[i] += deltaSpeed;
			mv->m_outWishVel[i] += deltaSpeed;
		}

		float flGhostXYSpeed = mv->m_vecVelocity.Length2D();
		if ( flGhostXYSpeed > tf_ghost_xy_speed.GetFloat() )
		{
			float flGhostXYSpeedScale = tf_ghost_xy_speed.GetFloat() / flGhostXYSpeed;
			mv->m_vecVelocity.x *= flGhostXYSpeedScale;
			mv->m_vecVelocity.y *= flGhostXYSpeedScale;
		}
	}
	else if (wishspeed >= 0.1f)  // old !
	{
		addspeed = wishspeed - newspeed;
		if (addspeed > 0)
		{
			VectorNormalize(vecWishVelocity);
			accelspeed = sv_accelerate.GetFloat() * wishspeed * gpGlobals->frametime * player->m_surfaceFriction;
			if (accelspeed > addspeed)
			{
				accelspeed = addspeed;
			}

			for ( int i = 0; i < 3; i++)
			{
				float deltaSpeed = accelspeed * vecWishVelocity[i];
				mv->m_vecVelocity[i] += deltaSpeed;
				mv->m_outWishVel[i] += deltaSpeed;
			}
		}
	}

	VectorAdd (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

	// Now move
	// assume it is a stair or a slope, so press down from stepheight above
	VectorMA (mv->GetAbsOrigin(), gpGlobals->frametime, mv->m_vecVelocity, dest);
	
	TracePlayerBBox( mv->GetAbsOrigin(), dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
	if ( pm.fraction == 1.0f )
	{
		VectorCopy( dest, start );
		if ( player->m_Local.m_bAllowAutoMovement )
		{
			start[2] += player->m_Local.m_flStepSize + 1;
		}
		
		TracePlayerBBox( start, dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );

		if (!pm.startsolid && !pm.allsolid)
		{	
#if 0
			float stepDist = pm.endpos.z - mv->GetAbsOrigin().z;
			mv->m_outStepHeight += stepDist;
			// walked up the step, so just keep result and exit

			Vector vecNewWaterPoint;
			VectorCopy( m_vecWaterPoint, vecNewWaterPoint );
			vecNewWaterPoint.z += ( dest.z - mv->GetAbsOrigin().z );
			bool bOutOfWater = !( enginetrace->GetPointContents( vecNewWaterPoint ) & MASK_WATER );
			if ( bOutOfWater && ( mv->m_vecVelocity.z > 0.0f ) && ( pm.fraction == 1.0f )  )
			{
				// Check the waist level water positions.
				trace_t traceWater;
				UTIL_TraceLine( vecNewWaterPoint, m_vecWaterPoint, CONTENTS_WATER, player, COLLISION_GROUP_NONE, &traceWater );
				if( traceWater.fraction < 1.0f )
				{
					float flFraction = 1.0f - traceWater.fraction;

//					Vector vecSegment;
//					VectorSubtract( mv->GetAbsOrigin(), dest, vecSegment );
//					VectorMA( mv->GetAbsOrigin(), flFraction, vecSegment, mv->GetAbsOrigin() );
					float flZDiff = dest.z - mv->GetAbsOrigin().z;
					float flSetZ = mv->GetAbsOrigin().z + ( flFraction * flZDiff );
					flSetZ -= 0.0325f;

					VectorCopy (pm.endpos, mv->GetAbsOrigin());
					mv->GetAbsOrigin().z = flSetZ;
					VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
					mv->m_vecVelocity.z = 0.0f;
				}

			}
			else
			{
				VectorCopy (pm.endpos, mv->GetAbsOrigin());
				VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
			}

			return;
#endif
			float stepDist = pm.endpos.z - mv->GetAbsOrigin().z;
			mv->m_outStepHeight += stepDist;
			// walked up the step, so just keep result and exit
			mv->SetAbsOrigin( pm.endpos );
			VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
			return;
		}

		// Try moving straight along out normal path.
		TryPlayerMove();
	}
	else
	{
		if ( !player->GetGroundEntity() )
		{
			TryPlayerMove();
			VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
			return;
		}

		StepMove( dest, pm );
	}
	
	VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::WalkMove( void )
{
	// Get the movement angles.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( mv->m_vecViewAngles, &vecForward, &vecRight, &vecUp );
	vecForward.z = 0.0f;
	vecRight.z = 0.0f;
	VectorNormalize( vecForward );
	VectorNormalize( vecRight );

	// Copy movement amounts
	float flForwardMove = mv->m_flForwardMove;
	float flSideMove = mv->m_flSideMove;

	// Find the direction,velocity in the x,y plane.
	Vector vecWishDirection( ( ( vecForward.x * flForwardMove ) + ( vecRight.x * flSideMove ) ),
		                     ( ( vecForward.y * flForwardMove ) + ( vecRight.y * flSideMove ) ),
							 0.0f );

	// Calculate the speed and direction of movement, then clamp the speed.
	float flWishSpeed = VectorNormalize( vecWishDirection );
	flWishSpeed = clamp( flWishSpeed, 0.0f, mv->m_flMaxSpeed );

	// Accelerate in the x,y plane.
	mv->m_vecVelocity.z = 0;

	float flAccelerate = sv_accelerate.GetFloat();
	float flFriction = sv_friction.GetFloat() * player->m_surfaceFriction;

	float flWishSpeedThreshold = 100.0f * flFriction / sv_accelerate.GetFloat();

	// if our wish speed is too low (attributes), we must increase acceleration or we'll never overcome friction
	// Reverse the basic friction calculation to find our required acceleration
	if ( flWishSpeed > 0 && flWishSpeed < flWishSpeedThreshold )
	{
		// Lost footing should not have the ability to gain bonus traction
		if ( !m_pTFPlayer->m_Shared.InCond( TF_COND_LOST_FOOTING ) )
		{
			// accelspeed = accel * gpGlobals->frametime * wishspeed * player->m_surfaceFriction;
			// accelspeed > drop;
			// drop = accel * frametime * wish * plFriction
			// accel > drop / (wish * gametime * plFriction)
			//		drop = control * (plFriction * sv_friction) * gameTime;
			// accel > control * sv_friction / wish
			float flSpeed = VectorLength( mv->m_vecVelocity );
			float flControl = (flSpeed < sv_stopspeed.GetFloat()) ? sv_stopspeed.GetFloat() : flSpeed;
			flAccelerate = (flControl * flFriction) / flWishSpeed + 1;
		}
	}

	Accelerate( vecWishDirection, flWishSpeed, flAccelerate );
	Assert( mv->m_vecVelocity.z == 0.0f );

	float flAdjustedMaxSpeed = /* ( m_pTFPlayer->m_Shared.GetAmountStunned( TF_STUN_MOVEMENT ) ) ? 
		mv->m_flMaxSpeed *= ( 1.f - m_pTFPlayer->m_Shared.GetAmountStunned( TF_STUN_MOVEMENT ) ) : 
		*/ mv->m_flMaxSpeed;

	// Clamp the players speed in x,y.
	float flNewSpeed = VectorLength( mv->m_vecVelocity );
	if ( flNewSpeed > flAdjustedMaxSpeed )
	{
		float flScale = ( flAdjustedMaxSpeed / flNewSpeed );
		mv->m_vecVelocity.x *= flScale;
		mv->m_vecVelocity.y *= flScale;
	}

	float flForwardPull = m_pTFPlayer->GetMovementForwardPull();

	if ( flForwardPull > 0.0f )
	{
		mv->m_vecVelocity += vecForward * flForwardPull;

		if ( mv->m_vecVelocity.Length2D() > flAdjustedMaxSpeed )
		{
			VectorNormalize( mv->m_vecVelocity );
			mv->m_vecVelocity *= flAdjustedMaxSpeed;
		}
	}

	// Now reduce their backwards speed to some percent of max, if they are traveling backwards
	// unless they are under some minimum, to not penalize deployed snipers or heavies
	if ( tf_clamp_back_speed.GetFloat() < 1.0 && VectorLength( mv->m_vecVelocity ) > tf_clamp_back_speed_min.GetFloat() )
	{
		float flDot = DotProduct( vecForward, mv->m_vecVelocity );

		// are we moving backwards at all?
		if ( flDot < 0 )
		{
			Vector vecBackMove = vecForward * flDot;
			Vector vecRightMove = vecRight * DotProduct( vecRight, mv->m_vecVelocity );

			// clamp the back move vector if it is faster than max
			float flBackSpeed = VectorLength( vecBackMove );
			float flMaxBackSpeed = ( flAdjustedMaxSpeed * tf_clamp_back_speed.GetFloat() );

			if ( flBackSpeed > flMaxBackSpeed )
			{
				vecBackMove *= flMaxBackSpeed / flBackSpeed;
			}
			
			// reassemble velocity	
			mv->m_vecVelocity = vecBackMove + vecRightMove;

			// Re-run this to prevent crazy values (clients can induce this via usercmd viewangles hacking)
			flNewSpeed = VectorLength( mv->m_vecVelocity );
			if ( flNewSpeed > flAdjustedMaxSpeed )
			{
				float flScale = ( flAdjustedMaxSpeed / flNewSpeed );
				mv->m_vecVelocity.x *= flScale;
				mv->m_vecVelocity.y *= flScale;
			}
		}
	}

	// Add base velocity to the player's current velocity - base velocity = velocity from conveyors, etc.
	VectorAdd( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

	// Calculate the current speed and return if we are not really moving.
	float flSpeed = VectorLength( mv->m_vecVelocity );
	if ( flSpeed < 1.0f )
	{
		// I didn't remove the base velocity here since it wasn't moving us in the first place.
		mv->m_vecVelocity.Init();
		return;
	}

	// Calculate the destination.
	Vector vecDestination;
	vecDestination.x = mv->GetAbsOrigin().x + ( mv->m_vecVelocity.x * gpGlobals->frametime );
	vecDestination.y = mv->GetAbsOrigin().y + ( mv->m_vecVelocity.y * gpGlobals->frametime );	
	vecDestination.z = mv->GetAbsOrigin().z;

#ifdef GAME_DLL
	// allow bot to approve position change for intentional movement
	INextBot *bot = player->MyNextBotPointer();
	if ( bot && bot->GetIntentionInterface()->IsPositionAllowed( bot, vecDestination ) == ANSWER_NO )
	{
		// rejected - stay put
		return;
	}
#endif

	// Try moving to the destination.
	trace_t trace;
	TracePlayerBBox( mv->GetAbsOrigin(), vecDestination, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	if ( trace.fraction == 1.0f )
	{
		// Made it to the destination (remove the base velocity).
		mv->SetAbsOrigin( trace.endpos );
		VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

		// Save the wish velocity.
		mv->m_outWishVel += ( vecWishDirection * flWishSpeed );

		// Try and keep the player on the ground.
		// NOTE YWB 7/5/07: Don't do this here, our version of CategorizePosition encompasses this test
		// StayOnGround();

#ifdef CLIENT_DLL
		// Track how far we moved (if we're a Scout or an Engineer carrying a building).
		CTFPlayer* pTFPlayer = ToTFPlayer( player );
		if ( pTFPlayer->IsPlayerClass( TF_CLASS_SCOUT ) ||
			( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) && pTFPlayer->m_Shared.IsCarryingObject() ) )
		{
			float fInchesToMeters = 0.0254f;
			float fWorldScale = 0.25;
			float fMeters = pTFPlayer->GetMetersRan();
			float fMetersRan = flSpeed*fInchesToMeters*fWorldScale*gpGlobals->frametime;
			pTFPlayer->SetMetersRan( fMeters + fMetersRan, gpGlobals->framecount );
		}
#endif
		return;
	}

	CTFPlayer* pBumpPlayer = ToTFPlayer( trace.m_pEnt );
	if ( pBumpPlayer )
	{
		m_pTFPlayer->m_Shared.EndCharge();
	}

	// Now try and do a step move.
	StepMove( vecDestination, trace );

	// Remove base velocity.
	Vector baseVelocity = player->GetBaseVelocity();
	VectorSubtract( mv->m_vecVelocity, baseVelocity, mv->m_vecVelocity );

	CheckKartWallBumping();

	// Save the wish velocity.
	mv->m_outWishVel += ( vecWishDirection * flWishSpeed );

	// Try and keep the player on the ground.
	// NOTE YWB 7/5/07: Don't do this here, our version of CategorizePosition encompasses this test
	// StayOnGround();

#if 0
	// Debugging!!!
	Vector vecTestVelocity = mv->m_vecVelocity;
	vecTestVelocity.z = 0.0f;
	float flTestSpeed = VectorLength( vecTestVelocity );
	if ( baseVelocity.IsZero() && ( flTestSpeed > ( flAdjustedMaxSpeed + 1.0f ) ) )
	{
		Msg( "Step Max Speed < %f\n", flTestSpeed );
	}

	if ( tf_showspeed.GetBool() )
	{
		Msg( "Speed=%f\n", flTestSpeed );
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::CheckKartWallBumping()
{
	// Karts need to drop their velocity when they bump into things
	if ( !m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return;

	const float flCurrentSpeed = m_pTFPlayer->GetCurrentTauntMoveSpeed();
	const float flMaxSpeed = mv->m_vecVelocity.Length();
	const float flClampedSpeed = clamp( flCurrentSpeed, -flMaxSpeed, flMaxSpeed );

	m_pTFPlayer->SetCurrentTauntMoveSpeed( flClampedSpeed );
	// We hit a wall at a good speed
	if ( fabs( flCurrentSpeed ) > 100.f && ( flCurrentSpeed - flClampedSpeed > 100.f ) )
	{
		// Play a flinch to show we impacted something
		bool bDashing = m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART_DASH );
		m_pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, bDashing ? ACT_KART_IMPACT_BIG : ACT_KART_IMPACT );

		Vector vAim = m_pTFPlayer->GetLocalVelocity();
		vAim.z = 0;
		vAim.NormalizeInPlace();

		// Handle hitting skybox (disappear).
		trace_t pWallTrace;
		UTIL_TraceLine( m_pTFPlayer->GetAbsOrigin(), m_pTFPlayer->GetAbsOrigin() + vAim * 64, MASK_SOLID, m_pTFPlayer, COLLISION_GROUP_DEBRIS, &pWallTrace );

		// if we collide with a wall that is 90degrees or higher, bump backwards
		if ( pWallTrace.fraction < 1.0 && !( pWallTrace.surface.flags & SURF_SKY ) && pWallTrace.m_pEnt && !pWallTrace.m_pEnt->IsPlayer() && pWallTrace.plane.normal.z <= 0 )
		{
#ifdef GAME_DLL
			// Bounce off the wall, deflect in the direction of the normal of the surface that we collided with
			Vector vOld = m_pTFPlayer->GetLocalVelocity();
			Vector vNew = ( -2.0f * pWallTrace.plane.normal.Dot( vOld ) * pWallTrace.plane.normal + vOld );
			vNew.NormalizeInPlace();
			m_pTFPlayer->AddHalloweenKartPushEvent( m_pTFPlayer, NULL, NULL, vNew * vOld.Length() / 2.0f, 0 );
			if ( bDashing )
			{
				// Stop moving
				m_pTFPlayer->SetAbsVelocity( vec3_origin );
				m_pTFPlayer->SetCurrentTauntMoveSpeed( 0 );
				m_pTFPlayer->m_Shared.RemoveCond( TF_COND_HALLOWEEN_KART_DASH );
			}

			m_pTFPlayer->SetCurrentTauntMoveSpeed( 0.f );
#endif

#ifdef CLIENT_DLL
			if ( bDashing )
			{
				m_pTFPlayer->EmitSound( "BumperCar.BumpHard" );
				m_pTFPlayer->ParticleProp()->Create( "kart_impact_sparks", PATTACH_ABSORIGIN, NULL, vAim );
			}
			else
			{
				m_pTFPlayer->EmitSound( "BumperCar.Bump" );
				m_pTFPlayer->ParticleProp()->Create( "kart_impact_sparks", PATTACH_ABSORIGIN, NULL, vAim );
			}
#endif		
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGameMovement::GetAirSpeedCap( void )
{
	if ( m_pTFPlayer->GetGrapplingHookTarget() )
	{
		if ( m_pTFPlayer->m_Shared.GetCarryingRuneType() == RUNE_AGILITY )
		{
			switch ( m_pTFPlayer->GetPlayerClass()->GetClassIndex() )
			{
			case TF_CLASS_SOLDIER:
			case TF_CLASS_HEAVYWEAPONS:
				return 850.f;
			default:
				return 950.f;
			}
		}

		return tf_grapplinghook_move_speed.GetFloat();
	}
	else if ( m_pTFPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		return tf_max_charge_speed.GetFloat();
	}
	else
	{
		float flCap = BaseClass::GetAirSpeedCap();

/*
#ifdef STAGING_ONLY
		if ( m_pTFPlayer->m_Shared.InCond( TF_COND_SPACE_GRAVITY ) )
		{
			flCap *= tf_space_aircontrol.GetFloat();
		}
#endif
*/
		if ( m_pTFPlayer->m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE ) )
		{
			flCap *= tf_parachute_aircontrol.GetFloat();
		}

		if ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		{
			if ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART_DASH ) )
			{
				return tf_halloween_kart_dash_speed.GetFloat();
			}
			flCap *= tf_halloween_kart_aircontrol.GetFloat();
		}

		float flIncreasedAirControl = 1.f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pTFPlayer, flIncreasedAirControl, mod_air_control );

		if ( m_pTFPlayer->m_Shared.InCond( TF_COND_BLASTJUMPING ) )
		{
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pTFPlayer, flIncreasedAirControl, mod_air_control_blast_jump );
		}

		if ( m_pTFPlayer->m_Shared.InCond( TF_COND_ROCKETPACK ) )
		{
			flCap *= 0.5f;
		}

		return ( flCap * flIncreasedAirControl );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::AirMove( void )
{
	// check if grappling move should do step move
	if ( m_pTFPlayer->GetGrapplingHookTarget() )
	{
		// Try moving to the destination.
		Vector vecDestination = mv->GetAbsOrigin() + ( mv->m_vecVelocity * gpGlobals->frametime );
		trace_t trace;
		TracePlayerBBox( mv->GetAbsOrigin(), vecDestination, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
		if ( trace.fraction != 1.f )
		{
			StepMove( vecDestination, trace );
			return;
		}
	}

	int			i;
	Vector		wishvel;
	float		fmove, smove;
	Vector		wishdir;
	float		wishspeed;
	Vector forward, right, up;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;

	// Zero out z components of movement vectors
	forward[2] = 0;
	right[2]   = 0;
	VectorNormalize(forward);  // Normalize remainder of vectors
	VectorNormalize(right);    // 

	for (i=0 ; i<2 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] = 0;             // Zero out z part of velocity

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if ( wishspeed != 0 && (wishspeed > mv->m_flMaxSpeed))
	{
		VectorScale (wishvel, mv->m_flMaxSpeed/wishspeed, wishvel);
		wishspeed = mv->m_flMaxSpeed;
	}

	float flAirAccel = sv_airaccelerate.GetFloat();
	float flWallSlideCoeff = 0.f;
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_AIR_CURRENT ) )
	{
		flAirAccel *= tf_movement_aircurrent_aircontrol_mult.GetFloat();
		flWallSlideCoeff = Clamp( 1.f - tf_movement_aircurrent_friction_mult.GetFloat(), 0.f, 1.f );
	}
/*
#ifdef STAGING_ONLY
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_SPACE_GRAVITY ) )
	{
		flAirAccel *= tf_space_aircontrol.GetFloat();
	}
#endif
*/
	AirAccelerate( wishdir, wishspeed, flAirAccel );

	float flForwardPull = m_pTFPlayer->GetMovementForwardPull();

	if ( flForwardPull > 0.0f )
	{
		mv->m_vecVelocity += forward * flForwardPull;

		if ( mv->m_vecVelocity.Length2D() > mv->m_flMaxSpeed )
		{
			float flZ = mv->m_vecVelocity.z;
			mv->m_vecVelocity.z = 0.0f;
			VectorNormalize( mv->m_vecVelocity );
			mv->m_vecVelocity *= mv->m_flMaxSpeed;
			mv->m_vecVelocity.z = flZ;
		}
	}

	// Add in any base velocity to the current velocity.
	VectorAdd( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

	int iBlocked = TryPlayerMove( NULL, NULL, flWallSlideCoeff );

	// TryPlayerMove uses '2' to indictate wall colision wtf
	if ( iBlocked & 2 )
	{
		CheckKartWallBumping();
	}

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
}

extern void TracePlayerBBoxForGround( const Vector& start, const Vector& end, const Vector& minsSrc,
							  const Vector& maxsSrc, IHandleEntity *player, unsigned int fMask,
							  int collisionGroup, trace_t& pm );


//-----------------------------------------------------------------------------
// This filter checks against buildable objects.
//-----------------------------------------------------------------------------
class CTraceFilterObject : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterObject, CTraceFilterSimple );

	CTraceFilterObject( const IHandleEntity *passentity, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
};

CTraceFilterObject::CTraceFilterObject( const IHandleEntity *passentity, int collisionGroup ) :
BaseClass( passentity, collisionGroup )
{

}

bool CTraceFilterObject::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CBaseEntity *pMe = const_cast< CBaseEntity * >( EntityFromEntityHandle( GetPassEntity() ) );
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );

	if ( pEntity )
	{
	
		if ( pEntity->IsBaseObject() )
		{
			CBaseObject *pObject = assert_cast<CBaseObject *>( pEntity );
			if ( pObject && pObject->GetOwner() == pMe )
			{
#ifdef GAME_DLL
				// engineer-bots should not collide with their buildables to avoid nasty pathing issues
				CTFPlayer *pOwner = ToTFPlayer( pMe );
				if ( pOwner->IsBotOfType( TF_BOT_TYPE ) )
				{
					bool bHitObjectType = pObject->GetType() == OBJ_SENTRYGUN || pObject->GetType() == OBJ_DISPENSER;
					if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
					{
						bHitObjectType |= pObject->GetType() == OBJ_TELEPORTER;
					}

					if ( bHitObjectType )
					{
						// engineer bots not blocked by sentries or dispensers
						return false;
					}
				}
#endif
				// my buildings are solid to me
				return true;
			}
		}
#ifdef GAME_DLL
		else if ( pEntity->IsPlayer() )
		{
			if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
			{
				CTFBot *bot = ToTFBot( pEntity );

				if ( bot && ( bot->HasMission( CTFBot::MISSION_DESTROY_SENTRIES ) || bot->HasMission( CTFBot::MISSION_REPROGRAMMED ) ) )
				{
					// Don't collide with sentry busters since they don't collide with us
					return false;
				}

				CTFBot *meBot = ToTFBot( pMe );

				if ( meBot && ( meBot->HasMission( CTFBot::MISSION_DESTROY_SENTRIES ) || meBot->HasMission( CTFBot::MISSION_REPROGRAMMED ) ) )
				{
					// Sentry Busters don't collide with enemies (so they can't be body-blocked)
					return false;
				}
			}
		}
		else if ( pEntity->MyNextBotPointer() && !pEntity->MyNextBotPointer()->GetLocomotionInterface()->ShouldCollideWith( pMe ) )
		{
			return false;
		}
#endif
	}

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseHandle CTFGameMovement::TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm )
{
	if( tf_solidobjects.GetBool() == false )
		return BaseClass::TestPlayerPosition( pos, collisionGroup, pm );

	Ray_t ray;
	ray.Init( pos, pos, GetPlayerMins(), GetPlayerMaxs() );
	
	CTraceFilterObject traceFilter( mv->m_nPlayerHandle.Get(), collisionGroup );
	enginetrace->TraceRay( ray, PlayerSolidMask(), &traceFilter, &pm );

	if ( (pm.contents & PlayerSolidMask()) && pm.m_pEnt )
	{
		return pm.m_pEnt->GetRefEHandle();
	}
	else
	{	
		return INVALID_EHANDLE;
	}
}

//-----------------------------------------------------------------------------
// Traces player movement + position
//-----------------------------------------------------------------------------
void CTFGameMovement::TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm )
{
	if( tf_solidobjects.GetBool() == false )
		return BaseClass::TracePlayerBBox( start, end, fMask, collisionGroup, pm );

	Ray_t ray;
	ray.Init( start, end, GetPlayerMins(), GetPlayerMaxs() );
	
	CTraceFilterObject traceFilter( mv->m_nPlayerHandle.Get(), collisionGroup );

	enginetrace->TraceRay( ray, fMask, &traceFilter, &pm );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &input - 
//-----------------------------------------------------------------------------
void CTFGameMovement::CategorizePosition( void )
{
	// Observer.
	if ( player->IsObserver() )
		return;

	// Reset this each time we-recategorize, otherwise we have bogus friction when we jump into water and plunge downward really quickly
	player->m_surfaceFriction = 1.0f;

	// Doing this before we move may introduce a potential latency in water detection, but
	// doing it after can get us stuck on the bottom in water if the amount we move up
	// is less than the 1 pixel 'threshold' we're about to snap to.	Also, we'll call
	// this several times per frame, so we really need to avoid sticking to the bottom of
	// water on each call, and the converse case will correct itself if called twice.
	CheckWater();

	// If standing on a ladder we are not on ground.
	if ( player->GetMoveType() == MOVETYPE_LADDER )
	{
		SetGroundEntity( NULL );
		return;
	}

	// Check for a jump.
	if ( mv->m_vecVelocity.z > 250.0f )
	{
#if defined(GAME_DLL)
		if ( m_pTFPlayer->m_bTakenBlastDamageSinceLastMovement )
		{
			m_pTFPlayer->SetBlastJumpState( TF_PLAYER_ENEMY_BLASTED_ME );
		}
#endif

		SetGroundEntity( NULL );
		return;
	}

	// Calculate the start and end position.
	Vector vecStartPos = mv->GetAbsOrigin();
	Vector vecEndPos( mv->GetAbsOrigin().x, mv->GetAbsOrigin().y, ( mv->GetAbsOrigin().z - 2.0f ) );

	// NOTE YWB 7/5/07:  Since we're already doing a traceline here, we'll subsume the StayOnGround (stair debouncing) check into the main traceline we do here to see what we're standing on
	bool bUnderwater = ( player->GetWaterLevel() >= WL_Eyes );
	bool bMoveToEndPos = false;
	if ( player->GetMoveType() == MOVETYPE_WALK && 
		player->GetGroundEntity() != NULL && !bUnderwater )
	{
		// if walking and still think we're on ground, we'll extend trace down by stepsize so we don't bounce down slopes
		vecEndPos.z -= player->GetStepSize();
		bMoveToEndPos = true;
	}

	trace_t trace;
	TracePlayerBBox( vecStartPos, vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );

	bool bInAir = false;
	float flGroundFrictionMult = 1.f;
	float flAirFrictionMult = 1.f;
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_AIR_CURRENT ) )
	{
		flAirFrictionMult *= tf_movement_aircurrent_friction_mult.GetFloat();
	}

	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_LOST_FOOTING ) )
	{
		// If we have the lost footing condition, allow any away-from-ground velocity to make us airborn
		float flAwayFromGround = DotProduct( mv->m_vecVelocity, trace.plane.normal );
		if ( flAwayFromGround > 0.f )
		{
			bInAir = true;
		}
		else
		{
			if ( ( mv->m_vecVelocity - ( trace.plane.normal * flAwayFromGround ) ).Length() >= tf_movement_lost_footing_restick.GetFloat() )
			{
				// Sliding
				flGroundFrictionMult *= tf_movement_lost_footing_friction.GetFloat();
			}
			else
			{
				// Not moving fast enough and not moving away from ground normal, regain footing
				m_pTFPlayer->m_Shared.RemoveCond( TF_COND_LOST_FOOTING );
			}
		}
	}

	// Steep plane, not on ground.
	if ( !bInAir && trace.plane.normal.z < 0.7f )
	{
		// Test four sub-boxes, to see if any of them would have found shallower slope we could actually stand on.
		TracePlayerBBoxForGround( vecStartPos, vecEndPos, GetPlayerMins(), GetPlayerMaxs(), mv->m_nPlayerHandle.Get(), PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );

		if ( trace.plane.normal[2] < 0.7f )
		{
			// Too steep.
			bInAir = true;
			if ( ( mv->m_vecVelocity.z > 0.0f ) &&
				( player->GetMoveType() != MOVETYPE_NOCLIP ) )
			{
				player->m_surfaceFriction = 0.25f;
			}
		}
	}
	else if ( !bInAir )
	{
		// YWB:  This logic block essentially lifted from StayOnGround implementation
		if ( bMoveToEndPos &&
			!trace.startsolid &&				// not sure we need this check as fraction would == 0.0f?
			trace.fraction > 0.0f &&			// must go somewhere
			trace.fraction < 1.0f ) 			// must hit something
		{
			float flDelta = fabs( mv->GetAbsOrigin().z - trace.endpos.z );
			// HACK HACK:  The real problem is that trace returning that strange value 
			//  we can't network over based on bit precision of networking origins
			if ( flDelta > 0.5f * COORD_RESOLUTION )
			{
				Vector org = mv->GetAbsOrigin();
				org.z = trace.endpos.z;
				mv->SetAbsOrigin( org );
			}
		}
	}

	SetGroundEntity( bInAir ? NULL : &trace );
	player->m_surfaceFriction *= bInAir ? flAirFrictionMult : flGroundFrictionMult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::CheckWaterJump( void )
{
	Vector	flatforward;
	Vector	flatvelocity;
	float curspeed;

	// Jump button down?
	bool bJump = ( ( mv->m_nButtons & IN_JUMP ) != 0 );

	Vector forward, right;
	AngleVectors( mv->m_vecViewAngles, &forward, &right, NULL );  // Determine movement angles

	// Already water jumping.
	if (player->m_flWaterJumpTime)
		return;

	// Don't hop out if we just jumped in
	if (mv->m_vecVelocity[2] < -180)
		return; // only hop out if we are moving up

	// See if we are backing up
	flatvelocity[0] = mv->m_vecVelocity[0];
	flatvelocity[1] = mv->m_vecVelocity[1];
	flatvelocity[2] = 0;

	// Must be moving
	curspeed = VectorNormalize( flatvelocity );
	
#if 1
	// Copy movement amounts
	float fmove = mv->m_flForwardMove;
	float smove = mv->m_flSideMove;

	for ( int iAxis = 0; iAxis < 2; ++iAxis )
	{
		flatforward[iAxis] = forward[iAxis] * fmove + right[iAxis] * smove;
	}
#else
	// see if near an edge
	flatforward[0] = forward[0];
	flatforward[1] = forward[1];
#endif
	flatforward[2] = 0;
	VectorNormalize( flatforward );

	// Are we backing into water from steps or something?  If so, don't pop forward
	if ( curspeed != 0.0 && ( DotProduct( flatvelocity, flatforward ) < 0.0 ) && !bJump )
		return;

	Vector vecStart;
	// Start line trace at waist height (using the center of the player for this here)
 	vecStart = mv->GetAbsOrigin() + (GetPlayerMins() + GetPlayerMaxs() ) * 0.5;

	Vector vecEnd;
	VectorMA( vecStart, TF_WATERJUMP_FORWARD/*tf_waterjump_forward.GetFloat()*/, flatforward, vecEnd );
	
	trace_t tr;
	TracePlayerBBox( vecStart, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr );
	if ( tr.fraction < 1.0 )		// solid at waist
	{
		IPhysicsObject *pPhysObj = tr.m_pEnt->VPhysicsGetObject();
		if ( pPhysObj )
		{
			if ( pPhysObj->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
				return;
		}

		vecStart.z = mv->GetAbsOrigin().z + player->GetViewOffset().z + WATERJUMP_HEIGHT; 
		VectorMA( vecStart, TF_WATERJUMP_FORWARD/*tf_waterjump_forward.GetFloat()*/, flatforward, vecEnd );
		VectorMA( vec3_origin, -50.0f, tr.plane.normal, player->m_vecWaterJumpVel );

		TracePlayerBBox( vecStart, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr );
		if ( tr.fraction == 1.0 )		// open at eye level
		{
			// Now trace down to see if we would actually land on a standable surface.
			VectorCopy( vecEnd, vecStart );
			vecEnd.z -= 1024.0f;
			TracePlayerBBox( vecStart, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr );
			if ( ( tr.fraction < 1.0f ) && ( tr.plane.normal.z >= 0.7 ) )
			{
				mv->m_vecVelocity[2] = TF_WATERJUMP_UP/*tf_waterjump_up.GetFloat()*/;		// Push up
				mv->m_nOldButtons |= IN_JUMP;		// Don't jump again until released
				player->AddFlag( FL_WATERJUMP );
				player->m_flWaterJumpTime = 2000.0f;	// Do this for 2 seconds
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::CheckFalling( void )
{
	// if we landed on the ground
	if ( player->GetGroundEntity() != NULL && !IsDead() )
	{
		// turn off the jumping flag if we're on ground after a jump
		if ( m_pTFPlayer->m_Shared.IsJumping() )
		{
			m_pTFPlayer->m_Shared.SetJumping( false );

#ifdef CLIENT_DLL
			IGameEvent *event = gameeventmanager->CreateEvent( "landed" );
			if ( event && m_pTFPlayer->IsLocalPlayer() )
			{
				event->SetInt( "player", m_pTFPlayer->GetUserID() );
				gameeventmanager->FireEventClientSide( event );
			}
#endif // CLIENT_DLL
		}
	}

	BaseClass::CheckFalling();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::FullWalkMoveUnderwater()
{
	if ( player->GetWaterLevel() == WL_Waist )
	{
		CheckWaterJump();
	}

	// If we are falling again, then we must not trying to jump out of water any more.
	if ( ( mv->m_vecVelocity.z < 0.0f ) && player->m_flWaterJumpTime )
	{
		player->m_flWaterJumpTime = 0.0f;
	}

	// Was jump button pressed?
	if ( mv->m_nButtons & IN_JUMP )
	{
		CheckJumpButton();
	}
	else
	{
		mv->m_nOldButtons &= ~IN_JUMP;
	}

	// Perform regular water movement
	WaterMove();

	// Redetermine position vars
	CategorizePosition();

	// If we are on ground, no downward velocity.
	if ( player->GetGroundEntity() != NULL )
	{
		mv->m_vecVelocity[2] = 0;			
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::FullWalkMove()
{
	if ( !InWater() ) 
	{
		if ( m_pTFPlayer->m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE ) && mv->m_vecVelocity[2] < 0 )
		{
			mv->m_vecVelocity[2] = Max( mv->m_vecVelocity[2], tf_parachute_maxspeed_z.GetFloat() );
			
			float flDrag = tf_parachute_maxspeed_xy.GetFloat();
			// Instead of clamping, we'll dampen
			float flSpeedX = abs( mv->m_vecVelocity[0] );
			float flSpeedY = abs( mv->m_vecVelocity[1] );
			float flReductionX = flSpeedX > flDrag ? ( flSpeedX - flDrag ) / 3.0f - 10.0f : 0;
			float flReductionY = flSpeedY > flDrag ? ( flSpeedY - flDrag ) / 3.0f - 10.0f : 0;

			mv->m_vecVelocity[0] = Clamp( mv->m_vecVelocity[0], -flDrag - flReductionX, flDrag + flReductionX );
			mv->m_vecVelocity[1] = Clamp( mv->m_vecVelocity[1], -flDrag - flReductionY, flDrag + flReductionY );
		}

		StartGravity();
	}

	// If we are leaping out of the water, just update the counters.
	if ( player->m_flWaterJumpTime )
	{
		// Try to jump out of the water (and check to see if we still are).
		WaterJump();
		TryPlayerMove();
		CheckWater();
		return;
	}

	// If we are swimming in the water, see if we are nudging against a place we can jump up out
	//  of, and, if so, start out jump.  Otherwise, if we are not moving up, then reset jump timer to 0.
	//  Also run the swim code if we're a ghost or have the TF_COND_SWIMMING_NO_EFFECTS condition
	if ( InWater() || ( m_pTFPlayer && ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) || m_pTFPlayer->m_Shared.InCond( TF_COND_SWIMMING_NO_EFFECTS ) ) ) )
	{
		FullWalkMoveUnderwater();
		return;
	}

	if (mv->m_nButtons & IN_JUMP)
	{
		CheckJumpButton();
	}
	else
	{
		mv->m_nOldButtons &= ~IN_JUMP;
	}

	// Make sure velocity is valid.
	CheckVelocity();

	if (player->GetGroundEntity() != NULL)
	{
		mv->m_vecVelocity[2] = 0.0;
		Friction();
		WalkMove();
	}
	else
	{
		AirMove();
	}

	// Set final flags.
	CategorizePosition();

	// Add any remaining gravitational component if we are not in water.
	if ( !InWater() )
	{
		FinishGravity();
	}

	// If we are on ground, no downward velocity.
	if ( player->GetGroundEntity() != NULL )
	{
		mv->m_vecVelocity[2] = 0;
	}

	// Handling falling.
	CheckFalling();

	// Make sure velocity is valid.
	CheckVelocity();

// #ifdef GAME_DLL
// 	if ( m_pTFPlayer->IsPlayerClass( TF_CLASS_SCOUT ) )
// 	{
// 		CTFWeaponBase* pWeapon = m_pTFPlayer->GetActiveTFWeapon();
// 		if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_SODA_POPPER )
// 		{
// 			float speed = VectorLength( mv->m_vecVelocity );
// 			float fDist = speed*gpGlobals->frametime;
// 			float fHype = m_pTFPlayer->m_Shared.GetScoutHypeMeter() + (fDist / tf_scout_hype_mod.GetFloat());
// 			if ( fHype > 100.f )
// 				fHype = 100.f;
// 			m_pTFPlayer->m_Shared.SetScoutHypeMeter( fHype );
// 		}
// 	}
// #endif

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::FullTossMove( void )
{
	trace_t pm;
	Vector move;

	// add velocity if player is moving 
	if ( (mv->m_flForwardMove != 0.0f) || (mv->m_flSideMove != 0.0f) || (mv->m_flUpMove != 0.0f))
	{
		Vector forward, right, up;
		float fmove, smove;
		Vector wishdir, wishvel;
		float wishspeed;
		int i;

		AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

		// Copy movement amounts
		fmove = mv->m_flForwardMove;
		smove = mv->m_flSideMove;

		VectorNormalize (forward);  // Normalize remainder of vectors.
		VectorNormalize (right);    // 

		for (i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
			wishvel[i] = forward[i]*fmove + right[i]*smove;

		wishvel[2] += mv->m_flUpMove;

		VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
		wishspeed = VectorNormalize(wishdir);

		//
		// Clamp to server defined max speed
		//
		if (wishspeed > mv->m_flMaxSpeed)
		{
			VectorScale (wishvel, mv->m_flMaxSpeed/wishspeed, wishvel);
			wishspeed = mv->m_flMaxSpeed;
		}

		// Set pmove velocity
		Accelerate ( wishdir, wishspeed, sv_accelerate.GetFloat() );
	}

	if ( mv->m_vecVelocity[2] > 0 )
	{
		SetGroundEntity( NULL );
	}

	// If on ground and not moving, return.
	if ( player->GetGroundEntity() != NULL )
	{
		if (VectorCompare(player->GetBaseVelocity(), vec3_origin) &&
			VectorCompare(mv->m_vecVelocity, vec3_origin))
			return;
	}

	CheckVelocity();

	// add gravity
	if ( player->GetMoveType() == MOVETYPE_FLYGRAVITY )
	{
		AddGravity();
	}

	// move origin
	// Base velocity is not properly accounted for since this entity will move again after the bounce without
	// taking it into account
	VectorAdd (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

	CheckVelocity();

	VectorScale (mv->m_vecVelocity, gpGlobals->frametime, move);
	VectorSubtract (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

	PushEntity( move, &pm );	// Should this clear basevelocity

	CheckVelocity();

	if (pm.allsolid)
	{	
		// entity is trapped in another solid
		SetGroundEntity( &pm );
		mv->m_vecVelocity.Init();
		return;
	}

	if ( pm.fraction != 1.0f )
	{
		PerformFlyCollisionResolution( pm, move );
	}

	// Check for in water
	CheckWater();
}

//-----------------------------------------------------------------------------
// Purpose: Does the basic move attempting to climb up step heights.  It uses
//          the mv->GetAbsOrigin() and mv->m_vecVelocity.  It returns a new
//          new mv->GetAbsOrigin(), mv->m_vecVelocity, and mv->m_outStepHeight.
//-----------------------------------------------------------------------------
void CTFGameMovement::StepMove( Vector &vecDestination, trace_t &trace )
{
	trace_t saveTrace;
	saveTrace = trace;

	Vector vecEndPos;
	VectorCopy( vecDestination, vecEndPos );

	Vector vecPos, vecVel;
	VectorCopy( mv->GetAbsOrigin(), vecPos );
	VectorCopy( mv->m_vecVelocity, vecVel );

	bool bLowRoad = false;
	bool bUpRoad = true;

	// First try the "high road" where we move up and over obstacles
	if ( player->m_Local.m_bAllowAutoMovement )
	{
		// Trace up by step height
		VectorCopy( mv->GetAbsOrigin(), vecEndPos );
		vecEndPos.z += player->m_Local.m_flStepSize + DIST_EPSILON;
		TracePlayerBBox( mv->GetAbsOrigin(), vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
		if ( !trace.startsolid && !trace.allsolid )
		{
			mv->SetAbsOrigin( trace.endpos );
		}

		// Trace over from there
		TryPlayerMove();

		// Then trace back down by step height to get final position
		VectorCopy( mv->GetAbsOrigin(), vecEndPos );
		vecEndPos.z -= player->m_Local.m_flStepSize + DIST_EPSILON;
		TracePlayerBBox( mv->GetAbsOrigin(), vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
		// If the trace ended up in empty space, copy the end over to the origin.
		if ( !trace.startsolid && !trace.allsolid )
		{
			mv->SetAbsOrigin( trace.endpos );
		}

		// If we are not on the standable ground any more or going the "high road" didn't move us at all, then we'll also want to check the "low road"
		if ( ( trace.fraction != 1.0f && 
			trace.plane.normal[2] < 0.7 ) || VectorCompare( mv->GetAbsOrigin(), vecPos ) )
		{
			bLowRoad = true;
			bUpRoad = false;
		}
	}
	else
	{
		bLowRoad = true;
		bUpRoad = false;
	}

	if ( bLowRoad )
	{
		// Save off upward results
		Vector vecUpPos = vec3_origin, vecUpVel = vec3_origin;
		if ( bUpRoad )
		{
			VectorCopy( mv->GetAbsOrigin(), vecUpPos );
			VectorCopy( mv->m_vecVelocity, vecUpVel );
		}

		// Take the "low" road
		mv->SetAbsOrigin( vecPos );
		VectorCopy( vecVel, mv->m_vecVelocity );
		VectorCopy( vecDestination, vecEndPos );
		TryPlayerMove( &vecEndPos, &saveTrace );

		// Down results.
		Vector vecDownPos, vecDownVel;
		VectorCopy( mv->GetAbsOrigin(), vecDownPos );
		VectorCopy( mv->m_vecVelocity, vecDownVel );

		if ( bUpRoad )
		{
			float flUpDist = ( vecUpPos.x - vecPos.x ) * ( vecUpPos.x - vecPos.x ) + ( vecUpPos.y - vecPos.y ) * ( vecUpPos.y - vecPos.y );
			float flDownDist = ( vecDownPos.x - vecPos.x ) * ( vecDownPos.x - vecPos.x ) + ( vecDownPos.y - vecPos.y ) * ( vecDownPos.y - vecPos.y );
	
			// decide which one went farther
			if ( flUpDist >= flDownDist )
			{
				mv->SetAbsOrigin( vecUpPos );
				VectorCopy( vecUpVel, mv->m_vecVelocity );

				// copy z value from the Low Road move
				mv->m_vecVelocity.z = vecDownVel.z;
			}
		}
	}

	float flStepDist = mv->GetAbsOrigin().z - vecPos.z;
	if ( flStepDist > 0 )
	{
		mv->m_outStepHeight += flStepDist;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameMovement::GameHasLadders() const
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::SetGroundEntity( trace_t *pm )
{
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) && !m_pTFPlayer->GetGroundEntity() && pm && pm->m_pEnt )
	{
		m_pTFPlayer->EmitSound( "BumperCar.JumpLand" );
	}

	BaseClass::SetGroundEntity( pm );
	if ( pm && pm->m_pEnt )
	{
#ifdef GAME_DLL
		int iAirDash = m_pTFPlayer->m_Shared.GetAirDash();
		if ( iAirDash > 0 )
		{
			m_pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_DOUBLE_JUMP, "started_jumping:0" );
		}
		m_pTFPlayer->m_Shared.SetWeaponKnockbackID( -1 );
		m_pTFPlayer->m_bScattergunJump = false;
#endif // GAME_DLL
		m_pTFPlayer->m_Shared.SetAirDash( 0 );
		m_pTFPlayer->m_Shared.SetAirDucked( 0 );

		if ( m_pTFPlayer->m_Shared.InCond( TF_COND_GRAPPLINGHOOK_SAFEFALL ) )
		{
			// CheckFalling happens after this. reset the fall velocity to prevent fall damage
			if ( tf_grapplinghook_prevent_fall_damage.GetBool() )
				player->m_Local.m_flFallVelocity = 0;

			m_pTFPlayer->m_Shared.RemoveCond( TF_COND_GRAPPLINGHOOK_SAFEFALL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::PlayerRoughLandingEffects( float fvol )
{
	if ( m_pTFPlayer )
	{
/*
#ifdef STAGING_ONLY
		// No impact effects if we're in space low-grav
		if ( m_pTFPlayer->m_Shared.InCond( TF_COND_SPACE_GRAVITY  ) )
		{
			return;
		}
#endif // STAGING_ONLY
*/
  		// don't play landing sound when grappling hook into a surface
		if ( m_pTFPlayer->m_Shared.InCond( TF_COND_GRAPPLINGHOOK ) )
		{
			return;
		}

		if ( m_pTFPlayer->IsPlayerClass(TF_CLASS_SCOUT) )
		{
			// Scouts don't play rumble unless they take damage.
			if ( fvol < 1.0 )
			{
				fvol = 0;
			}
		}
	}

	BaseClass::PlayerRoughLandingEffects( fvol );
}

#if 0
// Not being used currently - part of TestDuck!
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::HandleDuck( int nButtonsPressed )
{
	// XBOX SERVER ONLY
#if !defined(CLIENT_DLL)
	if ( IsX360() && nButtonsPressed & IN_DUCK )
	{
		// Hinting logic
		if ( player->GetToggledDuckState() && player->m_nNumCrouches < NUM_CROUCH_HINTS )
		{
			UTIL_HudHintText( player, "#Valve_Hint_Crouch" );
			player->m_nNumCrouches++;
		}
	}
#endif

	bool bInAir = ( player->GetGroundEntity() == NULL );
	bool bInDuck = ( player->GetFlags() & FL_DUCKING ) ? true : false;

	// Starting a duck.
	if ( ( nButtonsPressed & IN_DUCK ) && !bInDuck )
	{
		if ( !player->m_Local.m_bDucking )
		{
			player->m_Local.m_flDucktime = TIME_TO_DUCK_MS;
			player->m_Local.m_bDucking = true;
		}
		else
		{
			// Find unduck percentage and calcluate the duck time.
			float flPercentage = player->m_Local.m_flDucktime /  TIME_TO_UNDUCK_MS;
			player->m_Local.m_flDucktime = TIME_TO_DUCK_MS * ( 1.0f - flPercentage );
		}

		if ( m_pTFPlayer->m_Shared.GetAirDash() > 0 )
		{
			m_pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_DOUBLEJUMP_CROUCH );
		}
	}

	// Handle the ducking.
	if ( player->m_Local.m_bDucking )
	{
		// Finish in duck transition when transition time is over, in "duck", in air.
		if ( ( player->m_Local.m_flDucktime <= 0.0f ) || bInDuck || bInAir )
		{
			FinishDuck();
		}
		else
		{
			// Calculate the eye offset.
			float flDuckFraction = SimpleSpline( 1.0f - ( player->m_Local.m_flDucktime / TIME_TO_DUCK_MS ) );
			SetDuckedEyeOffset( flDuckFraction );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::HandleUnDuck( int nButtonsReleased )
{
	if ( !player->m_Local.m_bAllowAutoMovement )
		return;

	bool bInAir = ( player->GetGroundEntity() == NULL );
	bool bInDuck = ( player->GetFlags() & FL_DUCKING ) ? true : false;

	// Ending a duck (or trying to).
	if ( nButtonsReleased & IN_DUCK )
	{
		if ( bInDuck )
		{
			player->m_Local.m_flDucktime = TIME_TO_UNDUCK_MS;
			player->m_Local.m_bDucking = true;
		}
		else if ( player->m_Local.m_bDucking )
		{
			// Find unduck percentage and calcluate the duck time.
			float flPercentage = player->m_Local.m_flDucktime / TIME_TO_DUCK_MS;
			player->m_Local.m_flDucktime = TIME_TO_UNDUCK_MS * ( 1.0f - flPercentage );
		}
	}

	// Check to see if we are capable of unducking given our environment.
	if ( CanUnduck() )
	{
		if ( ( player->m_Local.m_bDucking || player->m_Local.m_bDucked ) )
		{
			// We are unducking now.
			player->m_Local.m_bDucking = true;

			// Finish ducking immediately if duck time is over or we are in the air.
			if ( player->m_Local.m_flDucktime <= 0.0f || bInAir )
			{
				FinishUnDuck();
			}
			else
			{
				// Calculate the eye offset.
				float flDuckFraction = SimpleSpline( ( player->m_Local.m_flDucktime / TIME_TO_UNDUCK_MS ) );
				SetDuckedEyeOffset( flDuckFraction );
			}
		}
	}
	else
	{
		// Under something where we cannot unduck - rest.
		if ( player->m_Local.m_flDucktime != TIME_TO_UNDUCK_MS )
		{
			player->m_Local.m_flDucktime = TIME_TO_UNDUCK_MS;
			player->m_Local.m_bDucked = true;
			player->m_Local.m_bDucking = false;
			player->AddFlag( FL_DUCKING );

			// Reset the eye offset.
			SetDuckedEyeOffset( 1.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::TestDuck(  )
{

	// Handle buttons.
	int nButtonsChanged	= ( mv->m_nOldButtons ^ mv->m_nButtons );
	int nButtonsPressed	= nButtonsChanged & mv->m_nButtons;
	int nButtonsReleased = nButtonsChanged & mv->m_nOldButtons;
	if ( mv->m_nButtons & IN_DUCK )
	{
		mv->m_nOldButtons |= IN_DUCK;
	}
	else
	{
		mv->m_nOldButtons &= ~IN_DUCK;
	}

	// Handle death.
	if ( IsDead() )
		return;
	// Slow down ducked players.
	HandleDuckingSpeedCrop();

	// In some ducked state - button press to duck, duck transitions, or fully ducked.
	bool bInDuck = ( player->GetFlags() & FL_DUCKING ) ? true : false;
	if ( ( mv->m_nButtons & IN_DUCK ) || player->m_Local.m_bDucking || bInDuck )
	{
		// Duck State
		if ( ( mv->m_nButtons & IN_DUCK ) )
		{
			HandleDuck( nButtonsPressed );
		}
		// Unduck State.
		else
		{
			HandleUnDuck( nButtonsReleased );
		}
	}
}
#endif


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGameMovement::DuckOverrides()
{
	bool bOnGround = ( player->GetGroundEntity() != NULL );

	// Don't allowing ducking in water.
	if ( ( ( player->GetWaterLevel() >= WL_Feet ) && !bOnGround ) ||
		player->GetWaterLevel() >= WL_Eyes )
	{
		mv->m_nButtons &= ~IN_DUCK;
	}

	if ( !tf_clamp_airducks.GetBool() )
		return;

	// Check the duck timer and disable the duck button.
	if ( gpGlobals->curtime < m_pTFPlayer->m_Shared.GetDuckTimer() && bOnGround )
	{
		mv->m_nButtons &= ~IN_DUCK;
	}

	// If we're trying to stand up, don't let the player try to re-duck.  This
	// prevents what the community calls the "Quantum Crouch".  The above ducktimer
	// covers most of the cases where users play nice and duck and unduck while standing.
	// The "Quantum Crouch" occurs when users do the following:
	//		0: Get a Dispenser or other waist-high platform in front of you
	//		1: Press Jump + Crouch and move towards the platform
	//		2: Release Crouch while jumping
	//			( this causes the duck timer to start counting down )
	//		3: Land on the platform
	//		4: While starting to stand up, press Crouch
	//			( when the duck timer finishes, your view will be locked )
	// The intent of the duck timer is to require you to stand up after you've started
	// to unduck and to throttle duck spamming.  This just enforces the unduck
	// requirement.
	if ( player->m_Local.m_bDucked && player->m_Local.m_bDucking )
	{
		mv->m_nButtons &= ~IN_DUCK;
	}

	// Only allow one duck per air event.
	if ( !bOnGround && m_pTFPlayer->m_Shared.AirDuckedCount() >= TF_AIRDUCKED_COUNT )
	{
		mv->m_nButtons &= ~IN_DUCK;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGameMovement::OnDuck( int nButtonsPressed )
{
	// Check to see if we are in the air or ducking.
	bool bInAir = ( player->GetGroundEntity() == NULL );
	bool bInDuck = ( player->GetFlags() & FL_DUCKING ) ? true : false;

	// XBOX SERVER ONLY
#if !defined(CLIENT_DLL)
	if ( IsX360() && nButtonsPressed & IN_DUCK )
	{
		// Hinting logic
		if ( player->GetToggledDuckState() && player->m_nNumCrouches < NUM_CROUCH_HINTS )
		{
			UTIL_HudHintText( player, "#Valve_Hint_Crouch" );
			player->m_nNumCrouches++;
		}
	}
#endif

	// Have the duck button pressed, but the player currently isn't in the duck position.
	if ( ( nButtonsPressed & IN_DUCK ) && !bInDuck )
	{
		player->m_Local.m_flDucktime = GAMEMOVEMENT_DUCK_TIME;
		player->m_Local.m_bDucking = true;

		if ( m_pTFPlayer->m_Shared.GetAirDash() > 0 )
		{
			m_pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_DOUBLEJUMP_CROUCH );
		}
	}

	// The player is in duck transition and not duck-jumping.
	if ( player->m_Local.m_bDucking )
	{
		float flDuckMilliseconds = MAX( 0.0f, GAMEMOVEMENT_DUCK_TIME - ( float )player->m_Local.m_flDucktime );
		float flDuckSeconds = flDuckMilliseconds * 0.001f;

		// Finish in duck transition when transition time is over, in "duck", in air.
		if ( ( flDuckSeconds > TIME_TO_DUCK ) || bInDuck || bInAir )
		{
			FinishDuck();
		}
		else
		{
			// Calc parametric time
			float flDuckFraction = SimpleSpline( flDuckSeconds / TIME_TO_DUCK );
			SetDuckedEyeOffset( flDuckFraction );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGameMovement::OnUnDuck( int nButtonsReleased )
{
	// Check to see if we are in the air or ducking.
	bool bInAir = ( player->GetGroundEntity() == NULL );
	bool bInDuck = ( player->GetFlags() & FL_DUCKING ) ? true : false;

	// Once the duck button is released, start a timer. The player will not be able to engage in a duck
	// until the timer expires.  In addition, set that we have ducked in air (will be allowed only once
	// while in air).
	if ( nButtonsReleased & IN_DUCK )
	{
		m_pTFPlayer->m_Shared.SetDuckTimer( gpGlobals->curtime + TF_TIME_TO_DUCK );
		if ( bInAir )
		{
			// Increment the number of times we have ducked in air.
			int nCount = m_pTFPlayer->m_Shared.AirDuckedCount() + 1;
			m_pTFPlayer->m_Shared.SetAirDucked( nCount );
		}
	}

	// Try to unduck unless automovement is not allowed
	// NOTE: When not onground, you can always unduck
	if ( player->m_Local.m_bAllowAutoMovement || bInAir || player->m_Local.m_bDucking )
	{
		// We released the duck button, we aren't in "duck" and we are not in the air - start unduck transition.
		if ( ( nButtonsReleased & IN_DUCK ) )
		{
			if ( bInDuck )
			{
				player->m_Local.m_flDucktime = GAMEMOVEMENT_DUCK_TIME;
			}
			else if ( player->m_Local.m_bDucking && !player->m_Local.m_bDucked )
			{
				// Invert time if release before fully ducked!!!
				float unduckMilliseconds = 1000.0f * TIME_TO_UNDUCK;
				float duckMilliseconds = 1000.0f * TIME_TO_DUCK;
				float elapsedMilliseconds = GAMEMOVEMENT_DUCK_TIME - player->m_Local.m_flDucktime;

				float fracDucked = elapsedMilliseconds / duckMilliseconds;
				float remainingUnduckMilliseconds = fracDucked * unduckMilliseconds;

				player->m_Local.m_flDucktime = GAMEMOVEMENT_DUCK_TIME - unduckMilliseconds + remainingUnduckMilliseconds;
			}
		}

		// Check to see if we are capable of unducking.
		if ( CanUnduck() )
		{
			// or unducking
			if ( ( player->m_Local.m_bDucking || player->m_Local.m_bDucked ) )
			{
				float flDuckMilliseconds = MAX( 0.0f, GAMEMOVEMENT_DUCK_TIME - (float)player->m_Local.m_flDucktime );
				float flDuckSeconds = flDuckMilliseconds * 0.001f;

				// Finish ducking immediately if duck time is over or not on ground
				if ( flDuckSeconds > TIME_TO_UNDUCK || bInAir )
				{
					FinishUnDuck();
				}
				else
				{
					// Calc parametric time
					float flDuckFraction = SimpleSpline( 1.0f - ( flDuckSeconds / TIME_TO_UNDUCK ) );
					SetDuckedEyeOffset( flDuckFraction );
					player->m_Local.m_bDucking = true;
				}
			}
		}
		else
		{
			// Still under something where we can't unduck, so make sure we reset this timer so
			//  that we'll unduck once we exit the tunnel, etc.
			if ( player->m_Local.m_flDucktime != GAMEMOVEMENT_DUCK_TIME )
			{
				SetDuckedEyeOffset(1.0f);
				player->m_Local.m_flDucktime = GAMEMOVEMENT_DUCK_TIME;
				player->m_Local.m_bDucked = true;
				player->m_Local.m_bDucking = false;
				player->AddFlag( FL_DUCKING );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Crop the speed of the player when ducking and on the ground.
//-----------------------------------------------------------------------------
void CTFGameMovement::HandleDuckingSpeedCrop( void )
{
	BaseClass::HandleDuckingSpeedCrop();

	if ( m_iSpeedCropped & SPEED_CROPPED_DUCK )
	{
		if ( m_pTFPlayer->m_Shared.IsLoser() )
		{
			mv->m_flForwardMove	*= 0;
			mv->m_flSideMove	*= 0;
			mv->m_flUpMove		*= 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: See if duck button is pressed and do the appropriate things
//-----------------------------------------------------------------------------
void CTFGameMovement::Duck( void )
{
	// Check duck overrides.
	DuckOverrides();

	// Calculate the button state.
	int buttonsChanged	= ( mv->m_nOldButtons ^ mv->m_nButtons );	// These buttons have changed this frame
	int buttonsPressed	=  buttonsChanged & mv->m_nButtons;			// The changed ones still down are "pressed"
	int buttonsReleased	=  buttonsChanged & mv->m_nOldButtons;		// The changed ones which were previously down are "released"
	if ( mv->m_nButtons & IN_DUCK )
	{
		mv->m_nOldButtons |= IN_DUCK;
	}
	else
	{
		mv->m_nOldButtons &= ~IN_DUCK;
	}

	// Handle death.
	if ( IsDead() )
	{
		// Reset view offset when dead
		Vector vecStandViewOffset = GetPlayerViewOffset( false );
		Vector vecOffset = player->GetViewOffset();
		if ( vecOffset.z != vecStandViewOffset.z )
		{
			vecOffset.z = vecStandViewOffset.z;
			player->SetViewOffset( vecOffset );
		}

		return;
	}

	// Slow down ducked players.
	HandleDuckingSpeedCrop();

	// If the player is holding down the duck button, the player is in duck transition, ducking, or duck-jumping.
	bool bFirstTimePredicted = true; // Assumes we never rerun commands on the server.
#ifdef CLIENT_DLL
	bFirstTimePredicted = prediction->IsFirstTimePredicted();
#endif

	bool bInDuck = ( player->GetFlags() & FL_DUCKING ) ? true : false;
	if ( ( mv->m_nButtons & IN_DUCK ) || player->m_Local.m_bDucking || bInDuck )
	{
		if ( ( mv->m_nButtons & IN_DUCK ) && m_pTFPlayer->CanDuck() )
		{
			// DUCK
			OnDuck( buttonsPressed );
		}
		else
		{
			// UNDUCK (or attempt to...)
			OnUnDuck( buttonsReleased );
		}
	}
	// HACK: (jimd 5/25/2006) we have a reoccuring bug (#50063 in Tracker) where the player's
	// view height gets left at the ducked height while the player is standing, but we haven't
	// been  able to repro it to find the cause.  It may be fixed now due to a change I'm
	// also making in UpdateDuckJumpEyeOffset but just in case, this code will sense the 
	// problem and restore the eye to the proper position.  It doesn't smooth the transition,
	// but it is preferable to leaving the player's view too low.
	//
	// If the player is still alive and not an observer, check to make sure that
	// his view height is at the standing height.
	else if ( bFirstTimePredicted && !IsDead() && !player->IsObserver() && !player->IsInAVehicle() && !( TFGameRules() && TFGameRules()->ShowMatchSummary() ) )
	{
		float flOffsetDelta = player->GetViewOffset().z - GetPlayerViewOffset( false ).z;
		if ( ( fabs( flOffsetDelta ) > 0.1 ) )
		{
			// we should rarely ever get here, so assert so a coder knows when it happens
			AssertMsg2( 0, "Restoring player view height at %i %0.3f\n", gpGlobals->tickcount, gpGlobals->curtime );
			DevMsg( 1, "Restoring player view height at %i %0.3f.  Delta: %f.\n", gpGlobals->tickcount, gpGlobals->curtime, flOffsetDelta );

			// set the eye height to the non-ducked height
			SetDuckedEyeOffset(0.0f);
		}
	}

	if ( tf_duck_debug_spew.GetBool() )
	{
#ifdef GAME_DLL
		engine->Con_NPrintf( 0, "SERVER" );
		engine->Con_NPrintf( 1, "m_flDucktime %3.2f", player->m_Local.m_flDucktime.Get() );
		engine->Con_NPrintf( 2, "m_flDuckJumpTime %3.2f", player->m_Local.m_flDuckJumpTime.Get() );
		engine->Con_NPrintf( 3, "m_bDucked %d", player->m_Local.m_bDucked.Get() );
		engine->Con_NPrintf( 4, "m_bDucking %d", player->m_Local.m_bDucking.Get() );
		engine->Con_NPrintf( 5, "m_bInDuckJump %d", player->m_Local.m_bInDuckJump.Get() );
		engine->Con_NPrintf( 6, "viewoffset %3.2f, %3.2f, %3.2f", player->GetViewOffset().x, player->GetViewOffset().y, player->GetViewOffset().z );
		engine->Con_NPrintf( 7, "IN_DUCK %d", mv->m_nButtons & IN_DUCK );
		engine->Con_NPrintf( 8, "GetDuckTimer %3.2f", Max( 0.f, m_pTFPlayer->m_Shared.GetDuckTimer() - gpGlobals->curtime ) );
#else 
		engine->Con_NPrintf( 10 + 0, "CLIENT" );
		engine->Con_NPrintf( 10 + 1, "m_flDucktime %3.2f", player->m_Local.m_flDucktime );
		engine->Con_NPrintf( 10 + 2, "m_flDuckJumpTime %3.2f", player->m_Local.m_flDuckJumpTime );
		engine->Con_NPrintf( 10 + 3, "m_bDucked %d", player->m_Local.m_bDucked );
		engine->Con_NPrintf( 10 + 4, "m_bDucking %d", player->m_Local.m_bDucking );
		engine->Con_NPrintf( 10 + 5, "m_bInDuckJump %d", player->m_Local.m_bInDuckJump );
		engine->Con_NPrintf( 10 + 6, "viewoffset %3.2f, %3.2f, %3.2f", player->GetViewOffset().x, player->GetViewOffset().y, player->GetViewOffset().z );
		engine->Con_NPrintf( 10 + 7, "IN_DUCK %d", mv->m_nButtons & IN_DUCK );
		engine->Con_NPrintf( 10 + 8, "GetDuckTimer %3.2f", Max( 0.f, m_pTFPlayer->m_Shared.GetDuckTimer() - gpGlobals->curtime )  );
#endif
	}
}

