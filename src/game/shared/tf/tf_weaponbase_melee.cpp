//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weaponbase_melee.h"
#include "effect_dispatch_data.h"
#include "tf_gamerules.h"

// Server specific.
#if !defined( CLIENT_DLL )
#include "tf_player.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#include "tf_passtime_logic.h"
// Client specific.
#else
#include "c_tf_gamestats.h"
#include "c_tf_player.h"
// NVNT haptics system interface
#include "haptics/ihaptics.h"
#endif

ConVar tf_weapon_criticals_melee( "tf_weapon_criticals_melee", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Controls random crits for melee weapons. 0 - Melee weapons do not randomly crit. 1 - Melee weapons can randomly crit only if tf_weapon_criticals is also enabled. 2 - Melee weapons can always randomly crit regardless of the tf_weapon_criticals setting." );

//=============================================================================
//
// TFWeaponBase Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseMelee, DT_TFWeaponBaseMelee )

BEGIN_NETWORK_TABLE( CTFWeaponBaseMelee, DT_TFWeaponBaseMelee )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBaseMelee )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weaponbase_melee, CTFWeaponBaseMelee );

// Server specific.
#if !defined( CLIENT_DLL ) 
BEGIN_DATADESC( CTFWeaponBaseMelee )
DEFINE_THINKFUNC( Smack )
END_DATADESC()
#endif

#ifndef CLIENT_DLL
ConVar tf_meleeattackforcescale( "tf_meleeattackforcescale", "80.0", FCVAR_CHEAT | FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY );
#endif

#ifdef _DEBUG
extern ConVar tf_weapon_criticals_force_random;
#endif // _DEBUG

//=============================================================================
//
// TFWeaponBase Melee functions.
//

// -----------------------------------------------------------------------------
// Purpose: Constructor.
// -----------------------------------------------------------------------------
CTFWeaponBaseMelee::CTFWeaponBaseMelee()
{
	WeaponReset();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_flSmackTime = -1.0f;
	m_bConnected = false;
	m_bMiniCrit = false;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CanHolster( void ) const
{
	// For fist users, energy buffs come from steak sandviches which lock us into attacking with melee.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_CANNOT_SWITCH_FROM_MELEE ) )
		return false;

	return BaseClass::CanHolster();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Precache()
{
	BaseClass::Precache();

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		char szMeleeSoundStr[128] = "MVM_";
		const char *shootsound = GetShootSound( MELEE_HIT );
		if ( shootsound && shootsound[0] )
		{
			V_strcat(szMeleeSoundStr, shootsound, sizeof( szMeleeSoundStr ));
			CBaseEntity::PrecacheScriptSound( szMeleeSoundStr );
		}
	}
	CBaseEntity::PrecacheScriptSound("MVM_Weapon_Default.HitFlesh");
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Spawn()
{
	Precache();

	// Get the weapon information.
	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( GetClassname() );
	Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
	CTFWeaponInfo *pWeaponInfo = dynamic_cast< CTFWeaponInfo* >( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	Assert( pWeaponInfo && "Failed to get CTFWeaponInfo in melee weapon spawn" );
	m_pWeaponInfo = pWeaponInfo;
	Assert( m_pWeaponInfo );

	// No ammo.
	m_iClip1 = -1;

	BaseClass::Spawn();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flSmackTime = -1.0f;
	if ( GetPlayerOwner() )
	{
		GetPlayerOwner()->m_flNextAttack = gpGlobals->curtime + 0.5;
	}

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		pPlayer->m_Shared.SetNextMeleeCrit( MELEE_NOCRIT );
	
		int iSelfMark = 0;
		CALL_ATTRIB_HOOK_INT( iSelfMark, self_mark_for_death );
		if ( iSelfMark )
		{
			pPlayer->m_Shared.AddCond( TF_COND_MARKEDFORDEATH_SILENT, iSelfMark );
		}
	}

	return BaseClass::Holster( pSwitchingTo );
}

int	CTFWeaponBaseMelee::GetSwingRange( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && pOwner->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		return 128;
	}
	else
	{
		int iIsSword = 0;
		CALL_ATTRIB_HOOK_INT( iIsSword, is_a_sword )
		if ( iIsSword )
		{
			return 72; // swords are typically 72
		}
		return 48;
	}
}


// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::PrimaryAttack()
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_bConnected = false;

	pPlayer->EndClassSpecialSkill();

	// Swing the weapon.
	Swing( pPlayer );

	m_bCurrentAttackIsDuringDemoCharge = pPlayer->m_Shared.GetNextMeleeCrit() != MELEE_NOCRIT;

	if ( pPlayer->m_Shared.GetNextMeleeCrit() == MELEE_MINICRIT )
	{
		m_bMiniCrit = true;
	}
	else
	{
		m_bMiniCrit = false;
	}


#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );

	if ( pPlayer->m_Shared.IsStealthed() && ShouldRemoveInvisibilityOnPrimaryAttack() )
	{
		pPlayer->RemoveInvisibility();
	}
#endif

	pPlayer->m_Shared.OnAttack();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::SecondaryAttack()
{
	if ( !CanAttack() )
		return;

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();

	m_bInAttack2 = true;


	m_flNextSecondaryAttack = gpGlobals->curtime + GetNextSecondaryAttackDelay(); // default: 0.5f
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::PlaySwingSound( void )
{
	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Swing( CTFPlayer *pPlayer )
{
	CalcIsAttackCritical();

#ifdef GAME_DLL
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif
#ifdef CLIENT_DLL
	C_CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Play the melee swing and miss (whoosh) always.
	SendPlayerAnimEvent( pPlayer );

	DoViewModelAnimation();

	// Set next attack times.
	float flFireDelay = ApplyFireDelay( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay );

	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;
	m_flNextSecondaryAttack = gpGlobals->curtime + flFireDelay;
	pPlayer->m_Shared.SetNextStealthTime( m_flNextSecondaryAttack );

	SetWeaponIdleTime( m_flNextPrimaryAttack + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeIdleEmpty );

	PlaySwingSound();

#ifdef GAME_DLL
	// Remember if there are potential targets when we start our swing.
	// If there are, the player is exempt from taking "hurt self on miss" damage
	// if ALL of these players have died when our swing has finished, and we didn't hit.
	// This guards against me performing a "good" swing and being punished by a friend
	// killing my target "out from under me".
	CUtlVector< CTFPlayer * > enemyVector;
	CollectPlayers( &enemyVector, GetEnemyTeam( pPlayer->GetTeamNumber() ), COLLECT_ONLY_LIVING_PLAYERS );

	m_potentialVictimVector.RemoveAll();
	const float looseSwingRange = 1.2f * GetSwingRange();

	for( int i=0; i<enemyVector.Count(); ++i )
	{
		Vector toVictim = enemyVector[i]->WorldSpaceCenter() - pPlayer->Weapon_ShootPosition();

		if ( toVictim.IsLengthLessThan( looseSwingRange ) )
		{
			m_potentialVictimVector.AddToTail( enemyVector[i] );
		}
	}
#endif

	m_flSmackTime = GetSmackTime( m_iWeaponMode );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::DoViewModelAnimation( void )
{
	if ( IsCurrentAttackACrit() )
	{
		if ( SendWeaponAnim( ACT_VM_SWINGHARD ) )
		{
			// check that weapon has the activity
			return;
		}
	}

	Activity act = ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE ) ? ACT_VM_HITCENTER : ACT_VM_SWINGHARD;

	SendWeaponAnim( act );
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::ItemPreFrame( void )
{
	int iSelfMark = 0;
	CALL_ATTRIB_HOOK_INT( iSelfMark, self_mark_for_death );
	if ( iSelfMark )
	{
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->m_Shared.AddCond( TF_COND_MARKEDFORDEATH_SILENT, iSelfMark );
		}
	}

	return BaseClass::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::ItemPostFrame()
{
	// Check for smack.
	if ( m_flSmackTime > 0.0f && gpGlobals->curtime > m_flSmackTime )
	{
		m_flSmackTime = -1.0f;
		Smack();
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->m_Shared.SetNextMeleeCrit( MELEE_NOCRIT );
		}
	}

	BaseClass::ItemPostFrame();
}


bool CTFWeaponBaseMelee::DoSwingTraceInternal( trace_t &trace, bool bCleave, CUtlVector< trace_t >* pTargetTraceVector )
{
	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMinsBase( -18, -18, -18 );
	static Vector vecSwingMaxsBase( 18, 18, 18 );

	float fBoundsScale = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( fBoundsScale, melee_bounds_multiplier );
	Vector vecSwingMins = vecSwingMinsBase * fBoundsScale;
	Vector vecSwingMaxs = vecSwingMaxsBase * fBoundsScale;

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	// Setup the swing range.
	float fSwingRange = GetSwingRange();

	// Scale the range and bounds by the model scale if they're larger
	// Not scaling down the range for smaller models because midgets need all the help they can get
	if ( pPlayer->GetModelScale() > 1.0f )
	{
		fSwingRange *= pPlayer->GetModelScale();
		vecSwingMins *= pPlayer->GetModelScale();
		vecSwingMaxs *= pPlayer->GetModelScale();
	}

	CALL_ATTRIB_HOOK_FLOAT( fSwingRange, melee_range_multiplier );

	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * fSwingRange;

	// In MvM, melee hits from the robot team wont hit teammates to ensure mobs of melee bots don't 
	// swarm so tightly they hit each other and no-one else
	bool bDontHitTeammates = pPlayer->GetTeamNumber() == TF_TEAM_PVE_INVADERS && TFGameRules()->IsMannVsMachineMode();
	CTraceFilterIgnoreTeammates ignoreTeammatesFilter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );

	if ( bCleave )
	{
		Ray_t ray;
		ray.Init( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs );
		CBaseEntity *pList[256];
		int nTargetCount = UTIL_EntitiesAlongRay( pList, ARRAYSIZE( pList ), ray, FL_CLIENT|FL_OBJECT );
		
		int nHitCount = 0;
		for ( int i=0; i<nTargetCount; ++i )
		{
			CBaseEntity *pTarget = pList[i];
			if ( pTarget == pPlayer )
			{
				// don't hit yourself
				continue;
			}

			if ( bDontHitTeammates && pTarget->GetTeamNumber() == pPlayer->GetTeamNumber() )
			{
				// don't hit teammate
				continue;
			}

			if ( pTargetTraceVector )
			{
				trace_t tr;
				UTIL_TraceModel( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, pTarget, COLLISION_GROUP_NONE, &tr );
				pTargetTraceVector->AddToTail();
				pTargetTraceVector->Tail() = tr;
			}
			nHitCount++;
		}

		return nHitCount > 0;
	}
	else
	{
		bool bSapperHit = false;

		// if this weapon can damage sappers, do that trace first
		int iDmgSappers = 0;
		CALL_ATTRIB_HOOK_INT( iDmgSappers, set_dmg_apply_to_sapper );
		if ( iDmgSappers != 0 )
		{
			CTraceFilterIgnorePlayers ignorePlayersFilter( NULL, COLLISION_GROUP_NONE );
			UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &ignorePlayersFilter, &trace );
			if ( trace.fraction >= 1.0 )
			{
				UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &ignorePlayersFilter, &trace );
			}

			if ( trace.fraction < 1.0f &&
				 trace.m_pEnt &&
				 trace.m_pEnt->IsBaseObject() &&
				 trace.m_pEnt->GetTeamNumber() == pPlayer->GetTeamNumber() )
			{
				CBaseObject *pObject = static_cast< CBaseObject* >( trace.m_pEnt );
				if ( pObject->HasSapper() )
				{
					bSapperHit = true;
				}
			}
		}

		if ( !bSapperHit )
		{
			// See if we hit anything.
			if ( bDontHitTeammates )
			{
				UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &ignoreTeammatesFilter, &trace );
			}
			else
			{
				CTraceFilterIgnoreFriendlyCombatItems filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
				UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &filter, &trace );
			}

			if ( trace.fraction >= 1.0 )
			{
				if ( bDontHitTeammates )
				{
					UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &ignoreTeammatesFilter, &trace );
				}
				else
				{
					CTraceFilterIgnoreFriendlyCombatItems filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
					UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &filter, &trace );
				}

				if ( trace.fraction < 1.0 )
				{
					// Calculate the point of intersection of the line (or hull) and the object we hit
					// This is and approximation of the "best" intersection
					CBaseEntity *pHit = trace.m_pEnt;
					if ( !pHit || pHit->IsBSPModel() )
					{
						// Why duck hull min/max?
						FindHullIntersection( vecSwingStart, trace, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, pPlayer );
					}

					// This is the point on the actual surface (the hull could have hit space)
					vecSwingEnd = trace.endpos;	
				}
			}
		}

		return ( trace.fraction < 1.0f );
	}
}


bool CTFWeaponBaseMelee::DoSwingTrace( trace_t &trace )
{
	return DoSwingTraceInternal( trace, false, NULL );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::OnSwingHit( trace_t &trace )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	// NVNT if this is the client dll and the owner is the local player
	//	Notify the haptics system the local player just hit something.
#ifdef CLIENT_DLL
	if(pPlayer==C_TFPlayer::GetLocalTFPlayer() && haptics)
		haptics->ProcessHapticEvent(2,"Weapons","meleehit");
#endif

	bool bHitEnemyPlayer = false;

	// Hit sound - immediate.
	if( trace.m_pEnt->IsPlayer() )
	{
		CTFPlayer *pTargetPlayer = ToTFPlayer( trace.m_pEnt );

		bool bPlayMvMHitOnly = false;
		// handle hitting a robot	
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			if ( pTargetPlayer  && pTargetPlayer->GetTeamNumber() == TF_TEAM_PVE_INVADERS && !pTargetPlayer->IsPlayer() )
			{
				bPlayMvMHitOnly = true;

				CBroadcastRecipientFilter filter;
				// 					CSingleUserRecipientFilter filter( ToBasePlayer( GetOwner() ) );
				// 					if ( IsPredicted() && CBaseEntity::GetPredictionPlayer() )
				// 					{
				// 						filter.UsePredictionRules();
				// 					}

				char szMeleeSoundStr[128] = "MVM_";
				const char *shootsound = GetShootSound( MELEE_HIT );
				if ( shootsound && shootsound[0] )
				{
					V_strcat(szMeleeSoundStr, shootsound, sizeof( szMeleeSoundStr ));
					CSoundParameters params;
					if ( CBaseEntity::GetParametersForSound( szMeleeSoundStr, params, NULL ) )
					{
						EmitSound( filter, GetOwner()->entindex(), szMeleeSoundStr, NULL );
					}
					else
					{
						EmitSound( filter, GetOwner()->entindex(), "MVM_Weapon_Default.HitFlesh", NULL );
					}
				}
				else
				{
					EmitSound( filter, GetOwner()->entindex(), "MVM_Weapon_Default.HitFlesh", NULL );
				}
			}
		} 
		if(! bPlayMvMHitOnly )
		{
			WeaponSound( MELEE_HIT );
		}

#if !defined (CLIENT_DLL)

		if ( pTargetPlayer->m_Shared.HasPasstimeBall() && g_pPasstimeLogic ) 
		{
			// This handles stealing the ball from teammates since there's no damage involved
			// TODO find a better place for this
			g_pPasstimeLogic->OnBallCarrierMeleeHit( pTargetPlayer, pPlayer );
		}

		if ( pPlayer->GetTeamNumber() != pTargetPlayer->GetTeamNumber() )
		{
			bHitEnemyPlayer = true;

			if ( TFGameRules()->IsIT( pPlayer ) )
			{
				IGameEvent *pEvent = gameeventmanager->CreateEvent( "tagged_player_as_it" );
				if ( pEvent )
				{
					pEvent->SetInt( "player", pPlayer->GetUserID() );
					gameeventmanager->FireEvent( pEvent, true );
				}

				// Tag! You're IT!
				TFGameRules()->SetIT( pTargetPlayer );

				pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_YES );

				UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_ANNOUNCE_TAG", pPlayer->GetPlayerName(), pTargetPlayer->GetPlayerName() );

				CSingleUserReliableRecipientFilter filter( pPlayer );
				pPlayer->EmitSound( filter, pPlayer->entindex(), "Player.TaggedOtherIT" );
			}
		}

		if ( pTargetPlayer->InSameTeam( pPlayer ) || pTargetPlayer->m_Shared.GetDisguiseTeam() == GetTeamNumber() )
		{
			int iSpeedBuffOnHit = 0;
			CALL_ATTRIB_HOOK_INT( iSpeedBuffOnHit, speed_buff_ally );
			if ( iSpeedBuffOnHit > 0 && trace.m_pEnt )
			{
				pTargetPlayer->m_Shared.AddCond( TF_COND_SPEED_BOOST, 2.f );
				pPlayer->m_Shared.AddCond( TF_COND_SPEED_BOOST, 3.6f );		// give the soldier a bit of additional time to allow them to keep up better with faster classes

				EconEntity_OnOwnerKillEaterEvent( this, pPlayer, pTargetPlayer, kKillEaterEvent_TeammatesWhipped );	// Strange
			}

			// Give health to teammates on hit
			int nGiveHealthOnHit = 0;
			CALL_ATTRIB_HOOK_INT( nGiveHealthOnHit, add_give_health_to_teammate_on_hit );
			if ( nGiveHealthOnHit != 0 )
			{
				// Always keep at least 1 health for ourselves
				nGiveHealthOnHit = Min( pPlayer->GetHealth() - 1, nGiveHealthOnHit );
				int nHealthGiven = pTargetPlayer->TakeHealth( nGiveHealthOnHit, DMG_GENERIC );

				if ( nHealthGiven > 0 )
				{
					// Subtract health given from my own
					CTakeDamageInfo info( pPlayer, pPlayer, this, nHealthGiven, DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
					pPlayer->TakeDamage( info );
				}
			}
		}
		else
		{
			float flSpeedBoostOnHitEnemy = 0.f;
			CALL_ATTRIB_HOOK_FLOAT( flSpeedBoostOnHitEnemy, speed_boost_on_hit_enemy );
			if ( flSpeedBoostOnHitEnemy > 0 && trace.m_pEnt )
			{
				pPlayer->m_Shared.AddCond( TF_COND_SPEED_BOOST, flSpeedBoostOnHitEnemy );
			}
		}
#endif
	}
	else
	{
		WeaponSound( MELEE_HIT_WORLD );
	}

	DoMeleeDamage( trace.m_pEnt, trace );

	return bHitEnemyPlayer;
}


// -----------------------------------------------------------------------------
// Purpose:
// Note: Think function to delay the impact decal until the animation is finished 
//       playing.
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Smack( void )
{
	trace_t trace;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

#if !defined (CLIENT_DLL)
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	bool bHitEnemyPlayer = false;

	int nCleaveAttack = 0;
	CALL_ATTRIB_HOOK_INT( nCleaveAttack, melee_cleave_attack );
	bool bCleave = nCleaveAttack > 0;

	// We hit, setup the smack.
	CUtlVector<trace_t> targetTraceVector;
	if ( DoSwingTraceInternal( trace, bCleave, &targetTraceVector ) )
	{
		if ( bCleave )
		{
			for ( int i=0; i<targetTraceVector.Count(); ++i )
			{
				bHitEnemyPlayer |= OnSwingHit( targetTraceVector[i] );
			}
		}
		else
		{
			bHitEnemyPlayer = OnSwingHit( trace );
		}
	}
	else
	{
		// if ALL of my potential targets have been killed by someone else between the 
		// time I started my swing and the time my swing would have landed, don't
		// punish me for it.
		bool bIsCleanMiss = true;

#ifdef GAME_DLL
		for( int i=0; i<m_potentialVictimVector.Count(); ++i )
		{
			if ( m_potentialVictimVector[i] != NULL && m_potentialVictimVector[i]->IsAlive() )
			{
				bIsCleanMiss = false;
				break;
			}
		}
#endif

		if ( bIsCleanMiss )
		{
			int iHitSelf = 0;
			CALL_ATTRIB_HOOK_INT( iHitSelf, hit_self_on_miss );
			if ( iHitSelf == 1 )
			{
				DoMeleeDamage( GetTFPlayerOwner(), trace, 0.5f );
			}
		}
	}

#if !defined (CLIENT_DLL)

	// ACHIEVEMENT_TF_MEDIC_BONESAW_NOMISSES
	if ( GetWeaponID() == TF_WEAPON_BONESAW )
	{
		int iCount = pPlayer->GetPerLifeCounterKV( "medic_bonesaw_hits" );

		if ( bHitEnemyPlayer )
		{
			if ( ++iCount >= 5 )
			{
				pPlayer->AwardAchievement( ACHIEVEMENT_TF_MEDIC_BONESAW_NOMISSES );
			}
		}
		else
		{
			iCount = 0;
		}

		pPlayer->SetPerLifeCounterKV( "medic_bonesaw_hits", iCount );
	}

	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

float CTFWeaponBaseMelee::GetSmackTime( int iWeaponMode )
{
	return gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( iWeaponMode ).m_flSmackDelay;
}

void CTFWeaponBaseMelee::DoMeleeDamage( CBaseEntity* ent, trace_t& trace )
{
	DoMeleeDamage( ent, trace, 1.f );
}

void CTFWeaponBaseMelee::DoMeleeDamage( CBaseEntity* ent, trace_t& trace, float flDamageMod )
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * 48;

#ifndef CLIENT_DLL
	// Do Damage.
	int iCustomDamage = GetDamageCustom();
	int iDmgType = DMG_MELEE | DMG_NEVERGIB | DMG_CLUB;

	int iCritFromBehind = 0;
	CALL_ATTRIB_HOOK_INT( iCritFromBehind, crit_from_behind );
	if ( iCritFromBehind > 0 )
	{
		Vector entForward; 
		AngleVectors( ent->EyeAngles(), &entForward );

		Vector toEnt = ent->GetAbsOrigin() - pPlayer->GetAbsOrigin();
		toEnt.NormalizeInPlace();

		if ( DotProduct( toEnt, entForward ) > 0.7071f )
		{
			iDmgType |= DMG_CRITICAL;
		}
	}

	float flDamage = GetMeleeDamage( ent, &iDmgType, &iCustomDamage ) * flDamageMod;

	// Base melee damage increased because we disallow random crits in this mode. Without random crits, melee is underpowered
	if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		if ( !IsCurrentAttackACrit() ) // Don't multiply base damage if attack is a crit
		{
			if ( pPlayer && pPlayer->m_Shared.GetCarryingRuneType() == RUNE_KNOCKOUT )
			{
				flDamage *= ( pPlayer->m_Shared.InCond( TF_COND_POWERUPMODE_DOMINANT ) ? 1.4f : 1.9f );
			}
			// Strength powerup multiplies damage later and we only want double regular damage
			// Shields are a source of increased melee damage (charge crit) so they don't need a base boost
			else if ( pPlayer && pPlayer->m_Shared.GetCarryingRuneType() != RUNE_STRENGTH && !pPlayer->m_Shared.IsShieldEquipped() )
			{
				flDamage *= 1.3f;
			}
		}
	}

	if ( IsCurrentAttackACrit() )
	{
		// TODO: Not removing the old critical path yet, but the new custom damage is marking criticals as well for melee now.
		iDmgType |= DMG_CRITICAL;
	}
	else if ( m_bMiniCrit )
	{
		iDmgType |= DMG_RADIUS_MAX; // Unused for melee, indicates this should be a minicrit.
	}

	CTakeDamageInfo info( pPlayer, pPlayer, this, flDamage, iDmgType, iCustomDamage );

	if ( fabs( flDamage ) >= 1.0f )
	{
		CalculateMeleeDamageForce( &info, vecForward, vecSwingEnd, 1.0f / flDamage * GetForceScale() );
	}
	else
	{
		info.SetDamageForce( vec3_origin );
	}
	
	ent->DispatchTraceAttack( info, vecForward, &trace ); 
	ApplyMultiDamage();

	OnEntityHit( ent, &info );

	bool bTruce = TFGameRules() && TFGameRules()->IsTruceActive() && pPlayer->IsTruceValidForEnt();
	if ( !bTruce )
	{
		int iCritsForceVictimToLaugh = 0;
		CALL_ATTRIB_HOOK_INT( iCritsForceVictimToLaugh, crit_forces_victim_to_laugh );
		if ( iCritsForceVictimToLaugh > 0 && ( IsCurrentAttackACrit() || iDmgType & DMG_CRITICAL ) )
		{
			CTFPlayer *pVictimPlayer = ToTFPlayer( ent );

			if ( pVictimPlayer && pVictimPlayer->CanBeForcedToLaugh() && ( pPlayer->GetTeamNumber() != pVictimPlayer->GetTeamNumber() ) )
			{
				// force victim to laugh!
				pVictimPlayer->Taunt( TAUNT_MISC_ITEM, MP_CONCEPT_TAUNT_LAUGH );

				// strange stat tracking
				EconEntity_OnOwnerKillEaterEvent( this,
												  ToTFPlayer( GetOwner() ),
												  pVictimPlayer,
												  kKillEaterEvent_PlayerTickle );
			}
		}

		int iTickleEnemiesWieldingSameWeapon = 0;
		CALL_ATTRIB_HOOK_INT( iTickleEnemiesWieldingSameWeapon, tickle_enemies_wielding_same_weapon );
		if ( iTickleEnemiesWieldingSameWeapon > 0 )
		{
			CTFPlayer *pVictimPlayer = ToTFPlayer( ent );

			if ( pVictimPlayer && pVictimPlayer->CanBeForcedToLaugh() && ( pPlayer->GetTeamNumber() != pVictimPlayer->GetTeamNumber() ) )
			{
				CTFWeaponBase *myWeapon = pPlayer->GetActiveTFWeapon();
				CTFWeaponBase *theirWeapon = pVictimPlayer->GetActiveTFWeapon();

				if ( myWeapon && theirWeapon )
				{
					CEconItemView *myItem = myWeapon->GetAttributeContainer()->GetItem();
					CEconItemView *theirItem = theirWeapon->GetAttributeContainer()->GetItem();

					if ( myItem && theirItem && myItem->GetItemDefIndex() == theirItem->GetItemDefIndex() )
					{
						// force victim to laugh!
						pVictimPlayer->Taunt( TAUNT_MISC_ITEM, MP_CONCEPT_TAUNT_LAUGH );
					}
				}
			}
		}
	}
	if ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_KNOCKOUT )
	{
		CTFPlayer *pVictimPlayer = ToTFPlayer( ent );

		if ( pVictimPlayer && !pVictimPlayer->InSameTeam( pPlayer ) )
		{
			CPASAttenuationFilter filter( pPlayer );
			Vector origin = pPlayer->GetAbsOrigin();
			Vector vecDir = pVictimPlayer->GetAbsOrigin() - origin;
			VectorNormalize( vecDir );
				
			if ( !pVictimPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_USER_BUFF ) &&
				!pVictimPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) )
			{
				if ( pVictimPlayer->m_Shared.IsCarryingRune() ) 
				{
					pVictimPlayer->DropRune();
					ClientPrint( pVictimPlayer, HUD_PRINTCENTER, "#TF_Powerup_Knocked_Out" );
				}
				else if ( pVictimPlayer->HasTheFlag() )	
				{
					pVictimPlayer->DropFlag();
					ClientPrint( pVictimPlayer, HUD_PRINTCENTER, "#TF_CTF_PlayerDrop" );
				}
			}
			EmitSound( filter, entindex(), "Powerup.Knockout_Melee_Hit" );
			pVictimPlayer->ApplyGenericPushbackImpulse( vecDir * 400.0f, pPlayer );
		}
	}

#endif
	// Don't impact trace friendly players or objects
	if ( ent && ent->GetTeamNumber() != pPlayer->GetTeamNumber() )
	{
#ifdef CLIENT_DLL
		UTIL_ImpactTrace( &trace, DMG_CLUB );
#endif
		m_bConnected = true;
	}
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBaseMelee::GetForceScale( void )
{
	return tf_meleeattackforcescale.GetFloat();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBaseMelee::GetMeleeDamage( CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage )
{
	float flDamage = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
	CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );

	int iCritDoesNoDamage = 0;
	CALL_ATTRIB_HOOK_INT( iCritDoesNoDamage, crit_does_no_damage );
	if ( iCritDoesNoDamage > 0 )
	{
		if ( IsCurrentAttackACrit() )
		{
			return 0.0f;	
		}

		if ( piDamageType && *piDamageType & DMG_CRITICAL )
		{
			return 0.0f;
		}
	}

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		float flHalfHealth = pPlayer->GetMaxHealth() * 0.5f;
		if ( pPlayer->GetHealth() < flHalfHealth )
		{
			CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg_bonus_while_half_dead );
		}
		else
		{
			CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg_penalty_while_half_alive );
		}

		// Some weapons change damage based on player's health
		float flReducedHealthBonus = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT( flReducedHealthBonus, mult_dmg_with_reduced_health );
		if ( flReducedHealthBonus != 1.0f )
		{
			float flHealthFraction = clamp( pPlayer->HealthFraction(), 0.0f, 1.0f );
			flReducedHealthBonus = Lerp( flHealthFraction, flReducedHealthBonus, 1.0f );

			flDamage *= flReducedHealthBonus;
		}
	}

	return flDamage;
}

void CTFWeaponBaseMelee::OnEntityHit( CBaseEntity *pEntity, CTakeDamageInfo *info )
{
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CalcIsAttackCriticalHelperNoCrits( void )
{
	// This function was called because the tf_weapon_criticals ConVar is off, but if
	// melee crits are set to be forced on, then call the regular crit helper function.
	if ( tf_weapon_criticals_melee.GetInt() > 1 )
	{
		return CalcIsAttackCriticalHelper();
	}

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	m_bCurrentAttackIsDuringDemoCharge = pPlayer->m_Shared.GetNextMeleeCrit() != MELEE_NOCRIT;

	if ( pPlayer->m_Shared.GetNextMeleeCrit() == MELEE_CRIT )
	{
		return true;
	}
	else
	{
		return BaseClass::CalcIsAttackCriticalHelperNoCrits();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CalcIsAttackCriticalHelper( void )
{
	// If melee crits are off, then check the NoCrits helper.
	if ( tf_weapon_criticals_melee.GetInt() == 0 )
	{
		return CalcIsAttackCriticalHelperNoCrits();
	}

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	if ( !CanFireCriticalShot() )
		return false;

	// Crit boosted players fire all crits
	if ( pPlayer->m_Shared.IsCritBoosted() )
		return true;

	float flPlayerCritMult = pPlayer->GetCritMult();
	float flCritChance = TF_DAMAGE_CRIT_CHANCE_MELEE * flPlayerCritMult;
	CALL_ATTRIB_HOOK_FLOAT( flCritChance, mult_crit_chance );

	// mess with the crit chance seed so it's not based solely on the prediction seed
	int iMask = ( entindex() << 16 ) | ( pPlayer->entindex() << 8 );
	int iSeed = CBaseEntity::GetPredictionRandomSeed() ^ iMask;
	if ( iSeed != m_iCurrentSeed )
	{
		m_iCurrentSeed = iSeed;
		RandomSeed( m_iCurrentSeed );
	}

	m_bCurrentAttackIsDuringDemoCharge = pPlayer->m_Shared.GetNextMeleeCrit() != MELEE_NOCRIT;

	if ( pPlayer->m_Shared.GetNextMeleeCrit() == MELEE_CRIT )
	{
		return true;
	}

	// Regulate crit frequency to reduce client-side seed hacking
	float flDamage = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
	CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );
	AddToCritBucket( flDamage );

	// Track each request
	m_nCritChecks++;

	bool bCrit = ( RandomInt( 0, WEAPON_RANDOM_RANGE-1 ) < ( flCritChance ) * WEAPON_RANDOM_RANGE );

#ifdef _DEBUG
	// Force seed to always say yes
	if ( tf_weapon_criticals_force_random.GetInt() )
	{
		bCrit = true;
	}
#endif // _DEBUG

	if ( bCrit )
	{
		// Seed says crit.  Run it by the manager.
		bCrit = IsAllowedToWithdrawFromCritBucket( flDamage );
	}

	return bCrit;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CTFWeaponBaseMelee::GetShootSound( int iIndex ) const
{
	// Custom Melee weapons may override their hit effects
	if ( iIndex == MELEE_HIT )
	{
		const CEconItemView *pItem = GetAttributeContainer()->GetItem();
		if ( pItem->IsValid() )
		{
			const char *pszSound = pItem->GetStaticData()->GetCustomSound( GetTeamNumber(), 1 );
			if ( pszSound )
				return pszSound;
		}
	}

	return BaseClass::GetShootSound(iIndex);
}
