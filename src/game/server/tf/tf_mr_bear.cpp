//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Rainbow Is Magic mod: team objective "Mr. Bear" prop.
//
//=============================================================================
#include "cbase.h"
#include "tf_mr_bear.h"
#include "tf_gamerules.h"
#include "entity_tfstart.h"
#include "tf_player.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "func_respawnroom.h"
#include "team_train_watcher.h"
#include "trains.h"
#include "props.h"
#include "ndebugoverlay.h"
#include "enginecallback.h"
#include "datacache/imdlcache.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_mr_bear, CTFMrBear );

IMPLEMENT_SERVERCLASS_ST( CTFMrBear, DT_TFMrBear )
END_SEND_TABLE()

#define MR_BEAR_MODEL_TEDDY		"models/props_halloween/pumpkin_loot.mdl"
#define MR_BEAR_MODEL_FALLBACK	"models/props_farm/wooden_barrel.mdl"
#define MR_BEAR_MODEL_CONE		"models/props_gameplay/orange_cone001.mdl"

static const char *s_pszRimSaxtonModels[] =
{
	"models/soldier_statue/soldier_statue.mdl",
	"models/bots/heavy/bot_heavy_boss.mdl",
	"models/props_halloween/pumpkin_loot.mdl",
	"models/player/saxton_hale/saxton_hale.mdl",
	"models/player/saxton_hale.mdl",
};

ConVar tf_rim_bear_health( "tf_rim_bear_health", "50", FCVAR_GAMEDLL, "RIM objective health (low = quick testing kills)." );
ConVar tf_rim_bear_model( "tf_rim_bear_model", "models/soldier_statue/soldier_statue.mdl", FCVAR_GAMEDLL, "Default RIM hostage model (TF2 statue; cone is fallback if missing)." );
ConVar tf_rim_bear_model_red( "tf_rim_bear_model_red", "", FCVAR_GAMEDLL, "RED hostage model (e.g. your GLaDOS .mdl path). Empty = use tf_rim_bear_model." );
ConVar tf_rim_bear_model_blue( "tf_rim_bear_model_blue", "", FCVAR_GAMEDLL, "BLU hostage model. Empty = use tf_rim_bear_model." );
ConVar tf_rim_spawn_relaxed( "tf_rim_spawn_relaxed", "0", FCVAR_GAMEDLL, "RIM: 0 = strict (no walls); 1 = looser hull only." );
ConVar tf_rim_bear_scale( "tf_rim_bear_scale", "1.0", FCVAR_GAMEDLL, "Visual size only (does not change hitbox)." );
ConVar tf_rim_bear_hull_halfwidth( "tf_rim_bear_hull_halfwidth", "14", FCVAR_GAMEDLL, "Hostage hitbox half-width (units). Cone default ~14." );
ConVar tf_rim_bear_hull_height( "tf_rim_bear_hull_height", "40", FCVAR_GAMEDLL, "Hostage hitbox height (units)." );
ConVar tf_rim_debug_bear( "tf_rim_debug_bear", "0", FCVAR_GAMEDLL, "Draw objective spawn markers." );
ConVar tf_rim_bear_beacon( "tf_rim_bear_beacon", "1", FCVAR_GAMEDLL, "Draw a tall team-colored beacon above each hostage." );
ConVar tf_rim_spawn_forward( "tf_rim_spawn_forward", "400", FCVAR_GAMEDLL, "Ideal distance from spawn toward the courtyard (units)." );
ConVar tf_rim_spawn_min_dist( "tf_rim_spawn_min_dist", "400", FCVAR_GAMEDLL, "Minimum horizontal distance from team spawn (keeps out of spawn room)." );
ConVar tf_rim_spawn_max_dist( "tf_rim_spawn_max_dist", "650", FCVAR_GAMEDLL, "Never place hostage farther than this from team spawn (avoids void/out of map)." );
ConVar tf_rim_spawn_require_nav( "tf_rim_spawn_require_nav", "0", FCVAR_GAMEDLL, "RIM: require nav reachability (0 = outdoor+ground only, works on PL maps)." );

static bool Rim_IsInRespawnTrigger( const Vector &vecOrigin, int iTeam );
static bool Rim_IsInRespawnTrigger( const Vector &vecOrigin, int iTeam );
static bool Rim_IsBlockedByRespawnRoom( const Vector &vecOrigin, int iTeam );
static bool Rim_IsInsideSolid( const Vector &vecOrigin, int iTeam );
static bool Rim_NudgeOriginOutOfRespawnRoom( int iTeam, Vector &vecOrigin, QAngle &angles );
static CTFTeamSpawn *Rim_FindTeamSpawn( int iTeam );

//-----------------------------------------------------------------------------
static bool Rim_IsHumanoidBossModel( const char *pszModel )
{
	if ( !pszModel || !pszModel[0] )
	{
		return false;
	}

	return Q_stristr( pszModel, "saxton" ) != NULL ||
		Q_stristr( pszModel, "bot_heavy_boss" ) != NULL ||
		Q_stristr( pszModel, "soldier_statue" ) != NULL ||
		Q_stristr( pszModel, "/player/" ) != NULL ||
		Q_stristr( pszModel, "models/bots/" ) != NULL;
}

//-----------------------------------------------------------------------------
static bool Rim_IsModelLoadable( const char *pszModel )
{
	if ( !pszModel || !pszModel[0] )
	{
		return false;
	}

	if ( !engine || !mdlcache )
	{
		return false;
	}

	engine->PrecacheModel( pszModel, false );

	const MDLHandle_t hModel = mdlcache->FindMDL( pszModel );
	if ( hModel == MDLHANDLE_INVALID || mdlcache->IsErrorModel( hModel ) )
	{
		return false;
	}

	return ( modelinfo->GetModelIndex( pszModel ) > 0 );
}

//-----------------------------------------------------------------------------
static bool Rim_UseBossSizedHull( const char *pszModel )
{
	if ( !pszModel )
	{
		return false;
	}

	return Q_stristr( pszModel, "saxton" ) != NULL || Q_stristr( pszModel, "bot_heavy_boss" ) != NULL;
}

//-----------------------------------------------------------------------------
static void Rim_GetObjectiveHull( const char *pszModel, float &flHalfWidth, float &flHeight )
{
	if ( Rim_UseBossSizedHull( pszModel ) )
	{
		const float flScale = tf_rim_bear_scale.GetFloat();
		flHalfWidth = 36.0f * flScale;
		flHeight = 110.0f * flScale;
	}
	else
	{
		flHalfWidth = tf_rim_bear_hull_halfwidth.GetFloat();
		flHeight = tf_rim_bear_hull_height.GetFloat();
	}
}

//-----------------------------------------------------------------------------
static const char *Rim_GetBearModelNameForTeam( int iTeam )
{
	const char *pszTeamModel = NULL;
	if ( iTeam == TF_TEAM_RED )
	{
		pszTeamModel = tf_rim_bear_model_red.GetString();
	}
	else if ( iTeam == TF_TEAM_BLUE )
	{
		pszTeamModel = tf_rim_bear_model_blue.GetString();
	}

	if ( pszTeamModel && pszTeamModel[0] && Rim_IsModelLoadable( pszTeamModel ) )
	{
		return pszTeamModel;
	}

	const char *pszRequested = tf_rim_bear_model.GetString();
	if ( pszRequested && pszRequested[0] && Rim_IsModelLoadable( pszRequested ) )
	{
		return pszRequested;
	}

	for ( int i = 0; i < ARRAYSIZE( s_pszRimSaxtonModels ); ++i )
	{
		if ( Rim_IsModelLoadable( s_pszRimSaxtonModels[i] ) )
		{
			return s_pszRimSaxtonModels[i];
		}
	}

	if ( Rim_IsModelLoadable( MR_BEAR_MODEL_TEDDY ) )
	{
		return MR_BEAR_MODEL_TEDDY;
	}

	if ( Rim_IsModelLoadable( MR_BEAR_MODEL_FALLBACK ) )
	{
		return MR_BEAR_MODEL_FALLBACK;
	}

	return MR_BEAR_MODEL_CONE;
}

//-----------------------------------------------------------------------------
static const char *Rim_GetBearModelName()
{
	return Rim_GetBearModelNameForTeam( TF_TEAM_RED );
}

//-----------------------------------------------------------------------------
static void Rim_PlayBearIdle( CTFMrBear *pBear )
{
	if ( !pBear )
	{
		return;
	}

	static const char *s_pszIdleNames[] = { "idle", "stand", "reference", "a_idle", "c_idle" };
	for ( int i = 0; i < ARRAYSIZE( s_pszIdleNames ); ++i )
	{
		const int iSequence = pBear->LookupSequence( s_pszIdleNames[i] );
		if ( iSequence >= 0 )
		{
			pBear->ResetSequence( iSequence );
			pBear->ResetSequenceInfo();
			return;
		}
	}

	pBear->ResetSequence( 0 );
}

//-----------------------------------------------------------------------------
static void Rim_SetupBearHull( CTFMrBear *pBear, const char *pszModel )
{
	if ( !pBear )
	{
		return;
	}

	float flHalfWidth = 0.0f;
	float flHeight = 0.0f;
	Rim_GetObjectiveHull( pszModel, flHalfWidth, flHeight );
	UTIL_SetSize( pBear, Vector( -flHalfWidth, -flHalfWidth, 0.0f ), Vector( flHalfWidth, flHalfWidth, flHeight ) );
}

//-----------------------------------------------------------------------------
static void Rim_SetupBearAppearance( CTFMrBear *pBear, const char *pszModel )
{
	if ( !pBear )
	{
		return;
	}

	pBear->SetModelScale( tf_rim_bear_scale.GetFloat() );
	Rim_PlayBearIdle( pBear );

	if ( pBear->GetTeamNumber() == TF_TEAM_RED )
	{
		pBear->SetRenderColor( 255, 64, 64 );
	}
	else if ( pBear->GetTeamNumber() == TF_TEAM_BLUE )
	{
		pBear->SetRenderColor( 64, 140, 255 );
	}

	if ( Rim_IsHumanoidBossModel( pszModel ) && ( pBear->GetTeamNumber() == TF_TEAM_RED || pBear->GetTeamNumber() == TF_TEAM_BLUE ) )
	{
		pBear->m_nSkin = pBear->GetTeamNumber() - TF_TEAM_RED;
	}
}

//-----------------------------------------------------------------------------
CTFMrBear::CTFMrBear()
{
	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
void CTFMrBear::Precache()
{
	const char *pszRequested = tf_rim_bear_model.GetString();
	if ( pszRequested && pszRequested[0] )
	{
		PrecacheModel( pszRequested );
	}

	const char *pszRed = tf_rim_bear_model_red.GetString();
	if ( pszRed && pszRed[0] )
	{
		PrecacheModel( pszRed );
	}

	const char *pszBlue = tf_rim_bear_model_blue.GetString();
	if ( pszBlue && pszBlue[0] )
	{
		PrecacheModel( pszBlue );
	}

	for ( int i = 0; i < ARRAYSIZE( s_pszRimSaxtonModels ); ++i )
	{
		PrecacheModel( s_pszRimSaxtonModels[i] );
	}
	PrecacheModel( MR_BEAR_MODEL_TEDDY );
	PrecacheModel( MR_BEAR_MODEL_FALLBACK );
	PrecacheModel( MR_BEAR_MODEL_CONE );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
void CTFMrBear::Spawn()
{
	Precache();

	const char *pszModel = Rim_GetBearModelNameForTeam( GetTeamNumber() );
	SetModel( pszModel );

	BaseClass::Spawn();

	m_lifeState = LIFE_ALIVE;

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_BBOX );
	m_takedamage = DAMAGE_YES;
	const int iHealth = Max( 1, tf_rim_bear_health.GetInt() );
	SetMaxHealth( iHealth );
	SetHealth( iHealth );

	Rim_SetupBearAppearance( this, pszModel );
	Rim_SetupBearHull( this, pszModel );

	AddEffects( EF_BRIGHTLIGHT );
	RemoveEffects( EF_NODRAW );

	if ( !GetModelPtr() )
	{
		Warning( "RIM: Saxton model failed to load: %s\n", pszModel );
	}

	if ( tf_rim_bear_beacon.GetBool() )
	{
		SetContextThink( &CTFMrBear::BearBeaconThink, gpGlobals->curtime + 0.1f, "RIM_BearBeacon" );
	}

	const char *pszLabel = "Hostage";
	if ( Q_stristr( pszModel, "saxton" ) != NULL )
	{
		pszLabel = "Saxton Hale";
	}
	else if ( Q_stristr( pszModel, "bot_heavy_boss" ) != NULL )
	{
		pszLabel = "Boss Heavy";
	}
	else if ( Q_stristr( pszModel, "soldier_statue" ) != NULL )
	{
		pszLabel = "Statue hostage";
	}
	else if ( Q_stristr( pszModel, "orange_cone" ) != NULL )
	{
		pszLabel = "Hostage (cone)";
	}
	FinalizeOutdoorPlacement( this );

	SetContextThink( &CTFMrBear::OutdoorVerifyThink, gpGlobals->curtime + 0.25f, "RIM_OutdoorVerify" );

	const bool bOutdoor = !Rim_IsInRespawnTrigger( GetAbsOrigin(), GetTeamNumber() );
	Msg( "RIM %s spawned for team %d at %.0f %.0f %.0f using %s (%s)\n",
		pszLabel, GetTeamNumber(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, pszModel,
		bOutdoor ? "outside respawn trigger" : "inside respawn trigger — retrying" );
}

//-----------------------------------------------------------------------------
void CTFMrBear::OutdoorVerifyThink( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsRainbowIsMagicMode() )
	{
		return;
	}

	if ( Rim_IsInRespawnTrigger( GetAbsOrigin(), GetTeamNumber() ) )
	{
		FinalizeOutdoorPlacement( this );
	}

	if ( Rim_IsInRespawnTrigger( GetAbsOrigin(), GetTeamNumber() ) )
	{
		SetNextThink( gpGlobals->curtime + 2.0f, "RIM_OutdoorVerify" );
	}
}

//-----------------------------------------------------------------------------
void CTFMrBear::BearBeaconThink( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsRainbowIsMagicMode() || !tf_rim_bear_beacon.GetBool() )
	{
		return;
	}

	const Vector vecTop = GetAbsOrigin() + Vector( 0, 0, 450.0f );
	const int r = ( GetTeamNumber() == TF_TEAM_BLUE ) ? 80 : 255;
	const int g = 80;
	const int b = ( GetTeamNumber() == TF_TEAM_BLUE ) ? 255 : 80;

	NDebugOverlay::VertArrow( GetAbsOrigin(), vecTop, 24.0f, r, g, b, 255, true, 0.15f );
	NDebugOverlay::Cross3D( GetAbsOrigin() + Vector( 0, 0, 32 ), 48.0f, r, g, b, true, 0.15f );

	SetNextThink( gpGlobals->curtime + 0.15f, "RIM_BearBeacon" );
}

//-----------------------------------------------------------------------------
int CTFMrBear::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !TFGameRules() || !TFGameRules()->IsRainbowIsMagicMode() )
	{
		return 0;
	}

	CBaseEntity *pAttacker = info.GetAttacker();
	if ( pAttacker )
	{
		pAttacker = pAttacker->GetOwnerEntity() ? pAttacker->GetOwnerEntity() : pAttacker;
	}

	if ( pAttacker && pAttacker->GetTeamNumber() == GetTeamNumber() )
	{
		return 0;
	}

	return BaseClass::OnTakeDamage( info );
}

//-----------------------------------------------------------------------------
void CTFMrBear::Event_Killed( const CTakeDamageInfo &info )
{
	if ( TFGameRules() && TFGameRules()->IsRainbowIsMagicMode() )
	{
		int iWinningTeam = GetEnemyTeam( GetTeamNumber() );
		if ( iWinningTeam == TF_TEAM_RED || iWinningTeam == TF_TEAM_BLUE )
		{
			CBaseEntity *pAttacker = info.GetAttacker();
			if ( pAttacker )
			{
				pAttacker = pAttacker->GetOwnerEntity() ? pAttacker->GetOwnerEntity() : pAttacker;
			}
			TFGameRules()->Rim_OnMrBearKilled( iWinningTeam, pAttacker );
		}
	}

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
CTFMrBear *CTFMrBear::Create( const Vector &vecOrigin, const QAngle &angles, int iTeam )
{
	CTFMrBear *pBear = static_cast<CTFMrBear *>( CreateEntityByName( "tf_mr_bear" ) );
	if ( !pBear )
	{
		return NULL;
	}

	pBear->SetAbsOrigin( vecOrigin );
	pBear->SetAbsAngles( angles );
	pBear->ChangeTeam( iTeam );
	pBear->Spawn();
	pBear->Activate();

	return pBear;
}

//-----------------------------------------------------------------------------
static CTFTeamSpawn *Rim_FindTeamSpawn( int iTeam )
{
	CTFTeamSpawn *pBestSpawn = NULL;
	for ( int i = 0; i < ITFTeamSpawnAutoList::AutoList().Count(); ++i )
	{
		CTFTeamSpawn *pSpawn = static_cast<CTFTeamSpawn *>( ITFTeamSpawnAutoList::AutoList()[i] );
		// Ignore IsDisabled — we only need a position; spawns are often disabled during setup.
		if ( !pSpawn || pSpawn->GetTeamNumber() != iTeam )
		{
			continue;
		}

		if ( !pBestSpawn )
		{
			pBestSpawn = pSpawn;
			continue;
		}

		// Prefer a spawn not tied to a control point so we work on partial captures.
		if ( pBestSpawn->GetControlPoint() && !pSpawn->GetControlPoint() )
		{
			pBestSpawn = pSpawn;
		}
	}

	// Autolist can be empty during early deferred setup; scan entities directly.
	if ( !pBestSpawn )
	{
		CBaseEntity *pEnt = NULL;
		while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "info_player_teamspawn" ) ) != NULL )
		{
			CTFTeamSpawn *pSpawn = assert_cast<CTFTeamSpawn *>( pEnt );
			if ( pSpawn && pSpawn->GetTeamNumber() == iTeam )
			{
				return pSpawn;
			}
		}
	}

	return pBestSpawn;
}

//-----------------------------------------------------------------------------
void CTFMrBear::RemoveAllMrBears( void )
{
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "tf_mr_bear" ) ) != NULL )
	{
		UTIL_Remove( pEnt );
	}
}

//-----------------------------------------------------------------------------
void CTFMrBear::RemoveAllRimObjectives( void )
{
	RemoveAllMrBears();

	for ( int iTeam = TF_TEAM_RED; iTeam <= TF_TEAM_BLUE; iTeam += ( TF_TEAM_BLUE - TF_TEAM_RED ) )
	{
		const char *pszName = ( iTeam == TF_TEAM_RED ) ? "rim_red_objective" : "rim_blue_objective";
		CBaseEntity *pObjective = gEntList.FindEntityByName( NULL, pszName );
		if ( pObjective )
		{
			UTIL_Remove( pObjective );
		}
	}
}

//-----------------------------------------------------------------------------
struct Rim_SpawnContext_t
{
	int		iTeam;
	float	flRefFloorZ;
	Vector	vecSpawnOrigin;
	QAngle	anglesRef;
};

static bool Rim_GetSpawnFloorZ( const Vector &vecNear, float &flFloorZ );
static bool Rim_TryBearSpot( Vector &vecOrigin, QAngle &angles, const Vector &vecCandidate, const Rim_SpawnContext_t &ctx, bool bLenient, bool bRelaxedZ );

//-----------------------------------------------------------------------------
static bool Rim_IsOnNavSpawnRoomVolume( const Vector &vecOrigin, int iTeam )
{
	if ( !TheTFNavMesh() )
	{
		return false;
	}

	CTFNavArea *pArea = static_cast<CTFNavArea *>( TheTFNavMesh()->GetNearestNavArea( vecOrigin ) );
	if ( !pArea )
	{
		return false;
	}

	if ( iTeam == TF_TEAM_RED && pArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_RED ) )
	{
		return true;
	}

	if ( iTeam == TF_TEAM_BLUE && pArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE ) )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// func_respawnroom trigger only (nav "spawn room" areas can extend into courtyards on PL maps).
//-----------------------------------------------------------------------------
static bool Rim_IsInRespawnTrigger( const Vector &vecOrigin, int iTeam )
{
	if ( PointInRespawnRoom( NULL, vecOrigin ) )
	{
		return true;
	}

	float flHalfWidth = 0.0f;
	float flHeight = 0.0f;
	Rim_GetObjectiveHull( Rim_GetBearModelNameForTeam( iTeam ), flHalfWidth, flHeight );

	const Vector vecSamples[] =
	{
		Vector( 0, 0, 8 ),
		Vector( 0, 0, flHeight * 0.5f ),
		Vector( flHalfWidth, 0, 8 ),
		Vector( -flHalfWidth, 0, 8 ),
		Vector( 0, flHalfWidth, 8 ),
		Vector( 0, -flHalfWidth, 8 ),
	};

	for ( int i = 0; i < ARRAYSIZE( vecSamples ); ++i )
	{
		if ( PointInRespawnRoom( NULL, vecOrigin + vecSamples[i] ) )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
static bool Rim_IsBlockedByRespawnRoom( const Vector &vecOrigin, int iTeam )
{
	if ( Rim_IsInRespawnTrigger( vecOrigin, iTeam ) )
	{
		return true;
	}

	if ( Rim_IsOnNavSpawnRoomVolume( vecOrigin, iTeam ) )
	{
		return true;
	}

	float flHalfWidth = 0.0f;
	float flHeight = 0.0f;
	Rim_GetObjectiveHull( Rim_GetBearModelNameForTeam( iTeam ), flHalfWidth, flHeight );
	const Vector vecSamples[] =
	{
		Vector( 0, 0, 8 ),
		Vector( flHalfWidth, 0, 8 ),
		Vector( -flHalfWidth, 0, 8 ),
		Vector( 0, flHalfWidth, 8 ),
		Vector( 0, -flHalfWidth, 8 ),
	};

	for ( int i = 0; i < ARRAYSIZE( vecSamples ); ++i )
	{
		if ( Rim_IsOnNavSpawnRoomVolume( vecOrigin + vecSamples[i], iTeam ) )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
static bool Rim_IsInsideSolid( const Vector &vecOrigin, int iTeam )
{
	const int iContents = UTIL_PointContents( vecOrigin + Vector( 0, 0, 16.0f ) );
	if ( iContents & MASK_PLAYERSOLID )
	{
		return true;
	}

	float flHalfWidth = 0.0f;
	float flHeight = 0.0f;
	Rim_GetObjectiveHull( Rim_GetBearModelNameForTeam( iTeam ), flHalfWidth, flHeight );
	const Vector vMins( -flHalfWidth, -flHalfWidth, 0.0f );
	const Vector vMaxs( flHalfWidth, flHalfWidth, flHeight );

	trace_t tr;
	UTIL_TraceHull( vecOrigin, vecOrigin, vMins, vMaxs, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
	return ( tr.startsolid || tr.allsolid );
}

//-----------------------------------------------------------------------------
static bool Rim_NudgeOriginOutOfRespawnRoom( int iTeam, Vector &vecOrigin, QAngle &angles )
{
	CTFTeamSpawn *pSpawn = Rim_FindTeamSpawn( iTeam );
	if ( !pSpawn )
	{
		return false;
	}

	Vector vForward, vRight;
	AngleVectors( pSpawn->GetAbsAngles(), &vForward, &vRight, NULL );
	angles = pSpawn->GetAbsAngles();

	const Vector vecStart = pSpawn->GetAbsOrigin();
	Rim_SpawnContext_t ctx;
	ctx.iTeam = iTeam;
	ctx.vecSpawnOrigin = vecStart;
	ctx.anglesRef = angles;
	if ( !Rim_GetSpawnFloorZ( ctx.vecSpawnOrigin, ctx.flRefFloorZ ) )
	{
		ctx.flRefFloorZ = vecStart.z;
	}

	for ( float flDist = tf_rim_spawn_min_dist.GetFloat(); flDist <= tf_rim_spawn_max_dist.GetFloat(); flDist += 48.0f )
	{
		static const float flSideOffsets[] = { 0.0f, 96.0f, -96.0f, 192.0f, -192.0f };
		for ( int s = 0; s < ARRAYSIZE( flSideOffsets ); ++s )
		{
			const Vector vecCandidate = vecStart + vForward * flDist + vRight * flSideOffsets[s];
			if ( Rim_TryBearSpot( vecOrigin, angles, vecCandidate, ctx, false, false ) )
			{
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
static bool Rim_HasGroundUnderfoot( const Vector &vecOrigin )
{
	trace_t tr;
	UTIL_TraceLine( vecOrigin + Vector( 0, 0, 4.0f ), vecOrigin - Vector( 0, 0, 128.0f ), MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
	return ( tr.fraction < 1.0f && tr.plane.normal.z >= 0.65f );
}

//-----------------------------------------------------------------------------
static bool Rim_IsReachableByTeams( const Vector &vecOrigin, int iOwnerTeam )
{
	if ( !tf_rim_spawn_require_nav.GetBool() )
	{
		return true;
	}

	if ( !TheTFNavMesh() )
	{
		return Rim_HasGroundUnderfoot( vecOrigin );
	}

	CTFNavArea *pArea = static_cast<CTFNavArea *>( TheTFNavMesh()->GetNearestNavArea( vecOrigin ) );
	if ( !pArea )
	{
		return false;
	}

	const int iEnemyTeam = GetEnemyTeam( iOwnerTeam );
	if ( !pArea->IsReachableByTeam( iEnemyTeam ) )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
static bool Rim_IsPlaygroundSpot( const Vector &vecOrigin, const Rim_SpawnContext_t &ctx )
{
	if ( Rim_IsBlockedByRespawnRoom( vecOrigin, ctx.iTeam ) )
	{
		return false;
	}

	const float flDist2D = ( vecOrigin.AsVector2D() - ctx.vecSpawnOrigin.AsVector2D() ).Length();
	if ( flDist2D < tf_rim_spawn_min_dist.GetFloat() )
	{
		return false;
	}

	if ( flDist2D > tf_rim_spawn_max_dist.GetFloat() )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
static bool Rim_GetSpawnFloorZ( const Vector &vecNear, float &flFloorZ )
{
	trace_t tr;
	UTIL_TraceLine( vecNear + Vector( 0, 0, 32.0f ), vecNear - Vector( 0, 0, 512.0f ), MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction >= 1.0f || tr.plane.normal.z < 0.7f )
	{
		flFloorZ = vecNear.z;
		return false;
	}

	flFloorZ = tr.endpos.z;
	return true;
}

//-----------------------------------------------------------------------------
static bool Rim_FindWalkableGround( Vector &vecOrigin, float flRefFloorZ, bool bRelaxedZ )
{
	trace_t tr;
	const Vector vecStart( vecOrigin.x, vecOrigin.y, flRefFloorZ + 768.0f );
	const Vector vecEnd( vecOrigin.x, vecOrigin.y, flRefFloorZ - 768.0f );
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction >= 1.0f || tr.plane.normal.z < 0.7f )
	{
		return false;
	}

	vecOrigin = tr.endpos;

	const float flMaxUp = bRelaxedZ ? 192.0f : 96.0f;
	const float flMaxDown = bRelaxedZ ? 384.0f : 128.0f;
	if ( vecOrigin.z > flRefFloorZ + flMaxUp )
	{
		return false;
	}

	if ( vecOrigin.z < flRefFloorZ - flMaxDown )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
static bool Rim_IsClearForBear( const Vector &vecOrigin, int iTeam )
{
	if ( Rim_IsInsideSolid( vecOrigin, iTeam ) )
	{
		return false;
	}

	float flHalfWidth = 0.0f;
	float flHeight = 0.0f;
	Rim_GetObjectiveHull( Rim_GetBearModelNameForTeam( iTeam ), flHalfWidth, flHeight );
	const Vector vMins( -flHalfWidth, -flHalfWidth, 0.0f );
	const Vector vMaxs( flHalfWidth, flHalfWidth, flHeight );

	trace_t tr;
	UTIL_TraceHull( vecOrigin, vecOrigin, vMins, vMaxs, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
	if ( tr.startsolid || tr.allsolid )
	{
		return false;
	}

	if ( !tf_rim_spawn_relaxed.GetBool() && tr.fraction < 1.0f )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
static bool Rim_IsValidOutdoorObjectiveSpot( const Vector &vecOrigin, const Rim_SpawnContext_t &ctx, bool bLenient )
{
	if ( Rim_IsBlockedByRespawnRoom( vecOrigin, ctx.iTeam ) )
	{
		return false;
	}

	if ( !Rim_IsPlaygroundSpot( vecOrigin, ctx ) )
	{
		return false;
	}

	if ( !Rim_HasGroundUnderfoot( vecOrigin ) )
	{
		return false;
	}

	if ( !Rim_IsReachableByTeams( vecOrigin, ctx.iTeam ) )
	{
		return false;
	}

	return Rim_IsClearForBear( vecOrigin, ctx.iTeam );
}

//-----------------------------------------------------------------------------
static bool Rim_IsValidPlacement( const Vector &vecOrigin, const Rim_SpawnContext_t &ctx )
{
	return Rim_IsValidOutdoorObjectiveSpot( vecOrigin, ctx, false );
}

//-----------------------------------------------------------------------------
static bool Rim_TryBearSpot( Vector &vecOrigin, QAngle &angles, const Vector &vecCandidate, const Rim_SpawnContext_t &ctx, bool bLenient, bool bRelaxedZ )
{
	vecOrigin.x = vecCandidate.x;
	vecOrigin.y = vecCandidate.y;
	vecOrigin.z = ctx.flRefFloorZ + 768.0f;
	if ( !Rim_FindWalkableGround( vecOrigin, ctx.flRefFloorZ, bLenient || bRelaxedZ ) )
	{
		return false;
	}

	vecOrigin.z += 4.0f;

	if ( !Rim_IsValidOutdoorObjectiveSpot( vecOrigin, ctx, bLenient ) )
	{
		return false;
	}

	angles = ctx.anglesRef;
	return true;
}

//-----------------------------------------------------------------------------
static bool Rim_IsValidEmergencySpot( const Vector &vecOrigin, const Rim_SpawnContext_t &ctx )
{
	const float flDist2D = ( vecOrigin.AsVector2D() - ctx.vecSpawnOrigin.AsVector2D() ).Length();
	if ( flDist2D < tf_rim_spawn_min_dist.GetFloat() * 0.5f || flDist2D > tf_rim_spawn_max_dist.GetFloat() )
	{
		return false;
	}

	return Rim_IsValidPlacement( vecOrigin, ctx );
}

//-----------------------------------------------------------------------------
static bool Rim_TryEmergencySpot( Vector &vecOrigin, QAngle &angles, const Vector &vecCandidate, const Rim_SpawnContext_t &ctx )
{
	vecOrigin.x = vecCandidate.x;
	vecOrigin.y = vecCandidate.y;
	vecOrigin.z = ctx.flRefFloorZ + 768.0f;
	if ( !Rim_FindWalkableGround( vecOrigin, ctx.flRefFloorZ, true ) )
	{
		return false;
	}

	vecOrigin.z += 4.0f;

	if ( !Rim_IsValidEmergencySpot( vecOrigin, ctx ) )
	{
		return false;
	}

	angles = ctx.anglesRef;
	return true;
}

//-----------------------------------------------------------------------------
// Last resort: courtyard in front of spawn — never inside respawn room, must have ground.
static bool Rim_TryEmergencyOutdoorSpawn( int iTeam, Vector &vecOrigin, QAngle &angles, const Rim_SpawnContext_t &ctx )
{
	Vector vForward, vRight, vUp;
	AngleVectors( ctx.anglesRef, &vForward, &vRight, &vUp );

	static const float flDistances[] = { 128.0f, 200.0f, 280.0f, 360.0f, 440.0f, 520.0f, 600.0f };
	static const float flSideOffsets[] = { 0.0f, 64.0f, -64.0f, 128.0f, -128.0f };

	for ( int d = 0; d < ARRAYSIZE( flDistances ); ++d )
	{
		for ( int s = 0; s < ARRAYSIZE( flSideOffsets ); ++s )
		{
			const Vector vecCandidate = ctx.vecSpawnOrigin + vForward * flDistances[d] + vRight * flSideOffsets[s];
			if ( Rim_TryEmergencySpot( vecOrigin, angles, vecCandidate, ctx ) )
			{
				Msg( "RIM: team %d hostage emergency spawn at %.0f %.0f %.0f\n",
					iTeam, vecOrigin.x, vecOrigin.y, vecOrigin.z );
				return true;
			}
		}
	}

	for ( int iDir = 0; iDir < 8; ++iDir )
	{
		const float flYaw = ( iDir / 8.0f ) * 360.0f;
		QAngle angDir( 0, flYaw, 0 );
		AngleVectors( angDir, &vForward, &vRight, &vUp );
		const Vector vecCandidate = ctx.vecSpawnOrigin + vForward * 280.0f;
		if ( Rim_TryEmergencySpot( vecOrigin, angles, vecCandidate, ctx ) )
		{
			Msg( "RIM: team %d hostage compass fallback at %.0f %.0f %.0f\n",
				iTeam, vecOrigin.x, vecOrigin.y, vecOrigin.z );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
static void Rim_DebugBearSpot( const Vector &vecOrigin, int r, int g, int b )
{
	if ( !tf_rim_debug_bear.GetBool() )
	{
		return;
	}

	NDebugOverlay::Box( vecOrigin, Vector( -40, -40, 0 ), Vector( 40, 40, 100 ), r, g, b, 64, 30.0f );
	NDebugOverlay::Cross3D( vecOrigin + Vector( 0, 0, 50 ), 32.0f, r, g, b, true, 30.0f );
}

//-----------------------------------------------------------------------------
static float Rim_ScoreSpawnCandidate( const Vector &vecCenter, const Rim_SpawnContext_t &ctx )
{
	const float flDist2D = ( vecCenter.AsVector2D() - ctx.vecSpawnOrigin.AsVector2D() ).Length();
	const float flZDiff = fabsf( vecCenter.z - ctx.flRefFloorZ );
	return -fabsf( flDist2D - tf_rim_spawn_forward.GetFloat() ) - ( flZDiff * 0.5f );
}

//-----------------------------------------------------------------------------
static bool Rim_TryNavAreaList( CUtlVector< CTFNavArea * > &areaVector, Vector &vecOrigin, QAngle &angles, const Rim_SpawnContext_t &ctx, bool bLenient )
{
	for ( int pass = 0; pass < areaVector.Count(); ++pass )
	{
		int iBest = pass;
		float flBestScore = Rim_ScoreSpawnCandidate( areaVector[pass]->GetCenter(), ctx );
		for ( int j = pass + 1; j < areaVector.Count(); ++j )
		{
			const float flScore = Rim_ScoreSpawnCandidate( areaVector[j]->GetCenter(), ctx );
			if ( flScore > flBestScore )
			{
				flBestScore = flScore;
				iBest = j;
			}
		}

		if ( iBest != pass )
		{
			V_swap( areaVector[pass], areaVector[iBest] );
		}

		CTFNavArea *pArea = areaVector[pass];
		if ( !pArea )
		{
			continue;
		}

		if ( pArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_RED | TF_NAV_SPAWN_ROOM_BLUE ) )
		{
			continue;
		}

		const Vector vecCandidate = pArea->GetCenter();
		if ( Rim_TryBearSpot( vecOrigin, angles, vecCandidate, ctx, false, false ) )
		{
			Rim_DebugBearSpot( vecOrigin, 0, 255, 0 );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
static bool Rim_TrySpawnExitNavAreas( int iTeam, Vector &vecOrigin, QAngle &angles, const Rim_SpawnContext_t &ctx )
{
	if ( !TheTFNavMesh() )
	{
		return false;
	}

	const CUtlVector< CTFNavArea * > *pExitAreas = TheTFNavMesh()->GetSpawnRoomExitAreas( iTeam );
	if ( !pExitAreas || pExitAreas->Count() == 0 )
	{
		return false;
	}

	CUtlVector< CTFNavArea * > areaVector;
	for ( int i = 0; i < pExitAreas->Count(); ++i )
	{
		CTFNavArea *pArea = ( *pExitAreas )[i];
		if ( !pArea )
		{
			continue;
		}

		if ( pArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_RED | TF_NAV_SPAWN_ROOM_BLUE ) )
		{
			continue;
		}

		if ( tf_rim_spawn_require_nav.GetBool() )
		{
			const int iEnemyTeam = GetEnemyTeam( iTeam );
			if ( !pArea->IsReachableByTeam( iEnemyTeam ) )
			{
				continue;
			}
		}

		areaVector.AddToTail( pArea );
	}

	if ( areaVector.Count() == 0 )
	{
		return false;
	}

	return Rim_TryNavAreaList( areaVector, vecOrigin, angles, ctx, false );
}

//-----------------------------------------------------------------------------
static bool Rim_TryThresholdAreasForTeam( int iTeam, Vector &vecOrigin, QAngle &angles, const Rim_SpawnContext_t &ctx )
{
	if ( !TheTFNavMesh() )
	{
		return false;
	}

	CUtlVector< CTFNavArea * > areaVector;
	TheTFNavMesh()->CollectSpawnRoomThresholdAreas( &areaVector, iTeam );
	if ( areaVector.Count() == 0 )
	{
		return false;
	}

	return Rim_TryNavAreaList( areaVector, vecOrigin, angles, ctx, false );
}

//-----------------------------------------------------------------------------
static bool Rim_TrySpawnExitOffsets( const Rim_SpawnContext_t &ctx, Vector &vecOrigin, QAngle &angles, bool bLenient )
{
	Vector vForward, vRight, vUp;
	AngleVectors( ctx.anglesRef, &vForward, &vRight, &vUp );

	const float flDefaultForward = tf_rim_spawn_forward.GetFloat();
	static const float flForwardOffsets[] = { 0.0f, 40.0f, -40.0f, 80.0f, -80.0f, 120.0f, -120.0f, 160.0f, -160.0f, 200.0f };
	static const float flSideOffsets[] = { 0.0f, 48.0f, -48.0f, 96.0f, -96.0f };

	for ( int f = 0; f < ARRAYSIZE( flForwardOffsets ); ++f )
	{
		for ( int s = 0; s < ARRAYSIZE( flSideOffsets ); ++s )
		{
			const Vector vecCandidate = ctx.vecSpawnOrigin + vForward * ( flDefaultForward + flForwardOffsets[f] ) + vRight * flSideOffsets[s];
			if ( Rim_TryBearSpot( vecOrigin, angles, vecCandidate, ctx, bLenient, false ) )
			{
				Rim_DebugBearSpot( vecOrigin, 0, 200, 255 );
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
static bool Rim_TrySpawnOffsets( CTFTeamSpawn *pSpawn, Vector &vecOrigin, QAngle &angles, const Rim_SpawnContext_t &ctx )
{
	Vector vForward, vRight, vUp;
	AngleVectors( pSpawn->GetAbsAngles(), &vForward, &vRight, &vUp );

	Rim_SpawnContext_t localCtx = ctx;
	localCtx.vecSpawnOrigin = pSpawn->GetAbsOrigin();
	localCtx.anglesRef = pSpawn->GetAbsAngles();
	Rim_GetSpawnFloorZ( localCtx.vecSpawnOrigin, localCtx.flRefFloorZ );

	static const float flForwardOffsets[] = { 200.0f, 160.0f, 240.0f, 120.0f };
	static const float flSideOffsets[] = { 0.0f, 64.0f, -64.0f };

	for ( int f = 0; f < ARRAYSIZE( flForwardOffsets ); ++f )
	{
		for ( int s = 0; s < ARRAYSIZE( flSideOffsets ); ++s )
		{
			const Vector vecCandidate = localCtx.vecSpawnOrigin + vForward * flForwardOffsets[f] + vRight * flSideOffsets[s];
			if ( Rim_TryBearSpot( vecOrigin, angles, vecCandidate, localCtx, false, false ) )
			{
				Rim_DebugBearSpot( vecOrigin, 0, 200, 255 );
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
static bool Rim_FindMapPlacedSpawn( int iTeam, Vector &vecOrigin, QAngle &angles )
{
	const char *pszName = ( iTeam == TF_TEAM_RED ) ? "rim_red_hostage" : "rim_blue_hostage";
	CBaseEntity *pMarker = gEntList.FindEntityByName( NULL, pszName );
	if ( !pMarker )
	{
		return false;
	}

	vecOrigin = pMarker->GetAbsOrigin();
	angles = pMarker->GetAbsAngles();
	if ( Rim_IsBlockedByRespawnRoom( vecOrigin, iTeam ) )
	{
		Warning( "RIM: map entity %s is inside a respawn room — move it outdoors so enemies can attack it.\n", pszName );
		return false;
	}

	if ( !Rim_HasGroundUnderfoot( vecOrigin ) )
	{
		Warning( "RIM: map entity %s is not on solid ground.\n", pszName );
		return false;
	}

	Msg( "RIM: using map-placed hostage %s at %.0f %.0f %.0f\n", pszName, vecOrigin.x, vecOrigin.y, vecOrigin.z );
	return true;
}

//-----------------------------------------------------------------------------
static bool Rim_TraceGroundAtXY( const Vector &vecXY, float flRefZ, Vector &vecGround )
{
	trace_t tr;
	const Vector vecStart( vecXY.x, vecXY.y, flRefZ + 4096.0f );
	const Vector vecEnd( vecXY.x, vecXY.y, flRefZ - 4096.0f );
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction >= 1.0f || tr.plane.normal.z < 0.5f )
	{
		return false;
	}

	vecGround = tr.endpos + Vector( 0, 0, 8.0f );
	return true;
}

//-----------------------------------------------------------------------------
// Outside func_respawnroom trigger with solid ground (ignore nav spawn flags on PL maps).
static bool Rim_IsOutdoorHostageSpot( const Vector &vecOrigin, int iTeam )
{
	if ( Rim_IsInRespawnTrigger( vecOrigin, iTeam ) )
	{
		return false;
	}

	if ( Rim_IsInsideSolid( vecOrigin, iTeam ) )
	{
		return false;
	}

	return Rim_HasGroundUnderfoot( vecOrigin );
}

//-----------------------------------------------------------------------------
bool CTFMrBear::FindGuaranteedSpawnLocation( int iTeam, Vector &vecOrigin, QAngle &angles )
{
	CTFTeamSpawn *pSpawn = Rim_FindTeamSpawn( iTeam );
	if ( !pSpawn )
	{
		return false;
	}

	angles = pSpawn->GetAbsAngles();
	Vector vForward, vRight;
	AngleVectors( angles, &vForward, &vRight, NULL );

	float flRefZ = pSpawn->GetAbsOrigin().z;
	Rim_GetSpawnFloorZ( pSpawn->GetAbsOrigin(), flRefZ );

	const Vector &vecSpawn = pSpawn->GetAbsOrigin();
	const float flMinOutdoorDist = Max( 280.0f, tf_rim_spawn_min_dist.GetFloat() );

	// Forward from main spawn door — works on 2fort, hightower, most stock maps.
	for ( float flDist = flMinOutdoorDist; flDist <= 1200.0f; flDist += 40.0f )
	{
		static const float flSides[] = { 0.0f, 72.0f, -72.0f, 144.0f, -144.0f };
		for ( int s = 0; s < ARRAYSIZE( flSides ); ++s )
		{
			const Vector vecXY = vecSpawn + vForward * flDist + vRight * flSides[s];
			Vector vecGround;
			if ( Rim_TraceGroundAtXY( vecXY, flRefZ, vecGround ) && Rim_IsOutdoorHostageSpot( vecGround, iTeam ) )
			{
				vecOrigin = vecGround;
				Msg( "RIM GUARANTEED team %d forward %.0f at %.0f %.0f %.0f\n", iTeam, flDist, vecOrigin.x, vecOrigin.y, vecOrigin.z );
				return true;
			}
		}
	}

	// Full circle around spawn.
	for ( int iStep = 0; iStep < 16; ++iStep )
	{
		const float flYaw = ( iStep / 16.0f ) * 360.0f;
		QAngle angDir( 0.0f, flYaw, 0.0f );
		AngleVectors( angDir, &vForward, &vRight, NULL );

		for ( float flDist = 200.0f; flDist <= 1000.0f; flDist += 80.0f )
		{
			const Vector vecXY = vecSpawn + vForward * flDist;
			Vector vecGround;
			if ( Rim_TraceGroundAtXY( vecXY, flRefZ, vecGround ) && Rim_IsOutdoorHostageSpot( vecGround, iTeam ) )
			{
				vecOrigin = vecGround;
				angles = angDir;
				Msg( "RIM GUARANTEED team %d ring yaw %.0f at %.0f %.0f %.0f (outdoor)\n", iTeam, flYaw, vecOrigin.x, vecOrigin.y, vecOrigin.z );
				return true;
			}
		}
	}

	// Every team spawn entity on this team.
	for ( int i = 0; i < ITFTeamSpawnAutoList::AutoList().Count(); ++i )
	{
		CTFTeamSpawn *pTeamSpawn = static_cast<CTFTeamSpawn *>( ITFTeamSpawnAutoList::AutoList()[i] );
		if ( !pTeamSpawn || pTeamSpawn->GetTeamNumber() != iTeam )
		{
			continue;
		}

		AngleVectors( pTeamSpawn->GetAbsAngles(), &vForward, &vRight, NULL );
		flRefZ = pTeamSpawn->GetAbsOrigin().z;
		Rim_GetSpawnFloorZ( pTeamSpawn->GetAbsOrigin(), flRefZ );

		for ( float flDist = 200.0f; flDist <= 900.0f; flDist += 60.0f )
		{
			const Vector vecXY = pTeamSpawn->GetAbsOrigin() + vForward * flDist;
			Vector vecGround;
			if ( Rim_TraceGroundAtXY( vecXY, flRefZ, vecGround ) && Rim_IsOutdoorHostageSpot( vecGround, iTeam ) )
			{
				vecOrigin = vecGround;
				angles = pTeamSpawn->GetAbsAngles();
				Msg( "RIM GUARANTEED team %d alt spawn at %.0f %.0f %.0f (outdoor)\n", iTeam, vecOrigin.x, vecOrigin.y, vecOrigin.z );
				return true;
			}
		}
	}

	// Force placement: march forward until fully outside spawn room + nav spawn volumes.
	for ( float flDist = flMinOutdoorDist; flDist <= 1500.0f; flDist += 32.0f )
	{
		const Vector vecXY = vecSpawn + vForward * flDist;
		Vector vecGround;
		if ( Rim_TraceGroundAtXY( vecXY, flRefZ, vecGround ) && Rim_IsOutdoorHostageSpot( vecGround, iTeam ) )
		{
			vecOrigin = vecGround;
			Warning( "RIM FORCE team %d outdoors at %.0f %.0f %.0f (dist %.0f)\n",
				iTeam, vecOrigin.x, vecOrigin.y, vecOrigin.z, flDist );
			return true;
		}
	}

	// Panic: try many directions from spawn (large PL/Hightower bases).
	for ( int iDir = 0; iDir < 16; ++iDir )
	{
		QAngle angTry( 0.0f, angles.y + ( iDir * 22.5f ), 0.0f );
		AngleVectors( angTry, &vForward, &vRight, NULL );

		for ( float flDist = flMinOutdoorDist; flDist <= 1800.0f; flDist += 48.0f )
		{
			const Vector vecXY = vecSpawn + vForward * flDist;
			Vector vecGround;
			if ( Rim_TraceGroundAtXY( vecXY, flRefZ, vecGround ) && Rim_IsOutdoorHostageSpot( vecGround, iTeam ) )
			{
				vecOrigin = vecGround;
				angles = angTry;
				Warning( "RIM PANIC team %d outdoors at %.0f %.0f %.0f (yaw %.0f dist %.0f)\n",
					iTeam, vecOrigin.x, vecOrigin.y, vecOrigin.z, angTry.y, flDist );
				return true;
			}
		}
	}

	if ( Rim_NudgeOriginOutOfRespawnRoom( iTeam, vecOrigin, angles ) &&
		!Rim_IsInRespawnTrigger( vecOrigin, iTeam ) )
	{
		Warning( "RIM NUDGE team %d outdoors at %.0f %.0f %.0f\n",
			iTeam, vecOrigin.x, vecOrigin.y, vecOrigin.z );
		return true;
	}

	// Relaxed: clear of respawn trigger + ground (Hightower courtyards often fail hull-solid).
	for ( int iDir = 0; iDir < 24; ++iDir )
	{
		QAngle angTry( 0.0f, angles.y + ( iDir * 15.0f ), 0.0f );
		AngleVectors( angTry, &vForward, &vRight, NULL );

		for ( float flDist = flMinOutdoorDist; flDist <= 2200.0f; flDist += 56.0f )
		{
			const Vector vecXY = vecSpawn + vForward * flDist;
			Vector vecGround;
			if ( Rim_TraceGroundAtXY( vecXY, flRefZ, vecGround ) &&
				!Rim_IsInRespawnTrigger( vecGround, iTeam ) &&
				Rim_HasGroundUnderfoot( vecGround ) )
			{
				vecOrigin = vecGround;
				angles = angTry;
				Warning( "RIM RELAXED team %d at %.0f %.0f %.0f (dist %.0f)\n",
					iTeam, vecOrigin.x, vecOrigin.y, vecOrigin.z, flDist );
				return true;
			}
		}
	}

	// Entity still spawns; OutdoorVerifyThink keeps hunting for a valid courtyard spot.
	vecOrigin = vecSpawn + vForward * 768.0f;
	Rim_GetSpawnFloorZ( vecOrigin, vecOrigin.z );
	vecOrigin.z += 8.0f;
	Warning( "RIM WARN team %d hostage placed at %.0f %.0f %.0f — still hunting outdoor spot\n",
		iTeam, vecOrigin.x, vecOrigin.y, vecOrigin.z );
	return true;
}

//-----------------------------------------------------------------------------
bool CTFMrBear::FinalizeOutdoorPlacement( CTFMrBear *pBear )
{
	if ( !pBear )
	{
		return false;
	}

	const int iTeam = pBear->GetTeamNumber();
	if ( !Rim_IsInRespawnTrigger( pBear->GetAbsOrigin(), iTeam ) )
	{
		return true;
	}

	Vector vecFixed;
	QAngle angFixed;
	for ( int iPass = 0; iPass < 4; ++iPass )
	{
		if ( FindSpawnLocation( iTeam, vecFixed, angFixed ) && !Rim_IsInRespawnTrigger( vecFixed, iTeam ) )
		{
			pBear->SetAbsOrigin( vecFixed );
			pBear->SetAbsAngles( angFixed );
			Warning( "RIM: team %d hostage moved outdoors to %.0f %.0f %.0f\n",
				iTeam, vecFixed.x, vecFixed.y, vecFixed.z );
			return true;
		}

		if ( FindGuaranteedSpawnLocation( iTeam, vecFixed, angFixed ) && !Rim_IsInRespawnTrigger( vecFixed, iTeam ) )
		{
			pBear->SetAbsOrigin( vecFixed );
			pBear->SetAbsAngles( angFixed );
			Warning( "RIM: team %d hostage moved outdoors to %.0f %.0f %.0f\n",
				iTeam, vecFixed.x, vecFixed.y, vecFixed.z );
			return true;
		}

		if ( Rim_NudgeOriginOutOfRespawnRoom( iTeam, vecFixed, angFixed ) &&
			!Rim_IsInRespawnTrigger( vecFixed, iTeam ) )
		{
			pBear->SetAbsOrigin( vecFixed );
			pBear->SetAbsAngles( angFixed );
			Warning( "RIM: team %d hostage nudged outdoors to %.0f %.0f %.0f\n",
				iTeam, vecFixed.x, vecFixed.y, vecFixed.z );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
bool CTFMrBear::FindSpawnLocation( int iTeam, Vector &vecOrigin, QAngle &angles )
{
	if ( Rim_FindMapPlacedSpawn( iTeam, vecOrigin, angles ) )
	{
		return true;
	}

	CTFTeamSpawn *pSpawn = Rim_FindTeamSpawn( iTeam );
	if ( !pSpawn )
	{
		Warning( "RIM mode: no team spawns for team %d.\n", iTeam );
		return false;
	}

	Rim_SpawnContext_t ctx;
	ctx.iTeam = iTeam;
	ctx.vecSpawnOrigin = pSpawn->GetAbsOrigin();
	ctx.anglesRef = pSpawn->GetAbsAngles();
	if ( !Rim_GetSpawnFloorZ( ctx.vecSpawnOrigin, ctx.flRefFloorZ ) )
	{
		ctx.flRefFloorZ = ctx.vecSpawnOrigin.z;
	}

	// 1) Nav courtyard just outside spawn (TF marks these areas on stock maps).
	if ( Rim_TryThresholdAreasForTeam( iTeam, vecOrigin, angles, ctx ) )
	{
		Msg( "RIM: team %d hostage in courtyard (nav threshold)\n", iTeam );
		return true;
	}

	if ( Rim_TrySpawnExitNavAreas( iTeam, vecOrigin, angles, ctx ) )
	{
		Msg( "RIM: team %d hostage at spawn exit (nav)\n", iTeam );
		return true;
	}

	// 2) Strict forward march from spawn door (no nav on map).
	Vector vForward, vRight;
	AngleVectors( ctx.anglesRef, &vForward, &vRight, NULL );
	for ( float flDist = tf_rim_spawn_min_dist.GetFloat(); flDist <= tf_rim_spawn_max_dist.GetFloat(); flDist += 48.0f )
	{
		static const float flSideOffsets[] = { 0.0f, 64.0f, -64.0f, 128.0f, -128.0f };
		for ( int s = 0; s < ARRAYSIZE( flSideOffsets ); ++s )
		{
			const Vector vecCandidate = ctx.vecSpawnOrigin + vForward * flDist + vRight * flSideOffsets[s];
			if ( Rim_TryBearSpot( vecOrigin, angles, vecCandidate, ctx, false, false ) )
			{
				Msg( "RIM: team %d hostage forward of spawn at %.0f %.0f %.0f\n",
					iTeam, vecOrigin.x, vecOrigin.y, vecOrigin.z );
				return true;
			}
		}
	}

	if ( Rim_TryEmergencyOutdoorSpawn( iTeam, vecOrigin, angles, ctx ) )
	{
		return true;
	}

	return FindGuaranteedSpawnLocation( iTeam, vecOrigin, angles );
}

//-----------------------------------------------------------------------------
bool CTFMrBear::FindRelaxedSpawnLocation( int iTeam, Vector &vecOrigin, QAngle &angles )
{
	CTFTeamSpawn *pSpawn = Rim_FindTeamSpawn( iTeam );
	if ( !pSpawn )
	{
		return false;
	}

	Rim_SpawnContext_t ctx;
	ctx.iTeam = iTeam;
	ctx.vecSpawnOrigin = pSpawn->GetAbsOrigin();
	ctx.anglesRef = pSpawn->GetAbsAngles();
	if ( !Rim_GetSpawnFloorZ( ctx.vecSpawnOrigin, ctx.flRefFloorZ ) )
	{
		ctx.flRefFloorZ = ctx.vecSpawnOrigin.z;
	}

	Vector vForward, vRight;
	AngleVectors( ctx.anglesRef, &vForward, &vRight, NULL );

	for ( float flDist = tf_rim_spawn_min_dist.GetFloat(); flDist <= tf_rim_spawn_max_dist.GetFloat(); flDist += 32.0f )
	{
		static const float flSideOffsets[] = { 0.0f, 96.0f, -96.0f, 192.0f, -192.0f, 288.0f };
		for ( int s = 0; s < ARRAYSIZE( flSideOffsets ); ++s )
		{
			const Vector vecCandidate = ctx.vecSpawnOrigin + vForward * flDist + vRight * flSideOffsets[s];
			if ( Rim_TryBearSpot( vecOrigin, angles, vecCandidate, ctx, true, true ) )
			{
				Msg( "RIM: relaxed outdoor spawn team %d at %.0f %.0f %.0f\n", iTeam, vecOrigin.x, vecOrigin.y, vecOrigin.z );
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
extern ConVar tf_rim_use_payload;

//-----------------------------------------------------------------------------
static CBaseEntity *Rim_FindCartModelOnTrain( CFuncTrackTrain *pTrain )
{
	if ( !pTrain )
	{
		return NULL;
	}

	for ( CBaseEntity *pChild = pTrain->FirstMoveChild(); pChild; pChild = pChild->NextMovePeer() )
	{
		if ( FStrEq( pChild->GetClassname(), "prop_dynamic" ) ||
			FStrEq( pChild->GetClassname(), "prop_dynamic_override" ) )
		{
			return pChild;
		}
	}

	return pTrain;
}

//-----------------------------------------------------------------------------
static void Rim_FreezePayloadWatcher( CTeamTrainWatcher *pWatcher )
{
	if ( !pWatcher || pWatcher->IsDisabled() )
	{
		return;
	}

	inputdata_t data;
	pWatcher->InputDisable( data );

	CFuncTrackTrain *pTrain = dynamic_cast<CFuncTrackTrain *>( pWatcher->GetTrainEntity() );
	if ( !pTrain )
	{
		return;
	}

	pTrain->Stop();
	pTrain->SetSpeed( 0 );
}

//-----------------------------------------------------------------------------
static void Rim_MakePayloadCartKillable( CBaseEntity *pEnt, int iTeam )
{
	if ( !pEnt )
	{
		return;
	}

	const int iHealth = tf_rim_bear_health.GetInt();

	pEnt->ChangeTeam( iTeam );
	pEnt->m_takedamage = DAMAGE_YES;
	pEnt->SetHealth( iHealth );
	pEnt->SetMaxHealth( iHealth );

	CBaseAnimating *pAnim = pEnt->GetBaseAnimating();
	if ( pAnim )
	{
		pAnim->SetModelScale( tf_rim_bear_scale.GetFloat() );
	}

	CFuncTrackTrain *pTrain = dynamic_cast<CFuncTrackTrain *>( pEnt );
	if ( pTrain )
	{
		pTrain->SetDamageChild( true );
		Rim_MakePayloadCartKillable( Rim_FindCartModelOnTrain( pTrain ), iTeam );
	}
}

//-----------------------------------------------------------------------------
static CBaseEntity *Rim_SpawnOutdoorPayloadCart( int iTeam, const char *pszModel )
{
	Vector vecOrigin;
	QAngle angles;
	if ( !CTFMrBear::FindSpawnLocation( iTeam, vecOrigin, angles ) )
	{
		return NULL;
	}

	CDynamicProp *pProp = dynamic_cast<CDynamicProp *>( CreateEntityByName( "prop_dynamic_override" ) );
	if ( !pProp )
	{
		return NULL;
	}

	pProp->SetModel( pszModel );
	pProp->SetAbsOrigin( vecOrigin );
	pProp->SetAbsAngles( angles );
	pProp->Spawn();
	pProp->Activate();

	Rim_MakePayloadCartKillable( pProp, iTeam );
	pProp->SetName( AllocPooledString( ( iTeam == TF_TEAM_RED ) ? "rim_red_objective" : "rim_blue_objective" ) );
	Msg( "RIM payload cart for team %d at %.0f %.0f %.0f\n", iTeam, vecOrigin.x, vecOrigin.y, vecOrigin.z );
	return pProp;
}

//-----------------------------------------------------------------------------
bool Rim_TrySetupPayloadObjectives( CTFGameRules *pRules )
{
	if ( !pRules || !pRules->IsRainbowIsMagicMode() || !tf_rim_use_payload.GetBool() )
	{
		return false;
	}

	if ( pRules->GetGameType() != TF_GAMETYPE_ESCORT )
	{
		return false;
	}

	CTeamTrainWatcher *pPrimaryWatcher = NULL;
	CFuncTrackTrain *pPrimaryTrain = NULL;
	CBaseEntity *pPrimaryCart = NULL;

	CTeamTrainWatcher *pWatcher = NULL;
	while ( ( pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) ) ) != NULL )
	{
		if ( pWatcher->IsDisabled() )
		{
			continue;
		}

		Rim_FreezePayloadWatcher( pWatcher );

		CFuncTrackTrain *pTrain = dynamic_cast<CFuncTrackTrain *>( pWatcher->GetTrainEntity() );
		if ( !pTrain )
		{
			continue;
		}

		if ( !pPrimaryWatcher )
		{
			pPrimaryWatcher = pWatcher;
			pPrimaryTrain = pTrain;
			pPrimaryCart = Rim_FindCartModelOnTrain( pTrain );
		}
	}

	if ( !pPrimaryCart )
	{
		Warning( "RIM payload: no payload cart found on this map.\n" );
		return false;
	}

	const char *pszCartModel = STRING( pPrimaryCart->GetModelName() );
	if ( !pszCartModel || !pszCartModel[0] )
	{
		pszCartModel = "models/props_trainyard/bomb_cart.mdl";
	}

	// Hide the moving PL cart so we only fight the outdoor "teddy" carts.
	pPrimaryCart->AddEffects( EF_NODRAW );
	if ( pPrimaryTrain )
	{
		pPrimaryTrain->AddEffects( EF_NODRAW );
	}

	CBaseEntity *pBlueCart = Rim_SpawnOutdoorPayloadCart( TF_TEAM_BLUE, pszCartModel );
	CBaseEntity *pRedCart = Rim_SpawnOutdoorPayloadCart( TF_TEAM_RED, pszCartModel );

	if ( pBlueCart )
	{
		pRules->m_hBlueRimObjective = pBlueCart;
		pRules->m_bBlueRimObjectiveActive = true;
	}

	if ( pRedCart )
	{
		pRules->m_hRedRimObjective = pRedCart;
		pRules->m_bRedRimObjectiveActive = true;
	}

	if ( !pBlueCart && !pRedCart )
	{
		Warning( "RIM payload: failed to spawn any outdoor carts.\n" );
		return false;
	}

	Msg( "RIM payload mode: killable cart(s) spawned (%s).\n", pszCartModel );
	return true;
}

//-----------------------------------------------------------------------------
void Rim_CheckObjectiveEntity( CTFGameRules *pRules, CHandle<CBaseEntity> &hObjective, bool &bWasActive, int iOwnerTeam )
{
	if ( !pRules || !bWasActive )
	{
		return;
	}

	CBaseEntity *pEnt = hObjective.Get();
	const bool bDestroyed = ( pEnt == NULL ) || ( pEnt->m_takedamage != DAMAGE_NO && pEnt->GetHealth() <= 0 );

	if ( bDestroyed )
	{
		const int iWinner = GetEnemyTeam( iOwnerTeam );
		pRules->Rim_OnMrBearKilled( iWinner, NULL );
		hObjective = NULL;
		bWasActive = false;
	}
}

//-----------------------------------------------------------------------------
static void CC_RimRespawnTeddies( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsRainbowIsMagicMode() )
	{
		Msg( "rim_respawn_teddies: tf_rim_mode must be 1\n" );
		return;
	}

	TFGameRules()->Rim_SpawnMrBears( true );
}

static ConCommand rim_respawn_teddies( "rim_respawn_teddies", CC_RimRespawnTeddies, "Respawn RIM Saxton objectives at team spawns.", FCVAR_GAMEDLL );

//-----------------------------------------------------------------------------
static void Rim_PrintObjective( const char *pszLabel, CBaseEntity *pEnt )
{
	if ( !pEnt )
	{
		Msg( "%s: NOT SPAWNED\n", pszLabel );
		return;
	}

	const Vector &origin = pEnt->GetAbsOrigin();
	const bool bInRespawn = Rim_IsInRespawnTrigger( origin, pEnt->GetTeamNumber() );
	Msg( "%s: %s team %d HP %d at %.0f %.0f %.0f%s\n",
		pszLabel, pEnt->GetClassname(), pEnt->GetTeamNumber(), pEnt->GetHealth(),
		origin.x, origin.y, origin.z, bInRespawn ? " *** INSIDE RESPAWN ROOM (INVALID) ***" : " (outdoors OK)" );
}

//-----------------------------------------------------------------------------
static void CC_RimWhereBears( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsRainbowIsMagicMode() )
	{
		Msg( "rim_where_bears: tf_rim_mode must be 1\n" );
		return;
	}

	CTFGameRules *pRules = TFGameRules();
	Rim_PrintObjective( "RED objective (rules)", pRules ? pRules->Rim_GetFriendlyObjective( TF_TEAM_RED ) : NULL );
	Rim_PrintObjective( "BLU objective (rules)", pRules ? pRules->Rim_GetFriendlyObjective( TF_TEAM_BLUE ) : NULL );

	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "tf_mr_bear" ) ) != NULL )
	{
		Rim_PrintObjective( "tf_mr_bear entity", pEnt );
	}

	pEnt = gEntList.FindEntityByName( NULL, "rim_red_objective" );
	if ( pEnt )
	{
		Rim_PrintObjective( "rim_red_objective", pEnt );
	}

	pEnt = gEntList.FindEntityByName( NULL, "rim_blue_objective" );
	if ( pEnt )
	{
		Rim_PrintObjective( "rim_blue_objective", pEnt );
	}
}

static ConCommand rim_where_bears( "rim_where_bears", CC_RimWhereBears, "Print RIM objective positions and health.", FCVAR_GAMEDLL );

//-----------------------------------------------------------------------------
static void CC_RimSpawnObjectiveHere( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer || !TFGameRules() || !TFGameRules()->IsRainbowIsMagicMode() )
	{
		Msg( "rim_spawn_objective_here: need a player in RIM mode (tf_rim_mode 1)\n" );
		return;
	}

	const int iTeam = pPlayer->GetTeamNumber();
	if ( iTeam != TF_TEAM_RED && iTeam != TF_TEAM_BLUE )
	{
		Msg( "Join RED or BLU first.\n" );
		return;
	}

	Vector vecOrigin;
	QAngle angles;
	if ( !CTFMrBear::FindSpawnLocation( iTeam, vecOrigin, angles ) )
	{
		vecOrigin = pPlayer->GetAbsOrigin() + pPlayer->BodyDirection2D() * tf_rim_spawn_min_dist.GetFloat();
		vecOrigin.z += 8.0f;
		angles = pPlayer->GetAbsAngles();
		if ( Rim_IsBlockedByRespawnRoom( vecOrigin, iTeam ) )
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "RIM: Leave the respawn room first, then run this again." );
			Msg( "rim_spawn_objective_here: player is inside a respawn room.\n" );
			return;
		}
	}

	CTFMrBear *pBear = CTFMrBear::Create( vecOrigin, angles, iTeam );
	if ( !pBear )
	{
		Msg( "Failed to create tf_mr_bear.\n" );
		return;
	}

	TFGameRules()->Rim_SetTeamObjective( iTeam, pBear );

	ClientPrint( pPlayer, HUD_PRINTTALK, "RIM: Hostage spawned in front of you — shoot the ENEMY one to win!" );
	Msg( "RIM debug objective for team %d at your feet.\n", iTeam );
}

static ConCommand rim_spawn_objective_here( "rim_spawn_objective_here", CC_RimSpawnObjectiveHere, "Spawn your team's RIM hostage in front of you (debug).", FCVAR_GAMEDLL );

//-----------------------------------------------------------------------------
static CTFMrBear *Rim_FindBearForTeam( int iTeam )
{
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "tf_mr_bear" ) ) != NULL )
	{
		CTFMrBear *pBear = static_cast<CTFMrBear *>( pEnt );
		if ( pBear && pBear->GetTeamNumber() == iTeam )
		{
			return pBear;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
static void Rim_TeleportPlayerToBear( CTFPlayer *pPlayer, CTFMrBear *pBear, const char *pszLabel )
{
	if ( !pPlayer || !pBear )
	{
		return;
	}

	Vector vecDest = pBear->GetAbsOrigin() + Vector( 0, 0, 64.0f );
	QAngle angles = pPlayer->EyeAngles();
	pPlayer->Teleport( &vecDest, &angles, &vec3_origin );

	ClientPrint( pPlayer, HUD_PRINTCENTER, CFmtStr( "Teleported to %s Saxton", pszLabel ) );
	ClientPrint( pPlayer, HUD_PRINTTALK, CFmtStr( "%s Saxton is at %.0f %.0f %.0f", pszLabel,
		pBear->GetAbsOrigin().x, pBear->GetAbsOrigin().y, pBear->GetAbsOrigin().z ) );
}

//-----------------------------------------------------------------------------
static void CC_RimGotoMySaxton( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer || !TFGameRules() || !TFGameRules()->IsRainbowIsMagicMode() )
	{
		return;
	}

	if ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
	{
		ClientPrint( pPlayer, HUD_PRINTTALK, "Join RED or BLU first." );
		return;
	}

	CTFMrBear *pBear = Rim_FindBearForTeam( pPlayer->GetTeamNumber() );
	if ( !pBear )
	{
		ClientPrint( pPlayer, HUD_PRINTTALK, "Your Saxton is missing — try: rim_respawn_teddies" );
		CC_RimWhereBears();
		return;
	}

	Rim_TeleportPlayerToBear( pPlayer, pBear, "YOUR" );
}

//-----------------------------------------------------------------------------
static void CC_RimGotoEnemySaxton( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer || !TFGameRules() || !TFGameRules()->IsRainbowIsMagicMode() )
	{
		return;
	}

	if ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
	{
		ClientPrint( pPlayer, HUD_PRINTTALK, "Join RED or BLU first." );
		return;
	}

	const int iEnemyTeam = GetEnemyTeam( pPlayer->GetTeamNumber() );
	CTFMrBear *pBear = Rim_FindBearForTeam( iEnemyTeam );
	if ( !pBear )
	{
		ClientPrint( pPlayer, HUD_PRINTTALK, "Enemy Saxton is missing — try: rim_respawn_teddies" );
		CC_RimWhereBears();
		return;
	}

	Rim_TeleportPlayerToBear( pPlayer, pBear, "ENEMY" );
}

static ConCommand rim_goto_my_saxton( "rim_goto_my_saxton", CC_RimGotoMySaxton, "Teleport to your team's Saxton (RIM).", FCVAR_GAMEDLL );
static ConCommand rim_goto_enemy_saxton( "rim_goto_enemy_saxton", CC_RimGotoEnemySaxton, "Teleport to the enemy Saxton (RIM).", FCVAR_GAMEDLL );
