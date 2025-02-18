//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entities for use in the Robot Destruction TF2 game mode.
//
//=========================================================================//

#include "cbase.h"
#include "tf_logic_player_destruction.h"

#ifdef GAME_DLL
#include "tf_player.h"
#include "entity_capture_flag.h"
#include "tf_obj_dispenser.h"
#include "tf_gamerules.h"
#else
#include "c_tf_player.h"
#endif // GAME_DLL

BEGIN_DATADESC( CPlayerDestructionDispenser )
END_DATADESC()

IMPLEMENT_NETWORKCLASS_ALIASED( PlayerDestructionDispenser, DT_PlayerDestructionDispenser )
LINK_ENTITY_TO_CLASS( pd_dispenser, CPlayerDestructionDispenser );

BEGIN_NETWORK_TABLE( CPlayerDestructionDispenser, DT_PlayerDestructionDispenser )
END_NETWORK_TABLE()
				   
#ifdef GAME_DLL
BEGIN_DATADESC( CTFPlayerDestructionLogic )
	DEFINE_KEYFIELD( m_iszPropModelName, FIELD_STRING, "prop_model_name" ),
	DEFINE_KEYFIELD( m_iszPropDropSound, FIELD_STRING, "prop_drop_sound" ),
	DEFINE_KEYFIELD( m_iszPropPickupSound, FIELD_STRING, "prop_pickup_sound" ),
	DEFINE_KEYFIELD( m_nMinPoints, FIELD_INTEGER, "min_points" ),
	DEFINE_KEYFIELD( m_nPointsPerPlayer, FIELD_INTEGER, "points_per_player" ),
	DEFINE_KEYFIELD( m_nFlagResetDelay, FIELD_INTEGER, "flag_reset_delay" ),
	DEFINE_KEYFIELD( m_nHealDistance, FIELD_INTEGER, "heal_distance" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ScoreRedPoints", InputScoreRedPoints ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ScoreBluePoints", InputScoreBluePoints ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableMaxScoreUpdating", InputEnableMaxScoreUpdating ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableMaxScoreUpdating", InputDisableMaxScoreUpdating ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetCountdownTimer", InputSetCountdownTimer ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCountdownImage", InputSetCountdownImage ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetFlagResetDelay", InputSetFlagResetDelay ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetPointsOnPlayerDeath", InputSetPointsOnPlayerDeath ),

	DEFINE_OUTPUT( m_OnRedScoreChanged, "OnRedScoreChanged" ),
	DEFINE_OUTPUT( m_OnBlueScoreChanged, "OnBlueScoreChanged" ),
	DEFINE_OUTPUT( m_OnCountdownTimerExpired, "OnCountdownTimerExpired" ),
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_logic_player_destruction, CTFPlayerDestructionLogic );
IMPLEMENT_NETWORKCLASS_ALIASED( TFPlayerDestructionLogic, DT_TFPlayerDestructionLogic )

BEGIN_NETWORK_TABLE( CTFPlayerDestructionLogic, DT_TFPlayerDestructionLogic )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hRedTeamLeader ) ),
	RecvPropEHandle( RECVINFO( m_hBlueTeamLeader ) ),
	RecvPropString( RECVINFO( m_iszCountdownImage ) ),
	RecvPropBool( RECVINFO( m_bUsingCountdownImage ) ),
#else
	SendPropEHandle( SENDINFO( m_hRedTeamLeader ) ),
	SendPropEHandle( SENDINFO( m_hBlueTeamLeader ) ),
	SendPropStringT( SENDINFO( m_iszCountdownImage ) ),
	SendPropBool( SENDINFO( m_bUsingCountdownImage ) ),
#endif
END_NETWORK_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerDestructionLogic::CTFPlayerDestructionLogic()
{
#ifdef GAME_DLL
	m_iszPropModelName = MAKE_STRING( "models/flag/flag.mdl" );
	ListenForGameEvent( "player_disconnect" );
	m_bMaxScoreUpdatingAllowed = false;
	m_nFlagResetDelay = 60;
	m_nHealDistance = 450;
	m_nPointsOnPlayerDeath = 1;
#endif // GAME_DLL

	m_hRedTeamLeader = NULL;
	m_hBlueTeamLeader = NULL;

	m_bUsingCountdownImage = false;

#ifdef CLIENT_DLL
	m_iszCountdownImage[0] = '\0';
#else
	m_iszCountdownImage.Set( NULL_STRING );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerDestructionLogic* CTFPlayerDestructionLogic::GetPlayerDestructionLogic()
{
	return assert_cast< CTFPlayerDestructionLogic* >( CTFRobotDestructionLogic::GetRobotDestructionLogic() );
}


#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerDestructionLogic::Precache()
{
	BaseClass::Precache();

	PrecacheModel( GetPropModelName() );
	PrecacheScriptSound( STRING( m_iszPropDropSound ) );
	PrecacheScriptSound( STRING( m_iszPropPickupSound ) );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFPlayerDestructionLogic::GetPropModelName() const
{
	return STRING( m_iszPropModelName );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerDestructionLogic::CalcTeamLeader( int iTeam )
{
	// team leader's changed team, recalculate team leader for that team
	if ( m_hRedTeamLeader.Get() && m_hRedTeamLeader.Get()->GetTeamNumber() != TF_TEAM_RED )
	{
		m_hRedTeamLeader = NULL;
		CalcTeamLeader( TF_TEAM_RED );
	}
	if ( m_hBlueTeamLeader.Get() && m_hBlueTeamLeader.Get()->GetTeamNumber() != TF_TEAM_BLUE )
	{
		m_hBlueTeamLeader = NULL;
		CalcTeamLeader( TF_TEAM_BLUE );
	}

	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, iTeam, COLLECT_ONLY_LIVING_PLAYERS );

	CTFPlayer *pTeamLeader = iTeam == TF_TEAM_RED ? m_hRedTeamLeader.Get() : m_hBlueTeamLeader.Get();
	int iCurrentLeadingPoint = 0;
	if ( pTeamLeader && pTeamLeader->HasItem() )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*>( pTeamLeader->GetItem() );
		if ( pFlag )
		{
			iCurrentLeadingPoint = pFlag->GetPointValue();
		}
	}
	else
	{
		// reset team leader
		pTeamLeader = NULL;
		if ( iTeam == TF_TEAM_RED )
		{
			m_hRedTeamLeader = NULL;
			UTIL_Remove( m_hRedDispenser );
			m_hRedDispenser = NULL;
		}
		else
		{
			m_hBlueTeamLeader = NULL;
			UTIL_Remove( m_hBlueDispenser );
			m_hBlueDispenser = NULL;
		}
	}

	// find new team leader
	CTFPlayer *pNewTeamLeader = NULL;
	FOR_EACH_VEC( playerVector, i )
	{
		CTFPlayer *pPlayer = playerVector[i];
		if ( pPlayer == pTeamLeader )
			continue;

		// community request from Watergate author to never have a SPY be the team leader
		if ( pPlayer->HasItem() && !pPlayer->IsPlayerClass( TF_CLASS_SPY ) )
		{
			CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag* >( pPlayer->GetItem() );
			if ( pFlag && pFlag->GetPointValue() > iCurrentLeadingPoint )
			{
				iCurrentLeadingPoint = pFlag->GetPointValue();
				pNewTeamLeader = pPlayer;
			}
		}
	}

	// set new leader
	if ( pNewTeamLeader )
	{
		CObjectDispenser *pDispenser = NULL;
		if ( iTeam == TF_TEAM_RED )
		{
			m_hRedTeamLeader = pNewTeamLeader;

			if ( !m_hRedDispenser )
			{
				m_hRedDispenser = CreateDispenser( iTeam );
			}
			pDispenser = m_hRedDispenser;
		}
		else
		{
			m_hBlueTeamLeader = pNewTeamLeader;

			if ( !m_hBlueDispenser )
			{
				m_hBlueDispenser = CreateDispenser( iTeam );
			}
			pDispenser = m_hBlueDispenser;
		}

		if ( pDispenser )
		{
			pDispenser->SetOwnerEntity( pNewTeamLeader );
			pDispenser->FollowEntity( pNewTeamLeader );
			pDispenser->SetBuilder( pNewTeamLeader );
		}
	}
}

void CTFPlayerDestructionLogic::FireGameEvent( IGameEvent *pEvent )
{
	const char* pszName = pEvent->GetName();
	if ( FStrEq( pszName, "player_spawn" ) || FStrEq( pszName, "player_disconnect" ) )
	{
		EvaluatePlayerCount();
		return;
	}
	else if( FStrEq( pszName, "teamplay_pre_round_time_left" ) )
	{
		// Eat this event so the RD logic doesn't talk
		return;
	}

	BaseClass::FireGameEvent( pEvent );
}

void CTFPlayerDestructionLogic::OnRedScoreChanged()
{
	m_OnRedScoreChanged.Set( (float)m_nRedScore / m_nMaxPoints, this, this );
}

void CTFPlayerDestructionLogic::OnBlueScoreChanged()
{
	m_OnBlueScoreChanged.Set( (float)m_nBlueScore / m_nMaxPoints, this, this );
}

void CTFPlayerDestructionLogic::EvaluatePlayerCount()
{
	// Bail if we're not allowed
	if ( !m_bMaxScoreUpdatingAllowed )
		return;

	CUtlVector< CTFPlayer* > vecAllPlayers;
	CollectPlayers( &vecAllPlayers );

	m_nMaxPoints = Max( m_nMinPoints, m_nPointsPerPlayer * vecAllPlayers.Count() );
}

void CTFPlayerDestructionLogic::InputScoreRedPoints( inputdata_t& inputdata )
{
	ScorePoints( TF_TEAM_RED, 1, SCORE_CORES_COLLECTED, NULL );
}

void CTFPlayerDestructionLogic::InputScoreBluePoints( inputdata_t& inputdata )
{
	ScorePoints( TF_TEAM_BLUE, 1, SCORE_CORES_COLLECTED, NULL );
}

void CTFPlayerDestructionLogic::InputEnableMaxScoreUpdating( inputdata_t& inputdata )
{
	m_bMaxScoreUpdatingAllowed = true;
	EvaluatePlayerCount();
}

void CTFPlayerDestructionLogic::InputDisableMaxScoreUpdating( inputdata_t& inputdata )
{
	EvaluatePlayerCount();
	m_bMaxScoreUpdatingAllowed = false;
}

void CTFPlayerDestructionLogic::InputSetCountdownTimer( inputdata_t& inputdata )
{
	int nTime = inputdata.value.Int();

	if ( nTime > 0 )
	{
		SetCountdownEndTime( gpGlobals->curtime + nTime );
		SetThink( &CTFPlayerDestructionLogic::CountdownThink );
		SetNextThink( gpGlobals->curtime + 0.05f );
	}
	else
	{
		SetCountdownEndTime( -1.f );
		SetThink( NULL );
	}
}

void CTFPlayerDestructionLogic::CountdownThink( void )
{
	if ( m_flCountdownEndTime > -1.f )
	{
		// if we're done, just reset the end time
		if ( m_flCountdownEndTime < gpGlobals->curtime )
		{
			m_OnCountdownTimerExpired.FireOutput( this, this );
			m_flCountdownEndTime = -1.f;
			SetThink( NULL );
			return;
		}
	}

	SetNextThink( gpGlobals->curtime + 0.05f );
}

void CTFPlayerDestructionLogic::InputSetCountdownImage( inputdata_t& inputdata )
{
	m_bUsingCountdownImage = true;
	m_iszCountdownImage = inputdata.value.StringID();
}


void CTFPlayerDestructionLogic::InputSetFlagResetDelay( inputdata_t& inputdata )
{
	int nDelay = inputdata.value.Int();
	if ( nDelay < 0 )
	{
		nDelay = 0;
	}

	m_nFlagResetDelay = nDelay;
}

void CTFPlayerDestructionLogic::InputSetPointsOnPlayerDeath( inputdata_t& inputdata )
{
	int nPointsOnPlayerDeath = inputdata.value.Int();
	if ( nPointsOnPlayerDeath < 0 )
	{
		nPointsOnPlayerDeath = 0;
	}

	m_nPointsOnPlayerDeath = nPointsOnPlayerDeath;
}

CObjectDispenser *CTFPlayerDestructionLogic::CreateDispenser( int iTeam )
{
	CPlayerDestructionDispenser *pDispenser = static_cast< CPlayerDestructionDispenser* >( CBaseEntity::CreateNoSpawn( "pd_dispenser", vec3_origin, vec3_angle, NULL ) );
	pDispenser->ChangeTeam( iTeam );
	pDispenser->SetObjectFlags( pDispenser->GetObjectFlags() | OF_DOESNT_HAVE_A_MODEL | OF_PLAYER_DESTRUCTION );
	pDispenser->m_iUpgradeLevel = 1;
	DispatchSpawn( pDispenser );
	pDispenser->FinishedBuilding();
	pDispenser->AddEffects( EF_NODRAW );
	pDispenser->DisableAmmoPickupSound();
	pDispenser->DisableGenerateMetalSound();
	pDispenser->m_takedamage = DAMAGE_NO;

	CBaseEntity *pTouchTrigger = pDispenser->GetTouchTrigger();
	if ( pTouchTrigger )
	{
		pTouchTrigger->FollowEntity( pDispenser );
	}

	return pDispenser;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerDestructionLogic::PlayPropDropSound( CTFPlayer *pPlayer )
{
	PlaySound( STRING( m_iszPropDropSound ), pPlayer );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerDestructionLogic::PlayPropPickupSound( CTFPlayer *pPlayer )
{
	PlaySound( STRING( m_iszPropPickupSound ), pPlayer );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerDestructionLogic::PlaySound( const char *pszSound, CTFPlayer *pPlayer )
{
	EmitSound_t params;
	params.m_pSoundName = pszSound;
	params.m_flSoundTime = 0;
	params.m_pflSoundDuration = 0;
	params.m_SoundLevel = SNDLVL_70dB;
	CPASFilter filter( pPlayer->GetAbsOrigin() );
	pPlayer->EmitSound( filter, pPlayer->entindex(), params );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPlayerDestructionDispenser::Spawn( void )
{
	// This cast is for the benefit of GCC
	m_fObjectFlags |= (int)OF_DOESNT_HAVE_A_MODEL;
	m_takedamage = DAMAGE_NO;
	m_iUpgradeLevel = 1;

	TFGameRules()->OnDispenserBuilt( this );
}

//-----------------------------------------------------------------------------
// Purpose: Finished building
//-----------------------------------------------------------------------------
void CPlayerDestructionDispenser::OnGoActive( void )
{
	BaseClass::OnGoActive();

	if ( m_hTouchTrigger )
	{
		m_hTouchTrigger->SetParent( GetParent() );
	}

	SetModel( "" ); 
}

//-----------------------------------------------------------------------------
// Spawn the vgui control screens on the object
//-----------------------------------------------------------------------------
void CPlayerDestructionDispenser::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	// no panels
	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerDestructionLogic::TeamWin( int nTeam )
{
	if ( TFGameRules() )
	{
		TFGameRules()->SetWinningTeam( nTeam, WINREASON_PD_POINTS );
	}
}
#endif // GAME_DLL


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer *CTFPlayerDestructionLogic::GetTeamLeader( int iTeam ) const
{
	return iTeam == TF_TEAM_RED ? m_hRedTeamLeader.Get() : m_hBlueTeamLeader.Get();
}

