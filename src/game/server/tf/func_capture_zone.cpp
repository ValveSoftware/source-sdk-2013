//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Flag Capture Zone.
//
//=============================================================================//
#include "cbase.h"
#include "func_capture_zone.h"
#include "tf_player.h"
#include "tf_item.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "entity_capture_flag.h"
#include "tf_logic_player_destruction.h"

//=============================================================================
//
// CTF Flag Capture Zone tables.
//

BEGIN_DATADESC( CCaptureZone )

// Keyfields.
DEFINE_KEYFIELD( m_nCapturePoint, FIELD_INTEGER, "CapturePoint" ),
DEFINE_KEYFIELD( m_flCaptureDelay, FIELD_FLOAT, "capture_delay" ),
DEFINE_KEYFIELD( m_flCaptureDelayOffset, FIELD_FLOAT, "capture_delay_offset" ),
DEFINE_KEYFIELD( m_bShouldBlock, FIELD_BOOLEAN, "shouldBlock" ),

// Functions.
DEFINE_FUNCTION( CCaptureZoneShim::Touch ),

// Inputs.
DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

// Outputs.
DEFINE_OUTPUT( m_outputOnCapture, "OnCapture" ),
DEFINE_OUTPUT( m_OnCapTeam1, "OnCapTeam1" ),
DEFINE_OUTPUT( m_OnCapTeam2, "OnCapTeam2" ),
DEFINE_OUTPUT( m_OnCapTeam1_PD, "OnCapTeam1_PD" ),
DEFINE_OUTPUT( m_OnCapTeam2_PD, "OnCapTeam2_PD" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_capturezone, CCaptureZone );


IMPLEMENT_SERVERCLASS_ST( CCaptureZone, DT_CaptureZone )
	SendPropBool( SENDINFO( m_bDisabled ) ),
END_SEND_TABLE()

IMPLEMENT_AUTO_LIST( ICaptureZoneAutoList );

//=============================================================================
//
// CTF Flag Capture Zone functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CCaptureZone::CCaptureZone()
{
	m_bShouldBlock = true;
	m_flCaptureDelay = 1.1f;
	m_flCaptureDelayOffset = 0.025f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureZone::Spawn()
{
	InitTrigger();
	SetTouch( &CCaptureZoneShim::Touch );

	if ( m_bDisabled )
	{
		SetDisabled( true );
	}

	m_flNextTouchingEnemyZoneWarning = -1;
	AddSpawnFlags( SF_TRIGGER_ALLOW_ALL ); // so we can keep track of who is touching us
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureZone::Activate( void )
{
	BaseClass::Activate();

	if ( TFGameRules() && ( TFGameRules()->GetGameType() == TF_GAMETYPE_PD ) )
	{
		SetThink( &CCaptureZone::PlayerDestructionThink );
		SetNextThink( gpGlobals->curtime + 0.1 );
	}
	else
	{
		SetThink( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureZone::PlayerDestructionThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.1 );

	if ( !IsDisabled() )
	{
		bool bRedInZone = false;
		bool bBlueInZone = false;

		// nothing to do while no-one is touching us
		if ( m_hTouchingEntities.Count() == 0 )
			return;

		// loop through the touching players to figure out the teams involved
		for ( int i = 0; i < m_hTouchingEntities.Count(); i++ )
		{
			CBaseEntity *pEnt = m_hTouchingEntities[i];
			if ( pEnt && pEnt->IsPlayer() )
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( pEnt );
				if ( pTFPlayer && pTFPlayer->IsAlive() )
				{
					bool bHidden = ( pTFPlayer->m_Shared.IsStealthed() || pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) || pTFPlayer->m_Shared.InCond( TF_COND_DISGUISING ) );
					if ( !bHidden )
					{
						if ( pTFPlayer->GetTeamNumber() == TF_TEAM_RED )
						{
							bRedInZone = true;
						}
						else if ( pTFPlayer->GetTeamNumber() == TF_TEAM_BLUE )
						{
							bBlueInZone = true;
						}
					}
				}
			}
		}

		// safety check, but this should have already been caught by the ( m_hTouchingEntities.Count() == 0 ) check above
		if ( !bRedInZone && !bBlueInZone )
			return;

		if ( m_bShouldBlock )
		{
			// both teams are touching the zone and they block each other
			if ( bRedInZone && bBlueInZone )
				return;
		}

		CUtlVector< CTFPlayer* > playerVector;
		CollectPlayers( &playerVector, TF_TEAM_RED );
		CollectPlayers( &playerVector, TF_TEAM_BLUE, false, APPEND_PLAYERS );
		float flCaptureDelay = m_flCaptureDelay	- ( m_flCaptureDelayOffset * playerVector.Count() );

		// let's see if anyone has any player destruction points to capture
		for ( int i = 0; i < m_hTouchingEntities.Count(); i++ )
		{
			CBaseEntity *pEnt = m_hTouchingEntities[i];
			if ( pEnt && pEnt->IsPlayer() )
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( pEnt );
				if ( pTFPlayer && pTFPlayer->IsAlive() )
				{
					// does this capture point have a team number assigned?
					if ( ( GetTeamNumber() != TEAM_UNASSIGNED ) && ( pTFPlayer->GetTeamNumber() != GetTeamNumber() ) )
						continue;

					if ( pTFPlayer->HasTheFlag() && pTFPlayer->CanScorePointForPD() )
					{
						CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag* >( pTFPlayer->GetItem() );

						int nPoints = pFlag->GetPointValue();
						if ( nPoints > 0 )
						{
							// decrease the number of points
							pFlag->AddPointValue( -1 );

							// fire the output
							switch ( pTFPlayer->GetTeamNumber() )
							{
							case TF_TEAM_RED:
								m_OnCapTeam1_PD.FireOutput( this, this );
								break;
							case TF_TEAM_BLUE:
								m_OnCapTeam2_PD.FireOutput( this, this );
								break;
							default:
								break;
							}

							IGameEvent *event = gameeventmanager->CreateEvent( "special_score" );
							if ( event )
							{
								event->SetInt( "player", pTFPlayer->entindex() );
								gameeventmanager->FireEvent( event );
							}
						}

						// remove this flag if this was the last point
						if ( pFlag->GetPointValue() == 0 )
						{
							UTIL_Remove( pFlag );
						}

						if ( CTFPlayerDestructionLogic::GetPlayerDestructionLogic() )
						{
							CTFPlayerDestructionLogic::GetPlayerDestructionLogic()->CalcTeamLeader( pTFPlayer->GetTeamNumber() );
						}

						pTFPlayer->SetNextScorePointForPD( gpGlobals->curtime + flCaptureDelay );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureZone::ShimTouch( CBaseEntity *pOther )
{
	// Is the zone enabled?
	if ( IsDisabled() )
		return;

	// Get the TF player.
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( pPlayer )
	{
		// Check to see if the player has the capture flag.
		if ( pPlayer->HasItem() && ( pPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG ) )
		{
			CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*>( pPlayer->GetItem() );
			if ( pFlag )
			{
				// we have a special think that will handle the player destruction flags
				if ( pFlag->GetType() == TF_FLAGTYPE_PLAYER_DESTRUCTION )
					return;

				if ( !pFlag->IsCaptured() )
				{
					// does this capture point have a team number assigned?
					if ( GetTeamNumber() != TEAM_UNASSIGNED )
					{
						// Check to see if the capture zone team matches the player's team.
						if ( pPlayer->GetTeamNumber() != TEAM_UNASSIGNED && pPlayer->GetTeamNumber() != GetTeamNumber() )
						{
							if ( pFlag->GetType() == TF_FLAGTYPE_CTF )
							{
								// Do this at most once every 5 seconds
								if ( m_flNextTouchingEnemyZoneWarning < gpGlobals->curtime )
								{
									CSingleUserRecipientFilter filter( pPlayer );
									TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_TOUCHING_ENEMY_CTF_CAP );
									m_flNextTouchingEnemyZoneWarning = gpGlobals->curtime + 5;
								}
							}
		// 						else if ( pFlag->GetGameType() == TF_FLAGTYPE_INVADE )
		// 						{
		// 						}

							return;
						}
					}
				}

				// in MvM, the "flag" is the bomb and is captured when the carrying bot deploys it
				if ( TFGameRules()->FlagsMayBeCapped() && !TFGameRules()->IsMannVsMachineMode() )
				{
					Capture( pOther );
				}
			}
		}
	}
}

void CCaptureZone::Capture( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( pPlayer )
	{
		// Check to see if the player has the capture flag and flag is allowed to be captured.
		if ( pPlayer->HasItem() && ( pPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG ) && TFGameRules()->CanFlagBeCaptured( pOther ) )
		{
			CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag* >( pPlayer->GetItem() );
			if ( !pFlag )
				return;

			// we have a special think that will handle the player destruction flags
			if ( pFlag->GetType() == TF_FLAGTYPE_PLAYER_DESTRUCTION )
				return;

			if ( !pFlag->IsCaptured() )
			{
				pFlag->Capture( pPlayer, m_nCapturePoint );

				// Outputs
				m_outputOnCapture.FireOutput( this, this );

				switch ( pPlayer->GetTeamNumber() )
				{
				case TF_TEAM_RED:
					m_OnCapTeam1.FireOutput( this, this );
					break;
				case TF_TEAM_BLUE:
					m_OnCapTeam2.FireOutput( this, this );
					break;
				default:
					break;
				}

				IGameEvent *event = gameeventmanager->CreateEvent( "ctf_flag_captured" );
				if ( event )
				{
					int iCappingTeam = pPlayer->GetTeamNumber();
					int	iCappingTeamScore = 0;
					CTFTeam* pCappingTeam = pPlayer->GetTFTeam();
					if ( pCappingTeam )
					{
						iCappingTeamScore = pCappingTeam->GetFlagCaptures();
					}

					event->SetInt( "capping_team", iCappingTeam );
					event->SetInt( "capping_team_score", iCappingTeamScore );
					event->SetInt( "capper", pPlayer->GetUserID() );
					event->SetInt( "priority", 9 ); // HLTV priority

					gameeventmanager->FireEvent( event );
				}

				if ( TFGameRules() )
				{
					if ( TFGameRules()->IsHolidayActive( kHoliday_EOTL ) )
					{
						TFGameRules()->DropBonusDuck( pPlayer->GetAbsOrigin(), pPlayer, NULL, NULL, false, true );
					}
					else if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
					{
						TFGameRules()->DropHalloweenSoulPackToTeam( 5, GetAbsOrigin(), pPlayer->GetTeamNumber(), TEAM_SPECTATOR );
					}
				}
			}
		}
		else if ( !TFGameRules()->CanFlagBeCaptured( pOther ) && TFGameRules()->IsPowerupMode() )
		{ 
			ClientPrint( pPlayer, HUD_PRINTCENTER, "#TF_CTF_Cannot_Capture" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: The timer is always transmitted to clients
//-----------------------------------------------------------------------------
int CCaptureZone::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureZone::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureZone::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureZone::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureZone::SetDisabled( bool bDisabled )
{
	m_bDisabled.Set( bDisabled );

	if ( bDisabled )
	{
		BaseClass::Disable();
		SetTouch( NULL );
	}
	else
	{
		BaseClass::Enable();
		SetTouch( &CCaptureZoneShim::Touch );
	}
}


//=============================================================================
//
// Flag Detection Zone tables.
//

BEGIN_DATADESC( CFlagDetectionZone )

	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN,	"StartDisabled" ),
	DEFINE_KEYFIELD( m_bShouldAlarm, FIELD_BOOLEAN,	"alarm" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Test", InputTest ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnStartTouchFlag, "OnStartTouchFlag" ),
	DEFINE_OUTPUT( m_outputOnEndTouchFlag, "OnEndTouchFlag" ),
	DEFINE_OUTPUT( m_outputOnDroppedFlag, "OnDroppedFlag" ),
	DEFINE_OUTPUT( m_outputOnPickedUpFlag, "OnPickedUpFlag" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_flagdetectionzone, CFlagDetectionZone );

IMPLEMENT_AUTO_LIST( IFlagDetectionZoneAutoList );


//=============================================================================
//
// Flag Detection Zone functions.
//

CFlagDetectionZone::CFlagDetectionZone()
{
	m_bShouldAlarm = false;
}

void CFlagDetectionZone::Spawn()
{
	InitTrigger();

	if ( m_bDisabled )
	{
		SetDisabled( true );
	}
}

void CFlagDetectionZone::StartTouch( CBaseEntity *pOther )
{
	// Is the zone enabled?
	if ( IsDisabled() )
		return;

	if ( pOther->IsPlayer() )
	{
		EHANDLE hOther;
		hOther = pOther;

		if ( m_hTouchingPlayers.Find( hOther ) == m_hTouchingPlayers.InvalidIndex() )
		{
			m_hTouchingPlayers.AddToTail( hOther );
		}
	}

	if ( EntityIsFlagCarrier( pOther ) )
	{
		EHANDLE hOther;
		hOther = pOther;

		bool bAdded = false;
		if ( m_hTouchingEntities.Find( hOther ) == m_hTouchingEntities.InvalidIndex() )
		{
			m_hTouchingEntities.AddToTail( hOther );
			bAdded = true;
		}

		m_OnStartTouch.FireOutput( pOther, this );

		if ( bAdded && ( m_hTouchingEntities.Count() == 1 ) )
		{
			// First entity to touch us that passes our filters
			m_outputOnStartTouchFlag.FireOutput( pOther, this );
			m_OnStartTouchAll.FireOutput( pOther, this );
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "flag_carried_in_detection_zone" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}
}

void CFlagDetectionZone::EndTouch( CBaseEntity *pOther )
{
	// Is the zone enabled?
	if ( IsDisabled() )
		return;

	if ( pOther->IsPlayer() )
	{
		EHANDLE hOther;
		hOther = pOther;
		
		m_hTouchingPlayers.FindAndRemove( hOther );
	}

	if ( IsTouching( pOther ) )
	{
		EHANDLE hOther;
		hOther = pOther;

		m_hTouchingEntities.FindAndRemove( hOther );

		m_OnEndTouch.FireOutput(pOther, this);

		// If there are no more entities touching this trigger, fire the lost all touches
		// Loop through the touching entities backwards. Clean out old ones, and look for existing
		bool bFoundOtherTouchee = false;
		int iSize = m_hTouchingEntities.Count();
		for ( int i = iSize-1; i >= 0; i-- )
		{
			EHANDLE hOther;
			hOther = m_hTouchingEntities[i];

			if ( !hOther )
			{
				m_hTouchingEntities.Remove( i );
			}
			else if ( hOther->IsPlayer() && !hOther->IsAlive() )
			{
				m_hTouchingEntities.Remove( i );
			}
			else
			{
				bFoundOtherTouchee = true;
			}
		}

		//FIXME: Without this, triggers fire their EndTouch outputs when they are disabled!
		// Didn't find one?
		if ( !bFoundOtherTouchee /*&& !m_bDisabled*/ )
		{
			m_outputOnEndTouchFlag.FireOutput( this, this );
			m_OnEndTouchAll.FireOutput( pOther, this);
		}
	}
}

void CFlagDetectionZone::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

void CFlagDetectionZone::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

void CFlagDetectionZone::InputTest( inputdata_t &inputdata )
{
	// Loop through the touching entities backwards. Clean out old ones, and look for existing
	int iSize = m_hTouchingEntities.Count();
	for ( int i = iSize-1; i >= 0; i-- )
	{
		EHANDLE hOther;
		hOther = m_hTouchingEntities[i];

		if ( !hOther )
		{
			m_hTouchingEntities.Remove( i );
		}
		else if ( hOther->IsPlayer() && !hOther->IsAlive() )
		{
			m_hTouchingEntities.Remove( i );
		}
	}

	if ( m_hTouchingEntities.Count() )
	{
		m_OnStartTouch.FireOutput( m_hTouchingEntities[ 0 ], this );
		m_outputOnStartTouchFlag.FireOutput( this, this );
	}
	else
	{
		m_outputOnEndTouchFlag.FireOutput( this, this );
		m_OnEndTouchAll.FireOutput( this, this );
	}
}

void CFlagDetectionZone::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;

	if ( bDisabled )
	{
		BaseClass::Disable();
		SetTouch( NULL );
	}
	else
	{
		BaseClass::Enable();
	}
}

void CFlagDetectionZone::FlagDropped( CBasePlayer *pPlayer )
{
	EHANDLE hOther;
	hOther = pPlayer;

	if ( m_hTouchingEntities.Find( hOther ) != m_hTouchingEntities.InvalidIndex() )
	{
		m_outputOnDroppedFlag.FireOutput( pPlayer, this );
		EndTouch( pPlayer );

		// Still touching as a non-carrierz
		m_hTouchingPlayers.AddToTail( hOther );
	}
}

void CFlagDetectionZone::FlagPickedUp( CBasePlayer *pPlayer )
{
	EHANDLE hOther;
	hOther = pPlayer;
	
	if ( m_hTouchingPlayers.Find( hOther ) != m_hTouchingPlayers.InvalidIndex() )
	{
		m_outputOnPickedUpFlag.FireOutput( pPlayer, this );
		StartTouch( pPlayer );
	}
}

bool CFlagDetectionZone::EntityIsFlagCarrier( CBaseEntity *pEntity )
{
	// Get the TF player.
	CTFPlayer *pPlayer = ToTFPlayer( pEntity );
	if ( pPlayer )
	{
		// Check to see if the player has the capture flag.
		if ( pPlayer->HasItem() && ( pPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG ) )
		{
			CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*>( pPlayer->GetItem() );
			if ( pFlag && !pFlag->IsCaptured() )
			{
				return true;
			}
		}
	}

	return false;
}

void CFlagDetectionZone::FlagCaptured( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	if ( FStrEq( "sd_doomsday", STRING( gpGlobals->mapname ) ) )
	{
		EHANDLE hOther;
		hOther = pPlayer;

		if ( m_hTouchingPlayers.Find( hOther ) != m_hTouchingPlayers.InvalidIndex() )
		{
			int nWinningTeam = pPlayer->GetTeamNumber();
			CUtlVector< EHANDLE > winningPlayers;

			for ( int i = 0 ; i < m_hTouchingPlayers.Count() ; i++ )
			{
				EHANDLE hTemp = m_hTouchingPlayers[i];
				if ( hTemp && ( hTemp->GetTeamNumber() == nWinningTeam ) )
				{
					winningPlayers.AddToHead( hTemp );
				}
			}

			// ACHIEVEMENT_TF_MAPS_DOOMSDAY_RIDE_THE_ELEVATOR
			if ( winningPlayers.Count() >= 5 )
			{
				// loop through and award the achievement
				for ( int i = 0 ; i < winningPlayers.Count() ; i++ )
				{
					EHANDLE hTemp = winningPlayers[i];
					if ( hTemp )
					{
						CTFPlayer *pTFPlayer = ToTFPlayer( hTemp );
						if ( pTFPlayer )
						{
							pTFPlayer->AwardAchievement( ACHIEVEMENT_TF_MAPS_DOOMSDAY_RIDE_THE_ELEVATOR );
						}
					}
				}
			}
 		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles if the specified entity is dropped in a detection zone
//-----------------------------------------------------------------------------
void HandleFlagDroppedInDetectionZone( CBasePlayer *pPlayer )
{
	for ( int i=0; i<IFlagDetectionZoneAutoList::AutoList().Count(); ++i )
	{
		CFlagDetectionZone *pZone = static_cast<CFlagDetectionZone *>( IFlagDetectionZoneAutoList::AutoList()[i] );
		if ( !pZone->IsDisabled() )
		{
			pZone->FlagDropped( pPlayer );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles if the specified entity is picked up in a detection zone
//-----------------------------------------------------------------------------
void HandleFlagPickedUpInDetectionZone( CBasePlayer *pPlayer )
{
	for ( int i=0; i<IFlagDetectionZoneAutoList::AutoList().Count(); ++i )
	{
		CFlagDetectionZone *pZone = static_cast<CFlagDetectionZone *>( IFlagDetectionZoneAutoList::AutoList()[i] );
		if ( !pZone->IsDisabled() )
		{
			pZone->FlagPickedUp( pPlayer );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles if the specified entity is captured in a detection zone 
//-----------------------------------------------------------------------------
void HandleFlagCapturedInDetectionZone( CBasePlayer *pPlayer )
{
	for ( int i=0; i<IFlagDetectionZoneAutoList::AutoList().Count(); ++i )
	{
		CFlagDetectionZone *pZone = static_cast<CFlagDetectionZone *>( IFlagDetectionZoneAutoList::AutoList()[i] );
		if ( !pZone->IsDisabled() )
		{
			pZone->FlagCaptured( pPlayer );
		}
	}
}
