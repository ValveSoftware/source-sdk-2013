//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"

#ifdef CLIENT_DLL

#include "achievementmgr.h"
#include "baseachievement.h"
#include "tf_hud_statpanel.h"
#include "c_tf_team.h"
#include "c_tf_player.h"
#include "c_tf_playerresource.h"
#include "tf_gamerules.h"
#include "econ_wearable.h"
#include "achievements_tf.h"
#include "usermessages.h"

// NVNT include for tf2 damage
#include "haptics/haptic_utils.h"

CAchievementMgr g_AchievementMgrTF;	// global achievement mgr for TF

bool CheckWinNoEnemyCaps( IGameEvent *event, int iRole );

// Grace period that we allow a player to start after level init and still consider them to be participating for the full round.  This is fairly generous
// because it can in some cases take a client several minutes to connect with respect to when the server considers the game underway
#define TF_FULL_ROUND_GRACE_PERIOD	( 4 * 60.0f )

bool IsLocalTFPlayerClass( int iClass );


bool CBaseTFAchievementSimple::LocalPlayerCanEarn( void ) 
{ 
	if ( TFGameRules() )
	{
		bool bMVMAchievement = ( m_iAchievementID >= ACHIEVEMENT_TF_MVM_START_RANGE && m_iAchievementID <= ACHIEVEMENT_TF_MVM_END_RANGE );

		if ( bMVMAchievement )
		{
			if ( !TFGameRules()->IsMannVsMachineMode() || ( GetLocalPlayerTeam() != TF_TEAM_PVE_DEFENDERS ) )
			{
				return false;
			}
		}
		else
		{
			if ( TFGameRules()->IsMannVsMachineMode() )
			{
				return false;
			}
		}
	}

	return BaseClass::LocalPlayerCanEarn();
}

void CBaseTFAchievementSimple::FireGameEvent( IGameEvent *event )
{
	if ( !LocalPlayerCanEarn() )
		return;

	BaseClass::FireGameEvent( event );
}


bool CBaseTFAchievement::LocalPlayerCanEarn( void ) 
{ 
	// Swallow game events if we're not allowed to earn achievements, or if the local player isn't the right class
	if ( !GameRulesAllowsAchievements() )
	{
		return false;
	}

	// Determine class & check it
	if ( m_iAchievementID >= ACHIEVEMENT_START_CLASS_SPECIFIC && m_iAchievementID <= ACHIEVEMENT_END_CLASS_SPECIFIC )
	{
		int iClass = floor( (m_iAchievementID - ACHIEVEMENT_START_CLASS_SPECIFIC) / 100.0f ) + 1;
		if ( !IsLocalTFPlayerClass( iClass ) )
		{
			return false;
		}
	}

	return BaseClass::LocalPlayerCanEarn();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAchievementFullRound::Init() 
{
	m_iFlags |= ACH_FILTER_FULL_ROUND_ONLY;		
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAchievementFullRound::ListenForEvents()
{
	ListenForGameEvent( "teamplay_round_win" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAchievementFullRound::FireGameEvent_Internal( IGameEvent *event )
{
	if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			// is the player currently on a game team?
			int iTeam = pLocalPlayer->GetTeamNumber();
			if ( iTeam >= FIRST_GAME_TEAM ) 
			{
				float flRoundTime = event->GetFloat( "round_time", 0 );
				if ( flRoundTime > 0 )
				{
					Event_OnRoundComplete( flRoundTime, event );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFAchievementFullRound::PlayerWasInEntireRound( float flRoundTime )
{
	float flTeamplayStartTime = m_pAchievementMgr->GetTeamplayStartTime();
	if ( flTeamplayStartTime > 0 ) 
	{	
		// has the player been present and on a game team since the start of this round (minus a grace period)?
		if ( flTeamplayStartTime < ( gpGlobals->curtime - flRoundTime ) + TF_FULL_ROUND_GRACE_PERIOD )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: see if a round win was a win for the local player with no enemy caps
//-----------------------------------------------------------------------------
bool CheckWinNoEnemyCaps( IGameEvent *event, int iRole )
{
	if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
	{
		if ( event->GetInt( "team" ) == GetLocalPlayerTeam() )
		{
			int iLosingTeamCaps = event->GetInt( "losing_team_num_caps" );
			if ( 0 == iLosingTeamCaps )
			{
				C_TFTeam *pLocalTeam = GetGlobalTFTeam( GetLocalPlayerTeam() );
				if ( pLocalTeam )
				{
					int iRolePlayer = pLocalTeam->GetRole();
					if ( iRole > TEAM_ROLE_NONE && ( iRolePlayer != iRole ) )
						return false;
					return true;
				}
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to determine if local player is specified class
//-----------------------------------------------------------------------------
bool IsLocalTFPlayerClass( int iClass )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	return( pLocalPlayer && pLocalPlayer->IsPlayerClass( iClass ) );
}

//-----------------------------------------------------------------------------
// Purpose: Query if the gamerules allows achievement progress at this time
//-----------------------------------------------------------------------------
bool GameRulesAllowsAchievements( void )
{
	bool bRetVal = false;
	
	if ( ( TFGameRules()->State_Get() < GR_STATE_TEAM_WIN ) ||
		 ( TFGameRules()->State_Get() == GR_STATE_STALEMATE ) )
	{
		bRetVal = true;
	}

	return bRetVal;
}

//----------------------------------------------------------------------------------------------------------------
// Receive the PlayerIgnitedInv user message and send out a clientside event for achievements to hook.
USER_MESSAGE( PlayerIgnitedInv )
{
	int iPyroEntIndex = (int) msg.ReadByte();
	int iVictimEntIndex = (int) msg.ReadByte();
	int iMedicEntIndex = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_ignited_inv" );
	if ( event )
	{
		event->SetInt( "pyro_entindex", iPyroEntIndex );
		event->SetInt( "victim_entindex", iVictimEntIndex );
		event->SetInt( "medic_entindex", iMedicEntIndex );
		gameeventmanager->FireEventClientSide( event );
	}
}

// Receive the PlayerIgnited user message and send out a clientside event for achievements to hook.
USER_MESSAGE( PlayerIgnited )
{
	int iPyroEntIndex = (int) msg.ReadByte();
	int iVictimEntIndex = (int) msg.ReadByte();
	int iWeaponID = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_ignited" );
	if ( event )
	{
		event->SetInt( "pyro_entindex", iPyroEntIndex );
		event->SetInt( "victim_entindex", iVictimEntIndex );
		event->SetInt( "weaponid", iWeaponID );
		gameeventmanager->FireEventClientSide( event );
	}
}

// Receive the Damage user message and send out a clientside event for achievements to hook.
USER_MESSAGE( Damage )
{
	int iDamage = msg.ReadShort();
	int iDmgBits = msg.ReadLong();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_damaged" );
	if ( event )
	{
		event->SetInt( "amount", iDamage );
		event->SetInt( "type", iDmgBits );
		gameeventmanager->FireEventClientSide( event );
	}
	// NVNT START implementing rest of message for damage directions
	if(iDamage == 0)
		return; // no damage forces for no damage.
	
	// get the local player.
	C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
	if(!pLocal)
		return;// if we dont have a local player ignore this message.

	Vector attackerPosition(0,0,0);
	if(msg.ReadOneBit())
	{
		// if we read one bit then that means we have shooters origin.
		msg.ReadBitVec3Coord( attackerPosition );
	}else{
		// if it is non origin, just set the origin below the player
		attackerPosition = pLocal->GetAbsOrigin() + Vector(0,0,-10);
	}
	// get the direction in world
	Vector attackDirectionLocal(vec3_origin);
	// rotate the direction to the local players view
	pLocal->WorldToEntitySpace(attackerPosition, &attackDirectionLocal);
	
	if ( haptics )
	{
		Vector hapticSpace( attackDirectionLocal.y, -attackDirectionLocal.z, attackDirectionLocal.x );

		hapticSpace.NormalizeInPlace();

		haptics->ApplyDamageEffect((float)iDamage, iDmgBits, hapticSpace);
	}
	// NVNT END
}

// Receive the UpdateAchievement user message and send out a clientside event for achievements to hook.
USER_MESSAGE( UpdateAchievement )
{
	int iIndex = (int) msg.ReadShort();
	int nData = (int) msg.ReadShort();

	g_AchievementMgrTF.UpdateAchievement( iIndex, nData );
}

// Receive the PlayerJarated user message and send out a clientside event for achievements to hook.
USER_MESSAGE( PlayerJarated )
{
	int iThrowerEntIndex = (int) msg.ReadByte();
	int iVictimEntIndex = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_jarated" );
	if ( event )
	{
		event->SetInt( "thrower_entindex", iThrowerEntIndex );
		event->SetInt( "victim_entindex", iVictimEntIndex );
		gameeventmanager->FireEventClientSide( event );
	}
}

USER_MESSAGE( PlayerJaratedFade )
{
	int iThrowerEntIndex = (int) msg.ReadByte();
	int iVictimEntIndex = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_jarated_fade" );
	if ( event )
	{
		event->SetInt( "thrower_entindex", iThrowerEntIndex );
		event->SetInt( "victim_entindex", iVictimEntIndex );

		gameeventmanager->FireEventClientSide( event );
	}
}
//This is so dumb.
USER_MESSAGE( PlayerShieldBlocked )
{
	int iAttacker = (int) msg.ReadByte();
	int iBlocker = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_shield_blocked" );
	if ( event )
	{
		event->SetInt( "attacker_entindex", iAttacker );
		event->SetInt( "blocker_entindex", iBlocker );

		gameeventmanager->FireEventClientSide( event );
	}
}

// Receive the PlayerExtinguished user message and send out a clientside event for achievements to hook.
USER_MESSAGE( PlayerExtinguished )
{
	int iMedicEntIndex = (int) msg.ReadByte();
	int iVictimEntIndex = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_extinguished" );
	if ( event )
	{
		event->SetInt( "victim", iVictimEntIndex );
		event->SetInt( "healer", iMedicEntIndex );

		gameeventmanager->FireEventClientSide( event );
	}
}
#endif // CLIENT_DLL
