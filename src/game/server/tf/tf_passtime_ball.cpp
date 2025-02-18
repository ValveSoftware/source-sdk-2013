//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_passtime_ball.h"
#include "tf_passtime_logic.h"
#include "passtime_ballcontroller.h"
#include "passtime_convars.h"
#include "passtime_game_events.h"
#include "func_passtime_no_ball_zone.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "vcollide_parse.h"
#include "SpriteTrail.h"
#include "soundenvelope.h"
#include "soundent.h"
#include "tf_gamerules.h"
#include "inetchannelinfo.h"
#include "tf_gamestats.h"
#include "tf_team.h"

#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
static const float s_flPickupDist = 1000.f;
static const float s_flBlockDist = 30.0f;
static const float s_flClearDist = 50.0f;
static const char *s_pHalloweenBallModel = "models/passtime/ball/passtime_ball_halloween.mdl";

//-----------------------------------------------------------------------------
static objectparams_t SBallVPhysicsObjectParams()
{
	objectparams_t params = g_PhysDefaultObjectParams;
	params.mass = tf_passtime_ball_mass.GetFloat();
	params.dragCoefficient = tf_passtime_ball_drag_coefficient.GetFloat();
	params.damping = tf_passtime_ball_damping_scale.GetFloat();
	params.rotdamping = tf_passtime_ball_rotdamping_scale.GetFloat();
	params.inertia = tf_passtime_ball_inertia_scale.GetFloat();
	return params;
}

//-----------------------------------------------------------------------------
// CBallPlayerToucher exists because we need the ball to touch both players and 
// triggers. If the ball has FSOLID_TRIGGER, it will touch players but not 
// triggers. And if it doesn't have that, it will touch triggers but not players.
// So this is a hack (there's probably a right way to do this) so the ball can
// just be solid and touch triggers, and this will touch players.
class CBallPlayerToucher : public CBaseEntity
{
public:
	DECLARE_CLASS( CBallPlayerToucher, CBaseEntity );
	CBallPlayerToucher() : m_pBall( 0 ) {}

	//-----------------------------------------------------------------------------
	virtual void Spawn() OVERRIDE 
	{
		// NOTE: this used to create its own vphysics sphere, but it turns out that
		// the engine totally ignores it. 
		SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
		SetModelIndex( m_pBall->GetModelIndex() );
		SetMoveType( MOVETYPE_NONE ); // DIFFERENT
		m_takedamage = DAMAGE_NO;
		SetNextThink( TICK_NEVER_THINK );
		m_iHealth = 0;
		m_iMaxHealth = 1;
		VPhysicsInitNormal( SOLID_NONE, 0, false );
		SetSolid( SOLID_VPHYSICS );
		SetSolidFlags( FSOLID_TRIGGER );
		SetMoveType( MOVETYPE_NONE ); // DIFFERENT
		SetParent( m_pBall );
		SetLocalOrigin( Vector( 0,0,0 ) );
		SetLocalAngles( QAngle( 0,0,0 ) );
		SetTransmitState( FL_EDICT_DONTSEND );
		AddEffects( EF_NODRAW );
		SetTouch( &CBallPlayerToucher::OnTouch );
	}

	//-----------------------------------------------------------------------------
	bool ShouldCollide( int iCollisionGroup, int iContentsMask ) const OVERRIDE
	{
		NOTE_UNUSED( iContentsMask );
		return iCollisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT;
	}

private:
	friend class CPasstimeBall;
	CPasstimeBall *m_pBall;

	void OnTouch( CBaseEntity *pOther ) 
	{
		m_pBall->OnTouch( pOther );
	}
};

LINK_ENTITY_TO_CLASS( _ballplayertoucher, CBallPlayerToucher );

//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( CPasstimeBall, DT_PasstimeBall )
	SendPropInt(SENDINFO(m_iCollisionCount)),
	SendPropEHandle(SENDINFO(m_hHomingTarget)),
	SendPropEHandle(SENDINFO(m_hCarrier)),
	SendPropEHandle(SENDINFO(m_hPrevCarrier)),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( passtime_ball, CPasstimeBall );
PRECACHE_REGISTER( passtime_ball );

CTFPlayer *CPasstimeBall::GetCarrier() const { return m_hCarrier; }
CTFPlayer *CPasstimeBall::GetPrevCarrier() const { return m_hPrevCarrier; }

//-----------------------------------------------------------------------------
CPasstimeBall::CPasstimeBall()
{
	m_bLeftOwner = false;
	m_pHumLoop = 0;
	m_pBeepLoop = 0;
	m_pPlayerToucher = 0;
	m_flLastTeamChangeTime = 0;
	m_flBeginCarryTime = 0;
	m_flIdleRespawnTime = 0;
	m_bTrailActive = false;
}

//-----------------------------------------------------------------------------
void CPasstimeBall::Precache()
{
	PrecacheModel( "passtime/passtime_balltrail_red.vmt" );
	PrecacheModel( "passtime/passtime_balltrail_blu.vmt" );
	PrecacheModel( "passtime/passtime_balltrail_unassigned.vmt" );
	if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
	{
		PrecacheModel( s_pHalloweenBallModel );
	}
	else
	{
		PrecacheModel( tf_passtime_ball_model.GetString() );
	}
	PrecacheScriptSound( "Passtime.BallSmack" );
	PrecacheScriptSound( "Passtime.BallGet" );
	PrecacheScriptSound( "Passtime.BallIdle" );
	PrecacheScriptSound( "Passtime.BallHoming" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
CTFPlayer *CPasstimeBall::GetThrower() const 
{ 
	return m_hThrower.Get(); 
}

//-----------------------------------------------------------------------------
void CPasstimeBall::SetThrower( CTFPlayer *pPlayer ) 
{ 
	m_hThrower = pPlayer; 
	if ( !pPlayer )
	{
		ChangeTeam( TEAM_UNASSIGNED );
	}
	else 
	{
		ChangeTeam( pPlayer->GetTeamNumber() );
	}

}

//-----------------------------------------------------------------------------
unsigned int CPasstimeBall::PhysicsSolidMaskForEntity() const 
{ 
	return MASK_PLAYERSOLID; // must include CONTENT_PLAYERCLIP
}

//-----------------------------------------------------------------------------
int CPasstimeBall::GetCollisionCount() const { return m_iCollisionCount; }

//-----------------------------------------------------------------------------
int CPasstimeBall::GetCarryDuration() const 
{
	return ( (m_flBeginCarryTime > 0) && (m_flBeginCarryTime < gpGlobals->curtime) ) 
		? (gpGlobals->curtime - m_flBeginCarryTime)
		: 0;
}


//-----------------------------------------------------------------------------
static const char *GetTrailEffectForTeam( int iTeam )
{
	switch ( iTeam ) 
	{
	case TF_TEAM_RED: return "passtime/passtime_balltrail_red.vmt";
	case TF_TEAM_BLUE: return "passtime/passtime_balltrail_blu.vmt";
	default: return "passtime/passtime_balltrail_unassigned.vmt";
	};
}

//-----------------------------------------------------------------------------
void CPasstimeBall::ChangeTeam( int iTeam )
{
	// this isn't really the right place for this stats code, but its function
	// is directly dependent on m_flLastTeamChangeTime so I wanted to keep it
	// here to help avoid bugs creeping in.
	// NOTE you can't rely on m_hCarrier being valid or correct here, the order
	// of operations on calling ChangeTeam isn't stable between all the
	// different places where it's called.
	float flElapsedTimeOnThisTeam = gpGlobals->curtime - m_flLastTeamChangeTime;
	if ( TFGameRules() && TFGameRules()->IsPasstimeMode() && g_pPasstimeLogic )
	{
		gamerules_roundstate_t state = TFGameRules()->State_Get();
		if ( ((state == GR_STATE_RND_RUNNING) || (state == GR_STATE_STALEMATE) || (state == GR_STATE_TEAM_WIN)) && (flElapsedTimeOnThisTeam > 0) )
		{
			int nElapsedTimeOnThisTeam = MAX( 0, Float2Int( flElapsedTimeOnThisTeam ) );
			if ( GetTeamNumber() == TEAM_UNASSIGNED )
			{
				CTF_GameStats.m_passtimeStats.summary.nBallNeutralSec += nElapsedTimeOnThisTeam;
			}
			else
			{
				CTF_GameStats.m_passtimeStats.summary.nTotalCarrySec += nElapsedTimeOnThisTeam;
			}

			CTFPlayer *pPlayer = GetThrower();
			if ( !pPlayer ) pPlayer = GetCarrier(); // this happens when the round ends or player dies or something

			if ( pPlayer )
			{
				CTFTeam *pPlayerTeam = GetGlobalTFTeam( pPlayer->GetTeamNumber() );
				CTFTeam *pPlayerEnemyTeam = GetGlobalTFTeam( GetEnemyTeam( pPlayer->GetTeamNumber() ) );
				// NOTE: if the ball carrier switches teams and suicides, this will incorrectly
				// attribute the time to the wrong team, but I don't care.
				if ( pPlayerTeam->GetFlagCaptures() > pPlayerEnemyTeam->GetFlagCaptures() )
				{
					CTF_GameStats.m_passtimeStats.summary.nTotalWinningTeamBallCarrySec += Float2Int( flElapsedTimeOnThisTeam );
				}
				else if ( pPlayerTeam->GetFlagCaptures() < pPlayerEnemyTeam->GetFlagCaptures() )
				{
					CTF_GameStats.m_passtimeStats.summary.nTotalLosingTeamBallCarrySec += Float2Int( flElapsedTimeOnThisTeam );
				}
			}
		}
	}

	m_flLastTeamChangeTime = gpGlobals->curtime;
	BaseClass::ChangeTeam( iTeam );

	// teams: TEAM_UNASSIGNED, spectator, TF_TEAM_RED, TF_TEAM_BLUE
	// skins: red, blu, unassigned
	// NOTE: skins are in this order because we use the same model as the weapon viewmodel
	// and m_bHasTeamSkins_Viewmodel expects them in this order
	const int skinForTeam[] = { 2, 2, 0, 1 };
	iTeam = GetTeamNumber(); // paranoia; set by BaseClass::ChangeTeam
	Assert( iTeam >= 0 && iTeam < 4 );
	if ( iTeam >= 0 && iTeam < 4 ) // paranoia
	{
		m_nSkin = skinForTeam[iTeam];
	}

	if ( m_bTrailActive )
	{
		const char *pszTrailEffectName = GetTrailEffectForTeam( iTeam );
		m_pTrail->SetModel( pszTrailEffectName );
	}

	if ( iTeam == TEAM_UNASSIGNED )
	{
		// NOTE: don't call SetThrower here, it'll be recursive.
		m_hThrower = 0; 
	}
}

//-----------------------------------------------------------------------------
bool CPasstimeBall::CreateModelCollider()
{
	solid_t tmpSolid;
	PhysModelParseSolid( tmpSolid, this, GetModelIndex() );
	tmpSolid.params = SBallVPhysicsObjectParams();
	tmpSolid.params.pGameData = static_cast<void *>( this );

	auto *pPhysObj = VPhysicsInitNormal( SOLID_VPHYSICS, 0, false, &tmpSolid );
	if ( !pPhysObj ) 
	{
		return false;
	}

	SetSolidFlags( FSOLID_NOT_STANDABLE );
	AddFlag( FL_GRENADE ); // required for airblast deflection to work
	pPhysObj->Wake();

	return true;
}

//-----------------------------------------------------------------------------
void CPasstimeBall::CreateSphereCollider()
{
	// NOTE: calling VPhysicsInitNormal(SOLID_BBOX) doesn't work right. 
	// Not calling SetSolid after also doesn't work right.
	// In order for CreateSphereObject to work and not crash, you must do
	// VPhysicsInitNormal( SOLID_NONE followed by SetSolid(whatever)
	// Seems like VPHYSICS or BBOX do the same thing.
	// Must have FSOLID_TRIGGER to touch players. Unfortunately, triggers can't trigger triggers.

	VPhysicsInitNormal( SOLID_NONE, 0, false );
	SetSolid( SOLID_VPHYSICS );
	SetSolidFlags( FSOLID_NOT_STANDABLE );
	AddFlag( FL_GRENADE ); // required for airblast deflection to work

	auto params = SBallVPhysicsObjectParams();
	params.pGameData = static_cast<void *>( this );
	const float flBallRadius = tf_passtime_ball_sphere_radius.GetFloat();
	const float flFourThirdsPi = 4.1888f;
	params.volume = flFourThirdsPi * (flBallRadius*flBallRadius*flBallRadius);

	const int iPhysMat = physprops->GetSurfaceIndex("passtime_ball");
	IPhysicsObject *pPhysObj = physenv->CreateSphereObject( flBallRadius, iPhysMat, GetAbsOrigin(), GetAbsAngles(), &params, false );
	VPhysicsSetObject( pPhysObj );
	SetMoveType( MOVETYPE_VPHYSICS );
	pPhysObj->Wake();
}

//-----------------------------------------------------------------------------
void CPasstimeBall::Spawn()
{
	// not sure why this has to come first, but iirc it does.
	SetCollisionGroup( COLLISION_GROUP_NONE ); 

	// === CBaseProp::Spawn
	const char *pszModelName = (char*) STRING( GetModelName() );
	if ( !pszModelName || !*pszModelName )
	{
		if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
		{
			pszModelName = s_pHalloweenBallModel;
		}
		else
		{
			pszModelName = tf_passtime_ball_model.GetString();
		}
	}
	PrecacheModel( pszModelName );
	Precache();
	SetModel( pszModelName );
	SetMoveType( MOVETYPE_PUSH );
	m_takedamage = DAMAGE_NO;
	SetNextThink( TICK_NEVER_THINK );
	m_flAnimTime = gpGlobals->curtime;
	m_flPlaybackRate = 0.0f;
	SetCycle( 0 );

	// === CBreakableProp::Spawn
	m_flFadeScale = 1;
	m_iHealth = 0;
	m_takedamage = tf_passtime_ball_takedamage.GetBool() 
		? DAMAGE_EVENTS_ONLY 
		: DAMAGE_NO;
	m_iMaxHealth = 1;

	// === CPhysicsProp::Spawn
	if( IsMarkedForDeletion() )
	{
		return;
	}

	m_pPlayerToucher = CreateEntityByName( "_ballplayertoucher" );
	((CBallPlayerToucher*)m_pPlayerToucher)->m_pBall = this;
	DispatchSpawn( m_pPlayerToucher );

	if ( tf_passtime_ball_sphere_collision.GetBool() || !CreateModelCollider() )
	{
		CreateSphereCollider();
	}

	// === My spawn
	m_flLastTeamChangeTime = gpGlobals->curtime;
	m_flBeginCarryTime = -1;
	ResetTrail();
	ChangeTeam( TEAM_UNASSIGNED );
	
	if ( TFGameRules()->IsPasstimeMode() )
	{
		// TODO the ball used to be functional in non-wasabi maps, but I haven't maintained it
		SetThink( &CPasstimeBall::DefaultThink );
		SetNextThink( gpGlobals->curtime );
		SetTransmitState( FL_EDICT_ALWAYS );
		m_playerSeek.SetIsEnabled( true );
	}

	m_flLastCollisionTime = gpGlobals->curtime;
	m_flAirtimeDistance = 0;
	m_eState = STATE_OUT_OF_PLAY;
}

//-----------------------------------------------------------------------------
void CPasstimeBall::SetIdleRespawnTime() 
{
	auto *pTimer = TFGameRules()->GetActiveRoundTimer();
	if ( !pTimer ) return;
	auto ts = pTimer->GetTimerState();
	auto grs = TFGameRules()->State_Get();
	m_flIdleRespawnTime = ((grs == GR_STATE_RND_RUNNING) && (ts == RT_STATE_NORMAL))
		? (gpGlobals->curtime + tf_passtime_ball_reset_time.GetFloat())
		: 0;
}

//-----------------------------------------------------------------------------
void CPasstimeBall::DisableIdleRespawnTime()
{
	m_flIdleRespawnTime = 0;
}

//-----------------------------------------------------------------------------
bool CPasstimeBall::ShouldCollide( int iCollisionGroup, int iContentsMask ) const
{
	// note: returning false for COLLISION_GROUP_PLAYER_MOVEMENT means the ball won't
	// stop player movement. the only real visible effect when this function doesn't
	// return false for COLLISION_GROUP_PLAYER_MOVEMENT is that the ball is unable 
	// to impart physics forces on itself when a player blocks it, since the player
	// will set velocity to zero due to being "stuck" on the ball, even though the
	// ball won't actually prevent the player from moving through it.
	return (iCollisionGroup != COLLISION_GROUP_PLAYER_MOVEMENT);
}

//-----------------------------------------------------------------------------
void CPasstimeBall::ResetTrail()
{
	// ideally this would just drop all of the existing trail points instead of 
	// re-creating all the entities, but I couldn't find a clean way to do it in
	// a reasonable amount of time.
	HideTrail();

	const char *pszTrailEffect = GetTrailEffectForTeam( GetTeamNumber() );
	Vector origin = GetAbsOrigin();
	float flStartRadius = tf_passtime_ball_sphere_radius.GetFloat() * 2;
	float flEndRadius = tf_passtime_ball_sphere_radius.GetFloat() * 3;
	m_pTrail = CSpriteTrail::SpriteTrailCreate( pszTrailEffect, origin, true );
	m_pTrail->SetAttachment( this, 0 );
	m_pTrail->SetTransmit( true ); // this actually controls whether the attachment parent receives it
	m_pTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 200, kRenderFxNone );
	m_pTrail->SetStartWidth( flStartRadius );
	m_pTrail->SetEndWidth( flEndRadius );
	m_pTrail->SetTextureResolution( 1 );
	m_pTrail->SetLifeTime( 3.0f );

	m_bTrailActive = true;
}

//-----------------------------------------------------------------------------
void CPasstimeBall::HideTrail()
{
	// ideally this would just hide the existing trails instead of deleting
	// them all, but I couldn't find a clean way to do it in a reasonable 
	// amount of time.
	if ( !m_bTrailActive )
	{
		return;
	}

	// this is sometimes called from a physics callback (reset trail on collision)
	// so use PhysCallbackRemove instead of UTIL_Remove
	PhysCallbackRemove( m_pTrail->NetworkProp() );
	m_pTrail = nullptr;
	m_bTrailActive = false;
}

//-----------------------------------------------------------------------------
CPasstimeBall::~CPasstimeBall()
{
	// trail is automatically removed because it's a child
	// m_pPlayerToucher is automatically removed because it's a child

	if ( m_pHumLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pHumLoop );
	}
	if ( m_pBeepLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pBeepLoop );
	}
}

//-----------------------------------------------------------------------------
// OnBecomeNotCarried: common boilerplate between SetStateFree/OutOfPlay
void CPasstimeBall::OnBecomeNotCarried() 
{
	CTFPlayer *pCarrier = m_hCarrier;

	// 
	// Carrier management and events
	//
	if ( pCarrier && pCarrier->m_Shared.HasPasstimeBall() )
	{
		pCarrier->m_Shared.SetHasPasstimeBall( false );
		pCarrier->m_Shared.RemoveCond( TF_COND_SPEED_BOOST, true );
		pCarrier->m_Shared.RemoveCond( TF_COND_PASSTIME_INTERCEPTION, true );
		pCarrier->TeamFortress_SetSpeed();
		PasstimeGameEvents::BallFree( pCarrier->entindex() ).Fire();
	}

	//
	// Stats
	//
	if( m_flBeginCarryTime > 0 )
	{
		int nClass = pCarrier->GetPlayerClass()->GetClassIndex();
		int nCarrySec = MAX( 0, Float2Int( gpGlobals->curtime - m_flBeginCarryTime ) );
		CTF_GameStats.m_passtimeStats.classes[ nClass].nTotalCarrySec += nCarrySec;
		m_flBeginCarryTime = -1;
	}

	// 
	// Reset various tracking and counters
	//
	m_iCollisionCount = 0;
	m_flAirtimeDistance = 0;
	m_flLastCollisionTime = gpGlobals->curtime;
	m_bLeftOwner = false;
	//m_playerSeek.SetIsEnabled( false ); // TODO: seek will re-enable itself
	SetParent( 0 );
}

//-----------------------------------------------------------------------------
void CPasstimeBall::SetStateFree()
{
	if ( BOutOfPlay() )
	{
		// this is a hack to prevent the out-of-play time from counting in the stats
		m_flLastTeamChangeTime = gpGlobals->curtime;
	}

	//
	// Change state
	//
	m_eState = STATE_FREE;
	OnBecomeNotCarried();

	//
	// Make interactive
	//
	DisableIdleRespawnTime();
	RemoveEffects( EF_NODRAW );
	m_pPlayerToucher->RemoveSolidFlags( FSOLID_NOT_SOLID );
	m_pPlayerToucher->SetSolid( SOLID_VPHYSICS );
	m_takedamage = tf_passtime_ball_takedamage.GetBool() ? DAMAGE_EVENTS_ONLY : DAMAGE_NO;
	SetMoveType( MOVETYPE_VPHYSICS );
	SetSolid( SOLID_VPHYSICS );
	SetSolidFlags( FSOLID_NOT_STANDABLE );
	SetThrower( m_hCarrier );
	TFGameRules()->SetObjectiveObserverTarget( this );
	VPhysicsGetObject()->EnableGravity( true );
	VPhysicsGetObject()->Wake();

	// 
	// Trail management
	//
	if ( !m_bTrailActive )
	{
		// create trails if there aren't any
		ResetTrail();
	}

	//
	// Sounds
	//
	if ( !m_pHumLoop )
	{
		CReliableBroadcastRecipientFilter filter;
		m_pHumLoop = CSoundEnvelopeController::GetController().SoundCreate( 
			filter, entindex(), "Passtime.BallIdle" );
		CSoundEnvelopeController::GetController().Play( m_pHumLoop, 1, PITCH_NORM );
	}

	//
	// Bookeeping
	//
	if ( m_hCarrier )
	{
		m_hPrevCarrier = m_hCarrier;
	}
	m_hCarrier = 0;
}

//-----------------------------------------------------------------------------
bool CPasstimeBall::BOutOfPlay() const { return m_eState == STATE_OUT_OF_PLAY; }

//-----------------------------------------------------------------------------
void CPasstimeBall::SetStateOutOfPlay()
{
	// This can be called redundantly during RespawnBall
	if ( BOutOfPlay() )
	{
		return;
	}

	// this is a hack to make sure the carrier stats are captured because
	// ChangeTeam updates some stats and may not be called at end of round.
	ChangeTeam( TEAM_UNASSIGNED ); 
	
	//
	// Change state
	//
	m_eState = STATE_OUT_OF_PLAY;
	OnBecomeNotCarried();

	//
	// Make noninteractive
	//
	DisableIdleRespawnTime();
	AddEffects( EF_NODRAW );
	m_pPlayerToucher->AddSolidFlags( FSOLID_NOT_SOLID );
	m_pPlayerToucher->SetSolid( SOLID_NONE );
	m_takedamage = DAMAGE_NO;
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID );
	SetThrower( 0 );
	TFGameRules()->SetObjectiveObserverTarget( 0 );
	VPhysicsGetObject()->EnableGravity( false );

	// 
	// Trail management
	//
	HideTrail();

	//
	// Sounds
	//
	if ( m_pHumLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pHumLoop );
		m_pHumLoop = 0;
	}

	if ( m_pBeepLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pBeepLoop );
		m_pBeepLoop = 0;
	}

	//
	// Bookeeping
	//
	if ( m_hCarrier )
	{
		m_hPrevCarrier = m_hCarrier;
	}
	m_hCarrier = 0;
}

//-----------------------------------------------------------------------------
void CPasstimeBall::SetStateCarried( CTFPlayer *pCarrier )
{
	// this can be called when m_eState==STATE_CARRIED when the ball is being
	// directly transferred between players.
	m_eState = STATE_CARRIED;

	Assert( pCarrier );
	if ( !pCarrier )
	{
		SetStateOutOfPlay();
		return;
	}

	// 
	// Carrier management and events
	// FIXME move all of the event handling for ball events into CTFPasstimeLogic
	//
	Assert( !pCarrier->m_Shared.HasPasstimeBall() );
	pCarrier->RemoveInvisibility();
	pCarrier->RemoveDisguise();
	pCarrier->EndClassSpecialSkill(); // abort demo charge
	pCarrier->m_Shared.SetHasPasstimeBall( true );
	if ( pCarrier != m_hPrevCarrier )
	{
		pCarrier->m_Shared.AddCond( TF_COND_SPEED_BOOST, tf_passtime_speedboost_on_get_ball_time.GetFloat() );

		// Limit points by time so we can't just throw back and forth a ton for points.
		// FIXME awarding points here and also in passtime_logic?
		if ( gpGlobals->realtime - g_pPasstimeLogic->GetLastPassTime(pCarrier) > 6.0f ) // FIXME literal balance value
		{
			CTF_GameStats.Event_PlayerAwardBonusPoints(pCarrier, 0, 5); // FIXME literal balance value
			g_pPasstimeLogic->SetLastPassTime(pCarrier);
		}
	}
	pCarrier->TeamFortress_SetSpeed();

	// 
	// Adjust things common to all states
	//
	DisableIdleRespawnTime();
	AddEffects( EF_NODRAW );
	m_iCollisionCount = 0;
	m_flAirtimeDistance = 0;
	m_flLastCollisionTime = gpGlobals->curtime;
	m_bLeftOwner = false;
	//m_playerSeek.SetIsEnabled( false ); // TODO: seek will re-enable itself
	m_pPlayerToucher->AddSolidFlags( FSOLID_NOT_SOLID );
	m_pPlayerToucher->SetSolid( SOLID_NONE );
	m_takedamage = DAMAGE_NO;
	SetMoveType( MOVETYPE_NONE );
	SetParent( pCarrier, pCarrier->LookupAttachment( "effect_hand_R" ) );
	SetSolid( SOLID_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID );
	TFGameRules()->SetObjectiveObserverTarget( pCarrier );
	VPhysicsGetObject()->EnableGravity( false );

	//
	// Unique to this state
	//
	m_bTouchedSinceSpawn = true;
	SetLocalOrigin( Vector( 0,0,0 ) ); // because SetParent(pCarrier)

	// 
	// Sounds
	//
	EmitSound( "Passtime.BallGet" );
	if ( m_pHumLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pHumLoop );
		m_pHumLoop = 0;
	}

	if ( m_pBeepLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pBeepLoop );
		m_pBeepLoop = 0;
	}

	//
	// Stats
	//
	m_flBeginCarryTime = gpGlobals->curtime;

	//
	// Bookeeping
	//
	if ( m_hCarrier )
	{
		m_hPrevCarrier = m_hCarrier;
	}
	m_hCarrier = pCarrier;
	ChangeTeam( pCarrier->GetTeamNumber() );
}

//-----------------------------------------------------------------------------
void CPasstimeBall::MoveToSpawner( const Vector &pos )
{ 
	MoveTo( pos, Vector( 0,0,0 ) );
	m_bTouchedSinceSpawn = false;
	m_hPrevCarrier = 0;
}

//-----------------------------------------------------------------------------
bool CPasstimeBall::IsDeflectable()
{
	return m_eState == STATE_FREE;
}

//-----------------------------------------------------------------------------
int CPasstimeBall::UpdateTransmitState()
{
	if ( !TFGameRules()->IsPasstimeMode() )
	{
		return BaseClass::UpdateTransmitState();
	}
	return SetTransmitState(FL_EDICT_ALWAYS);
}

//-----------------------------------------------------------------------------
void CPasstimeBall::MoveTo( const Vector &pos, const Vector &vecVel )
{
	// NOTE: using Teleport() causes some weird interpolation errors
	// because it handles it specially as a "teleport list" etc

	SetAbsOrigin( pos );
	SetAbsVelocity( vecVel );
	SetAbsAngles( QAngle( 0, 0, 0 ) );

	IPhysicsObject *pPhys = VPhysicsGetObject();

	pPhys->SetPosition( pos, QAngle( 0, 0, 0 ), true );
	Vector fwd = vecVel.Normalized();
	AngularImpulse angular( fwd.x * 0, fwd.y * 0, fwd.z * 1 ); // TODO
	pPhys->SetVelocity( &vecVel, &angular );
		
	PhysicsTouchTriggers();

	m_vecPrevOrigin = pos; // used for tracking pass distance

	CPasstimeBallController::BallSpawned( this );
}

//-----------------------------------------------------------------------------
bool CPasstimeBall::BShouldPanicRespawn() const
{
	if ( !TFGameRules()
		|| ( TFGameRules()->State_Get() != GR_STATE_RND_RUNNING )
		|| ( m_eState != STATE_FREE ) )
	{
		return false;
	}

	if ( ( m_flIdleRespawnTime > 0 ) && ( m_flIdleRespawnTime < gpGlobals->curtime ) )
	{
		return true;
	}

	return ( enginetrace->GetPointContents( GetAbsOrigin() ) == CONTENTS_SOLID );
}

//-----------------------------------------------------------------------------
void CPasstimeBall::DefaultThink()
{
	UpdateLagCompensationHistory();

	if( IsMarkedForDeletion() || !g_pPasstimeLogic )
	{
		return;
	}

	SetNextThink( gpGlobals->curtime );

	if ( BShouldPanicRespawn() )
	{
		g_pPasstimeLogic->RespawnBall();
		return;
	}

	//
	// Eject the ball if the carrier isn't allowed to carry it
	//
	CTFPlayer *pCarrier = m_hCarrier;
	if ( pCarrier )
	{
		HudNotification_t ejectReason;
		if ( !g_pPasstimeLogic->BCanPlayerPickUpBall( pCarrier, &ejectReason ) )
		{
			if ( ejectReason && TFGameRules() ) 
			{
				CSingleUserReliableRecipientFilter filter( pCarrier );
				TFGameRules()->SendHudNotification( filter, ejectReason );
			}
			g_pPasstimeLogic->EjectBall( pCarrier, pCarrier );
			SetIdleRespawnTime(); // have to do this here because need to guarantee it happens for no ball zones
			EmitSound( "Passtime.BallDropped");
			return;
		}
	}

	//
	// Track airtime and apply controllers
	//
	if ( m_eState == STATE_FREE )
	{
		{
			Vector vecOrigin = GetAbsOrigin();
			m_flAirtimeDistance += vecOrigin.DistTo( m_vecPrevOrigin );
			m_vecPrevOrigin = vecOrigin;
		}

		IPhysicsObject *pPhysObj = VPhysicsGetObject();
		Vector vecVel;
		pPhysObj->GetVelocity( &vecVel, 0 );
		SetAbsVelocity( vecVel ); 	
		// this is a hack to work around some issues where GetAbsVelocity was just 
		// returning some huge value. this seems to fix it, so something is probably fubar in physics :/
		// hopefully just related to using the sphere collider that nothing else uses.

		pPhysObj->Wake(); // NEVER SLEEP

		//m_playerSeek.SetIsEnabled( !m_bTouchedSinceSpawn );
		CPasstimeBallController::ApplyTo( this );
	}
}

//-----------------------------------------------------------------------------
extern ConVar sv_maxunlag;
void CPasstimeBall::UpdateLagCompensationHistory()
{
	// adapted from CLagCompensationManager::FrameUpdatePostEntityThink

	Assert( m_lagCompensationHistory.Count() < 1000 ); // insanity check
	m_flLagCompensationTeleportDistanceSqr = 64*64;
	
	// remove tail records that are too old
	intp tailIndex = m_lagCompensationHistory.Tail();
	int flDeadtime = gpGlobals->curtime - sv_maxunlag.GetFloat();
	while ( m_lagCompensationHistory.IsValidIndex( tailIndex ) )
	{
		LagRecord &tail = m_lagCompensationHistory.Element( tailIndex );

		// if tail is within limits, stop
		if ( tail.flSimulationTime >= flDeadtime )
			break;
			
		// remove tail, get new tail
		m_lagCompensationHistory.Remove( tailIndex );
		tailIndex = m_lagCompensationHistory.Tail();
	}

	// check if head has same simulation time
	if ( m_lagCompensationHistory.Count() > 0 )
	{
		LagRecord &head = m_lagCompensationHistory.Element( m_lagCompensationHistory.Head() );

		// check if player changed simulation time since last time updated
		if ( head.flSimulationTime >= GetSimulationTime() )
			return; // don't add new entry for same or older time
	}

	// add new record to player track
	LagRecord &record = m_lagCompensationHistory.Element( m_lagCompensationHistory.AddToHead() );
	record.flSimulationTime = GetSimulationTime();
	record.vecOrigin = GetAbsOrigin();
}

//-----------------------------------------------------------------------------
void CPasstimeBall::StartLagCompensation( CBasePlayer *player, CUserCmd *cmd )
{
	m_bLagCompensationNeedsRestore = false; // set to true if it actually backtracks
	if ( m_lagCompensationHistory.Count() <= 0 )
		return;

	// adapted from CLagCompensationManager::StartLagCompensation

	int targettick = cmd->tick_count;
	{
		// correct is the amout of time we have to correct game time
		float correct = 0.0f;

		INetChannelInfo *nci = engine->GetPlayerNetInfo( player->entindex() ); 

		if ( nci )
		{
			// add network latency
			correct+= nci->GetLatency( FLOW_OUTGOING );
		}

		// calc number of view interpolation ticks - 1
		int lerpTicks = TIME_TO_TICKS( player->m_fLerpTime );

		// add view interpolation latency see C_BaseEntity::GetInterpolationAmount()
		correct += TICKS_TO_TIME( lerpTicks );
	
		// check bouns [0,sv_maxunlag]
		correct = clamp( correct, 0.0f, sv_maxunlag.GetFloat() );

		// correct tick send by player 
		targettick = cmd->tick_count - lerpTicks;

		// calc difference between tick send by player and our latency based tick
		float deltaTime =  correct - TICKS_TO_TIME(gpGlobals->tickcount - targettick);

		if ( fabs( deltaTime ) > 0.2f )
		{
			// difference between cmd time and latency is too big > 200ms, use time correction based on latency
			// DevMsg("StartLagCompensation: delta too big (%.3f)\n", deltaTime );
			targettick = gpGlobals->tickcount - TIME_TO_TICKS( correct );
		}
	}

	// copied from BacktrackPlayer
	Vector org;
	float flTargetTime = TICKS_TO_TIME( targettick );
	{
		intp curr = m_lagCompensationHistory.Head();
		LagRecord *prevRecord = 0;
		LagRecord *record = 0;
		Vector prevOrg = GetAbsOrigin();

		// Walk context looking for any invalidating pEvent
		while( m_lagCompensationHistory.IsValidIndex(curr) )
		{
			// remember last record
			prevRecord = record;

			// get next record
			record = &m_lagCompensationHistory.Element( curr );

			Vector delta = record->vecOrigin - prevOrg;
			if ( delta.Length2DSqr() > m_flLagCompensationTeleportDistanceSqr )
			{
				// lost track, too much difference
				return; 
			}

			// did we find a context smaller than target time ?
			if ( record->flSimulationTime <= flTargetTime )
				break; // hurra, stop

			prevOrg = record->vecOrigin;

			// go one step back
			curr = m_lagCompensationHistory.Next( curr );
		}

		Assert( record );
		if ( !record )
		{
			return; // that should never happen
		}


		float frac = 0.0f;
		if ( prevRecord && 
			 (record->flSimulationTime < flTargetTime) &&
			 (record->flSimulationTime < prevRecord->flSimulationTime) )
		{
			// we didn't find the exact time but have a valid previous record
			// so interpolate between these two records;

			Assert( prevRecord->flSimulationTime > record->flSimulationTime );
			Assert( flTargetTime < prevRecord->flSimulationTime );

			// calc fraction between both records
			frac = ( flTargetTime - record->flSimulationTime ) / 
				( prevRecord->flSimulationTime - record->flSimulationTime );

			Assert( frac > 0 && frac < 1 ); // should never extrapolate

			org	= Lerp( frac, record->vecOrigin, prevRecord->vecOrigin );
		}
		else
		{
			// we found the exact record or no other record to interpolate with
			// just copy these values since they are the best we have
			org	= record->vecOrigin;
		}
	}

	Vector orgdiff = GetAbsOrigin() - org;
	m_lagCompensationRestore.flSimulationTime = GetSimulationTime();
	m_lagCompensationRestore.vecOrigin = GetAbsOrigin();
	SetAbsOrigin( org );
	SetSimulationTime( flTargetTime );
	m_bLagCompensationNeedsRestore = true;
}

//-----------------------------------------------------------------------------
void CPasstimeBall::FinishLagCompensation( CBasePlayer *player )
{
	// adapted from CLagCompensationManager::BacktrackPlayer

	if ( !m_bLagCompensationNeedsRestore )
	{
		return;
	}

	SetAbsOrigin( m_lagCompensationRestore.vecOrigin ); // this is probably not correct?
	SetSimulationTime( m_lagCompensationRestore.flSimulationTime );
}

//-----------------------------------------------------------------------------
bool CPasstimeBall::BIgnorePlayer( CTFPlayer *pPlayer )
{
	// NOTE: it's possible to be !alive and !dead at the same time
	if ( !pPlayer || !pPlayer->IsAlive() )
	{
		return true;
	}

	if ( !m_bLeftOwner && (pPlayer == GetThrower()) )
	{
		const float flDist = CalcDistanceToAABB( 
			pPlayer->WorldAlignMins(),
			pPlayer->WorldAlignMaxs(),
			GetAbsOrigin() - pPlayer->GetAbsOrigin() );
		m_bLeftOwner = flDist > s_flClearDist;
		return !m_bLeftOwner;
	}
	else
	{
		m_bLeftOwner = true;
		return false;
	}
}

//-----------------------------------------------------------------------------
void CPasstimeBall::TouchPlayer( CTFPlayer *pPlayer )
{
	if ( !TFGameRules() )
	{
		return;
	}

	//
	// Is this player close enough to hit it?
	// TODO is this still necessary since we use actual physics touching now?
	//
	{
		const Vector& vecMyOrigin = GetAbsOrigin();
		const Vector& vecOtherOrigin = pPlayer->GetAbsOrigin();
		const Vector vecOtherHead = vecOtherOrigin + Vector( 0, 0, pPlayer->BoundingRadius() + 8 );
		float t = 0;
		const float flDist = CalcDistanceToLineSegment( vecMyOrigin, vecOtherOrigin, vecOtherHead, &t );
		if ( (flDist > s_flBlockDist) && (flDist > s_flPickupDist) )
		{
			return;
		}
	}

	const bool bSameTeam = GetThrower() && (pPlayer->GetTeamNumber() == GetThrower()->GetTeamNumber());

	//
	// Can this player get the ball?
	//
	bool bCanPickUp = false;
	{
		HudNotification_t cantPickUpReason;
		bCanPickUp = g_pPasstimeLogic->BCanPlayerPickUpBall( pPlayer, &cantPickUpReason );
		if ( cantPickUpReason )
		{
			CSingleUserReliableRecipientFilter filter( pPlayer );
			TFGameRules()->SendHudNotification( filter, cantPickUpReason );
		}
	}


	if ( bCanPickUp )
	{
		m_bTouchedSinceSpawn = true;
		g_pPasstimeLogic->OnPlayerTouchBall( pPlayer, this );
	}
	else if ( !bSameTeam )
	{
		// can't pick it up and not on the same team = block

		// NOTE: BlockDamage has to come after BlockReflect in order for 
		// the reflection to work right. BlockDamage might apply a force
		// to the player, which will taint the reflection vector.
		// NOTE: because some of these functions might change the ball's
		// velocity, get it once and then pass it to each.
		IPhysicsObject* pPhysObj = VPhysicsGetObject();
		Vector vecBallVel; 
		pPhysObj->GetVelocity( &vecBallVel, 0 );

		BlockReflect( pPlayer, pPlayer->GetAbsOrigin(), vecBallVel );
		BlockDamage( pPlayer, vecBallVel );

		if ( GetThrower() )
		{
			// ball was in flight
			PasstimeGameEvents::BallBlocked( GetThrower()->entindex(), pPlayer->entindex() ).Fire();
		}
			
		CPasstimeBallController::DisableOn( this );
		m_iCollisionCount++;
		SetThrower( 0 );
		m_flAirtimeDistance = 0;
		m_flLastCollisionTime = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
void CPasstimeBall::BlockReflect( CTFPlayer *pPlayer, const Vector& vecBallOrigin, const Vector& vecBallVel )
{
	if ( m_hBlocker == pPlayer )
	{
		// this helps prevent the ball from getting stuck inside players
		return;
	}

	m_hBlocker = pPlayer;

	const Vector vecMyOrigin = GetAbsOrigin();
	Vector vecBallDir = vecBallVel;
	vecBallDir.z = 0;
	const float flBallSpeed = vecBallDir.NormalizeInPlace();

	Vector vecReflectVel = vecMyOrigin - vecBallOrigin;
	vecReflectVel.z = 0;
	vecReflectVel.NormalizeInPlace();
	vecReflectVel = vecReflectVel.Cross( vecBallDir );
	vecReflectVel.NormalizeInPlace();
	vecReflectVel = vecBallDir.Cross( vecReflectVel );
	vecReflectVel.NormalizeInPlace();
	vecReflectVel -= vecBallDir;
	vecReflectVel *= flBallSpeed / 2.0f;
	vecReflectVel += pPlayer->GetAbsVelocity();

	AngularImpulse spin(0,0,0);
	SetAbsVelocity( vecReflectVel );
	VPhysicsGetObject()->SetVelocity( &vecReflectVel, &spin );

	if ( flBallSpeed > 300 )
	{
		EmitSound( "Passtime.BallSmack" );
	}
}

//-----------------------------------------------------------------------------
void CPasstimeBall::BlockDamage( CTFPlayer *pPlayer, const Vector& vecBallVel )
{
	const float flSpeed = vecBallVel.Length();
	const float flDamageSpeed = 1000;
	
	pPlayer->m_Shared.OnSpyTouchedByEnemy();

	if ( flSpeed >= flDamageSpeed )
	{
		CTakeDamageInfo di;
		di.SetAttacker( GetThrower() );
		di.SetDamage( 1 );
		di.SetDamageType( DMG_CLUB );
		di.SetInflictor( this );
		di.SetDamagePosition( GetAbsOrigin() );
		di.SetDamageForce( vecBallVel ); // needs to be set to nonzero
		if ( flSpeed > 1200 )
		{
			di.AddDamageType( DMG_CRITICAL );
		}
		pPlayer->TakeDamage( di );
	}
}

//-----------------------------------------------------------------------------
static bool IsGroundCollision( int index, const gamevcollisionevent_t *pEvent )
{
	// this little arcane incantation stolen from somewhere else
	const int otherindex = !index;
	IPhysicsObject *pPhysObj = pEvent->pObjects[otherindex];
	CBaseEntity *pOther = static_cast<CBaseEntity *>(pPhysObj->GetGameData());

	if ( !pOther || !pEvent->pInternalData )
	{
		return false; // paranoia
	}

	Vector vecNormal;
	pEvent->pInternalData->GetSurfaceNormal( vecNormal );
	return Vector( 0, 0, 1 ).Dot( vecNormal ) < -0.7f; // why is this backwards?
}

//-----------------------------------------------------------------------------
void CPasstimeBall::OnTouch( CBaseEntity *pOther )
{
	// If two players touch the ball in the same frame inside the physics system,
	// the ball will get a touch callback for both regardless of what happens
	// in response to the first call (i.e. it's just iterating a contact list).
	// This catches the case where the ball was already picked up this frame.
	if ( !TFGameRules()->IsPasstimeMode() || (m_eState != STATE_FREE) )
	{
		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !BIgnorePlayer( pPlayer ) )
	{
		TouchPlayer( pPlayer );
	}
}

//-----------------------------------------------------------------------------
void CPasstimeBall::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) 
{
	BaseClass::VPhysicsCollision( index, pEvent );
	
	if ( !TFGameRules()->IsPasstimeMode() )
	{
		return;
	}

	if ( g_pPasstimeLogic && (g_pPasstimeLogic->GetBall() == this) 
		&& g_pPasstimeLogic->OnBallCollision( this, index, pEvent )
		&& IsGroundCollision( index, pEvent ) )
	{
		OnCollision();
	}
	CPasstimeBallController::BallCollision( this, index, pEvent );
	m_hBlocker.Term();
}

//-----------------------------------------------------------------------------
void CPasstimeBall::OnCollision()
{
	m_flAirtimeDistance = 0;
	m_flLastCollisionTime = gpGlobals->curtime;
	++m_iCollisionCount;
	if ( m_iCollisionCount == 1 ) 
	{
		SetThrower( 0 );
		if ( m_bTouchedSinceSpawn )
		{
			SetIdleRespawnTime();
		}
	}
	m_hBlocker.Term();
}

//-----------------------------------------------------------------------------
int	CPasstimeBall::OnTakeDamage( const CTakeDamageInfo &info ) 
{
	if ( !tf_passtime_ball_takedamage.GetBool() )
	{
		// this can happen if the cvar is disabled after the ball has spawned
		return 0;
	}

	if ( !m_bTouchedSinceSpawn && (GetCollisionCount() == 0) )
	{
		++CTF_GameStats.m_passtimeStats.summary.nTotalBallSpawnShots;
	}

	if ( TFGameRules()->IsPasstimeMode() )
	{
		CPasstimeBallController::BallDamaged( this );
		CPasstimeBallController::DisableOn( this );
		OnCollision();
	}

	if ( IPhysicsObject* pPhysObj = VPhysicsGetObject() )
	{
		pPhysObj->EnableMotion( true );
		pPhysObj->ApplyForceOffset( info.GetDamageForce().Normalized() * tf_passtime_ball_takedamage_force.GetFloat(), GetAbsOrigin() );
	}

	return 0;
}

//-----------------------------------------------------------------------------
void CPasstimeBall::Deflected(CBaseEntity *pDeflectedBy, Vector& vecDir )
{
	NOTE_UNUSED( pDeflectedBy );
	IPhysicsObject* pPhysObj = VPhysicsGetObject();
	if ( !pPhysObj )
	{
		return;
	}

	// WeaponBase::DeflectEntity will redirect the velocity with the same flSpeed, 
	// which means that a stationary ball won't move since it has 0 flSpeed. this 
	// will just make sure the velocity is what it should be

	// vecDir points from the point under the player's crosshair to the ball's origin.
	// this will make ball deflection work just like rockets, except the velocity
	// is normalized instead of just being whatever magnitude it was before deflection.
	Vector vecVel = -vecDir * tf_passtime_ball_takedamage_force.GetFloat();
	pPhysObj->SetVelocity( &vecVel, 0 );

	if ( TFGameRules()->IsPasstimeMode() )
	{
		++CTF_GameStats.m_passtimeStats.summary.nTotalBallDeflects;

		// stop passing, etc
		CPasstimeBallController::DisableOn( this );

		// count as a collision
		OnCollision();
	}
}

//-----------------------------------------------------------------------------
//static 
CPasstimeBall *CPasstimeBall::Create( Vector vecPosition, QAngle angles )
{
	// mostly copied from CreatePhysicsToy
	MDLCACHE_CRITICAL_SECTION();
	MDLHandle_t hMdl = mdlcache->FindMDL( tf_passtime_ball_model.GetString() );
	Assert( hMdl != MDLHANDLE_INVALID );
	if( hMdl == MDLHANDLE_INVALID )
	{
		return 0;
	}

	studiohdr_t *pStudioHdr = mdlcache->GetStudioHdr( hMdl );
	Assert( pStudioHdr );
	if( !pStudioHdr ) 
	{
		return 0;
	}

	// i don't know what this "allow precache" stuff does, 
	// i copied it from other code and forgot to note where it was
	bool oldAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true ); 

	CPasstimeBall *pBall = dynamic_cast< CPasstimeBall* >( CreateEntityByName( "passtime_ball" ) ); 

	char pszBuf[512];
	Q_snprintf( pszBuf, sizeof( pszBuf ), "%.10f %.10f %.10f", vecPosition.x, vecPosition.y, vecPosition.z );
	pBall->KeyValue( "origin", pszBuf );
	Q_snprintf( pszBuf, sizeof( pszBuf ), "%.10f %.10f %.10f", angles.x, angles.y, angles.z );
	pBall->KeyValue( "angles", pszBuf );
	pBall->KeyValue( "fademindist", "-1" );
	pBall->KeyValue( "fademaxdist", "0" );
	pBall->KeyValue( "fadescale", "1" );
	DispatchSpawn( pBall );
	pBall->Activate();

	CBaseEntity::SetAllowPrecache( oldAllowPrecache );
	mdlcache->Release( hMdl );
	return pBall;
}

//-----------------------------------------------------------------------------
void CPasstimeBall::SetHomingTarget( CTFPlayer *pPlayer ) 
{ 
	m_hHomingTarget = pPlayer; 
	if ( m_hHomingTarget )
	{
		if ( !m_pBeepLoop )
		{
			CReliableBroadcastRecipientFilter filter;
			m_pBeepLoop = CSoundEnvelopeController::GetController().SoundCreate( 
				filter, entindex(), "Passtime.BallHoming" );
			CSoundEnvelopeController::GetController().Play( m_pBeepLoop, 1, PITCH_NORM );
		}
	}
	else
	{
		if ( m_pBeepLoop )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pBeepLoop );
			m_pBeepLoop = 0;
		}
	}
}

//-----------------------------------------------------------------------------
CTFPlayer *CPasstimeBall::GetHomingTarget() const
{
	return m_hHomingTarget;
}

//-----------------------------------------------------------------------------
float CPasstimeBall::GetAirtimeSec() const 
{
	return MAX( 0, gpGlobals->curtime - m_flLastCollisionTime );
}

//-----------------------------------------------------------------------------
float CPasstimeBall::GetAirtimeDistance() const 
{
	return m_flAirtimeDistance;
}

