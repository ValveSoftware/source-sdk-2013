//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "tf_replay.h"
#include "tf/tf_shareddefs.h"
#include "tf/c_tf_player.h"
#include "tf/c_tf_playerresource.h"
#include "tf/c_tf_gamestats.h"
#include "tf/tf_gamestats_shared.h"
#include "tf/tf_hud_statpanel.h"
#include "tf/c_obj_sentrygun.h"
#include "clientmode_shared.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/ireplayfactory.h"
#include "replay/ireplayscreenshotmanager.h"
#include "replay/screenshot.h"
#include <time.h>

//----------------------------------------------------------------------------------------

extern IReplayScreenshotManager *g_pReplayScreenshotManager;

//----------------------------------------------------------------------------------------

CTFReplay::CTFReplay()
:	m_flNextMedicUpdateTime( 0.0f )
{
}

CTFReplay::~CTFReplay()
{
}

void CTFReplay::OnBeginRecording()
{
	BaseClass::OnBeginRecording();

	// Setup the newly created replay
	C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		if ( pPlayer->GetPlayerClass() )
		{
			SetPlayerClass( pPlayer->GetPlayerClass()->GetClassIndex() );
		}

		SetPlayerTeam( pPlayer->GetTeamNumber() );
	}
}

void CTFReplay::OnEndRecording()
{
	if ( gameeventmanager )
	{
		gameeventmanager->RemoveListener( this );
	}

	BaseClass::OnEndRecording();
}

void CTFReplay::OnComplete()
{
	BaseClass::OnComplete();
}

void CTFReplay::Update()
{
	// If local player is medic and invuln'd someone, take a screenshot
	MedicUpdate();

	BaseClass::Update();
}

void CTFReplay::MedicUpdate()
{
	// Not ready for update?
	if ( gpGlobals->curtime < m_flNextMedicUpdateTime )
		return;

	// Local player doesn't exist?
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
	{
		Assert( 0 );	// Shouldn't happen
		return;
	}

	if ( pLocalPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_MEDIC )
		return;

	// Releasing charge?
	if ( pLocalPlayer->MedicIsReleasingCharge() )
	{
		// Take a sick screenshot
		CaptureScreenshotParams_t params;
		V_memset( &params, 0, sizeof( params ) );
		params.m_flDelay = 0.25f;
		g_pReplayScreenshotManager->CaptureScreenshot( params );

		// Set next update to minimum time it would be until next recharge
		extern ConVar weapon_medigun_chargerelease_rate;
		m_flNextMedicUpdateTime = gpGlobals->curtime + weapon_medigun_chargerelease_rate.GetFloat();
	}
	else
	{
		// Check again in a second
		m_flNextMedicUpdateTime = gpGlobals->curtime + 1.0f;
	}
}

float CTFReplay::GetSentryKillScreenshotDelay()
{
	ConVarRef replay_screenshotsentrykilldelay( "replay_screenshotsentrykilldelay" );
	return replay_screenshotsentrykilldelay.IsValid() ? replay_screenshotsentrykilldelay.GetFloat() : 0.5f;
}

void CTFReplay::FireGameEvent( IGameEvent *pEvent )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	CaptureScreenshotParams_t params;
	V_memset( &params, 0, sizeof( params ) );

	if ( FStrEq( pEvent->GetName(), "player_death" ) )
	{
		ConVarRef replay_debug( "replay_debug" );
		if ( replay_debug.IsValid() && replay_debug.GetBool() )
		{
			DevMsg( "%i: CTFReplay::FireGameEvent(): player_death\n", gpGlobals->tickcount );
		}

		int nKillerID = pEvent->GetInt( "attacker" );
		int nVictimID = pEvent->GetInt( "userid" );
		int nDeathFlags = pEvent->GetInt( "death_flags" );
		int nAssisterID = pEvent->GetInt( "assister" );

		const char *pWeaponName = pEvent->GetString( "weapon" );

		// Suicide?
		bool bSuicide = nKillerID == nVictimID;

		// Try to get killer
		C_TFPlayer *pKiller = ToTFPlayer( USERID2PLAYER( nKillerID ) );

		// Try to get victim
		C_TFPlayer *pVictim = ToTFPlayer( USERID2PLAYER( nVictimID ) );

		// Is local player healing the killer?
		bool bKillerLastHealerIsLocalPlayer = pKiller && pKiller->GetWasHealedByLocalPlayer();

		// Inflictor was a sentry gun?
		bool bSentry = V_strnicmp( pWeaponName, "obj_sentrygun", 13 ) == 0;
		int nInflictorEntIndex = pEvent->GetInt( "inflictor_entindex" );
		C_BaseEntity *pInflictor = ClientEntityList().GetEnt( nInflictorEntIndex );
		C_ObjectSentrygun *pSentry = dynamic_cast< C_ObjectSentrygun * >( pInflictor );
		bool bFeignDeath = pEvent->GetInt( "death_flags" ) & TF_DEATH_FEIGN_DEATH;

		// Is the killer the local player?
		if ( nKillerID == pLocalPlayer->GetUserID() &&
			 !bSuicide &&
			 !bSentry )
		{
			// Domination?
			if ( nDeathFlags & TF_DEATH_DOMINATION )
			{
				AddDomination( nVictimID );
			}

			// Assister domination?
			if ( ( nDeathFlags & TF_DEATH_ASSISTER_DOMINATION ) && ( nAssisterID > 0 ) )
			{
				AddAssisterDomination( nVictimID, nAssisterID );
			}
			
			// Revenge?
			if ( pEvent->GetInt( "death_flags" ) & TF_DEATH_REVENGE ) 
			{
				AddRevenge( nVictimID );
			}

			// Assister revenge?
			if ( pEvent->GetInt( "death_flags" ) & TF_DEATH_ASSISTER_REVENGE && ( nAssisterID > 0 ) ) 
			{
				AddAssisterRevenge( nVictimID, nAssisterID );
			}

			// Add victim info to kill list
			if ( pVictim )
			{
				AddKill( pVictim->GetPlayerName(), pVictim->GetPlayerClass()->GetClassIndex() );
			}
		
			// Take a quick screenshot with some delay
			ConVarRef replay_screenshotkilldelay( "replay_screenshotkilldelay" );
			if ( replay_screenshotkilldelay.IsValid() )
			{
				params.m_flDelay = GetKillScreenshotDelay();
				g_pReplayScreenshotManager->CaptureScreenshot( params );
			}
		}
		
		// Player death?
		else if ( pKiller &&
				  nVictimID == pLocalPlayer->GetUserID() )
		{
			// Record who killed the player if not a suicide
			if ( !bSuicide && !bFeignDeath )
			{
				RecordPlayerDeath( pKiller->GetPlayerName(), pKiller->GetPlayerClass()->GetClassIndex() );
			}

			// Take screenshot - taking a screenshot during feign death is cool, too.
			ConVarRef replay_deathcammaxverticaloffset( "replay_deathcammaxverticaloffset" );
			ConVarRef replay_playerdeathscreenshotdelay( "replay_playerdeathscreenshotdelay" );
			params.m_flDelay = replay_playerdeathscreenshotdelay.IsValid() ? replay_playerdeathscreenshotdelay.GetFloat() : 0.0f;
			params.m_nEntity = pLocalPlayer->entindex();
			params.m_posCamera.Init( 0,0, replay_deathcammaxverticaloffset.IsValid() ? replay_deathcammaxverticaloffset.GetFloat() : 150 );
			params.m_angCamera.Init( 90, 0, 0 );	// Look straight down
			params.m_bUseCameraAngles = true;
			params.m_bIgnoreMinTimeBetweenScreenshots = true;
			g_pReplayScreenshotManager->CaptureScreenshot( params );
		}

		// Killer is invuln/crit boosted and healer is local player?
		else if ( bKillerLastHealerIsLocalPlayer &&
			 ( pKiller->m_Shared.IsCritBoosted() ||
			   pKiller->m_Shared.InCond( TF_COND_INVULNERABLE ) ||
			   pKiller->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) ) )
		{
			// Take a quick screenshot with some delay
			params.m_flDelay = GetKillScreenshotDelay();
			g_pReplayScreenshotManager->CaptureScreenshot( params );
		}

		// Is the inflictor a sentry belonging to the local player?
		else if ( pLocalPlayer->IsAlive() &&
				  bSentry &&
				  pSentry &&
				  pSentry->GetOwner() == pLocalPlayer &&
				  pVictim )
		{
			ConVarRef replay_sentrycammaxverticaloffset( "replay_sentrycammaxverticaloffset" );
			ConVarRef replay_sentrycamoffset_frontback( "replay_sentrycamoffset_frontback" );
			ConVarRef replay_sentrycamoffset_leftright( "replay_sentrycamoffset_leftright" );
			ConVarRef replay_sentrycamoffset_updown( "replay_sentrycamoffset_updown" );

			// Setup screenshot params
			params.m_flDelay = GetSentryKillScreenshotDelay();
			params.m_nEntity = pSentry->entindex();
			params.m_bUseCameraAngles = true;

			// Calculate camera eye position
			static float s_aSentryEyeLevels[3] = { SENTRYGUN_EYE_OFFSET_LEVEL_1[2], SENTRYGUN_EYE_OFFSET_LEVEL_2[2], SENTRYGUN_EYE_OFFSET_LEVEL_3[2] };
			int iSentryUpgrade = clamp( pSentry->GetUpgradeLevel() - 1, 0, 2 );
			Vector vecSentryEyeOffset = Vector( 0, 0, s_aSentryEyeLevels[ iSentryUpgrade ] );

			Vector vecSentryAimDir;			// Since it seems the sentry's *actual* aim direction is only available on the server, use the victim's location to calculate a general idea of one
			Vector vecVictimUp = pVictim->WorldSpaceCenter() - pVictim->GetAbsOrigin();	// WorldSpaceCenter() seems to return player's eye level
			Vector vecVictimCenter = pVictim->GetAbsOrigin() + 0.5f * vecVictimUp;
			vecSentryAimDir = vecVictimCenter - pSentry->GetAbsOrigin() + vecSentryEyeOffset;
			VectorNormalizeFast( vecSentryAimDir );

			Vector vecX, vecY, vecZ;	// Construct a matrix to transform the eye point
			vecX = vecSentryAimDir;
			vecY = CrossProduct( Vector(0,0,1), vecSentryAimDir );
			vecZ = CrossProduct( vecX, vecY );
			matrix3x4_t m;
			m.Init( vecX, vecY, vecZ, vec3_origin );

			Vector out;					// Transform the point relative to the sentry's eye
			Vector vecOffset;
			if ( replay_sentrycamoffset_frontback.IsValid() &&
				 replay_sentrycamoffset_leftright.IsValid() &&
				 replay_sentrycamoffset_updown.IsValid() )
			{
				vecOffset.Init( replay_sentrycamoffset_frontback.GetFloat(), -replay_sentrycamoffset_leftright.GetFloat(), replay_sentrycamoffset_updown.GetFloat() );
			}
			else
			{
				vecOffset.Init( 0, 0, 0 );
			}
			VectorTransform( vecOffset, m, out );
			out += Vector( 0,0, s_aSentryEyeLevels[ iSentryUpgrade ] + ( replay_sentrycammaxverticaloffset.IsValid() ? replay_sentrycammaxverticaloffset.GetFloat() : 5 ) );
			params.m_posCamera = out;

			// Use the aim matrix we constructed as the camera's orientation
			MatrixAngles( m, params.m_angCamera );

			// Take the screenshot from the sentry's point of view
			g_pReplayScreenshotManager->CaptureScreenshot( params );
		}
	}
}

bool CTFReplay::IsValidClass( int nClass ) const
{
	return IsValidTFPlayerClass( nClass );
}

bool CTFReplay::IsValidTeam( int iTeam ) const
{
	return IsValidTFTeam( iTeam );
}

bool CTFReplay::GetCurrentStats( RoundStats_t &out )
{
	if ( !g_TF_PR )
		return false;
	
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return false;

	int iLocalPlayerIndex = GetLocalPlayerIndex();

	out.m_iStat[ TFSTAT_POINTSSCORED       ] = g_TF_PR->GetPlayerScore( iLocalPlayerIndex );
	out.m_iStat[ TFSTAT_DEATHS             ] = g_TF_PR->GetDeaths( iLocalPlayerIndex );

	out.m_iStat[ TFSTAT_CAPTURES           ] = pLocalPlayer->m_Shared.GetCaptures( iLocalPlayerIndex );
	out.m_iStat[ TFSTAT_DEFENSES           ] = pLocalPlayer->m_Shared.GetDefenses( iLocalPlayerIndex );
	out.m_iStat[ TFSTAT_DOMINATIONS        ] = pLocalPlayer->m_Shared.GetDominations( iLocalPlayerIndex );
	out.m_iStat[ TFSTAT_REVENGE            ] = pLocalPlayer->m_Shared.GetRevenge( iLocalPlayerIndex );
	out.m_iStat[ TFSTAT_BUILDINGSDESTROYED ] = pLocalPlayer->m_Shared.GetBuildingsDestroyed( iLocalPlayerIndex );
	out.m_iStat[ TFSTAT_HEADSHOTS          ] = pLocalPlayer->m_Shared.GetHeadshots( iLocalPlayerIndex );
	out.m_iStat[ TFSTAT_BACKSTABS          ] = pLocalPlayer->m_Shared.GetBackstabs( iLocalPlayerIndex );
	out.m_iStat[ TFSTAT_HEALING            ] = pLocalPlayer->m_Shared.GetHealPoints( iLocalPlayerIndex );
	out.m_iStat[ TFSTAT_INVULNS            ] = pLocalPlayer->m_Shared.GetInvulns( iLocalPlayerIndex );
	out.m_iStat[ TFSTAT_TELEPORTS          ] = pLocalPlayer->m_Shared.GetTeleports( iLocalPlayerIndex );
	out.m_iStat[ TFSTAT_KILLASSISTS        ] = pLocalPlayer->m_Shared.GetKillAssists( iLocalPlayerIndex );
	out.m_iStat[ TFSTAT_BONUS_POINTS       ] = pLocalPlayer->m_Shared.GetBonusPoints( iLocalPlayerIndex );

	return true;
}

const char *CTFReplay::GetStatString( int iStat ) const
{
	COMPILE_TIME_ASSERT( TFSTAT_TOTAL == ARRAYSIZE( s_pStatStrings ) );
	Assert( iStat >= TFSTAT_UNDEFINED && iStat < TFSTAT_TOTAL );
	return ClampedArrayElement( s_pStatStrings, iStat );
}

const char *CTFReplay::GetPlayerClass( int iClass ) const
{
	COMPILE_TIME_ASSERT( TF_CLASS_MENU_BUTTONS == ARRAYSIZE( g_aPlayerClassNames_NonLocalized ) );
	Assert( iClass >= TF_CLASS_UNDEFINED && iClass < TF_CLASS_COUNT );
	return ClampedArrayElement( g_aPlayerClassNames_NonLocalized, iClass );
}

bool CTFReplay::Read( KeyValues *pIn )
{
	return BaseClass::Read( pIn );
}

void CTFReplay::Write( KeyValues *pOut )
{
	BaseClass::Write( pOut );
}

const char *CTFReplay::GetMaterialFriendlyPlayerClass() const
{
	const char *pPlayerClass = BaseClass::GetMaterialFriendlyPlayerClass();
	if ( !V_stricmp( pPlayerClass, "heavyweapons" ) )
		return "heavy";

	else if ( !V_stricmp( pPlayerClass, "demoman" ) )
		return "demo";

	return pPlayerClass;
}

void CTFReplay::DumpGameSpecificData() const
{
	BaseClass::DumpGameSpecificData();
}

//----------------------------------------------------------------------------------------

class CTFReplayFactory : public IReplayFactory
{
public:
	virtual CReplay *Create()
	{
		 return new CTFReplay();
	}
};

static CTFReplayFactory s_ReplayManager;
IReplayFactory *g_pReplayFactory = &s_ReplayManager;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CTFReplayFactory, IReplayFactory, INTERFACE_VERSION_REPLAY_FACTORY, s_ReplayManager );

#endif
