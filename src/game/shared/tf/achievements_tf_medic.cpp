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
#include "achievements_tf.h"


//======================================================================================================================================
// MEDIC ACHIEVEMENT PACK
//======================================================================================================================================
class CAchievementTFMedic_TopScoreboard : public CAchievementTopScoreboard
{
	DECLARE_CLASS( CAchievementTFMedic_TopScoreboard, CAchievementTopScoreboard );

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_FILTER_FULL_ROUND_ONLY | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		m_bKilledAnyone = false;
		SetGoal(1);
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "teamplay_round_active" ) == 0 )
		{
			m_bKilledAnyone = false;
		}
		else
		{
			BaseClass::FireGameEvent_Internal( event );
		}
	}

	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent *event )
	{
		if ( !m_bKilledAnyone )
		{
			BaseClass::Event_OnRoundComplete( flRoundTime, event );
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_TFPlayer::GetLocalTFPlayer() )
		{
			m_bKilledAnyone = true;
		}
	}

private:
	bool	m_bKilledAnyone;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_TopScoreboard, ACHIEVEMENT_TF_MEDIC_TOP_SCOREBOARD, "TF_MEDIC_TOP_SCOREBOARD", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_ChargeBySetupEnd : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_setup_finished" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "teamplay_setup_finished" ) == 0 )
		{
			// If we're a medic, and we have a charge, we've got the achievement.
			if ( IsLocalTFPlayerClass( TF_CLASS_MEDIC ) )
			{
				C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( pLocalPlayer->MedicGetChargeLevel() >= 1.0 )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_ChargeBySetupEnd, ACHIEVEMENT_TF_MEDIC_SETUP_CHARGE, "TF_MEDIC_SETUP_CHARGE", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_HealTargetUnderFire : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_HealTargetUnderFire, ACHIEVEMENT_TF_MEDIC_HEAL_UNDER_FIRE, "TF_MEDIC_HEAL_UNDER_FIRE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_SimultaneousCharges : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_SimultaneousCharges, ACHIEVEMENT_TF_MEDIC_SIMUL_CHARGE, "TF_MEDIC_SIMUL_CHARGE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_RapidUbercharges : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_chargedeployed" );

		// Clear times on level init
		m_vecPreviousCharges.Purge();
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "player_chargedeployed" ) == 0 )
		{
			int iMedic = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			CBaseEntity *pMedic = UTIL_PlayerByIndex( iMedic );
			if ( pMedic && pMedic == C_TFPlayer::GetLocalTFPlayer() )
			{
				m_vecPreviousCharges.AddToTail( gpGlobals->curtime );
				if ( m_vecPreviousCharges.Count() > 3 )
				{
					m_vecPreviousCharges.Remove(0);
				}

				CheckForSuccess();
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
		if ( iAssisterIndex > 0 )
		{
			CBaseEntity *pAssister = UTIL_PlayerByIndex( iAssisterIndex );
			if ( pAssister && ( pAssister == C_TFPlayer::GetLocalTFPlayer() ) )
			{
				m_vecPreviousAssists.AddToTail( gpGlobals->curtime );
				if ( m_vecPreviousAssists.Count() > 5 )
				{
					m_vecPreviousAssists.Remove(0);
				}

				CheckForSuccess();
			}
		}				
	}

	void CheckForSuccess( void )
	{
		if ( m_vecPreviousCharges.Count() >= 3 )
		{
			// Have we done the last 3 charges in 5 minutes?
			float flTimeRequirement = (5 * 60);
			if ( gpGlobals->curtime - m_vecPreviousCharges[0] < flTimeRequirement )
			{
				// Now check to make sure we've assisted in 5 kills in that time too.
				if ( m_vecPreviousAssists.Count() >= 5 )
				{
					if ( gpGlobals->curtime - m_vecPreviousAssists[0] < flTimeRequirement )
					{
						IncrementCount();
					}
				}
			}
		}
	}

private:
	CUtlVector<float> m_vecPreviousCharges;
	CUtlVector<float> m_vecPreviousAssists;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_RapidUbercharges, ACHIEVEMENT_TF_MEDIC_RAPID_CHARGE, "TF_MEDIC_RAPID_CHARGE", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_CounterUbercharges : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_flPreviousEnemyCharge = 0;
		m_hEnemyMedic = NULL;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_chargedeployed" );

		// Clear enemy medic stats on level init
		m_flPreviousEnemyCharge = 0;
		m_hEnemyMedic = NULL;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "player_chargedeployed" ) == 0 )
		{
			int iMedic = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			CBaseEntity *pMedic = UTIL_PlayerByIndex( iMedic );
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pMedic && pLocalPlayer )
			{
				// If we're the medic, get the achievement if an enemy medic in our PVS has deployed in the last 8 seconds
				if ( pMedic == C_TFPlayer::GetLocalTFPlayer() )
				{
					if ( gpGlobals->curtime - m_flPreviousEnemyCharge < 8.0 && m_hEnemyMedic && !m_hEnemyMedic->IsDormant() )
					{
						IncrementCount();
					}
				}
				else if ( pMedic->GetTeamNumber() != pLocalPlayer->GetTeamNumber() )
				{
					// Only track enemy medics deploying charge if they're in our PVS
					if ( !pMedic->IsDormant() )
					{
						m_flPreviousEnemyCharge = gpGlobals->curtime;
						m_hEnemyMedic = pMedic;
					}
				}
			}
		}
	}

private:
	float		m_flPreviousEnemyCharge;
	EHANDLE		m_hEnemyMedic;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_CounterUbercharges, ACHIEVEMENT_TF_MEDIC_COUNTER_CHARGE, "TF_MEDIC_COUNTER_CHARGE", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_SwitchToMedic : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_flPreviousMedicCall = 0;
		m_bTrackingHealth = false;
		m_iPrevHealPoints = 0;
		m_iTrackedHealPoints = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_calledformedic" );
		ListenForGameEvent( "localplayer_changeclass" );
		ListenForGameEvent( "localplayer_becameobserver" );

		// Clear data on level init
		m_flPreviousMedicCall = 0;
		m_bTrackingHealth = false;
		m_iPrevHealPoints = 0;
		m_iTrackedHealPoints = 0;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		// no progress in endround
		if ( !GameRulesAllowsAchievements() )
			return;

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return;

		if ( Q_strcmp( event->GetName(), "player_calledformedic" ) == 0 )
		{
			// Only track these if we're not a medic
			if ( !IsLocalTFPlayerClass( TF_CLASS_MEDIC ) )
			{
				int iCaller = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
				CBaseEntity *pCaller = UTIL_PlayerByIndex( iCaller );
				if ( pCaller && pCaller->InSameTeam( pLocalPlayer ) )
				{
					m_flPreviousMedicCall = gpGlobals->curtime;
				}
			}
		}
		else if ( Q_strcmp( event->GetName(), "localplayer_changeclass" ) == 0 || Q_strcmp( event->GetName(), "localplayer_becameobserver" ) == 0 )
		{
			// See if we've switched to Medic right after someone called for medic.
			// We trigger off the localplayer_becameobserver as well, to catch the case where they've suicided to change class, but don't
			// respawn within the 10 second window (because we only get localplayer_changeclass on the respawn as the new class).
			bool bWantsMedic = (IsLocalTFPlayerClass( TF_CLASS_MEDIC ) || pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() == TF_CLASS_MEDIC );

			if ( m_flPreviousMedicCall && (gpGlobals->curtime - m_flPreviousMedicCall < 10.0) && bWantsMedic )
			{
				// Ensure we're the only medic on the team
				if ( g_TF_PR )
				{
					bool bOnlyMedic = true;
					int iLocalTeam = C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber();
					for( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
					{
						if ( !g_PR->IsConnected( playerIndex ) || g_PR->IsLocalPlayer( playerIndex ) )
							continue;
						if ( g_PR->GetTeam(playerIndex) != iLocalTeam )
							continue;

						if ( g_TF_PR->GetPlayerClass( playerIndex ) == TF_CLASS_MEDIC )
						{
							bOnlyMedic = false;
							break;
						}
					}

					if ( bOnlyMedic )
					{
						m_bTrackingHealth = true;
						m_iTrackedHealPoints = 0;
						ClassStats_t &classStats = CTFStatPanel::GetClassStats( TF_CLASS_MEDIC );
						m_iPrevHealPoints = classStats.accumulated.m_iStat[TFSTAT_HEALING];
					}
				}
			}
			else if ( !bWantsMedic )
			{
				m_bTrackingHealth = false;
			}
		}
	}

	void OnPlayerStatsUpdate()
	{
		if ( !m_bTrackingHealth || !IsLocalTFPlayerClass( TF_CLASS_MEDIC ) )
			return;

		ClassStats_t &classStats = CTFStatPanel::GetClassStats( TF_CLASS_MEDIC );
		int iCount = classStats.accumulated.m_iStat[TFSTAT_HEALING];
		if ( iCount > m_iPrevHealPoints )
		{
			m_iTrackedHealPoints += (iCount - m_iPrevHealPoints);
			m_iPrevHealPoints = iCount;

			if ( m_iTrackedHealPoints > 500 )
			{
				IncrementCount();
			}
		}
	}

private:
	float		m_flPreviousMedicCall;
	bool		m_bTrackingHealth;
	int			m_iPrevHealPoints;
	int			m_iTrackedHealPoints;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_SwitchToMedic, ACHIEVEMENT_TF_MEDIC_SWITCH_TO_MEDIC, "TF_MEDIC_SWITCH_TO_MEDIC", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_SaveTeammate : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_SaveTeammate, ACHIEVEMENT_TF_MEDIC_SAVE_TEAMMATE, "TF_MEDIC_SAVE_TEAMMATE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_ChargeBlocker : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_ChargeBlocker, ACHIEVEMENT_TF_MEDIC_CHARGE_BLOCKER, "TF_MEDIC_CHARGE_BLOCKER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_AssistMedic : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS | ACH_FILTER_VICTIM_IS_PLAYER_ENEMY );
		SetGoal( 1 );
		m_iAssists = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "localplayer_respawn" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "localplayer_respawn" ) )
		{
			m_iAssists = 0;
		}
	}

	// Assist a fellow Medic in killing 5 enemies in a single life.

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
			if ( iAssisterIndex > 0 )
			{
				C_TFPlayer *pAtkPlayer = ToTFPlayer( pAttacker );
				C_TFPlayer *pAssister = ToTFPlayer( UTIL_PlayerByIndex( iAssisterIndex ) );
				if ( pAssister == pLocalPlayer || pAttacker == pLocalPlayer )
				{
					if ( pAssister && pAssister->IsPlayerClass(TF_CLASS_MEDIC) && pAtkPlayer && pAtkPlayer->IsPlayerClass(TF_CLASS_MEDIC) )
					{
						m_iAssists++;

						if ( m_iAssists >= 3 )
						{
							IncrementCount();
						}
					}
				}
			}
		}
	}

private:
	int		m_iAssists;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_AssistMedic, ACHIEVEMENT_TF_MEDIC_ASSIST_MEDIC, "TF_MEDIC_ASSIST_MEDIC", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_KillScoutsWithSyringe : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 50 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pAttacker == pLocalPlayer )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsPlayerClass(TF_CLASS_SCOUT) && event->GetInt( "weaponid" ) == TF_WEAPON_SYRINGEGUN_MEDIC )
			{
				IncrementCount();
			}
		}
	}

private:
	int		m_iAssists;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_KillScoutsWithSyringe, ACHIEVEMENT_TF_MEDIC_SYRINGE_SCOUTS, "TF_MEDIC_SYRINGE_SCOUTS", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_KillMedicsWithBonesaw : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 10 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pAttacker == pLocalPlayer )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsPlayerClass(TF_CLASS_MEDIC) && event->GetInt( "weaponid" ) == TF_WEAPON_BONESAW )
			{
				IncrementCount();
			}
		}
	}

private:
	int		m_iAssists;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_KillMedicsWithBonesaw, ACHIEVEMENT_TF_MEDIC_BONESAW_MEDICS, "TF_MEDIC_BONESAW_MEDICS", 1 );

//----------------------------------------------------------------------------------------------------------------
#define MAX_PARTNERS 12
class CAchievementTFMedic_AssistHeavyLongStreak : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) )
		{
			m_Partners.Purge();
		}
		else if ( FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_Partners.Purge();
		}
	}

	int	GetPartnerIndex( CBaseEntity *pPlayer )
	{
		for ( int i = 0; i < m_Partners.Count(); i++ )
		{
			if ( m_Partners[i].hPartner == pPlayer )
				return i;
		}
		return -1;
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			if ( pVictim->GetTeamNumber() != pLocalPlayer->GetTeamNumber() )
			{
				int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
				if ( iAssisterIndex > 0 )
				{
					if ( UTIL_PlayerByIndex( iAssisterIndex ) == pLocalPlayer )
					{
						C_TFPlayer *pAtkPlayer = ToTFPlayer( pAttacker );
						if ( pAtkPlayer && pAtkPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
						{
							int index = GetPartnerIndex( pAtkPlayer );
							if ( index == -1 )
							{
								if ( m_Partners.Count() >= MAX_PARTNERS )
								{
									// Remove the one with the least assists
									int iLowest = 999;
									int iLowestIndex = -1;
									for ( int i = 0; i < m_Partners.Count(); i++ )
									{
										if ( !m_Partners[i].hPartner )
										{
											// Player is gone. Lets remove that one.
											iLowestIndex = i;
											break;
										}

										if ( m_Partners[i].iAssists < iLowest )
										{
											iLowestIndex = i;
											iLowest = m_Partners[i].iAssists;
										}
									}

									if ( iLowestIndex >= 0 )
									{
										//Msg("FULL Removed %d (%s)\n", iLowestIndex, g_PR->GetPlayerName(m_Partners[iLowestIndex].hPartner->entindex()) );
										m_Partners.Remove(iLowestIndex);
									}
								}

								int iNewIndex = m_Partners.AddToTail();
								m_Partners[iNewIndex].hPartner = pAtkPlayer;
								m_Partners[iNewIndex].iAssists = 1;

								//Msg("Inserted %s into %d\n", g_PR->GetPlayerName(pAtkPlayer->entindex()), iNewIndex );
							}
							else
							{
								m_Partners[index].iAssists++;

								//Msg("Incremented %s in %d to %d\n", g_PR->GetPlayerName(m_Partners[index].hPartner->entindex()), index, m_Partners[index].iAssists );

								if ( m_Partners[index].iAssists >= 10 )
								{
									IncrementCount();
								}
							}
						}
						else
						{
							// Ensure this guy isn't in our list. We can have non-heavies in our list if we
							// earn an assist with them, and then they switch classes in the respawn room.
							int index = GetPartnerIndex( pAtkPlayer );
							if ( index != -1 )
							{
								m_Partners.Remove(index);
							}
						}
					}
				}
			}

			// See if it's one of our partners
			int index = GetPartnerIndex( pVictim );
			if ( index != -1 )
			{
				//Msg("DEATH: Removed %d (%s)\n", index, g_PR->GetPlayerName(m_Partners[index].hPartner->entindex()) );
				m_Partners.Remove(index);
			}

			/*
			Msg("State:\n");
			for ( int i = 0; i < m_Partners.Count(); i++ )
			{
				if ( m_Partners[i].hPartner )
				{
					Msg("   %d: %s with %d\n", i, g_PR->GetPlayerName(m_Partners[i].hPartner->entindex()), m_Partners[i].iAssists );
				}
				else
				{
					Msg("   %d: EMPTY\n", i );
				}
			}
			*/
		}
	}

private:
	struct partners_t
	{
		EHANDLE hPartner;
		int		iAssists;
	};

	CUtlVector<partners_t>	m_Partners;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_AssistHeavyLongStreak, ACHIEVEMENT_TF_MEDIC_ASSIST_HEAVY_LONG, "TF_MEDIC_ASSIST_HEAVY_LONG", 1 );

//----------------------------------------------------------------------------------------------------------------
// Base helper for achievements that want to track assists while the medic is deploying his uber charge
class CAchievementTFMedic_BaseAssistWhileCharged : public CBaseTFAchievement
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_chargedeployed" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "player_chargedeployed" ) == 0 )
		{
			int iMedic = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			CBaseEntity *pMedic = UTIL_PlayerByIndex( iMedic );
			if ( pMedic == C_TFPlayer::GetLocalTFPlayer() )
			{
				DeployedCharge();
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
		if ( iAssisterIndex > 0 )
		{
			CBaseEntity *pAssister = UTIL_PlayerByIndex( iAssisterIndex );
			C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pAssister == pTFPlayer && pTFPlayer && pTFPlayer->MedicIsReleasingCharge() )
			{
				AssistedWhileCharged( pVictim, pAttacker, pInflictor, event );
			}
		}				
	}

	virtual void DeployedCharge( void ) = 0;
	virtual void AssistedWhileCharged( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) = 0;
};

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_AssistChargeScout : public CAchievementTFMedic_BaseAssistWhileCharged
{
	DECLARE_CLASS( CAchievementTFMedic_AssistChargeScout, CAchievementTFMedic_BaseAssistWhileCharged );

	void Init() 
	{
		BaseClass::Init();
		m_iAssists = 0;
	}

	// Assist in killing 4 enemies with a single Uber-Charge on a Scout

	virtual void DeployedCharge( void )
	{
		m_iAssists = 0;
	}

	virtual void AssistedWhileCharged( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		C_TFPlayer *pAtkPlayer = ToTFPlayer( pAttacker );
		if ( pAtkPlayer && pAtkPlayer->IsPlayerClass(TF_CLASS_SCOUT) )
		{
			m_iAssists++;
			if ( m_iAssists >= 3 )
			{
				IncrementCount();
			}
		}
	}

private:
	int		m_iAssists;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_AssistChargeScout, ACHIEVEMENT_TF_MEDIC_ASSIST_SCOUT, "TF_MEDIC_ASSIST_SCOUT", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_AssistChargeSoldier : public CAchievementTFMedic_BaseAssistWhileCharged
{
	DECLARE_CLASS( CAchievementTFMedic_AssistChargeSoldier, CAchievementTFMedic_BaseAssistWhileCharged );

	void Init() 
	{
		BaseClass::Init();
		m_iAssists = 0;
	}

	// Assist in exploding 5 enemies with a single Uber-Charge on a Soldier

	virtual void DeployedCharge( void )
	{
		m_iAssists = 0;
	}

	virtual void AssistedWhileCharged( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		C_TFPlayer *pAtkPlayer = ToTFPlayer( pAttacker );
		if ( pAtkPlayer && pAtkPlayer->IsPlayerClass(TF_CLASS_SOLDIER) )
		{
			if ( event->GetInt( "weaponid" ) == TF_WEAPON_ROCKETLAUNCHER )
			{
				m_iAssists++;
				if ( m_iAssists >= 5 )
				{
					IncrementCount();
				}
			}
		}
	}

private:
	int		m_iAssists;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_AssistChargeSoldier, ACHIEVEMENT_TF_MEDIC_ASSIST_SOLDIER, "TF_MEDIC_ASSIST_SOLDIER", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_AssistChargeDemoman : public CAchievementTFMedic_BaseAssistWhileCharged
{
	DECLARE_CLASS( CAchievementTFMedic_AssistChargeDemoman, CAchievementTFMedic_BaseAssistWhileCharged );

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_iAssists = 0;
	}

	virtual void ListenForEvents()
	{
		BaseClass::ListenForEvents();
		ListenForGameEvent( "object_destroyed" );
	}

	virtual void DeployedCharge( void )
	{
		m_iAssists = 0;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "object_destroyed" ) == 0 )
		{
			int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
			if ( iAssisterIndex > 0 )
			{
				CBaseEntity *pAssister = UTIL_PlayerByIndex( iAssisterIndex );
				C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( pAssister == pTFPlayer && pTFPlayer && pTFPlayer->MedicIsReleasingCharge() )
				{
					CBaseEntity *pAttacker = ClientEntityList().GetEnt( engine->GetPlayerForUserID( event->GetInt("attacker") ) );
					AssistedWhileCharged( NULL, pAttacker, NULL, event );
				}
			}
		}
		else
		{
			BaseClass::FireGameEvent_Internal( event );
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// don't count player kills in our assists
	}

	virtual void AssistedWhileCharged( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		C_TFPlayer *pAtkPlayer = ToTFPlayer( pAttacker );
		if ( pAtkPlayer && pAtkPlayer->IsPlayerClass(TF_CLASS_DEMOMAN) )
		{
			m_iAssists++;
			if ( m_iAssists >= 5 )
			{
				IncrementCount();
			}
		}
	}

private:
	int		m_iAssists;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_AssistChargeDemoman, ACHIEVEMENT_TF_MEDIC_ASSIST_DEMOMAN, "TF_MEDIC_ASSIST_DEMOMAN", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_HealEngineer : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_HealEngineer, ACHIEVEMENT_TF_MEDIC_HEAL_ENGINEER, "TF_MEDIC_HEAL_ENGINEER", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_AssistPyro : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_iPlayersIgnited = 0;
	}

	// Assist in burning 8 enemies with a single Uber-Charge on a Pyro. 

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_chargedeployed" );
		ListenForGameEvent( "player_ignited_inv" );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( !IsLocalTFPlayerClass( TF_CLASS_MEDIC ) )
			return;

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( FStrEq( event->GetName(), "player_chargedeployed" ) )
		{
			int iMedic = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			CBaseEntity *pMedic = UTIL_PlayerByIndex( iMedic );
			if ( pMedic == pLocalPlayer )
			{
				m_iPlayersIgnited = 0;
			}
		}
		else if ( FStrEq( event->GetName(), "player_ignited_inv" ) )
		{
			int iMedic = event->GetInt( "medic_entindex" );
			CBaseEntity *pMedic = UTIL_PlayerByIndex( iMedic );
			if ( pMedic == pLocalPlayer )
			{
				if ( ++m_iPlayersIgnited >= 5 )
				{
					IncrementCount(); 
				}
			}
		}
	}

private:
	int		m_iPlayersIgnited;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_AssistPyro, ACHIEVEMENT_TF_MEDIC_ASSIST_PYRO, "TF_MEDIC_ASSIST_PYRO", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_AssistHeavy : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary

private:

};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_AssistHeavy, ACHIEVEMENT_TF_MEDIC_ASSIST_HEAVY, "TF_MEDIC_ASSIST_HEAVY", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_AssistCapturer : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Assist in killing 3 enemies on an enemy control point, in a single life.
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_AssistCapturer, ACHIEVEMENT_TF_MEDIC_ASSIST_CAPTURER, "TF_MEDIC_ASSIST_CAPTURER", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_HealCallers : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 200 );

		m_flLastHealTime = -1;
		m_iLastHealee = -1;
	}

	// Heal 200 teammates after they've called for "Medic!". 

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_healedmediccall" );

		// Clear data on level init
		m_flLastHealTime = -1;
		m_iLastHealee = -1;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return;

		if ( Q_strcmp( event->GetName(), "player_healedmediccall" ) == 0 )
		{
			int iCaller = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			CBaseEntity *pCaller = UTIL_PlayerByIndex( iCaller );
			if ( pCaller && pCaller->InSameTeam( pLocalPlayer ) )
			{
				// don't count multiple heals on the same player unless they retrigger the saveme
				if ( iCaller == m_iLastHealee )
				{
					if ( gpGlobals->curtime - m_flLastHealTime < 10.0 )
						return;
				}

				IncrementCount();

				m_iLastHealee = iCaller;
				m_flLastHealTime = gpGlobals->curtime;
			}
		}
	}

private:
	float m_iLastHealee;
	float m_flLastHealTime;

};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_HealCallers, ACHIEVEMENT_TF_MEDIC_HEAL_CALLERS, "TF_MEDIC_HEAL_CALLERS", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_ExtinguishTeammates : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 100 );
	}

	// Extinguish 100 burning teammates.

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_extinguished" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_extinguished" ) )
		{
			CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( !pLocalPlayer )
				return;

			// userid of player that was extinguished by a medic
			// if local player is healing them, we get credit

			CTFPlayer *pTarget = ToTFPlayer( UTIL_PlayerByIndex( event->GetInt( "victim" ) ) );

			if ( pTarget && pTarget == pLocalPlayer->MedicGetHealTarget() )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_ExtinguishTeammates, ACHIEVEMENT_TF_MEDIC_EXTINGUISH_TEAMMATES, "TF_MEDIC_EXTINGUISH_TEAMMATES", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_AssistVsNemeses : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 20 );
	}

	// Assist in killing 20 nemeses. 

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return;

		// We count 2 ways to get this.
		// 1) Medic is healing someone who gets revenge - "revenge" is valid
		// 2) Medic heals a guy who kills the medic's nemesis - "assister_revenge" is valid
		// In both cases, "assister" is the medic.

		int iAssisterID = event->GetInt( "assister" );

		if ( iAssisterID > 0 && iAssisterID == pLocalPlayer->GetUserID() )
		{
			if ( event->GetInt( "death_flags" ) & TF_DEATH_REVENGE || event->GetInt( "death_flags" ) & TF_DEATH_ASSISTER_REVENGE )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_AssistVsNemeses, ACHIEVEMENT_TF_MEDIC_ASSIST_VS_NEMESES, "TF_MEDIC_ASSIST_VS_NEMESES", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_KillWhileCharged : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );

		m_bPlayerHasCharge = false;
		m_iKillCount = 0;
	}

	// Kill 5 enemies in a single life, while having your Uber-Charge ready, but undeployed. 

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "localplayer_chargeready" );
		ListenForGameEvent( "player_chargedeployed" );
		ListenForGameEvent( "localplayer_respawn" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		// sent from client, assumes that player with charge is the local player.
		if ( FStrEq( pszEventName, "localplayer_chargeready" ) )
		{
			m_bPlayerHasCharge = true;
			m_iKillCount = 0;
		}
		else if ( FStrEq( pszEventName, "player_chargedeployed" ) )
		{
			int iMedic = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			if ( UTIL_PlayerByIndex( iMedic ) == C_TFPlayer::GetLocalTFPlayer() )
			{
				m_bPlayerHasCharge = false;
				m_iKillCount = 0;
			}
		}
		else if ( FStrEq( pszEventName, "localplayer_respawn" ) )
		{
			m_iKillCount = 0;
			m_bPlayerHasCharge = false;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		Assert( pAttacker == C_TFPlayer::GetLocalTFPlayer() );

		if ( m_bPlayerHasCharge )
		{
			m_iKillCount++;

			if ( m_iKillCount >= 2 )
			{
				IncrementCount();
			}
		}
		else
		{
			m_iKillCount = 0;
		}
	}

private:
	int m_iKillCount;
	bool m_bPlayerHasCharge;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_KillWhileCharged, ACHIEVEMENT_TF_MEDIC_KILL_WHILE_CHARGED, "TF_MEDIC_KILL_WHILE_CHARGED", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_BonesawNoMisses : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
	// Hit enemies with your bonesaw 5 times in a row without dying or missing. 
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_BonesawNoMisses, ACHIEVEMENT_TF_MEDIC_BONESAW_NOMISSES, "TF_MEDIC_BONESAW_NOMISSES", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_HealLarge : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 7000 );
	}

	// Accumulate 7000 heal points health in a single life. 

	void OnPlayerStatsUpdate()
	{
		ClassStats_t &classStats = CTFStatPanel::GetClassStats( TF_CLASS_MEDIC );

		int iOldCount = m_iCount;
		m_iCount = classStats.max.m_iStat[TFSTAT_HEALING];

		if ( m_iCount != iOldCount )
		{
			m_pAchievementMgr->SetDirty( true );
		}

		if ( IsLocalTFPlayerClass( TF_CLASS_MEDIC ) )
		{
			EvaluateNewAchievement();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_HealLarge, ACHIEVEMENT_TF_MEDIC_HEAL_LARGE, "TF_MEDIC_HEAL_LARGE", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_HealHuge : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10000 );
	}

	// Accumulate 10000 heal points health in a single life. 

	void OnPlayerStatsUpdate()
	{
		ClassStats_t &classStats = CTFStatPanel::GetClassStats( TF_CLASS_MEDIC );

		int iOldCount = m_iCount;
		m_iCount = classStats.max.m_iStat[TFSTAT_HEALING];

		if ( m_iCount != iOldCount )
		{
			m_pAchievementMgr->SetDirty( true );
		}

		if ( IsLocalTFPlayerClass( TF_CLASS_MEDIC ) )
		{
			EvaluateNewAchievement();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_HealHuge, ACHIEVEMENT_TF_MEDIC_HEAL_HUGE, "TF_MEDIC_HEAL_HUGE", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_HealGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1000000 );
	}

	// Accumulate 1 million total heal points. 

	void OnPlayerStatsUpdate()
	{
		ClassStats_t &classStats = CTFStatPanel::GetClassStats( TF_CLASS_MEDIC );
		int iOldCount = m_iCount;
		m_iCount = classStats.accumulated.m_iStat[TFSTAT_HEALING];
		if ( m_iCount != iOldCount )
		{
			m_pAchievementMgr->SetDirty( true );
		}

		if ( IsLocalTFPlayerClass( TF_CLASS_MEDIC ) )
		{
			EvaluateNewAchievement();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_HealGrind, ACHIEVEMENT_TF_MEDIC_HEAL_GRIND, "TF_MEDIC_HEAL_GRIND", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_KillHealedSpy : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_KillHealedSpy, ACHIEVEMENT_TF_MEDIC_KILL_HEALED_SPY, "TF_MEDIC_KILL_HEALED_SPY", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_SaveFallingTeammate : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_SaveFallingTeammate, ACHIEVEMENT_TF_MEDIC_SAVE_FALLING_TEAMMATE, "TF_MEDIC_SAVE_FALLING_TEAMMATE", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_ChargeJuggle : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		ResetJuggle();
	}

	~CAchievementTFMedic_ChargeJuggle()
	{
		ResetJuggle();
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_invulned" );

		// Clear data on level init
		ResetJuggle();
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_invulned" ) )
		{
			C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if ( !pLocalPlayer )
				return;

			// Ignore local player getting invuln
			int iUserID = event->GetInt( "userid" );
			if ( iUserID == pLocalPlayer->GetUserID() )
				return;

			if ( event->GetInt( "medic_userid" ) == pLocalPlayer->GetUserID() && iUserID > 0 )
			{
				int index = m_InvulnHistory.AddToHead( new target_history_t );
				m_InvulnHistory[index]->m_iUserID = iUserID;
				m_InvulnHistory[index]->m_flTime = gpGlobals->curtime;

				EvaluateInvulnJuggle();
			}
		}
	}


	void EvaluateInvulnJuggle( void )
	{
		// first clear and entries that are older than max time
		float flTimeCutoff = 5.0f;

		int iCount = m_InvulnHistory.Count();

		for ( int i=iCount-1;i>= 0;i-- )
		{
			if ( ( gpGlobals->curtime - m_InvulnHistory.Element(i)->m_flTime ) > flTimeCutoff )
			{
				m_InvulnHistory.Remove( i );
			}
		}

		// If 1 or 0 targets, reset the count, we wont get a match later.
		if ( m_InvulnHistory.Count() < 2 )
		{
			m_iNumJuggles = 0;
			return;
		}

		// the first item is the guy we just invulned
		// see if the second guy still has TF_COND_INVULNERABLE_WEARINGOFF

		target_history_t *first = m_InvulnHistory.Element(0);		
		target_history_t *second = m_InvulnHistory.Element(1);

		if ( !first || !second )
		{
			ResetJuggle();
			return;
		}

		if ( first->m_iUserID == second->m_iUserID )
		{
			ResetJuggle();
			return;
		}

		// so we have two different players now.
		// make sure second has TF_COND_INVULNERABLE_WEARINGOFF
		int iTargetIndex = engine->GetPlayerForUserID( second->m_iUserID );
		if ( iTargetIndex > 0 )
		{
			C_TFPlayer *pTarget = ToTFPlayer( UTIL_PlayerByIndex( iTargetIndex ) );
			if ( pTarget )
			{
				if ( pTarget->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) || pTarget->m_Shared.InCond( TF_COND_INVULNERABLE ) )
				{	
					// assume it was us that gave him the invuln
					m_iNumJuggles++;
				}
				else
				{
					ResetJuggle();
				}
			}
		}

		if ( m_iNumJuggles >= 4 )
		{
			IncrementCount();
		}
	}

	void ResetJuggle( void )
	{
		m_iNumJuggles = 0;
		m_InvulnHistory.PurgeAndDeleteElements();
	}

private:
	int m_iNumJuggles;

	typedef struct
	{
		int m_iUserID;
		float m_flTime;
	} target_history_t;

	// array of userid's/times that we have recently invulned
	CUtlVector< target_history_t *> m_InvulnHistory;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_ChargeJuggle, ACHIEVEMENT_TF_MEDIC_CHARGE_JUGGLE, "TF_MEDIC_CHARGE_JUGGLE", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_FreezecamRagdoll : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_FreezecamRagdoll, ACHIEVEMENT_TF_MEDIC_FREEZECAM_RAGDOLL, "TF_MEDIC_FREEZECAM_RAGDOLL", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_BonesawSpyCallers : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	// Use your bonesaw to kill an enemy spy who has been calling for "Medic!". 

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		Assert( pAttacker == C_TFPlayer::GetLocalTFPlayer() );

		if ( event->GetInt( "weaponid" ) != TF_WEAPON_BONESAW )
			return;

		// victim is a spy and has called for medic recently
		// we will only have the saveme counter if we were able to see the
		// saveme effect, so he must have been disguised at that point

		C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );

		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SPY ) )
		{
			if ( pTFVictim->m_flSaveMeExpireTime > gpGlobals->curtime )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_BonesawSpyCallers, ACHIEVEMENT_TF_MEDIC_BONESAW_SPY_CALLERS, "TF_MEDIC_BONESAW_SPY_CALLERS", 5 );


//----------------------------------------------------------------------------------------------------------------

ConVar tf_chargedfriends( "tf_chargedfriends", "", FCVAR_ARCHIVE );

class CAchievementTFMedic_ChargeFriends : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 5 );
	}

	// Uber-charge ten of your Steam Community Friends

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_invulned" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_invulned" ) )
		{
			C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if ( !pLocalPlayer )
				return;

			int iTargetIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );

			if ( event->GetInt( "medic_userid" ) == pLocalPlayer->GetUserID() && iTargetIndex > 0 )
			{
				CBasePlayer *pTarget = UTIL_PlayerByIndex( iTargetIndex );

				if ( !steamapicontext->SteamFriends() || !steamapicontext->SteamUtils() || !g_pGameRules->IsMultiplayer() )
					return;

				player_info_t pi;
				if ( !engine->GetPlayerInfo( pTarget->entindex(), &pi ) )
					return;

				if ( !pi.friendsID )
					return;

				// check and see if they're on the local player's friends list
				CSteamID steamID( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );
				if ( steamapicontext->SteamFriends()->HasFriend( steamID, k_EFriendFlagImmediate ) )
				{	
					// get the friendsID
					// 

					//  parse tf_chargedfriends into an array

					CUtlStringList vecChargedFriends;
					V_SplitString( tf_chargedfriends.GetString(), ":", vecChargedFriends );

					char szTargetFriendsID[16];
					Q_snprintf( szTargetFriendsID, sizeof(szTargetFriendsID), "%d", pi.friendsID );

					bool bFound = false;

					for ( int i=0;i<vecChargedFriends.Count();i++ )
					{
						if ( (uint32)atoi(vecChargedFriends[i]) == pi.friendsID )
						{
							bFound = true;
							break;
						}
					}

					if ( !bFound )
					{
						// If this would get us the achievement, validate the friends ids that we have in our list
						if ( ( vecChargedFriends.Count() + 1 ) > m_iCount )
						{
							EUniverse universe = GetUniverse();

							// validate the friends
							for ( int i=vecChargedFriends.Count()-1;i>=0;i-- )
							{
								uint32 iFriendID = (uint32)atoi(vecChargedFriends[i]);

								CSteamID steamIDFriend( iFriendID, 1, universe, k_EAccountTypeIndividual );
								if ( !steamapicontext->SteamFriends()->HasFriend( steamIDFriend, k_EFriendFlagImmediate ) )
								{
									// remove this person, not a friend anymore, or trying to cheat
									vecChargedFriends.Remove(i);
								}
							}
						}

						// If we still have more valid targets, increment for real
						if ( ( vecChargedFriends.Count() + 1 ) > m_iCount )
						{
							IncrementCount();
						}

						// write friends back to the convar
						char buf[512];	// what is max size of 10 * steam ids?
						Q_snprintf( buf, sizeof(buf), "%d", pi.friendsID );

						for ( int i=0;i<vecChargedFriends.Count();i++ )
						{
							Q_strncat( buf, VarArgs( ":%d", (uint32)atoi(vecChargedFriends[i]) ), sizeof(buf), COPY_ALL_CHARACTERS );
						}			

						tf_chargedfriends.SetValue( buf );
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_ChargeFriends, ACHIEVEMENT_TF_MEDIC_CHARGE_FRIENDS, "TF_MEDIC_CHARGE_FRIENDS", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_InviteJoinCharge : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Join a game that one of your friends is in and then deploy an Uber-Charge on him.

	// when we first select a class, it has to be medic. Then we make a list of all our connected friends
	// the first 

	// called every changelevel - doh
	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "localplayer_changeclass" );
		ListenForGameEvent( "player_invulned" );

		m_iConnectedFriends.Purge();
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return;

		const char *pszName = event->GetName();

		if ( FStrEq( pszName, "localplayer_changeclass" ) )
		{
			// if the server has been on this map for less than some amount of time
			// then this is probably a changelevel, and doesn't count.
			if ( gpGlobals->curtime < ( 4 * 60 ) )
			{
				StopListeningForAllEvents();
				return;
			}

			// if this is not medic, stop listening
			bool bWantsMedic = (IsLocalTFPlayerClass( TF_CLASS_MEDIC ) ||
				pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() == TF_CLASS_MEDIC );

			if ( bWantsMedic )
			{
				CountConnectedFriends();
			}
			else
			{
				StopListeningForAllEvents();
			}
		}
		else if ( FStrEq( pszName, "player_invulned" ) )
		{
			if ( event->GetInt( "medic_userid" ) == pLocalPlayer->GetUserID() )
			{
				// is this person a friend who was on when we joined?

				int iTargetIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );

				if ( IsInvulnTargetInFriendsList(iTargetIndex) )
				{
					IncrementCount();
				}
				else
				{
					StopListeningForAllEvents();
				}
			}
		}
	}

	void CountConnectedFriends( void )
	{
		if ( !g_PR )
			return;

		Assert( g_pGameRules->IsMultiplayer() );

		// determine local player team
		int iLocalPlayerIndex =  GetLocalPlayerIndex();

		if ( !steamapicontext->SteamFriends() || !steamapicontext->SteamUtils() || !g_pGameRules->IsMultiplayer() )
			return;

		// store connected friends' steam ids
		for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			if( ( iPlayerIndex != iLocalPlayerIndex ) && ( g_PR->IsConnected( iPlayerIndex ) ) )
			{
				player_info_t pi;
				if ( !engine->GetPlayerInfo( iPlayerIndex, &pi ) )
					continue;

				if ( !pi.friendsID )
					continue;

				// check and see if they're on the local player's friends list
				CSteamID steamID( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );
				if ( !steamapicontext->SteamFriends()->HasFriend( steamID, k_EFriendFlagImmediate ) )
					continue;

				m_iConnectedFriends.AddToTail( steamID );
			}
		}
	}

	bool IsInvulnTargetInFriendsList( int iTargetIndex )
	{
		player_info_t pi;
		if ( !engine->GetPlayerInfo( iTargetIndex, &pi ) )
			return false;

		if ( !pi.friendsID )
			return false;

		if ( !steamapicontext->SteamUtils() )
			return false;

		CSteamID steamID( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );

		return ( m_iConnectedFriends.Find( steamID ) != m_iConnectedFriends.InvalidIndex() );
	}

private:
	CUtlVector< CSteamID >	m_iConnectedFriends;
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_InviteJoinCharge, ACHIEVEMENT_TF_MEDIC_INVITE_JOIN_CHARGE, "TF_MEDIC_INVITE_JOIN_CHARGE", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_HealAchiever : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Be healing a teammate as he achieves an achievement of his own. 

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "achievement_earned" );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return;

		const char *pszName = event->GetName();

		if ( FStrEq( pszName, "achievement_earned" ) )
		{
			int iPlayerIndex = event->GetInt( "player" );

			C_BasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerIndex );

			if ( pPlayer && !pPlayer->IsDormant() )
			{
				// if he is our heal target
				if ( pLocalPlayer->MedicGetHealTarget() == pPlayer )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_HealAchiever, ACHIEVEMENT_TF_MEDIC_HEAL_ACHIEVER, "TF_MEDIC_HEAL_ACHIEVER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_AchieveProgress1 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFMedic_AchieveProgress1, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 10, ACHIEVEMENT_TF_MEDIC_START_RANGE, ACHIEVEMENT_TF_MEDIC_END_RANGE );
	}

	// Earn 10 of the 36 Medic Pack Achievements ( 3 achievement count achievements not included )
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_AchieveProgress1, ACHIEVEMENT_TF_MEDIC_ACHIEVE_PROGRESS1, "TF_MEDIC_ACHIEVE_PROGRESS1", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_AchieveProgress2 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFMedic_AchieveProgress2, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 16, ACHIEVEMENT_TF_MEDIC_START_RANGE, ACHIEVEMENT_TF_MEDIC_END_RANGE );
	}

	// Earn 15 of the 36 Medic Pack Achievements ( 3 achievement count achievements not included )
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_AchieveProgress2, ACHIEVEMENT_TF_MEDIC_ACHIEVE_PROGRESS2, "TF_MEDIC_ACHIEVE_PROGRESS2", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFMedic_AchieveProgress3 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFMedic_AchieveProgress3, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 22, ACHIEVEMENT_TF_MEDIC_START_RANGE, ACHIEVEMENT_TF_MEDIC_END_RANGE );
	}

	// Earn 20 of the 36 Medic Pack Achievements ( 3 achievement count achievements not included )
};
DECLARE_ACHIEVEMENT( CAchievementTFMedic_AchieveProgress3, ACHIEVEMENT_TF_MEDIC_ACHIEVE_PROGRESS3, "TF_MEDIC_ACHIEVE_PROGRESS3", 5 );

#endif // CLIENT_DLL
