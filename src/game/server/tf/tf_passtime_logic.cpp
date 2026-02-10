//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_passtime_logic.h"
#include "countdown_announcer.h"
#include "entity_passtime_ball_spawn.h"
#include "func_passtime_goal.h"
#include "func_passtime_no_ball_zone.h"
#include "tf_passtime_ball.h"
#include "passtime_ballcontroller.h"
#include "passtime_convars.h"
#include "passtime_game_events.h"
#include "func_passtime_goalie_zone.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"
#include "pathtrack.h"
#include "tf_fx.h"
#include "tf_weapon_passtime_gun.h"
#include "team_objectiveresource.h"
#include "mapentities.h"
#include "soundenvelope.h"
#include "eventqueue.h"
#include "hl2orange.spa.h" // achievement defines from tf_shareddefs depend on this
#include "tier0/memdbgon.h"

CTFPasstimeLogic *g_pPasstimeLogic;

#ifdef _DEBUG
#define SECRETROOM_LOG Warning
#else
#define SECRETROOM_LOG (void)
#endif

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( passtime_logic, CTFPasstimeLogic );
PRECACHE_REGISTER( passtime_logic );
IMPLEMENT_SERVERCLASS_ST( CTFPasstimeLogic, DT_TFPasstimeLogic )
	SendPropEHandle( SENDINFO( m_hBall ) ),
	SendPropArray( SendPropVector( SENDINFO_ARRAY( m_trackPoints ), -1, SPROP_COORD_MP_INTEGRAL ), m_trackPoints ),
	SendPropInt( SENDINFO( m_iNumSections ) ),
	SendPropInt( SENDINFO( m_iCurrentSection ) ),
	SendPropFloat( SENDINFO( m_flMaxPassRange ) ),
	SendPropInt( SENDINFO( m_iBallPower ), 8 ),
	SendPropFloat( SENDINFO( m_flPackSpeed ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerIsPackMember ), SendPropInt( SENDINFO_ARRAY( m_bPlayerIsPackMember ), 1, SPROP_UNSIGNED ) ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
BEGIN_DATADESC( CTFPasstimeLogic )
	DEFINE_KEYFIELD( m_iNumSections, FIELD_INTEGER, "num_sections" ),
	DEFINE_KEYFIELD( m_iBallSpawnCountdownSec, FIELD_INTEGER, "ball_spawn_countdown" ),
	DEFINE_KEYFIELD( m_flMaxPassRange, FIELD_FLOAT, "max_pass_range" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "SpawnBall", InputSpawnBall ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetSection", InputSetSection ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TimeUp", InputTimeUp ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SpeedBoostUsed", InputSpeedBoostUsed ),
	DEFINE_INPUTFUNC( FIELD_VOID, "JumpPadUsed", InputJumpPadUsed ),
	
	// secret room inputs
	// these strings are obfuscated for fun, not for protection
	DEFINE_INPUTFUNC( FIELD_VOID, "statica", statica ), // SecretRoom_InputStartTouchPlayerSlot NOTE: intentionally not in FGD
	DEFINE_INPUTFUNC( FIELD_VOID, "staticb", staticb ), // SecretRoom_InputEndTouchPlayerSlot NOTE: intentionally not in FGD
	DEFINE_INPUTFUNC( FIELD_VOID, "staticc", staticc ), // SecretRoom_InputPlugDamaged NOTE: intentionally not in FGD
	DEFINE_INPUTFUNC( FIELD_VOID, "RoomTriggerOnTouch", InputRoomTriggerOnTouch ),

	DEFINE_OUTPUT( m_onBallFree, "OnBallFree" ),
	DEFINE_OUTPUT( m_onBallGetBlu, "OnBallGetBlu" ),
	DEFINE_OUTPUT( m_onBallGetRed, "OnBallGetRed" ),
	DEFINE_OUTPUT( m_onBallGetAny, "OnBallGetAny" ),
	DEFINE_OUTPUT( m_onBallRemoved, "OnBallRemoved" ),
	DEFINE_OUTPUT( m_onScoreBlu, "OnScoreBlu" ),
	DEFINE_OUTPUT( m_onScoreRed, "OnScoreRed" ),
	DEFINE_OUTPUT( m_onScoreAny, "OnScoreAny" ),
	DEFINE_OUTPUT( m_onBallPowerUp, "OnBallPowerUp" ),
	DEFINE_OUTPUT( m_onBallPowerDown, "OnBallPowerDown" ),
END_DATADESC()

//-----------------------------------------------------------------------------
static const CCountdownAnnouncer::TimeSounds sCountdownSoundsRoundBegin = {
	"Announcer.RoundBegins60seconds",
	"Announcer.RoundBegins30seconds",
	"Announcer.RoundBegins10seconds",
	"Announcer.RoundBegins5seconds",
	"Announcer.RoundBegins4seconds",
	"Announcer.RoundBegins3seconds",
	"Announcer.RoundBegins2seconds",
	"Announcer.RoundBegins1seconds",
};

//-----------------------------------------------------------------------------
static const CCountdownAnnouncer::TimeSounds sCountdownSoundsRoundBeginMerasmus = {
	"Announcer.RoundBegins60seconds",
	"Announcer.RoundBegins30seconds",
	"Announcer.RoundBegins10seconds",
	"Merasmus.RoundBegins5seconds",
	"Merasmus.RoundBegins4seconds",
	"Merasmus.RoundBegins3seconds",
	"Merasmus.RoundBegins2seconds",
	"Merasmus.RoundBegins1seconds",
};

//-----------------------------------------------------------------------------
static bool IsGamestatePlayable()
{
	gamerules_roundstate_t state = TFGameRules()->State_Get();
	return (state == GR_STATE_RND_RUNNING) || (state == GR_STATE_STALEMATE);
}

//-----------------------------------------------------------------------------
CPasstimeBall *CTFPasstimeLogic::GetBall() const { return m_hBall; }

//-----------------------------------------------------------------------------
CTFPasstimeLogic::CTFPasstimeLogic() 
{ 
	m_SecretRoom_pTv = nullptr;
	m_SecretRoom_pTvSound = nullptr;
	m_SecretRoom_state = SecretRoomState::None;
	memset( m_SecretRoom_slottedPlayers, 0, sizeof( m_SecretRoom_slottedPlayers ) );

	m_flNextCrowdReactionTime = 0.0f;
	m_nPackMemberBits = 0;
	m_nPrevPackMemberBits = 0;
}

//-----------------------------------------------------------------------------
CTFPasstimeLogic::~CTFPasstimeLogic() 
{
	// note:
	// it doesn't seem possible on the server that this destructor would be called
	// after a new CTFPasstimeLogic is spawned, and it's worked fine so far, but
	// this has been a problem in the client code.
	g_pPasstimeLogic = NULL;
	delete m_pRespawnCountdown;
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::Spawn()
{
	g_pPasstimeLogic = this;
	m_iBallSpawnCountdownSec = MAX( 1, m_iBallSpawnCountdownSec );
	if ( m_flMaxPassRange == 0 )
	{
		m_flMaxPassRange = FLT_MAX;
	}

	for ( int i = 0; i < m_bPlayerIsPackMember.Count(); ++i )
	{
		m_bPlayerIsPackMember.Set( i, 0 );
	}

	for ( int i = 0; i < m_trackPoints.Count(); ++i )
	{
		m_trackPoints.GetForModify(i).Zero();
	}

	const auto *pCountdownSounds = TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Halloween )
		? &sCountdownSoundsRoundBeginMerasmus
		: &sCountdownSoundsRoundBegin;
	m_pRespawnCountdown = new CCountdownAnnouncer( pCountdownSounds );

	SetContextThink( &CTFPasstimeLogic::PostSpawn, gpGlobals->curtime, "postspawn" );
	SetContextThink( &CTFPasstimeLogic::BallPower_PackHealThink, gpGlobals->curtime + 1, "packheal" );

	ListenForGameEvent( "teamplay_round_stalemate" );
	ListenForGameEvent( "teamplay_setup_finished" );
}

//------------------------------------------------------------------------------
// Purpose: Utility function for hooking up entity connections from code.
// Would belong in BaseEnity, but this this hacky so I'm just hiding it here.
//------------------------------------------------------------------------------
static CBaseEntityOutput *FindOutput( CBaseEntity *pEnt, const char *pOutputName )
{
	if ( !pEnt || !pOutputName || !pOutputName[0] )
	{
		return nullptr;
	}

	// loop taken from ValidateEntityConnections
	datamap_t *dmap = pEnt->GetDataDescMap();
	while ( dmap )
	{
		int fields = dmap->dataNumFields;
		for ( int i = 0; i < fields; i++ )
		{
			typedescription_t *dataDesc = &dmap->dataDesc[i];
			if ( ( dataDesc->fieldType == FIELD_CUSTOM ) 
				&& ( dataDesc->flags & FTYPEDESC_OUTPUT ) 
				&& !strcmp( dataDesc->externalName, pOutputName ) )
			{
				return (CBaseEntityOutput *)((intptr_t)pEnt + (int)dataDesc->fieldOffset[0]);
			}
		}
		dmap = dmap->baseMap;
	}

	return nullptr;
}

//------------------------------------------------------------------------------
// Purpose: Utility function for hooking up entity connections from code.
// Would belong in BaseEnity, but this this hacky so I'm just hiding it here.
//------------------------------------------------------------------------------
static void HookOutput( const char *pSourceName, string_t pTargetName, 
	const char *pOutputName, const char *pInputName, 
	const char *pParameter = nullptr, int nTimesToFire = EVENT_FIRE_ALWAYS )
{
	Assert( pSourceName && pSourceName[0] );
	Assert( pTargetName.ToCStr() && pTargetName.ToCStr()[0] );
	CBaseEntity *pEnt = gEntList.FindEntityByName( nullptr, pSourceName );
	if ( !pEnt )
	{
		Warning( "Entity %s missing", pSourceName );
		return;
	}

	CBaseEntityOutput *pOut = FindOutput( pEnt, pOutputName );
	if ( !pOut )
	{
		Warning( "Entity %s missing output %s", pSourceName, pOutputName );
		return;
	}

	CEventAction *pAction = new CEventAction();
	pAction->m_iTarget = pTargetName;
	pAction->m_iTargetInput = AllocPooledString( pInputName );
	pAction->m_nTimesToFire = nTimesToFire;
	pAction->m_iParameter = pParameter 
		? AllocPooledString( pParameter ) 
		: string_t();
	pOut->AddEventAction( pAction );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::PostSpawn()
{
	// This can't be done in spawn because GetTeamNumber doesn't return the
	// correct value for any entity until after it's been Activate()d, which
	// happens after all the Spawns. And it can't be done in Activate() because
	// the order of Activation seems kinda non-deterministic.
	const auto &goals = CFuncPasstimeGoal::GetAutoList();

	if ( ( m_iNumSections == 0 ) && ( goals.Count() == 2 ) )
	{
		// FIXME support > 2 goals properly
		CFuncPasstimeGoal *pRed = static_cast<CFuncPasstimeGoal *>( goals[0] );
		CFuncPasstimeGoal *pBlu = static_cast<CFuncPasstimeGoal *>( goals[1] );
		if ( pRed->GetTeamNumber() != TF_TEAM_BLUE ) // goal's color is who can score there
		{
			V_swap( pRed, pBlu );
		}
		m_trackPoints.Set( 0, pBlu->GetAbsOrigin() );
		m_trackPoints.Set( 1, pRed->GetAbsOrigin() );
		m_iNumSections = 1;
	}

	//
	// Determine goal type for stats
	//
	int nTotalEndzone = 0;
	int nTotalBasket = 0;
	for ( const auto *pGoalNode : goals )
	{
		const auto *pGoal = (const CFuncPasstimeGoal *)pGoalNode;
		if ( !pGoal->BDisableBallScore() )
		{
			++nTotalBasket;
		}
		if ( pGoal->BEnablePlayerScore() )
		{
			++nTotalEndzone;
		}
	}
	if ( nTotalBasket && !nTotalEndzone )
	{
		CTF_GameStats.m_passtimeStats.summary.nGoalType = 1;
	}
	else if ( !nTotalBasket && nTotalEndzone )
	{
		CTF_GameStats.m_passtimeStats.summary.nGoalType = 2;
	}
	else
	{
		CTF_GameStats.m_passtimeStats.summary.nGoalType = 3;
	}

	CTF_GameStats.m_passtimeStats.summary.nRoundMaxSec = TFGameRules()->GetActiveRoundTimer()->GetTimerInitialLength();

	// These used to happen from teamplay_setup_ended, but that event doesn't happen if there's no setup time
	// These functions should be able to determine whether or not to actually do anything based on game state
	SetContextThink( &CTFPasstimeLogic::BallHistSampleThink, gpGlobals->curtime, "BallHistSampleThink" );
	SetContextThink( &CTFPasstimeLogic::OneSecStatsUpdateThink, gpGlobals->curtime, "OneSecStatsUpdateThink" );

	BallPower_PowerThink();
	BallPower_PackThink();

	// secret room puzzle
	if ( !V_stricmp( gpGlobals->mapname.ToCStr(), "pass_brickyard" ) )
	{
		SecretRoom_Spawn();
	}
}

//-----------------------------------------------------------------------------
bool CTFPasstimeLogic::AddBallPower( int iPower )
{
	int iThreshold = tf_passtime_powerball_threshold.GetInt();
	bool bWasAboveThreshold = m_iBallPower > iThreshold;
	m_iBallPower = clamp( m_iBallPower + iPower, 0, 100 );
	bool bIsAboveThreshold = m_iBallPower > iThreshold;
	if ( bWasAboveThreshold && !bIsAboveThreshold ) 
	{
		m_onBallPowerDown.FireOutput( this, this );
		TFGameRules()->BroadcastSound( 255, "Powerup.Reflect.Reflect" );
		return true;
	}
	else if ( !bWasAboveThreshold && bIsAboveThreshold )
	{
		m_onBallPowerUp.FireOutput( this, this );
		TFGameRules()->BroadcastSound( 255, "Powerup.Volume.Use" );

		// reschedule think so that decay stops for a while
		SetContextThink( &CTFPasstimeLogic::BallPower_PowerThink, 
			gpGlobals->curtime + tf_passtime_powerball_decay_delay.GetFloat(),
			"BallPower_PowerThink" );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::ClearBallPower()
{
	AddBallPower( -m_iBallPower );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::BallPower_PowerThink()
{
	CPasstimeBall *pBall = GetBall();
	if ( !IsGamestatePlayable() || !pBall )
	{
		SetContextThink( &CTFPasstimeLogic::BallPower_PowerThink, 
			gpGlobals->curtime, "BallPower_PowerThink" );
		return;
	}

	float flTickTime = (pBall->GetTeamNumber() == TEAM_UNASSIGNED)
		? tf_passtime_powerball_decaysec_neutral.GetFloat()
		: tf_passtime_powerball_decaysec.GetFloat();

	SetContextThink( &CTFPasstimeLogic::BallPower_PowerThink,
		gpGlobals->curtime + flTickTime, "BallPower_PowerThink" );


	if ( !pBall->GetHomingTarget() )
	{
		AddBallPower( -tf_passtime_powerball_decayamount.GetInt() );
	}
}

//-----------------------------------------------------------------------------
static uint64 CalcPackMemberBits( CTFPlayer *pBallCarrier ) 
{
	if ( !pBallCarrier || !pBallCarrier->IsAlive() ) 
	{
		return 0;
	}

	float flPackRangeSqr = tf_passtime_pack_range.GetFloat();
	flPackRangeSqr *= flPackRangeSqr;
	int iCarrierTeam = pBallCarrier->GetTeamNumber();
	Vector vecCarrierPos = pBallCarrier->GetAbsOrigin();
	uint64 nNewPackMemberBits = 0;
	uint64 nMask = 1;
	for ( int i = 1; i <= MAX_PLAYERS; ++i, nMask <<= 1 )
	{
		CTFPlayer *pPlayer = (CTFPlayer*) UTIL_PlayerByIndex( i );

		// must be a valid team member within range
		if ( !pPlayer
			|| !pPlayer->IsAlive()
			|| ( pPlayer->GetTeamNumber() != iCarrierTeam )
			|| ( pPlayer->GetAbsOrigin().DistToSqr( vecCarrierPos ) > flPackRangeSqr ) )
		{
			continue;
		}

		// must not be aiming (heavy spin, sniper scope, etc)
		if ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
		{
			continue;
		}

		nNewPackMemberBits |= nMask;
	}
	return nNewPackMemberBits;
}

//-----------------------------------------------------------------------------
static void SetSpeedOnFlaggedPlayers( uint64 playerBits )
{
	if ( playerBits == 0 )
	{
		return;
	}

	uint64 nMask = 1;
	for ( int i = 1; i <= MAX_PLAYERS; ++i, nMask <<= 1 )
	{
		if ( playerBits & nMask )
		{
			CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer->IsAlive() )
			{
				pPlayer->TeamFortress_SetSpeed();
			}
		}
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::ReplicatePackMemberBits()
{
	uint64 nMask = 1;
	for ( int i = 1; i <= MAX_PLAYERS; ++i, nMask <<= 1 )
	{
		m_bPlayerIsPackMember.Set( i, ( m_nPackMemberBits & nMask ) ? 1 : 0 );
	}
}

//-----------------------------------------------------------------------------
// solo carrier is marked for death
// any close teammate (2x cart distance) removes marked for death
// close teammates are sped up to fastest teammate speed
void CTFPasstimeLogic::BallPower_PackThink()
{
	SetContextThink( &CTFPasstimeLogic::BallPower_PackThink, 
		gpGlobals->curtime, "BallPower_PackThink" );

	m_flPackSpeed = 0.0f;
	m_nPrevPackMemberBits = m_nPackMemberBits;
	m_nPackMemberBits = 0;

	CTFPlayer *pCarrier = GetBallCarrier();

	// Check if pack speed is active
	if ( !tf_passtime_pack_speed.GetBool() || !IsGamestatePlayable() || !pCarrier )
	{
		m_nPackMemberBits = 0; // redundant assignment for clarity
		ReplicatePackMemberBits();
		SetSpeedOnFlaggedPlayers( m_nPrevPackMemberBits );
		return;
	}

	// Find the pack members
	m_nPackMemberBits = CalcPackMemberBits( pCarrier );
	ReplicatePackMemberBits();
	
	// Find the maximum MaxSpeed of the pack
	bool bHasNearbyTeammate = false;
	float flMaxMaxSpeed = -1;
	uint64 nMask = 1;
	for ( int i = 1; i <= MAX_PLAYERS; ++i, nMask <<= 1 )
	{
		if ( m_nPackMemberBits & nMask )
		{
			CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer->IsAlive() )
			{
				bHasNearbyTeammate = bHasNearbyTeammate || ( pPlayer != pCarrier );
				flMaxMaxSpeed = MAX( flMaxMaxSpeed, pPlayer->TeamFortress_CalculateMaxSpeed() );
			}
		}
	}

	// Apply marked for death if no teammates
	if ( !bHasNearbyTeammate )
	{
		pCarrier->m_Shared.AddCond( TF_COND_PASSTIME_PENALTY_DEBUFF, TICK_INTERVAL * 2 );
	}
	else
	{
		m_flPackSpeed = flMaxMaxSpeed;
	}

	// Now tell all the relevant players to refresh their maxspeed
	SetSpeedOnFlaggedPlayers( m_nPackMemberBits | m_nPrevPackMemberBits );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::BallPower_PackHealThink()
{
	SetContextThink( &CTFPasstimeLogic::BallPower_PackHealThink, gpGlobals->curtime + 1, "packheal" );

	CTFPlayer *pCarrier = GetBallCarrier();
	if ( !pCarrier )
	{
		return;
	}

	uint64 nMask = 1;
	uint64 nPackMemberBits = m_nPackMemberBits;
	float flHealAmount = tf_passtime_pack_hp_per_sec.GetFloat();
	for ( int i = 1; i <= MAX_PLAYERS; ++i, nMask <<= 1 )
	{
		if ( ( nPackMemberBits & nMask ) == 0 )
		{
			continue;
		}

		CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( i );
		if ( !pPlayer || ( pPlayer == pCarrier ) || !pPlayer->IsAlive() )
		{
			continue;
		}

		int iActualHealAmount = pPlayer->TakeHealth( flHealAmount, DMG_GENERIC );
		if ( iActualHealAmount <= 0 )
		{
			continue;
		}

		// I'm abusing the player_healonhit event because it does the visual fx I want
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( pEvent )
		{
			pEvent->SetInt( "amount", iActualHealAmount );
			pEvent->SetInt( "entindex", pPlayer->entindex() );
			gameeventmanager->FireEvent( pEvent ); 
		}
	}
}

//-----------------------------------------------------------------------------
float CTFPasstimeLogic::GetPackSpeed( CTFPlayer *pPlayer ) const
{
	if ( pPlayer )
	{
		uint64 nMask = (uint64)1 << ( pPlayer->entindex() - 1 );
		if ( m_nPackMemberBits & nMask )
		{
			return m_flPackSpeed;
		}
	}
	return 0;
}


//-----------------------------------------------------------------------------
void CTFPasstimeLogic::FireGameEvent( IGameEvent *pEvent ) 
{
	const char *pEventName = pEvent->GetName();
	if ( !V_strcmp( pEventName, "teamplay_round_stalemate" ) )
	{
		// this only happens when mp_stalemate_enable is on
		CTF_GameStats.m_passtimeStats.summary.nRoundMaxSec = 
			TFGameRules()->GetActiveRoundTimer()->GetTimeRemaining();
		RespawnBall();
	}
	else if ( !V_strcmp( pEventName, "teamplay_setup_finished" ) )
	{
		// respawn the ball even though it already exists so that it doesn't
		// catch any rotation from any spawn box it might be sitting in that
		// hasn't opened yet.
		SpawnBallAtRandomSpawner();
	}
}

//-----------------------------------------------------------------------------
// FIXME copypasta with tf_hud_passtime.cpp
// For stats
float CTFPasstimeLogic::CalcProgressFrac() const
{
	if ( !GetBall() || (m_iNumSections == 0) )
	{
		return 0;
	}

	//
	// Which point are we trying to classify?
	//
	Vector vecOrigin;
	{
		CPasstimeBall *pBall = GetBall();
		CTFPlayer *pCarrier = pBall->GetCarrier();
		vecOrigin = pCarrier 
			? pCarrier->GetAbsOrigin()
			: pBall->GetAbsOrigin();
	}

	// 
	// Find distance along track from first goal to last goal
	//
	float flBestLen = 0;
	float flTotalLen = 1; // don't set 0 so div by zero is impossible
	{
		float flBestDist = FLT_MAX;

		Vector vecThisPoint;
		Vector vecPointOnLine;
		Vector vecPrevPoint = m_trackPoints[0];
		float flThisFrac = 0;
		float flThisLen = 0;
		float flThisDist = 0;
		for ( int i = 1; i < 16; ++i )
		{
			vecThisPoint = m_trackPoints[i];
			if ( vecThisPoint.IsZero() )
			{
				break;
			}
			flThisLen = (vecThisPoint - vecPrevPoint).Length();
			flTotalLen += flThisLen;
			CalcClosestPointOnLineSegment( vecOrigin, vecPrevPoint, vecThisPoint, vecPointOnLine, &flThisFrac );
			flThisDist = (vecPointOnLine - vecOrigin).Length();
			if ( flThisDist < flBestDist )
			{
				flBestDist = flThisDist;
				flBestLen = flTotalLen - (flThisLen * (1.0f - flThisFrac));
			}
			vecPrevPoint = vecThisPoint;
		}
	}

	return (float)(flBestLen / flTotalLen);
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::BallHistSampleThink()
{
	CPasstimeBall *pBall = m_hBall;
	if ( IsGamestatePlayable() && pBall && !pBall->BOutOfPlay() )
	{
		CTF_GameStats.m_passtimeStats.AddBallFracSample( CalcProgressFrac() );
	}

	SetContextThink( &CTFPasstimeLogic::BallHistSampleThink, gpGlobals->curtime + 0.125f, "BallHistSampleThink" );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::OneSecStatsUpdateThink()
{
	CTFTeam *pBlue = GetGlobalTFTeam( TF_TEAM_BLUE );
	CTFTeam *pRed = GetGlobalTFTeam( TF_TEAM_RED );

	// FIXME this is a hack but it'll work for now
	CTF_GameStats.m_passtimeStats.summary.nPlayersBlueMax = MAX( CTF_GameStats.m_passtimeStats.summary.nPlayersBlueMax, pBlue->GetNumPlayers() );
	CTF_GameStats.m_passtimeStats.summary.nPlayersRedMax = MAX( CTF_GameStats.m_passtimeStats.summary.nPlayersRedMax, pRed->GetNumPlayers() );
	
	SetContextThink( &CTFPasstimeLogic::OneSecStatsUpdateThink, gpGlobals->curtime + 1, "OneSecStatsUpdateThink" );
}

//-----------------------------------------------------------------------------
static void MapEventStat( CBaseEntity *pActivator, CPasstimeBall *pBall, int *pTotal, int *pCarrierTotal )
{
	CTFPlayer *pPlayer = ToTFPlayer( pActivator );
	if ( pPlayer )
	{
		++(*pTotal);
		if ( pBall && (pBall->GetCarrier() == pPlayer) )
		{
			++(*pCarrierTotal);
		}
	}
}

//-----------------------------------------------------------------------------
CTFPlayer *CTFPasstimeLogic::GetBallCarrier() const
{
	const CPasstimeBall *pBall = m_hBall.Get();
	if ( !pBall )
	{
		return nullptr;
	}
	return pBall->GetCarrier();
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::InputSpeedBoostUsed( inputdata_t &input )
{
	MapEventStat( input.pActivator, GetBall(), 
		&CTF_GameStats.m_passtimeStats.summary.nTotalSpeedBoosts,
		&CTF_GameStats.m_passtimeStats.summary.nTotalCarrierSpeedBoosts );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::InputJumpPadUsed( inputdata_t &input )
{
	MapEventStat( input.pActivator, GetBall(), 
		&CTF_GameStats.m_passtimeStats.summary.nTotalJumpPads,
		&CTF_GameStats.m_passtimeStats.summary.nTotalCarrierJumpPads );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::Precache()
{
	PrecacheScriptSound( "Passtime.BallIntercepted" );
	PrecacheScriptSound( "Passtime.BallStolen" );
	PrecacheScriptSound( "Passtime.BallDropped" );
	PrecacheScriptSound( "Passtime.BallCatch" );
	PrecacheScriptSound( "Passtime.BallSpawn" );

	PrecacheScriptSound( "Passtime.Crowd.Boo" );
	PrecacheScriptSound( "Passtime.Crowd.Cheer" );
	PrecacheScriptSound( "Passtime.Crowd.React.Neg" );
	PrecacheScriptSound( "Passtime.Crowd.React.Pos" );

	PrecacheScriptSound( "Powerup.Reflect.Reflect" ); // for powerball
	PrecacheScriptSound( "Powerup.Volume.Use" );

	PrecacheScriptSound( "Announcer.RoundBegins60seconds");
	PrecacheScriptSound( "Announcer.RoundBegins30seconds");
	PrecacheScriptSound( "Announcer.RoundBegins10seconds");

	if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Halloween ) ) 
	{
		PrecacheScriptSound( "Merasmus.RoundBegins5seconds");
		PrecacheScriptSound( "Merasmus.RoundBegins4seconds");
		PrecacheScriptSound( "Merasmus.RoundBegins3seconds");
		PrecacheScriptSound( "Merasmus.RoundBegins2seconds");
		PrecacheScriptSound( "Merasmus.RoundBegins1seconds");

		PrecacheScriptSound( "sf14.Merasmus.Soccer.GoalRed" );
		PrecacheScriptSound( "sf14.Merasmus.Soccer.GoalBlue" );
		PrecacheScriptSound( "Passtime.Merasmus.Laugh" );
	}
	else
	{
		PrecacheScriptSound( "Announcer.RoundBegins5seconds");
		PrecacheScriptSound( "Announcer.RoundBegins4seconds");
		PrecacheScriptSound( "Announcer.RoundBegins3seconds");
		PrecacheScriptSound( "Announcer.RoundBegins2seconds");
		PrecacheScriptSound( "Announcer.RoundBegins1seconds");
	}

	PrecacheScriptSound( "Game.Overtime");
	PrecacheScriptSound( "Passtime.AskForBall" );

	// secret room stuff
	if ( !V_stricmp( gpGlobals->mapname.ToCStr(), "pass_brickyard" ) )
	{
		PrecacheScriptSound( "Passtime.Tv1" );
		PrecacheScriptSound( "Passtime.Tv2" );
		PrecacheScriptSound( "Passtime.Tv3" );
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::OnEnterGoal( CPasstimeBall *pBall, CFuncPasstimeGoal *pGoal )
{
	if ( pGoal->BDisableBallScore() || !IsGamestatePlayable() )
	{
		return;
	}

	// -1 iPoints is a special hacked value that means "kill zone"
	if ( pGoal->Points() == -1 ) 
	{
		m_onBallRemoved.FireOutput( pGoal, pGoal );
		SetContextThink( &CTFPasstimeLogic::RespawnBall, gpGlobals->curtime, "spawnball" );
		return;
	}

	if ( (pBall->GetCollisionCount() > 0) || (pBall->GetTeamNumber() == TEAM_UNASSIGNED) )
	{
		return;
	}

	CTFPlayer *pOwner = pBall->GetThrower();
	if ( pOwner && (pBall->GetTeamNumber() == pGoal->GetTeamNumber()) )
	{
		Score( pBall, pGoal );
	}
}


//-----------------------------------------------------------------------------
void CTFPasstimeLogic::OnEnterGoal( CTFPlayer *pPlayer, CFuncPasstimeGoal *pGoal )
{
	if ( IsGamestatePlayable() 
		&& pGoal->BEnablePlayerScore()
		&& pPlayer->m_Shared.HasPasstimeBall() 
		&& (pPlayer->GetTeamNumber() == pGoal->GetTeamNumber()) )
	{
		Score( pPlayer, pGoal );
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::OnExitGoal( CPasstimeBall *pBall, CFuncPasstimeGoal *pGoal )
{
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::OnStayInGoal( CTFPlayer *pPlayer, CFuncPasstimeGoal *pGoal ) 
{
	OnEnterGoal( pPlayer, pGoal );
}

//-----------------------------------------------------------------------------
bool CTFPasstimeLogic::OnBallCollision( CPasstimeBall *pBall, int index, gamevcollisionevent_t *pEvent )
{
	if ( !IsGamestatePlayable() )
	{
		return false;
	}

	// FIXME
	//if ( pBall && pBall->BAnyControllerApplied() )
	//{
	//	return false;
	//}

	return true;
}

//-----------------------------------------------------------------------------
bool CTFPasstimeLogic::BCanPlayerPickUpBall( CTFPlayer *pPlayer, HudNotification_t *pReason ) const
{
	if ( pReason ) *pReason = (HudNotification_t) 0;

	const auto *pBall = m_hBall.Get();
	if ( !pBall ) 
	{ 
		return false; 
	}

	if ( !pPlayer || !IsGamestatePlayable() )
	{
		return false;
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) 
		|| pPlayer->m_Shared.InCond( TF_COND_PHASE ) 
		|| pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
	{
		if ( pReason ) *pReason = HUD_NOTIFY_PASSTIME_NO_INVULN;
		return false;
	}

	if ( pPlayer->m_Shared.IsStealthed() )
	{
		if ( pReason ) *pReason = HUD_NOTIFY_PASSTIME_NO_CLOAK;
		return false;
	}

	// let disguised spies pick up enemy ball, which amounts to interception and fake passes
	auto bEnemyBall = ( pBall->GetTeamNumber() != TEAM_UNASSIGNED )
		&& ( pBall->GetTeamNumber() != pPlayer->GetTeamNumber() );
	if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && !bEnemyBall )
	{
		if ( pReason ) *pReason = HUD_NOTIFY_PASSTIME_NO_DISGUISE;
		return false;
	}

	if ( pPlayer->m_Shared.IsCarryingObject() ) 
	{
		if ( pReason ) *pReason = HUD_NOTIFY_PASSTIME_NO_CARRY;
		return false;
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_SELECTED_TO_TELEPORT ) )
	{
		if ( pReason ) *pReason = HUD_NOTIFY_PASSTIME_NO_TELE;
		return false;
	}

	if ( pPlayer->IsTaunting() )
	{
		if ( pReason ) *pReason = HUD_NOTIFY_PASSTIME_NO_TAUNT;
		return false;
	}

	if ( !pPlayer->IsAllowedToPickUpFlag()
		|| !pPlayer->IsAlive() // NOTE: it's possible to be !alive and !dead at the same time
		|| pPlayer->IsAwayFromKeyboard()
		|| pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE )
		|| pPlayer->m_Shared.IsControlStunned() )
	{
		return false;
	}

	if ( pPlayer->m_bIsTeleportingUsingEurekaEffect )
	{
		if ( pReason ) *pReason = HUD_NOTIFY_PASSTIME_NO_TELE;
		return false;
	}

	CTFWeaponBase *pActiveWeapon = pPlayer->GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		bool bCanHolster = pActiveWeapon->CanHolster()
			&& !( pActiveWeapon->IsReloading() && pActiveWeapon->ReloadsSingly() && pActiveWeapon->CanOverload() ); // semihack to fix beggars bazooka problems
		if ( pActiveWeapon && ( pActiveWeapon->GetWeaponID() != TF_WEAPON_PASSTIME_GUN ) && !bCanHolster )
		{
			if ( pReason ) *pReason = HUD_NOTIFY_PASSTIME_NO_HOLSTER;
			return false;
		}
	}

	if ( EntityIsInNoBallZone( pPlayer ) )
	{
		if ( pReason ) *pReason = HUD_NOTIFY_PASSTIME_NO_OOB;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
int CTFPasstimeLogic::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::RespawnBall()
{
	Assert( m_hBall );
	if ( !m_hBall ) // paranoia
	{
		return;
	}

	ClearBallPower();

	// TFGameRules only checks capture limit once per second, so this code can't rely on game state changing
	int iScoreLimit = tf_passtime_scores_per_round.GetInt();
	bool bGameOver = ( TFTeamMgr()->GetFlagCaptures( TF_TEAM_RED ) >= iScoreLimit )
		|| ( TFTeamMgr()->GetFlagCaptures( TF_TEAM_BLUE ) >= iScoreLimit );

	gamerules_roundstate_t state = TFGameRules()->State_Get();
	if ( bGameOver || (state == GR_STATE_GAME_OVER) || (state == GR_STATE_TEAM_WIN) || (state == GR_STATE_RESTART) )
	{
		m_hBall->SetStateOutOfPlay();
		MoveBallToSpawner();
	}
	else if ( (state == GR_STATE_RND_RUNNING) || (state == GR_STATE_STALEMATE) )
	{
		// TODO just end the game if there's not enough time to respawn the ball
		m_hBall->SetStateOutOfPlay();
		MoveBallToSpawner();
		CTeamRoundTimer *pTimer = TFGameRules()->GetActiveRoundTimer();
		if ( !pTimer || ( pTimer->GetTimeRemaining() > m_iBallSpawnCountdownSec ) )
		{
			m_pRespawnCountdown->Start( m_iBallSpawnCountdownSec );
			SpawnBallAtRandomSpawnerThink();
		}
	}
	else // pre-round etc
	{
		SpawnBallAtRandomSpawner(); 
	}

	m_ballLastHeldTimes.RemoveAll();
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::SpawnBallAtRandomSpawnerThink()
{
	if ( TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
	{
		m_hBall->SetStateOutOfPlay();
		m_pRespawnCountdown->Disable();
	}
	else if ( m_pRespawnCountdown->Tick( 1 ) )
	{
		SpawnBallAtRandomSpawner();
	}
	else
	{
		SetContextThink( &CTFPasstimeLogic::SpawnBallAtRandomSpawnerThink, gpGlobals->curtime + 1, "spawnball" );
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::SpawnBallAtRandomSpawner()
{
	const auto &allSpawns = IPasstimeBallSpawnAutoList::AutoList();
	int i = RandomInt( 0, allSpawns.Count() - 1 );
	CPasstimeBallSpawn *pSpawner = static_cast< CPasstimeBallSpawn *>( allSpawns[i] );	
	SpawnBallAtSpawner( pSpawner );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::MoveBallToSpawner()
{
	const auto &allSpawns = IPasstimeBallSpawnAutoList::AutoList();
	int i = RandomInt( 0, allSpawns.Count() - 1 );
	CPasstimeBallSpawn *pSpawner = static_cast< CPasstimeBallSpawn *>( allSpawns[i] );	
	m_hBall->MoveTo( pSpawner->GetAbsOrigin(), Vector( 0, 0, 0 ) );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::SpawnBallAtSpawner( CPasstimeBallSpawn *pSpawner )
{
	if ( !m_hBall )
	{
		// NOTE: this is the first place where the ball is created - on first spawn
		m_hBall = CPasstimeBall::Create( pSpawner->GetAbsOrigin(), QAngle(0,0,0) );
	}

	StopAskForBallEffects();
	m_hBall->SetStateFree();
	m_hBall->MoveToSpawner( pSpawner->GetAbsOrigin() );
	m_hBall->ChangeTeam( pSpawner->GetTeamNumber() );
	m_onBallFree.FireOutput( m_hBall, this );
	pSpawner->m_onSpawnBall.FireOutput( pSpawner, pSpawner );

	TFGameRules()->BroadcastSound( 255, "Passtime.BallSpawn" );
	if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
	{
		TFGameRules()->BroadcastSound( 255, "Passtime.Merasmus.Laugh" );
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::StopAskForBallEffects()
{
	for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->m_Shared.SetAskForBallTime( 0 );
		}
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::OnBallCarrierMeleeHit( CTFPlayer *pPlayer, CTFPlayer *pAttacker ) 
{
	// TODO refactor OnBallCarrierMeleeHit and OnBallCarrierDamaged for less copypasta

	if ( !pPlayer || !pAttacker || (pPlayer == pAttacker) || !TFGameRules() )
	{
		// shouldn't happen, but who knows
		return;
	}

	Assert( pPlayer->m_Shared.HasPasstimeBall() );
	if ( !pPlayer->m_Shared.HasPasstimeBall() ) 
	{ 
		return; 
	}

	if ( !pPlayer->InSameTeam( pAttacker) )
	{
		// currently handled by OnBallCarrierDamaged
		return;
	}

	Assert( m_hBall );
	if( !m_hBall )
	{
		return;
	}

	bool bTooLong = (m_hBall->GetCarryDuration() > tf_passtime_teammate_steal_time.GetFloat());
	if ( pPlayer->m_bPasstimeBallSlippery || bTooLong )
	{
		// once a player has held the ball too long, mark them as a jerk
		// so they can't hoard the ball ever again
		StealBall( pPlayer, pAttacker );
		pPlayer->m_bPasstimeBallSlippery = true;
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::OnBallCarrierDamaged( CTFPlayer *pPlayer, CTFPlayer *pAttacker, 
	const CTakeDamageInfo& info ) 
{
	// TODO refactor OnBallCarrierMeleeHit and OnBallCarrierDamaged for less copypasta

	// NOTE: it's possible that neither player has the ball if the attacker
	// killed the carrier, which would cause EjectBall to happen before
	// this call. There's no good way around it.

	if ( !pPlayer || !pAttacker || (pPlayer == pAttacker) || !TFGameRules() )
	{
		// happens from world damage
		return;
	}

	//
	// Only care about melee damage
	//
	// DMG_CLUB is demo charge
	if ( !tf_passtime_steal_on_melee.GetBool() || !(info.GetDamageType() & (DMG_MELEE | DMG_CLUB)) ) 
	{
		return;
	}

	Assert( m_hBall );
	if ( !m_hBall )
	{
		return;
	}

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BASEBALL )
	{
		auto launch = CPasstimeGun::CalcLaunch( pPlayer, false );
		LaunchBall(pPlayer, launch.startPos, launch.startVel );
	}
	else
	{
		StealBall( pPlayer, pAttacker );
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::CrowdReactionSound( int iTeam ) 
{
	if ( m_flNextCrowdReactionTime <= gpGlobals->curtime ) 
	{
		TFGameRules()->BroadcastSound( iTeam, "Passtime.Crowd.React.Pos" );
		TFGameRules()->BroadcastSound( GetEnemyTeam( iTeam ), "Passtime.Crowd.React.Neg" );
		m_flNextCrowdReactionTime = gpGlobals->curtime + 10.0f;
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::StealBall( CTFPlayer *pFrom, CTFPlayer *pTo ) 
{
	if ( pFrom->m_Shared.HasPasstimeBall() )
	{
		EjectBall( pFrom, pTo );
	}

	HudNotification_t cantPickUpReason;
	if ( BCanPlayerPickUpBall( pTo, &cantPickUpReason ) )
	{
		if ( !pFrom->m_bPasstimeBallSlippery )
		{
			CTF_GameStats.Event_PlayerAwardBonusPoints( pTo, pTo, 10 );
		}

		TFGameRules()->BroadcastSound( 255, "Passtime.BallStolen" );

		++CTF_GameStats.m_passtimeStats.summary.nTotalSteals;

		m_hBall->SetStateCarried( pTo );
		OnBallGet();
		pTo->m_Shared.AddCond( TF_COND_PASSTIME_INTERCEPTION, tf_passtime_speedboost_on_get_ball_time.GetFloat() );

		int pointsToAward = 5;
		if ( CFuncPasstimeGoalieZone::BPlayerInAny( pTo ) )
		{
			++CTF_GameStats.m_passtimeStats.summary.nTotalStealsNearGoal;
			pointsToAward = 10; // Extra points for last second defend.
		}

		if ( !pFrom->m_bPasstimeBallSlippery )
		{
			CTF_GameStats.Event_PlayerAwardBonusPoints( pTo, 0, pointsToAward );
		}

		PasstimeGameEvents::BallStolen( pFrom->entindex(), pTo->entindex() ).Fire();
		CrowdReactionSound( pTo->GetTeamNumber() );
	}
	else if ( cantPickUpReason )
	{
		CSingleUserReliableRecipientFilter filter( pTo );
		TFGameRules()->SendHudNotification( filter, cantPickUpReason );
	}
}

//-----------------------------------------------------------------------------
float CTFPasstimeLogic::GetLastHeldTime( CTFPlayer* pPlayer )
{
	float lastHeldTime = 0.0f;
	for ( int i = 0; i < m_ballLastHeldTimes.Count(); i++ )
	{
		if ( m_ballLastHeldTimes[i].first == pPlayer )
		{
			lastHeldTime = m_ballLastHeldTimes[i].second;
			break;
		}
	}

	return lastHeldTime;
}

//-----------------------------------------------------------------------------
float CTFPasstimeLogic::GetLastPassTime( CTFPlayer* pPlayer )
{
	float lastPassTime = 0.0f;
	for ( int i = 0; i < m_ballLastPassTimes.Count(); i++ )
	{
		if ( m_ballLastPassTimes[i].first == pPlayer )
		{
			lastPassTime = m_ballLastPassTimes[i].second;
			break;
		}
	}

	return lastPassTime;
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::SetLastPassTime( CTFPlayer* pPlayer )
{
	std::pair<CTFPlayer*, float> toAdd( pPlayer, gpGlobals->realtime );
	bool skipTheRest = false;
	for ( int i = 0; i < m_ballLastPassTimes.Count(); i++ )
	{
		if ( m_ballLastPassTimes[i].first == pPlayer )
		{
			m_ballLastPassTimes[i].second = toAdd.second;// replace old time rather than add a new pair to the vector
			skipTheRest = true;
			break;
		}
	}

	if ( !skipTheRest )
	{
		m_ballLastPassTimes.AddToTail( toAdd );
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::EjectBall( CTFPlayer *pPlayer, CTFPlayer *pAttacker )
{
	if ( !m_hBall )
	{
		// I'm not sure how this is possible, but if I'm recording with hltv in
		// a listen server that has bots in it (which requires a hack in the bot
		// concommand) and I restart the game while a bot is holding the ball...
		// then m_hBall is invalid.
		if ( pPlayer )
		{
			// This has to be true to get into this function for the case I just
			// described above, and since the ball has been deleted somehow during
			// the round restart, it probably isn't necessary to set this to 0
			// because the player's going to be reset anyway. But I want to make
			// sure it's correct.
			pPlayer->m_Shared.SetHasPasstimeBall( 0 );
		}
		return;
	}

	m_hBall->SetStateFree();
	m_hBall->ChangeTeam( TEAM_UNASSIGNED );
	
	Vector vecEjectVel( 0, 0, 600 );
	vecEjectVel += pPlayer->GetAbsVelocity() * 0.1f;
	m_hBall->MoveTo( pPlayer->GetAbsOrigin() + Vector( 0, 0, 32 ), vecEjectVel );
	if ( pPlayer != pAttacker ) 
	{
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_LOST_OBJECT );
		pAttacker->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_TAUNTS );
	}

	m_onBallFree.FireOutput( m_hBall, this );
	std::pair<CTFPlayer*, float> toAdd( pPlayer, gpGlobals->realtime );
	for ( int i = 0; i < m_ballLastHeldTimes.Count(); i++ )
	{
		if ( m_ballLastHeldTimes[i].first == pPlayer )
		{
			m_ballLastHeldTimes[i].second = toAdd.second;// replace old time rather than add a new pair to the vector
			return;
		}
	}

	m_ballLastHeldTimes.AddToTail( toAdd );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::LaunchBall( CTFPlayer *pPlayer, const Vector &vecPos, const Vector &vecVel )
{
	StopAskForBallEffects();
	m_hBall->SetStateFree();
	m_hBall->MoveTo( vecPos, vecVel );
	m_onBallFree.FireOutput( m_hBall, this );
	std::pair<CTFPlayer*, float> toAdd( pPlayer, gpGlobals->realtime );
	for ( int i = 0; i < m_ballLastHeldTimes.Count(); i++ )
	{
		if ( m_ballLastHeldTimes[i].first == pPlayer )
		{
			m_ballLastHeldTimes[i].second = toAdd.second;// replace old time rather than add a new pair to the vector
			return;
		}
	}

	m_ballLastHeldTimes.AddToTail( toAdd );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::Score( CTFPlayer *pPlayer, CFuncPasstimeGoal *pGoal )
{
	Assert( pPlayer && pGoal );
	pGoal->OnScore( pPlayer->GetTeamNumber() );
	Score( pPlayer, pGoal->GetTeamNumber(), pGoal->Points(), pGoal->BWinOnScore() );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::Score( CPasstimeBall *pBall, CFuncPasstimeGoal *pGoal )
{
	Assert( pBall && pGoal );
	CTFPlayer* pPlayer = pBall->GetThrower();
	Assert( pPlayer );
	pGoal->OnScore( pPlayer->GetTeamNumber() );
	Score( pPlayer, pGoal->GetTeamNumber(), pGoal->Points(), pGoal->BWinOnScore() );
}

//-----------------------------------------------------------------------------
// static
void CTFPasstimeLogic::AddCondToTeam( ETFCond eCond, int iTeam, float flTime )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTFPlayer && (pTFPlayer->GetTeamNumber() == iTeam) && pTFPlayer->IsAlive() )
		{
			pTFPlayer->m_Shared.AddCond( eCond, flTime );
		}
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::Score( CTFPlayer *pPlayer, int iTeam, int iPoints, bool bForceWin ) 
{
	StopAskForBallEffects();
	m_pRespawnCountdown->Disable();

	Assert( pPlayer );
	if ( !pPlayer || ( iTeam == TEAM_UNASSIGNED ) )
	{
		return;
	}

	if ( bForceWin )
	{
		iPoints = MAX( 1, tf_passtime_scores_per_round.GetInt() - TFTeamMgr()->GetFlagCaptures( iTeam ) );
	}
	
	//
	// Update stats
	//
	++CTF_GameStats.m_passtimeStats.summary.nTotalScores;
	++CTF_GameStats.m_passtimeStats.classes[ pPlayer->GetPlayerClass()->GetClassIndex() ].nTotalScores;
	CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );

	// 
	// Award player points
	//
	CTF_GameStats.Event_PlayerAwardBonusPoints( pPlayer, 0, 25 );

	//
	// Award player assist points
	//
	{
		CTFPlayer *pAssister = nullptr;
		float flAssisterTime = FLT_MAX;
		for ( unsigned short i = 0; i < m_ballLastHeldTimes.Count(); i++ )
		{
			auto &tempPair = m_ballLastHeldTimes[i];
			auto *pPossibleAssister = tempPair.first;
			auto timeLastHeld = tempPair.second;
			auto flSecAgo = gpGlobals->realtime - timeLastHeld;
			if ( ( pPossibleAssister->GetTeamNumber() == pPlayer->GetTeamNumber() ) 
				&& ( pPossibleAssister != pPlayer ) 
				&& ( flSecAgo < 10.0f ) 
				&& ( flSecAgo < flAssisterTime ) )
			{
				pAssister = pPossibleAssister;
				flAssisterTime = flSecAgo;
			}
		}

		if ( pAssister )
		{
			CTF_GameStats.Event_PlayerAwardBonusPoints( pAssister, 0, 10 );
			PasstimeGameEvents::Score( pPlayer->entindex(), pAssister->entindex(), iPoints ).Fire();
		}
		else
		{
			PasstimeGameEvents::Score( pPlayer->entindex(), iPoints ).Fire();
		}
	}

	//
	// Award team points
	//
	while ( iPoints-- > 0 )
	{
		TFTeamMgr()->IncrementFlagCaptures( iTeam );
	}

	//
	// Award bonus conditions
	//
	AddCondToTeam( TF_COND_CRITBOOSTED_CTF_CAPTURE, pPlayer->GetTeamNumber(), tf_passtime_score_crit_sec.GetFloat() );

	//
	// Feedback
	//
	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_FLAGCAPTURED );
	TFGameRules()->BroadcastSound( iTeam, "Passtime.Crowd.Cheer" );
	TFGameRules()->BroadcastSound( GetEnemyTeam( iTeam ), "Passtime.Crowd.Boo" );

	if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) ) 
	{
		const char* pszSound = ( iTeam == TF_TEAM_RED )
			? "sf14.Merasmus.Soccer.GoalRed"
			: "sf14.Merasmus.Soccer.GoalBlue";
		TFGameRules()->BroadcastSound( 255, pszSound );
	}

	//
	// Game state management
	//
	ClearBallPower();
	m_hBall->SetStateOutOfPlay();
	MoveBallToSpawner(); // move it now instead of when it spawns to avoid lerping

	//
	// Finish round or respawn ball
	//
	CTeamRoundTimer *pRoundTimer = TFGameRules()->GetActiveRoundTimer();
	if ( ( TFGameRules()->State_Get() == GR_STATE_STALEMATE ) || ( pRoundTimer && ( pRoundTimer->GetTimeRemaining() <= 0.0f ) ) ) 
	{
		EndRoundExpiredTimer();
	}
	else
	{
		SetContextThink( &CTFPasstimeLogic::RespawnBall, gpGlobals->curtime, "spawnball" );
	}

	//
	// Fire outputs
	//
	m_onScoreAny.FireOutput( pPlayer, this );
	if( iTeam == TF_TEAM_RED )
	{
		m_onScoreRed.FireOutput( pPlayer, this );
	}
	else if( iTeam == TF_TEAM_BLUE )
	{
		m_onScoreBlu.FireOutput( pPlayer, this );
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::OnPlayerTouchBall( CTFPlayer *pCatcher, CPasstimeBall *pBall )
{
	if ( pBall != m_hBall )
	{
		return;
	}

	const int iCatcherTeam = pCatcher->GetTeamNumber();
	float flFeet = pBall->GetAirtimeDistance() / 16.0f;
	auto iExperiment = (EPasstimeExperiment_Telepass) tf_passtime_experiment_telepass.GetInt();

	//
	// Check for pass and interception
	//
	CTFPlayer *pThrower = pBall->GetThrower();
	if ( pThrower // ball must must have been thrown...
		&& (pBall->GetCollisionCount() == 0) // and not bounced...
		&& (pBall->GetTeamNumber() != TEAM_UNASSIGNED) // and not be neutral...
		&& (pCatcher != pBall->GetPrevCarrier())) // and not passed to yourself...
	{
		PasstimeGameEvents::PassCaught( pThrower->entindex(), pCatcher->entindex(), flFeet, pBall->GetAirtimeSec() ).Fire();

		bool bAllowCheerSound = true;

		int iDistanceBonus = ( int ) ( pBall->GetAirtimeSec() * tf_passtime_powerball_airtimebonus.GetFloat() );
		iDistanceBonus = clamp( iDistanceBonus, 0, tf_passtime_powerball_maxairtimebonus.GetInt() );
		int iPassPoints = tf_passtime_powerball_passpoints.GetInt();
		AddBallPower( iPassPoints + iDistanceBonus );
		bAllowCheerSound = m_iBallPower < tf_passtime_powerball_threshold.GetInt();

		CPASFilter pasFilter( pCatcher->GetAbsOrigin() );
		pCatcher->EmitSound( pasFilter, pCatcher->entindex(), "Passtime.BallCatch" );

		// make sure this happens before BeginCarry/SEtOwner etc
		if ( pThrower->GetTeamNumber() == iCatcherTeam )
		{
			if ( pBall->GetHomingTarget() )
			{
				// pass was caught by teammate
				++CTF_GameStats.m_passtimeStats.summary.nTotalPassesCompleted;
				CTF_GameStats.m_passtimeStats.AddPassTravelDistSample( pBall->GetAirtimeDistance() );

				// award bonus effects for pass
				pCatcher->m_Shared.AddCond( TF_COND_SPEED_BOOST, tf_passtime_speedboost_on_get_ball_time.GetFloat() );

				if ( CFuncPasstimeGoalieZone::BPlayerInAny( pCatcher ) )
				{
					++CTF_GameStats.m_passtimeStats.summary.nTotalPassesCompletedNearGoal;
				}

				if ( iExperiment != EPasstimeExperiment_Telepass::None )
				{
					// origins need to be a copy
					auto throwerOrigin = pThrower->GetAbsOrigin();
					auto catcherOrigin = pCatcher->GetAbsOrigin();

					CPVSFilter filterThrower( throwerOrigin );
					switch( pThrower->GetTeamNumber() )
					{
					case TF_TEAM_RED:
						TE_TFParticleEffect( filterThrower, 0.0, "teleported_red", throwerOrigin, vec3_angle );
						TE_TFParticleEffect( filterThrower, 0.0, "player_sparkles_red", throwerOrigin, vec3_angle, pThrower, PATTACH_ABSORIGIN );
						break;
					case TF_TEAM_BLUE:
						TE_TFParticleEffect( filterThrower, 0.0, "teleported_blue", throwerOrigin, vec3_angle );
						TE_TFParticleEffect( filterThrower, 0.0, "player_sparkles_blue", throwerOrigin, vec3_angle, pThrower, PATTACH_ABSORIGIN );
						break;
					default:
						break;
					}

					pThrower->EmitSound( "Building_Teleporter.Send" );
					pCatcher->EmitSound( "Building_Teleporter.Receive" );

					// then move the player
					pThrower->Teleport( &catcherOrigin, nullptr, nullptr );
					if ( iExperiment == EPasstimeExperiment_Telepass::SwapWithCatcher )
					{
						pCatcher->Teleport( &throwerOrigin, nullptr, nullptr );

						CPVSFilter filterCatcher( catcherOrigin );
						switch( pCatcher->GetTeamNumber() )
						{
						case TF_TEAM_RED:
							TE_TFParticleEffect( filterCatcher, 0.0, "teleported_red", catcherOrigin, vec3_angle );
							TE_TFParticleEffect( filterCatcher, 0.0, "player_sparkles_red", catcherOrigin, vec3_angle, pCatcher, PATTACH_ABSORIGIN );
							break;
						case TF_TEAM_BLUE:
							TE_TFParticleEffect( filterCatcher, 0.0, "teleported_blue", catcherOrigin, vec3_angle );
							TE_TFParticleEffect( filterCatcher, 0.0, "player_sparkles_blue", catcherOrigin, vec3_angle, pCatcher, PATTACH_ABSORIGIN );
							break;
						default:
							break;
						}
					}

					// then start the effects
					pThrower->TeleportEffect();
				}
			}
			else
			{
				// toss was caught by teammate
				++CTF_GameStats.m_passtimeStats.summary.nTotalTossesCompleted;
			}

			float lastPassTime = 0.0f;
			for ( int i = 0; i < m_ballLastPassTimes.Count(); i++ )
			{
				if ( m_ballLastPassTimes[i].first == pThrower)
				{
					lastPassTime = m_ballLastPassTimes[i].second;
					break;
				}
			}

			// successful pass
			if ( flFeet > 30 )
			{
				// fanfare and points if the pass was long enough (and we haven't been spamming throw/catch for points)
				if ( gpGlobals->realtime - lastPassTime > 6.0f ) // FIXME literal balance value
				{
					CTF_GameStats.Event_PlayerAwardBonusPoints( pThrower, pThrower, 15 ); // FIXME literal balance value
				}

				if ( bAllowCheerSound && ( pBall->GetAirtimeSec() > 2.0f ) ) // FIXME literal balance value
				{
					TFGameRules()->BroadcastSound( 255, "TFPlayer.StunImpactRange" );
				}
			}
			else// flFeet <= 30
			{
				// (points conditional on we haven't had the ball in the last 6 seconds)
				if ( gpGlobals->realtime - lastPassTime > 6.0f ) // FIXME literal balance value
				{
					CTF_GameStats.Event_PlayerAwardBonusPoints( pThrower, pThrower, 5 ); // FIXME literal balance value
				}
			}

			std::pair<CTFPlayer*, float> toAdd( pThrower, gpGlobals->realtime );
			bool skipTheRest = false;
			for ( int i = 0; i < m_ballLastPassTimes.Count(); i++ )
			{
				if ( m_ballLastPassTimes[i].first == pThrower)
				{
					m_ballLastPassTimes[i].second = toAdd.second;// replace old time rather than add a new pair to the vector
					skipTheRest = true;
					break;
				}
			}

			if ( !skipTheRest )
			{
				m_ballLastPassTimes.AddToTail( toAdd );
			}
		}
		else
		{
			if ( pBall->GetHomingTarget() )
			{
				// pass was intercepted
				++CTF_GameStats.m_passtimeStats.summary.nTotalPassesIntercepted;
				CTF_GameStats.m_passtimeStats.AddPassTravelDistSample( pBall->GetAirtimeDistance() );
			}
			else
			{
				// toss was intercepted
				++CTF_GameStats.m_passtimeStats.summary.nTotalTossesIntercepted;
			}
			
			// interception can happen at any range, extra points if intercepted within the goal area
			int bonusPointsToAward = 15; // FIXME literal balance value
			if ( CFuncPasstimeGoalieZone::BPlayerInAny( pCatcher ) )
			{
				bonusPointsToAward = 25; // FIXME literal balance value
				if ( pBall->GetHomingTarget() )
				{
					++CTF_GameStats.m_passtimeStats.summary.nTotalPassesInterceptedNearGoal;
				}
				else
				{
					++CTF_GameStats.m_passtimeStats.summary.nTotalTossesInterceptedNearGoal;
				}
			}

			// award bonus effects for interception
			pCatcher->m_Shared.AddCond( TF_COND_PASSTIME_INTERCEPTION, tf_passtime_speedboost_on_get_ball_time.GetFloat() );
			pCatcher->m_Shared.AddCond( TF_COND_SPEED_BOOST, tf_passtime_speedboost_on_get_ball_time.GetFloat() );

			CTF_GameStats.Event_PlayerAwardBonusPoints( pCatcher, pCatcher, bonusPointsToAward );
			TFGameRules()->BroadcastSound( 255, "Passtime.BallIntercepted" );
			CrowdReactionSound( pCatcher->GetTeamNumber() );
		}
	}
	else 
	{
		++CTF_GameStats.m_passtimeStats.summary.nTotalRecoveries;
		CTFPlayer *pPrevCarrier = pBall->GetPrevCarrier();
		if ( pCatcher != pPrevCarrier )
		{
			// Gain a point for picking up a neutral ball.
			CTF_GameStats.Event_PlayerAwardBonusPoints( pCatcher, pThrower, 5 ); // FIXME literal balance value
		}

		PasstimeGameEvents::BallGet( pCatcher->entindex(), ( pCatcher != pPrevCarrier ) ? TEAM_UNASSIGNED: pCatcher->GetTeamNumber() ).Fire();
	}

	if ( ((iExperiment == EPasstimeExperiment_Telepass::TeleportToCatcherMaintainPossession)
			|| (iExperiment == EPasstimeExperiment_Telepass::SwapWithCatcher))
		&& BCanPlayerPickUpBall( pThrower, nullptr ) )
	{
		EjectBall( pCatcher, pThrower );
		m_hBall->SetStateCarried( pThrower );
		OnBallGet();					
	}
	else 
	{
		pBall->SetStateCarried( pCatcher );
		OnBallGet();
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::OnBallGet() 
{
	StopAskForBallEffects();
	if ( CTFPlayer *pPlayer = m_hBall->GetCarrier() )
	{
		m_onBallGetAny.FireOutput( pPlayer, this );
		if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
		{
			m_onBallGetRed.FireOutput( pPlayer, this );
		}
		else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
		{
			m_onBallGetBlu.FireOutput( pPlayer, this );
		}
		CPasstimeBallController::BallPickedUp( m_hBall, pPlayer );
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::InputSpawnBall( inputdata_t &input )
{
	RespawnBall();
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::InputTimeUp( inputdata_t &input )
{
	int iRedScore = TFTeamMgr()->GetFlagCaptures( TF_TEAM_RED );
	int iBlueScore = TFTeamMgr()->GetFlagCaptures( TF_TEAM_BLUE );
	int iPointDifference = abs( iRedScore - iBlueScore );
	
	// going through the list of goals to calculate the max possible point gain
	// is possible but tricky since goals can be enabled/disabled and there's no
	// way to know which goals are actually possible to score in, so this is 
	// simply hard-coded to work correctly for the official maps where there's
	// a 3-point unlockable goal.
	int iMaxPossibleScoreGain = 3;

	if ( ( iPointDifference <= iMaxPossibleScoreGain ) && !ShouldEndOvertime() )
	{
		m_pRespawnCountdown->Disable();
		TFGameRules()->BroadcastSound( 255, "Game.Overtime" );
		ThinkExpiredTimer();
	}
	else
	{
		EndRoundExpiredTimer();
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::ThinkExpiredTimer()
{
 	if ( TFGameRules() && (TFGameRules()->State_Get() != GR_STATE_RND_RUNNING) ) 
	{
		if ( m_pRespawnCountdown )
		{
			// just in case
			m_pRespawnCountdown->Disable();
		}
		return;
	}

	if ( ShouldEndOvertime() || m_pRespawnCountdown->Tick( gpGlobals->frametime ) )
	{
		EndRoundExpiredTimer();
		return;
	}

	// Check again every frame until either something else ends the round
	// or the conditions are met that allow an expired timer to end the round.
	SetContextThink( &CTFPasstimeLogic::ThinkExpiredTimer, gpGlobals->curtime, "ThinkExpiredTimer" );

	Assert( m_hBall ); // verified in ShouldEndOvertime
	Assert( m_pRespawnCountdown ); // always valid after Spawn
	bool bBallUnassigned = m_hBall->GetTeamNumber() == TEAM_UNASSIGNED;
	bool bCountdownRunning = !m_pRespawnCountdown->IsDisabled();
	if ( bBallUnassigned && !bCountdownRunning )
	{
		// start the countdown when the ball turns neutral
		m_pRespawnCountdown->Start( tf_passtime_overtime_idle_sec.GetFloat() );
	}
	else if ( !bBallUnassigned && bCountdownRunning ) 
	{
		// stop the countdown when the ball is picked up
		m_pRespawnCountdown->Disable();
	}
}


//-----------------------------------------------------------------------------
bool CTFPasstimeLogic::ShouldEndOvertime() const
{
	if ( !m_hBall || !TFGameRules() ) 
	{
		return true;
	}

	// if nobody has the ball, only the respawn countdown can end overtime
	CTFPlayer *pBallCarrier = m_hBall->GetCarrier();
	if ( m_hBall->GetTeamNumber() == TEAM_UNASSIGNED || !pBallCarrier )
	{
		return false;
	}

	// if the teams are tied, someone has to score
	int iRedScore = TFTeamMgr()->GetFlagCaptures( TF_TEAM_RED );
	int iBluScore = TFTeamMgr()->GetFlagCaptures( TF_TEAM_BLUE );
	if ( iRedScore == iBluScore )
	{
		return false;
	}

	// if the winning team has posession, they win
	int iWinningTeam = ( iRedScore > iBluScore )
		? TF_TEAM_RED
		: TF_TEAM_BLUE;
	return pBallCarrier->GetTeamNumber() == iWinningTeam;
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::EndRoundExpiredTimer()
{
	StopAskForBallEffects();
	m_pRespawnCountdown->Disable();
	SetContextThink( &CTFPasstimeLogic::ThinkExpiredTimer, TICK_NEVER_THINK, "ThinkExpiredTimer" );
	
	// copied from TeamplayRoundBasedGameRules::State_Think_RND_RUNNING
	int iDrawScoreCheck = -1;
	int iWinningTeam = 0;
	bool bTeamsAreDrawn = true;
	for ( int i = FIRST_GAME_TEAM; (i < GetNumberOfTeams()) && bTeamsAreDrawn; i++ )
	{
		int iTeamScore = TFTeamMgr()->GetFlagCaptures( i );

		if ( iTeamScore > iDrawScoreCheck )
		{
			iWinningTeam = i;
		}

		if ( iTeamScore != iDrawScoreCheck )
		{
			if ( iDrawScoreCheck == -1 )
			{
				iDrawScoreCheck = iTeamScore;
			}
			else
			{
				bTeamsAreDrawn = false;
			}
		}
	}

	if ( bTeamsAreDrawn )
	{
		TFGameRules()->SetStalemate( STALEMATE_SERVER_TIMELIMIT, true );
	}
	else
	{
		TFGameRules()->SetWinningTeam( iWinningTeam, WINREASON_TIMELIMIT, true, false, false );
	}
}

//-----------------------------------------------------------------------------
struct SetSectionParams
{
	int num;
	CPathTrack *pSectionStart;
	CPathTrack *pSectionEnd;
	SetSectionParams() : num(-1), pSectionStart(0), pSectionEnd(0) {}
};

//-----------------------------------------------------------------------------
bool CTFPasstimeLogic::ParseSetSection( const char *pStr, SetSectionParams &s ) const
{
	char pszStartName[64];
	char pszEndName[64];
	const int iScanCount = sscanf( pStr, "%i %63s %63s", &s.num, pszStartName, pszEndName ); // WHAT YEAR IS IT
	if ( iScanCount != 3 )
	{
		return false;
	}
	pszStartName[ ARRAYSIZE( pszStartName ) - 1 ] = '\0';
	pszEndName[ ARRAYSIZE( pszEndName ) - 1 ] = '\0';

	s.pSectionStart = dynamic_cast<CPathTrack*>( gEntList.FindEntityByName( 0, pszStartName ) );
	s.pSectionEnd = dynamic_cast<CPathTrack*>( gEntList.FindEntityByName( 0, pszEndName ) );

	if ( s.num < 0 ) 
		Warning( "SetSection number (%i) must be > 0\n", s.num );
	if ( s.num >= m_iNumSections ) 
		Warning( "SetSection number (%i) must be < section count (%i)\n", s.num, m_iNumSections.Get() );
	if ( !s.pSectionStart ) 
		Warning( "Failed to find section start path_track named %s\n", pszStartName );
	if ( !s.pSectionEnd) 
		Warning( "Failed to find section end path_track named %s\n", pszEndName );

	return (s.num >= 0)
		&& (s.num < m_iNumSections)
		&& s.pSectionStart
		&& s.pSectionEnd;
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::InputSetSection( inputdata_t &input )
{
	SetSectionParams params;
	if ( !ParseSetSection( input.value.String(), params ) )
	{
		Warning( "Error in SetSection input: %s\n", input.value.String() );
		return;
	}

	for ( int i = 0; i < m_trackPoints.Count(); ++i )
	{
		m_trackPoints.GetForModify(i).Zero();
	}

	int iTrackPoint = 0;
	for ( CPathTrack *pTrack = params.pSectionStart; pTrack; pTrack = pTrack->GetNext(), ++iTrackPoint )
	{
		if ( iTrackPoint == m_trackPoints.Count() )
		{
			Warning( "Too many track_path in section (%i max, easily changed but must be fixed).", m_trackPoints.Count() );
			return;
		}

		m_trackPoints.Set( iTrackPoint, pTrack->GetAbsOrigin() );
		if ( pTrack->GetAbsOrigin() == Vector( 0, 0, 0 ) )
		{
			// Because I'm using 0,0,0 to represent "no point" in a fixed 16-element array 
			Warning( "Can't have track_path at 0,0,0" );
		}

		if ( pTrack == params.pSectionEnd )
		{
			break;
		}
	}

	m_iCurrentSection = params.num;

}


//
// Secret Room
//

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::SecretRoom_Spawn()
{
	SECRETROOM_LOG( "@@@@  SECRET ROOM: Spawn\n" );
	m_SecretRoom_pTv = gEntList.FindEntityByName( nullptr, "tv" );

	string_t self = GetEntityName();

	// plug_breakable.OnDamaged -> this.InputPlugDamaged
	HookOutput( "plug_breakable", self, "OnDamaged", "staticc", nullptr, 1 );

	// player triggers
	// the names are generated gibberish words
	// (Blu)	(1)Scout: "comillow"
	HookOutput( "comillow", self, "OnStartTouch", "statica" );
	HookOutput( "comillow", self, "OnEndTouch", "staticb" );

	// (Red)	(2)Soldier: "unissubs"
	HookOutput( "unissubs", self, "OnStartTouch", "statica" );
	HookOutput( "unissubs", self, "OnEndTouch", "staticb" );

	// (Red)	(3)Pyro: "amment"
	HookOutput( "amment", self, "OnStartTouch", "statica" );
	HookOutput( "amment", self, "OnEndTouch", "staticb" );

	// (Blu)	(4)Demo: "memagold"
	HookOutput( "memagold", self, "OnStartTouch", "statica" );
	HookOutput( "memagold", self, "OnEndTouch", "staticb" );

	// (Red)	(5)Heavy: "subcla"
	HookOutput( "subcla", self, "OnStartTouch", "statica" );
	HookOutput( "subcla", self, "OnEndTouch", "staticb" );

	// (Blu)	(6)Engineer: "enempose"
	HookOutput( "enempose", self, "OnStartTouch", "statica" );
	HookOutput( "enempose", self, "OnEndTouch", "staticb" );

	// (Red)	(7)Medic: "irlenous"
	HookOutput( "irlenous", self, "OnStartTouch", "statica" );
	HookOutput( "irlenous", self, "OnEndTouch", "staticb" );

	// (Red)	(8)Sniper: "donked"
	HookOutput( "donked", self, "OnStartTouch", "statica" );
	HookOutput( "donked", self, "OnEndTouch", "staticb" );

	// (Blu)	(9)Spy: "finear"
	HookOutput( "finear", self, "OnStartTouch", "statica" );
	HookOutput( "finear", self, "OnEndTouch", "staticb" );

	// the room trigger for keeping track of who gets the achievement
	HookOutput( "room_trigger", self, "OnStartTouch", "RoomTriggerOnTouch" );
	g_EventQueue.AddEvent( "room_trigger", "Enable", variant_t(), 0.0f, this, this );
}

//-----------------------------------------------------------------------------
int CTFPasstimeLogic::SecretRoom_CountSlottedPlayers() const
{
	int iNumSlotsFilled = 0;
	for ( CTFPlayer *pPlayer : m_SecretRoom_slottedPlayers )
	{
		if ( pPlayer ) ++iNumSlotsFilled;
	}
	return iNumSlotsFilled;
}

//-----------------------------------------------------------------------------
// this doesn't need a template, but something like this in variant_t.h would
// be nice. Or maybe just some explicit overloaded constructors.
template <typename T> variant_t make_variant( T value );
template <> variant_t make_variant( int value )
{
	variant_t v;
	v.SetInt( value );
	return v;
}

//-----------------------------------------------------------------------------
static void SecretRoom_PlayTvSound( CSoundPatch **ppPatch, int iEntIndex, const char *pSoundName, float flVolume )
{
	Assert( ppPatch );
	Assert( iEntIndex > 0 );
	Assert( pSoundName && *pSoundName );
	Assert( flVolume > 0 );

	CSoundEnvelopeController &snd = CSoundEnvelopeController::GetController();
	if ( *ppPatch )
	{
		SECRETROOM_LOG( "  @@  SECRET ROOM: Destroy sound patch\n" );
		snd.SoundDestroy( *ppPatch );
		*ppPatch = nullptr;
	}

	SECRETROOM_LOG( "  @@  SECRET ROOM: Create sound patch for %s volume %f\n", pSoundName, flVolume );
	CReliableBroadcastRecipientFilter filter;
	*ppPatch = snd.SoundCreate( filter, iEntIndex, pSoundName );
	snd.Play( *ppPatch, flVolume, PITCH_NORM );
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::SecretRoom_UpdateTv( int iNumSlotsFilled )
{
	if ( iNumSlotsFilled == 9 )
	{
		SECRETROOM_LOG( "  @@  SECRET ROOM: Update TV all slots filled\n" );
		g_EventQueue.AddEvent( "screen", "Skin", make_variant( 3 ), 0.0f, this, this );
		SecretRoom_PlayTvSound( &m_SecretRoom_pTvSound, 
			m_SecretRoom_pTv->entindex(), "Passtime.Tv3", 1.0f );
	}
	else
	{
		// sound
		float volume = (float)( iNumSlotsFilled + 1 ) / 10.0f;
		const char *pSoundName = ( iNumSlotsFilled >= 4 )
			? "Passtime.Tv2"
			: "Passtime.Tv1";

		SECRETROOM_LOG( "  @@  SECRET ROOM: Update TV %i slots filled\n", iNumSlotsFilled );

		SecretRoom_PlayTvSound( &m_SecretRoom_pTvSound, 
			m_SecretRoom_pTv->entindex(), pSoundName, volume );

		// skin
		int iSkin = ( iNumSlotsFilled >= 4 ) ? 2 : 1;
		g_EventQueue.AddEvent( "screen", "Skin", make_variant( iSkin ), 0.0f, this, this );
	}
}

//-----------------------------------------------------------------------------
struct SecretRoom_TriggerInfo
{
	int iIndex;
	const char *pTriggerName;
	int iClass;
	int iTeam;
} static const s_SecretRoom_TriggerInfo[9] =
{
	{ 0, "comillow",	TF_CLASS_SCOUT,			TF_TEAM_BLUE },
	{ 1, "unissubs",	TF_CLASS_SOLDIER,		TF_TEAM_RED },
	{ 2, "amment",		TF_CLASS_PYRO,			TF_TEAM_RED },
	{ 3, "memagold",	TF_CLASS_DEMOMAN,		TF_TEAM_BLUE },
	{ 4, "subcla",		TF_CLASS_HEAVYWEAPONS,	TF_TEAM_RED },
	{ 5, "enempose",	TF_CLASS_ENGINEER,		TF_TEAM_BLUE },
	{ 6, "irlenous",	TF_CLASS_MEDIC,			TF_TEAM_RED },
	{ 7, "donked",		TF_CLASS_SNIPER,		TF_TEAM_RED },
	{ 8, "finear",		TF_CLASS_SPY,			TF_TEAM_BLUE },
};

//-----------------------------------------------------------------------------
static const SecretRoom_TriggerInfo &SecretRoom_GetSlotInfoForTrigger( 
	const char *pTriggerName )
{
	for ( const auto &info : s_SecretRoom_TriggerInfo )
	{
		if ( !V_strcmp( info.pTriggerName, pTriggerName ) )
		{
			return info;
		}
	}

	Error( "Invalid trigger" );
	
	// in case some platforms don't have noreturn attribute on Error
	static SecretRoom_TriggerInfo unused; 
	return unused;
}

//-----------------------------------------------------------------------------
//	SecretRoom_InputStartTouchPlayerSlot
void CTFPasstimeLogic::statica( inputdata_t &input )
{
	SECRETROOM_LOG( "@@@@  SECRET ROOM: Start touch player slot\n" );

	if ( m_SecretRoom_state != SecretRoomState::Open )
	{
		SECRETROOM_LOG( "   @  SECRET ROOM: Ignore - state is not open\n" );

		// shouldn't happen because triggers should be disabled
		return;
	}

	if ( !input.pCaller || !input.pActivator )
	{
		SECRETROOM_LOG( "   @  SECRET ROOM: Ignore - no caller or activator\n" );

		return;
	}

	CTFPlayer *pActivator = ToTFPlayer( input.pActivator );
	SECRETROOM_LOG( "   @  SECRET ROOM: Toucher is %s\n", pActivator->GetPlayerName() );

	if ( !pActivator || pActivator->IsDead() || !pActivator->IsAlive() )
	{
		SECRETROOM_LOG( "  @   SECRET ROOM: Ignore - bad player\n" );

		// not a player or not normal
		return;
	}

	const char *pTriggerName = input.pCaller->GetEntityName().ToCStr();
	const auto& info = SecretRoom_GetSlotInfoForTrigger( pTriggerName );

	SECRETROOM_LOG( "   @  SECRET ROOM: Trigger is %s, slot is %i\n", pTriggerName, info.iIndex );


	if ( m_SecretRoom_slottedPlayers[info.iIndex] )
	{
		SECRETROOM_LOG( "   @  SECRET ROOM: Ignore - slot already filled by %s\n", m_SecretRoom_slottedPlayers[info.iIndex]->GetPlayerName() );

		// already someone filling the slot
		return;
	}

	int iActivatorTeam = pActivator->GetTeamNumber();
	int iActivatorClass = pActivator->GetPlayerClass()->GetClassIndex();

	if ( !pActivator 
		|| ( info.iTeam != iActivatorTeam ) 
		|| ( info.iClass != iActivatorClass ) )
	{
		SECRETROOM_LOG( "   @  SECRET ROOM: Ignore - wrong class %i (%i) or team %i (%i) \n",
			iActivatorTeam, info.iTeam, info.iClass, iActivatorClass );

		// doesn't match
		return;
	}

	SECRETROOM_LOG( "   @  SECRET ROOM: Set slot %i to %s\n", info.iIndex, pActivator->GetPlayerName() );

	// set slot
	m_SecretRoom_slottedPlayers[info.iIndex] = pActivator;

	// either solve puzzle or update effects
	int iNumSlotsFilled = SecretRoom_CountSlottedPlayers();
	SECRETROOM_LOG( "   @  SECRET ROOM: %i slots filled\n", iNumSlotsFilled );

	if ( iNumSlotsFilled == 9 )
	{
		SecretRoom_Solve();
	}
	else
	{
		SecretRoom_UpdateTv( iNumSlotsFilled );
	}
}

//-----------------------------------------------------------------------------
//	SecretRoom_InputEndTouchPlayerSlot
void CTFPasstimeLogic::staticb( inputdata_t &input )
{
	SECRETROOM_LOG( "@@@@  SECRET ROOM: End touch player slot\n" );

	if ( m_SecretRoom_state != SecretRoomState::Open )
	{
		SECRETROOM_LOG( "   @  SECRET ROOM: Ignore - state is not open\n" );

		// shouldn't happen because triggers should be disabled
		return;
	}

	const char *pTriggerName = input.pCaller->GetEntityName().ToCStr();
	const auto& info = SecretRoom_GetSlotInfoForTrigger( pTriggerName );

	SECRETROOM_LOG( "   @  SECRET ROOM: Trigger is %s, slot is %i\n", pTriggerName, info.iIndex );


	// input.pActivator can be null if a player disconnects while inside 
	// the trigger. but there's no way to tell if it's the player occupying 
	// the slot, so clear the slot just in case
	if ( input.pActivator )
	{
		CTFPlayer *pActivator = ToTFPlayer( input.pActivator );
		SECRETROOM_LOG( "   @  SECRET ROOM: Toucher is %s\n", pActivator->GetPlayerName() );

		if ( !pActivator || pActivator->IsDead() || !pActivator->IsAlive() )
		{
			SECRETROOM_LOG( "   @  SECRET ROOM: Ignore - bad player\n" );

			// not a player or not normal
			return;
		}

		if ( m_SecretRoom_slottedPlayers[info.iIndex] != input.pActivator )
		{
			if ( m_SecretRoom_slottedPlayers[info.iIndex] )
				SECRETROOM_LOG( "   @  SECRET ROOM: Ignore - slot is held by %s\n", m_SecretRoom_slottedPlayers[info.iIndex]->GetPlayerName() );
			else
				SECRETROOM_LOG( "   @  SECRET ROOM: Ignore - slot is empty\n" );

			// slot is empty already or some other player exiting the trigger

			// if slot is empty: due to this code not using proper filters,
			// this can be caused by players suiciding after changing teams
			// while standing inside the trigger, because the suicide happens
			// after the team change. this case is the entire reason for
			// m_SecretRoom_slottedPlayers.
			return;
		}
	}

	// clear the slot
	// note: in the case where two matching players are in the trigger 
	// and the one that entered first exits, the remaining player won't count
	// and will have to re-enter the trigger
	SECRETROOM_LOG( "   @  SECRET ROOM: Clear slot %i\n", info.iIndex );

	m_SecretRoom_slottedPlayers[info.iIndex] = nullptr;

	// update effects
	SecretRoom_UpdateTv( SecretRoom_CountSlottedPlayers() );
}

//-----------------------------------------------------------------------------
//	SecretRoom_InputPlugDamaged
void CTFPasstimeLogic::staticc( inputdata_t &input ) 
{
	SECRETROOM_LOG( "@@@@  SECRET ROOM: Plug destroyed\n" );

	NOTE_UNUSED( input );
	m_SecretRoom_state = SecretRoomState::Open;

	// set fx for puzzle open
	SecretRoom_UpdateTv( 0 );

	// enable triggers
	for ( const auto& info : s_SecretRoom_TriggerInfo )
	{
		g_EventQueue.AddEvent( info.pTriggerName, "Enable", 
			variant_t(), 0.0f, this, this );
	}
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::SecretRoom_Solve()
{
	if ( m_SecretRoom_state != SecretRoomState::Open )
	{
		// paranoia
		Assert( m_SecretRoom_state == SecretRoomState::Open );
		return;
	}

	SECRETROOM_LOG( "@@@@  SECRET ROOM: Solved\n" );

	m_SecretRoom_state = SecretRoomState::Solved;

	// set fx for puzzle solved
	g_EventQueue.AddEvent( "light", "TurnOn", variant_t(), 0.0f, this, this );
	g_EventQueue.AddEvent( "spotlight", "LightOn", variant_t(), 0.0f, this, this );
	g_EventQueue.AddEvent( "tv_particles", "Start", variant_t(), 0.0f, this, this );
	g_EventQueue.AddEvent( "screen_image", "Enable", variant_t(), 0.0f, this, this );
	SecretRoom_UpdateTv( 9 );

	// disable triggers
	for ( const auto& info : s_SecretRoom_TriggerInfo )
	{
		g_EventQueue.AddEvent( info.pTriggerName, "Disable", 
			variant_t(), 0.0f, this, this );
	}

	// achieves
	for ( auto id : m_SecretRoom_playersThatTouchedRoom )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerBySteamID( id ) );
		if ( pPlayer )
		{
			pPlayer->AwardAchievement( ACHIEVEMENT_TF_PASS_TIME_HAT );
		}
	}
	m_SecretRoom_playersThatTouchedRoom.RemoveAll(); // paranoia
}

//-----------------------------------------------------------------------------
void CTFPasstimeLogic::InputRoomTriggerOnTouch( inputdata_t &input )
{
	CTFPlayer *pPlayer = ToTFPlayer( input.pActivator );
	if ( !pPlayer || pPlayer->IsBot() )
	{
		return;
	}

	CSteamID id;
	pPlayer->GetSteamID( &id );
	if ( id.IsValid() && ( m_SecretRoom_playersThatTouchedRoom.Find( id ) == -1 ) )
	{
		SECRETROOM_LOG( "@@@@  SECRET ROOM: Tracking %s for achievement\n", pPlayer->GetPlayerName() );
		m_SecretRoom_playersThatTouchedRoom.AddToTail( id );
	}
}
