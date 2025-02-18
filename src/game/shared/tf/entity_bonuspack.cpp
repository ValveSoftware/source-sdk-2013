//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#include "cbase.h"
#include "entity_bonuspack.h"

#ifdef GAME_DLL
	#include "tf_logic_robot_destruction.h"
	#include "tf_player.h"
	#include "particle_parse.h"
	#include "tf_fx.h"
#endif

#define TF_POWERCORE_RED_PICKUP "powercore_embers_red"
#define TF_POWERCORE_BLUE_PICKUP "powercore_embers_blue"
#define BONUS_PACK_BLINK_CONTEXT "blink_think"

ConVar tf_bonuspack_score( "tf_bonuspack_score", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
#define BLINK_TIME 5.f
#define REMOVE_TIME 20.f
#define PICKUP_TIME 0.5f

IMPLEMENT_NETWORKCLASS_ALIASED( BonusPack, DT_CBonusPack )

BEGIN_NETWORK_TABLE( CBonusPack, DT_CBonusPack  )
END_NETWORK_TABLE()

BEGIN_DATADESC( CBonusPack )
END_DATADESC()

LINK_ENTITY_TO_CLASS( item_bonuspack, CBonusPack );

IMPLEMENT_AUTO_LIST( IBonusPackAutoList );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBonusPack::CBonusPack()
{
#ifdef GAME_DLL
	m_bAutoMaterialize = false;
#else
	SetCycle( RandomFloat() );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBonusPack::Spawn( void )
{
	Precache();
	BaseClass::BaseClass::Spawn();

#ifdef GAME_DLL
	const char *pszParticleName = GetTeamNumber() == TF_TEAM_RED ? "powercore_alert_blue" : "powercore_alert_red";
	DispatchParticleEffect( pszParticleName, PATTACH_POINT_FOLLOW, this, "particle_spawn" );

	SetModel( GetPowerupModel() );
	CollisionProp()->UseTriggerBounds( true, 64 );
	m_flCanPickupTime = gpGlobals->curtime + PICKUP_TIME;
	m_nBlinkCount = 0;
	m_flKillTime = gpGlobals->curtime + REMOVE_TIME + BLINK_TIME;
	SetContextThink( &CBonusPack::BlinkThink, gpGlobals->curtime + REMOVE_TIME, BONUS_PACK_BLINK_CONTEXT );
	SetContextThink( &CBonusPack::SUB_Remove, m_flKillTime, "RemoveThink" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBonusPack::Precache( void )
{
	// We deliberately allow late precaches here
	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );
	PrecacheParticleSystem( TF_POWERCORE_RED_PICKUP );
	PrecacheParticleSystem( TF_POWERCORE_BLUE_PICKUP );
	BaseClass::Precache();
	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBonusPack::MyTouch( CBasePlayer *pPlayer )
{
	if ( ValidTouch( pPlayer ) && gpGlobals->curtime >= m_flCanPickupTime )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return true;

		// Play a particle colored the color of the player that picked it up
		Vector vecOrigin = GetAbsOrigin() + Vector( 0,0,5 );
		CPVSFilter pvsfilter( vecOrigin );
		const char *pszParticleName = pPlayer->GetTeamNumber() == TF_TEAM_RED ? TF_POWERCORE_RED_PICKUP : TF_POWERCORE_BLUE_PICKUP;
		TE_TFParticleEffect( pvsfilter, 0.f, pszParticleName, vecOrigin, vec3_angle );

 		if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
 		{
 			CTFRobotDestructionLogic::GetRobotDestructionLogic()->ScorePoints( GetTeamNumber()
																			  , tf_bonuspack_score.GetInt()
																			  , SCORE_CORES_COLLECTED
																			  , ToTFPlayer( pPlayer ) );
 		}

		int iBoostMax = pTFPlayer->m_Shared.GetMaxBuffedHealth();
		// Cap it to the max we'll boost a player's health
		int nHealthToAdd = clamp( 5, 0, iBoostMax - pTFPlayer->GetHealth() );
		// Give health
		pPlayer->TakeHealth( nHealthToAdd, DMG_GENERIC | DMG_IGNORE_MAXHEALTH );

		for ( int i=0;i<TF_AMMO_COUNT;i++ )
		{
			pPlayer->GiveAmmo( 5, i );
		}

		pPlayer->SetLastObjectiveTime( gpGlobals->curtime );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBonusPack::ValidTouch( CBasePlayer *pPlayer )
{
	if ( ( GetTeamNumber() > LAST_SHARED_TEAM ) && ( pPlayer->GetTeamNumber() != GetTeamNumber() ) )
		return false;

	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

	// No invis spies
	if ( pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED ) || pTFPlayer->m_Shared.GetPercentInvisible() > 0.25f )
		return false;

	// No disguised spies
	if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) || pTFPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
		return false;

	// No bonk'd scouts
	if ( pTFPlayer->m_Shared.InCond( TF_COND_PHASE ) || pTFPlayer->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
		return false;

	// No teleporting players
	if ( pTFPlayer->m_Shared.InCond( TF_COND_SELECTED_TO_TELEPORT ) )
		return false;

	// No invulns
	if ( pTFPlayer->m_Shared.IsInvulnerable() )
		return false;

	return BaseClass::ValidTouch( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBonusPack::BlinkThink()
{
	float flTimeToKill = m_flKillTime - gpGlobals->curtime;
	float flNextBlink = RemapValClamped( flTimeToKill, BLINK_TIME, 0.f, 0.5f, 0.1f );

	SetContextThink( &CBonusPack::BlinkThink, gpGlobals->curtime + flNextBlink, BONUS_PACK_BLINK_CONTEXT );

	SetRenderMode( kRenderTransAlpha );

	++m_nBlinkCount;
	if ( m_nBlinkCount % 2 == 0 )
	{
		SetRenderColorA( 25 );
	}
	else
	{
		SetRenderColorA( 255 );
	}
}
#endif
