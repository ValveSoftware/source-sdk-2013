//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tf_gamerules.h"
#include "teleport_vortex.h"

//-----------------------------------------------------------------------------

#ifdef CLIENT_DLL

	#include "c_tf_fx.h"

	inline float SCurve( float t )
	{
		t = clamp( t, 0.0f, 1.0f );
		return t * t * (3 - 2*t);
	}

#else

	#include "tf_fx.h"
	#include "tf_team.h"
	#include "tf_weapon_sniperrifle.h"

#endif

//-----------------------------------------------------------------------------

ConVar vortex_float_osc_speed( "vortex_float_osc_speed", "2.0", FCVAR_HIDDEN | FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar vortex_float_amp( "vortex_float_amp", "5.0", FCVAR_HIDDEN | FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar vortex_fade_fraction_denom( "vortex_fade_fraction_denom", "10.0", FCVAR_HIDDEN | FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar vortex_book_offset( "vortex_book_offset", "5.0", FCVAR_HIDDEN | FCVAR_CHEAT | FCVAR_REPLICATED );

//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( TeleportVortex, DT_TeleportVortex )

BEGIN_NETWORK_TABLE( CTeleportVortex, DT_TeleportVortex )
	#if defined( CLIENT_DLL )
		RecvPropInt( RECVINFO( m_iState ) ),
	#else
		SendPropInt( SENDINFO( m_iState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( teleport_vortex, CTeleportVortex );

BEGIN_DATADESC( CTeleportVortex )
END_DATADESC()

//-----------------------------------------------------------------------------

#define VORTEX_PARTICLE_EFFECT_EYEBALL_MOVED	"eyeboss_tp_vortex"
#define VORTEX_PARTICLE_EFFECT_EYEBALL_DIED		"eyeboss_aura_angry"

#define VORTEX_BOOK_MODEL						"models/props_halloween/bombonomicon.mdl"

#define VORTEX_SOUND_EYEBALL_MOVED				"Halloween.TeleportVortex.EyeballMovedVortex"
#define VORTEX_SOUND_EYEBALL_DIED				"Halloween.TeleportVortex.EyeballDiedVortex"
#define VORTEX_SOUND_BOOK_SPAWN					"Halloween.TeleportVortex.BookSpawn"
#define VORTEX_SOUND_BOOK_EXIT					"Halloween.TeleportVortex.BookExit"

#define VORTEX_OPEN_OPEN_ANIM					"flip_stimulated"

//-----------------------------------------------------------------------------

CTeleportVortex::CTeleportVortex()
#ifdef GAME_DLL
:	m_iAutoSetupVortex( -1 )
#endif
{
}

CTeleportVortex::~CTeleportVortex()
{
#ifdef CLIENT_DLL
	DestroyParticleEffect();
#endif
}

#ifdef CLIENT_DLL
void CTeleportVortex::DestroyParticleEffect()
{
	if ( m_pVortexEffect )
	{
		ParticleProp()->StopEmission( m_pVortexEffect );
		m_pVortexEffect = NULL;
	}
}
#else
bool CTeleportVortex::KeyValue( const char *szKeyname, const char *szValue )
{
	if ( !V_strnicmp( szKeyname, "type", 4 ) )
	{
		m_iAutoSetupVortex = atoi( szValue );
	}
	else
	{
		return BaseClass::KeyValue( szKeyname, szValue );
	}

	return true;
}
#endif

void CTeleportVortex::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	AddEffects( EF_NODRAW );
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_BBOX );
	CollisionProp()->SetCollisionBounds( Vector( -50, -50, -50 ), Vector( 50, 50, 50 ) );
	SetModelName( NULL_STRING );
	SetSolidFlags( FSOLID_TRIGGER );
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
	SetRenderMode( kRenderTransAlpha );
	SetRenderColorA( 255 );
	UseClientSideAnimation();

	m_lifeTimer.Start( 5.0f );

#ifdef CLIENT_DLL
	m_flScale = 1.0f;
	m_iOldState = VORTEXSTATE_INACTIVE;
	m_pVortexEffect = NULL;

	ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_ALWAYS );

	AddToLeafSystem( RENDER_GROUP_TWOPASS );
#else
	// Default to purgatory
	m_pszWhere = "spawn_purgatory";
	
	SetThink( &CTeleportVortex::VortexThink );
	SetNextThink( gpGlobals->curtime );

	m_nSoundCounter = 0;

	m_bSplitTeam = false;

	if ( m_iAutoSetupVortex >= 0 )
	{
		SetupVortex( m_iAutoSetupVortex != 0 );
	}
#endif
}


//-----------------------------------------------------------------------------

void CTeleportVortex::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( VORTEX_BOOK_MODEL );

	// We deliberately allow late precaches here.
	bool bAllowPrecache = CBaseAnimating::IsPrecacheAllowed();
	CBaseAnimating::SetAllowPrecache( true );

#if GAME_DLL
	PrecacheScriptSound( VORTEX_SOUND_EYEBALL_MOVED );
	PrecacheScriptSound( VORTEX_SOUND_EYEBALL_DIED );
	PrecacheScriptSound( VORTEX_SOUND_BOOK_SPAWN );
	PrecacheScriptSound( VORTEX_SOUND_BOOK_EXIT );
#endif

	PrecacheParticleSystem( VORTEX_PARTICLE_EFFECT_EYEBALL_MOVED );
	PrecacheParticleSystem( VORTEX_PARTICLE_EFFECT_EYEBALL_DIED );

	CBaseAnimating::SetAllowPrecache( bAllowPrecache );
}

//-----------------------------------------------------------------------------

#ifdef GAME_DLL

void CTeleportVortex::Touch( CBaseEntity *pOther )
{
	BaseClass::Touch( pOther );

	if ( pOther && pOther->IsPlayer() )
	{
		if ( m_bSplitTeam )
		{
			const char* pszTeam = pOther->GetTeamNumber() == TF_TEAM_RED ? "_red" : "_blue";
			SendPlayerToTheUnderworld( ToTFPlayer( pOther ), CFmtStr( "%s%s", m_pszWhere.Get(), pszTeam ) );
		}
		else
		{
			SendPlayerToTheUnderworld( ToTFPlayer( pOther ), m_pszWhere );
		}
	}
}


//-----------------------------------------------------------------------------

int CTeleportVortex::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_PVSCHECK );
}

//-----------------------------------------------------------------------------

void CTeleportVortex::StartTouch( CBaseEntity *pOther )
{
	CBaseAnimating::StartTouch( pOther );
}

//-----------------------------------------------------------------------------

void CTeleportVortex::SetupVortex( bool bIsDeathVortex, bool bSplitTeam /*= false*/ )
{
	m_bSplitTeam = bSplitTeam;
	if ( bIsDeathVortex )
	{
		m_iState = VORTEXSTATE_ACTIVE_EYEBALL_DIED;
		m_pszWhere = "spawn_loot";
		RemoveEffects( EF_NODRAW );
		SetModel( VORTEX_BOOK_MODEL );
	}
	else
	{
		m_iState = VORTEXSTATE_ACTIVE_EYEBALL_MOVED;
		m_pszWhere = "spawn_purgatory";
		AddEffects( EF_NODRAW );
	}
}

void CTeleportVortex::VortexThink()
{
	if ( m_lifeTimer.IsElapsed() )
	{
		UTIL_Remove( this );
	}
	else
	{
		StudioFrameAdvance();

		if ( m_iState == VORTEXSTATE_ACTIVE_EYEBALL_DIED )
		{
			if ( m_nSoundCounter == 0 && ShouldDoBookRampIn() )
			{
				EmitSound( VORTEX_SOUND_BOOK_SPAWN );
				++m_nSoundCounter;
			}
			else if ( m_nSoundCounter == 1 && ShouldDoBookRampOut() )
			{
				EmitSound( VORTEX_SOUND_BOOK_EXIT );
				++m_nSoundCounter;
			}
		}

		SetNextThink( gpGlobals->curtime );
	}

	// suck nearby players into the vortex
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	const float suctionRange = 500.0f;

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *player = playerVector[i];

		if ( player->GetFlags() & FL_ONGROUND )
		{
			// not airborne
			continue;
		}

		Vector toVortex = WorldSpaceCenter() - player->WorldSpaceCenter();
		float range = toVortex.NormalizeInPlace();

		if ( range > suctionRange )
		{
			// too far
			continue;
		}

		if ( player->IsLookingTowards( WorldSpaceCenter() ) )
		{
			if ( player->IsLineOfSightClear( WorldSpaceCenter(), CBaseCombatCharacter::IGNORE_ACTORS, player ) )
			{
				player->ApplyAbsVelocityImpulse( 30.0f * toVortex );
			}
		}
	}
}


//-----------------------------------------------------------------------------------------------------
void SendPlayerToTheUnderworld( CTFPlayer *teleportingPlayer, const char *where )
{
	if ( !teleportingPlayer )
	{
		return;
	}

	CUtlVector< CBaseEntity * > spawnVector;

	CBaseEntity *spawnPoint = NULL;
	while( ( spawnPoint = gEntList.FindEntityByClassname( spawnPoint, "info_target" ) ) != NULL )
	{
		if ( FStrEq( STRING( spawnPoint->GetEntityName() ), where ) )
		{
			spawnVector.AddToTail( spawnPoint );
		}
	}

	if ( spawnVector.Count() == 0 )
	{
		Warning( "SendPlayerToTheUnderworld: No info_target entities named '%s' found!\n", where );
		return;
	}

	// collect enemies that could block our spawning
	CUtlVector< CTFPlayer * > enemyVector;
	CollectPlayers( &enemyVector, GetEnemyTeam( teleportingPlayer->GetTeamNumber() ), COLLECT_ONLY_LIVING_PLAYERS );

	const float nearRange = 25.0f;

	// collect spots without players overlapping them
	CUtlVector< CBaseEntity * > openSpawnVector;
	for( int i=0; i<spawnVector.Count(); ++i )
	{
		int p;

		for( p=0; p<enemyVector.Count(); ++p )
		{
			if ( ( spawnVector[i]->GetAbsOrigin() - enemyVector[p]->GetAbsOrigin() ).IsLengthLessThan( nearRange ) )
			{
				// a player is occupying this spawn
				break;
			}
		}

		if ( p == enemyVector.Count() )
		{
			// no players are near this spawn point
			openSpawnVector.AddToTail( spawnVector[i] );
		}
	}

	CBaseEntity *teleportDestination = NULL;

	if ( openSpawnVector.Count() == 0 )
	{
		// there are no free spawns - pick one and telefrag enemies standing there
		int which = RandomInt( 0, spawnVector.Count()-1 );

		teleportDestination = spawnVector[ which ];

		for( int p=0; p<enemyVector.Count(); ++p )
		{
			if ( ( teleportDestination->GetAbsOrigin() - enemyVector[p]->GetAbsOrigin() ).IsLengthLessThan( nearRange ) )
			{
				// telefrag!
				enemyVector[p]->TakeDamage( CTakeDamageInfo( teleportingPlayer, teleportingPlayer, 1000, DMG_CRUSH, TF_DMG_CUSTOM_TELEFRAG ) );
			}
		}
	}
	else
	{
		// pick an open destination at random
		int which = RandomInt( 0, openSpawnVector.Count()-1 );

		teleportDestination = openSpawnVector[ which ];
	}

	if ( teleportDestination )
	{
		if ( teleportingPlayer->GetTeam() )
		{
			UTIL_LogPrintf( "HALLOWEEN: \"%s<%i><%s><%s>\" purgatory_teleport \"%s\"\n",
				teleportingPlayer->GetPlayerName(),
				teleportingPlayer->GetUserID(),
				teleportingPlayer->GetNetworkIDString(),
				teleportingPlayer->GetTeam()->GetName(),
			where );
		}

		teleportingPlayer->Teleport( &teleportDestination->GetAbsOrigin(), &teleportDestination->GetAbsAngles(), &vec3_origin );

		// When fighting Merasmus, give players full health on teleport
		if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_LAKESIDE ) )
		{
			if ( teleportingPlayer->IsAlive() )
			{
				teleportingPlayer->TakeHealth( teleportingPlayer->GetMaxHealth(), DMG_GENERIC );
				teleportingPlayer->m_Shared.HealthKitPickupEffects();
				teleportingPlayer->m_Shared.RemoveCond( TF_COND_HALLOWEEN_BOMB_HEAD );
			}
		}

		if ( FStrEq( where, "spawn_loot" ) )
		{
			CReliableBroadcastRecipientFilter filter;
			UTIL_SayText2Filter( filter, teleportingPlayer, false, "#TF_Halloween_Loot_Island", teleportingPlayer->GetPlayerName() );
		}

		teleportingPlayer->m_Shared.InstantlySniperUnzoom();

		color32 fadeColor = { 255, 255, 255, 100 };
		UTIL_ScreenFade( teleportingPlayer, fadeColor, 0.25, 0.4, FFADE_IN );
	}
}


#else

//-----------------------------------------------------------------------------

void CTeleportVortex::ClientThink()
{
	// Fade in and out for first and last fraction of time
	const float flDuration = m_lifeTimer.GetCountdownDuration();
	const float flRampTime = flDuration / vortex_fade_fraction_denom.GetInt();
	const float flElapsed = m_lifeTimer.GetElapsedTime();

	float t = 1.0f;
	if ( ShouldDoBookRampIn() )
	{
		t = SCurve( flElapsed / flRampTime );
	}
	else if ( ShouldDoBookRampOut() )
	{
		t = SCurve( 1.0f - ( flElapsed - flDuration + flRampTime ) / flRampTime );
	}

//	SetRenderColorA( t * 255 );

	m_flScale = t;
}

//-----------------------------------------------------------------------------

void CTeleportVortex::BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	// Translate root bone towards the player and oscillate a bit - if in the scale in/out period, translate up directly from the underworld, or down towards it.
	const Vector vecBob( vortex_book_offset.GetFloat(), 0.0f, vortex_float_amp.GetFloat() * sinf( vortex_float_osc_speed.GetFloat() * gpGlobals->curtime ) );
	pos[0] = vecBob + Vector( 0.0f, 0.0f, Lerp( m_flScale, -500.0f, 0.0f ) );

	matrix3x4_t mRootBoneRotation;

	// If the local player exists and is alive, render the book so that it's facing him
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer && pLocalPlayer->IsAlive() )
	{
		const Vector vecBook = GetAbsOrigin();
		const Vector vecPlayerPos = pLocalPlayer->GetAbsOrigin();
		Vector vecForward = vecPlayerPos - vecBook;
		if ( VectorNormalize( vecForward ) > 0.1f )
		{
			// Calculate a matrix based on the forward direction
			VectorMatrix( vecForward, mRootBoneRotation );
		}
	}
	else
	{
		// Use whatever rotation is in the root bone already
		QuaternionMatrix( q[0], mRootBoneRotation );
	}

	// Convert the matrix to a quaternion and slam the root bone rotation
	MatrixQuaternion( mRootBoneRotation, q[0] );

	// Let the base class actually build the global transforms
	BaseClass::BuildTransformations( pStudioHdr, pos, q, cameraTransform, boneMask, boneComputed );
}

//-----------------------------------------------------------------------------

void CTeleportVortex::PlayBookAnimation( const char *pAnimName )
{
	int iAnimSequence = LookupSequence( pAnimName );
	if ( iAnimSequence )
	{
		SetSequence( iAnimSequence );
		SetPlaybackRate( 1.0f );
		SetCycle( 0 );
		ResetSequenceInfo();
	}
}

//-----------------------------------------------------------------------------

void CTeleportVortex::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	if ( m_iState != m_iOldState )
	{
		if ( m_iState != VORTEXSTATE_INACTIVE )
		{
			// Stop any existing particle effect if necessary
			AssertMsg( !m_pVortexEffect, "Particle effect should not be active!" );
			
			// Create the particle effect and play a sound
			const char *pszEffectName;
			const char *pszSound;
			if ( m_iState == VORTEXSTATE_ACTIVE_EYEBALL_MOVED )
			{
				pszEffectName = VORTEX_PARTICLE_EFFECT_EYEBALL_MOVED;
				pszSound = VORTEX_SOUND_EYEBALL_MOVED;
			}
			else
			{
				pszEffectName = VORTEX_PARTICLE_EFFECT_EYEBALL_DIED;
				pszSound = VORTEX_SOUND_EYEBALL_DIED;

				PlayBookAnimation( VORTEX_OPEN_OPEN_ANIM );
			}
			m_pVortexEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN );
			EmitSound( pszSound );
		}

		m_iOldState = m_iState;
	}
}

#endif

float CTeleportVortex::GetRampTime()
{
	const float flDuration = m_lifeTimer.GetCountdownDuration();
	return flDuration / vortex_fade_fraction_denom.GetInt();
}

bool CTeleportVortex::ShouldDoBookRampIn()
{
	return m_lifeTimer.GetElapsedTime() <= GetRampTime();
}

bool CTeleportVortex::ShouldDoBookRampOut()
{
	const float flDuration = m_lifeTimer.GetCountdownDuration();
	return m_lifeTimer.GetElapsedTime() >= flDuration - GetRampTime();
}

#ifdef CLIENT_DLL
	#define CHightower_TeleportVortex C_Hightower_TeleportVortex
#endif
class CHightower_TeleportVortex : public CTeleportVortex
{
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS(); 
	DECLARE_CLASS( CHightower_TeleportVortex, CTeleportVortex );

public:
	virtual void Spawn()
	{
		BaseClass::Spawn();
#ifdef GAME_DLL
		m_nWinningTeam = TF_TEAM_COUNT;	// Invalid
		SetupVortex( false, false );
		m_lifeTimer.Start( m_flDuration );
#endif
	}

	virtual void Touch( CBaseEntity *pOther )
	{
#ifdef GAME_DLL
		const char *pszTarget = CFmtStr( "%s_%s", m_pszDestinationBaseName, ( pOther->GetTeamNumber() == m_nWinningTeam ) ? "winner" : "loser" );
		SendPlayerToTheUnderworld( ToTFPlayer( pOther ), pszTarget );
#endif
	}

#ifdef GAME_DLL
	void SetAdvantageTeam( inputdata_t &inputdata )
	{
		m_nWinningTeam = FStrEq( inputdata.value.String(), "red" ) ? TF_TEAM_RED : TF_TEAM_BLUE;
	}
#endif
private:

#ifdef GAME_DLL
	int m_nWinningTeam;
	const char* m_pszDestinationBaseName;
	float m_flDuration;
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED( Hightower_TeleportVortex, DT_Hightower_TeleportVortex )

BEGIN_NETWORK_TABLE( CHightower_TeleportVortex, DT_Hightower_TeleportVortex )
	#if defined( CLIENT_DLL )
		RecvPropInt( RECVINFO( m_iState ) ),
	#else
		SendPropInt( SENDINFO( m_iState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( hightower_teleport_vortex, CHightower_TeleportVortex );

BEGIN_DATADESC( CHightower_TeleportVortex )
#ifdef GAME_DLL
	DEFINE_INPUTFUNC( FIELD_STRING,	"SetAdvantageTeam", SetAdvantageTeam ),
	DEFINE_KEYFIELD( m_flDuration, FIELD_FLOAT, "lifetime" ),
	DEFINE_KEYFIELD( m_pszDestinationBaseName, FIELD_STRING, "target_base_name" ),
#endif
END_DATADESC()