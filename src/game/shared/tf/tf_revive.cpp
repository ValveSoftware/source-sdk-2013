//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Revive
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_revive.h"
#include "tf_gamerules.h"
#ifdef CLIENT_DLL
#include "tf_hud_target_id.h"
#include "view.h"
#include "tf_hud_mediccallers.h"
#else
#include "tf_gamestats.h"
#include "particle_parse.h"
#include "world.h"
#include "collisionutils.h"
#include "triggers.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MARKER_MODEL	"models/props_mvm/mvm_revive_tombstone.mdl"

static const int REVIVE_EASY_LIMIT = 4;
static const int REVIVE_MEDIUM_LIMIT = 8;

#ifdef GAME_DLL
extern void HandleRageGain( CTFPlayer *pPlayer, unsigned int iRequiredBuffFlags, float flDamage, float fInverseRageGainScale );
#else
extern void AddMedicCaller( C_BaseEntity *pEntity, float flDuration, Vector &vecOffset, bool bAutoCaller = false );
#endif // GAME_DLL


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFReviveMarker, DT_TFReviveMarker )

BEGIN_NETWORK_TABLE( CTFReviveMarker, DT_TFReviveMarker )
#ifdef GAME_DLL
	SendPropEHandle( SENDINFO( m_hOwner ) ),
	SendPropInt( SENDINFO( m_iHealth ), -1, SPROP_VARINT | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iMaxHealth ), -1, SPROP_VARINT ),
	SendPropInt( SENDINFO( m_nRevives ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
#else
	RecvPropEHandle( RECVINFO( m_hOwner ) ),
	RecvPropInt( RECVINFO( m_iHealth ) ),
	RecvPropInt( RECVINFO( m_iMaxHealth ) ),
	RecvPropInt( RECVINFO( m_nRevives ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( entity_revive_marker, CTFReviveMarker );
PRECACHE_REGISTER( entity_revive_marker );

BEGIN_DATADESC( CTFReviveMarker )
#ifdef GAME_DLL
	DEFINE_THINKFUNC( ReviveThink ),
#endif // GAME_DLL
END_DATADESC()

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CTFReviveMarker::CTFReviveMarker()
{
#ifdef GAME_DLL
	m_flHealAccumulator = 0.f;
	m_flLastHealTime = 0.f;
	m_bOwnerPromptedToRevive = false;
	m_bOnGround = false;
#else
	m_iMaxHealth = 1;
	m_bCalledForMedic = false;
#endif // GAME_DLL
	m_nRevives = 0;

	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFReviveMarker::Precache()
{
	BaseClass::Precache();

	PrecacheModel( MARKER_MODEL );
	PrecacheScriptSound( "MVM.PlayerRevived" );
	PrecacheParticleSystem( "speech_revivecall" );
	PrecacheParticleSystem( "speech_revivecall_medium" );
	PrecacheParticleSystem( "speech_revivecall_hard" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFReviveMarker::Spawn( void )
{
	Precache();
	
	BaseClass::Spawn();

	SetHealth( 1 );
	SetModel( MARKER_MODEL );
	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_TRIGGER );
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	// SetCollisionBounds( VEC_HULL_MIN, VEC_HULL_MAX );
	SetBlocksLOS( false );
	AddEffects( EF_NOSHADOW );
	ResetSequence( LookupSequence( "idle" ) );

#ifdef GAME_DLL
	m_takedamage = DAMAGE_NO;

	SetThink( &CTFReviveMarker::ReviveThink );
	SetNextThink( gpGlobals->curtime );
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFReviveMarker::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
		return false;

	if ( collisionGroup == COLLISION_GROUP_PROJECTILE )
		return false;

	if ( collisionGroup == TFCOLLISION_GROUP_ROCKETS )
		return false;
	
	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFReviveMarker::OnDataChanged( DataUpdateType_t updateType )
{
	// Call for medic once the server's set maxhealth
	if ( !m_bCalledForMedic && m_iMaxHealth > 1 )
	{
		MedicCallerType nType = CALLER_TYPE_REVIVE_EASY;
		if ( m_nRevives >= REVIVE_EASY_LIMIT && m_nRevives < REVIVE_MEDIUM_LIMIT )
		{
			nType = CALLER_TYPE_REVIVE_MEDIUM;
		}
		else if ( m_nRevives >= REVIVE_MEDIUM_LIMIT )
		{
			nType = CALLER_TYPE_REVIVE_HARD;
		}
		
		Vector vecPos;
		if ( GetAttachmentLocal( LookupAttachment( "mediccall" ), vecPos ) )
		{
			CTFMedicCallerPanel::AddMedicCaller( this, 5.0, vecPos, nType );
		}
		
		m_bCalledForMedic = true;
	}

	BaseClass::OnDataChanged( updateType );
}
#endif // CLIENT_DLL

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFReviveMarker *CTFReviveMarker::Create( CTFPlayer *pOwner )
{
	if ( pOwner )
	{
		CTFReviveMarker *pMarker = static_cast< CTFReviveMarker* >( CBaseEntity::Create( "entity_revive_marker", pOwner->GetAbsOrigin() + Vector( 0, 0, 50 ), pOwner->GetAbsAngles() ) );		
		if ( pMarker )
		{
			pMarker->SetOwner( pOwner );
			pMarker->ChangeTeam( pOwner->GetTeamNumber() );

			return pMarker;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int CTFReviveMarker::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_FULLCHECK );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int CTFReviveMarker::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	return FL_EDICT_ALWAYS;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFReviveMarker::ReviveThink( void )
{
	if ( !m_hOwner || !InSameTeam( m_hOwner ) )
	{
		UTIL_Remove( this );
		return;
	}

	if ( !GetMaxHealth() )
	{
		// Set health of marker based on class, and number of previous revives
		float flHealth = m_hOwner->GetMaxHealth() / 2;
		Assert( flHealth > 0.f );
		PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( m_hOwner );
		if ( pPlayerStats ) 
		{
			m_nRevives.Set( pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_REVIVED] );
			flHealth += ( (float)m_nRevives * 10.f );
		}
		SetMaxHealth( flHealth );
	}

	// At rest?
	if ( !m_bOnGround && ( GetFlags() & FL_ONGROUND ) )
	{
		SetMoveType( MOVETYPE_NONE );
		m_bOnGround = true;

		// See if we've in a trigger_hurt
		for ( int i = 0; i < ITriggerHurtAutoList::AutoList().Count(); i++ )
		{
			CTriggerHurt *pTrigger = static_cast<CTriggerHurt*>( ITriggerHurtAutoList::AutoList()[i] );
			if ( !pTrigger->m_bDisabled )
			{
				Vector vecMins, vecMaxs;
				pTrigger->GetCollideable()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );
				if ( IsPointInBox( GetCollideable()->GetCollisionOrigin(), vecMins, vecMaxs ) )
				{
					UTIL_Remove( this );
					return;
				}
			}
		}

		// Different particle based on difficulty of this revive
		const char *pszParticle = NULL;
		if ( m_nRevives < REVIVE_EASY_LIMIT )
		{
			pszParticle = "speech_revivecall";
		}
		else if ( m_nRevives < REVIVE_MEDIUM_LIMIT )
		{
			pszParticle = "speech_revivecall_medium";
		}
		else
		{
			pszParticle = "speech_revivecall_hard";
		}

		// DispatchParticleEffect( pszParticle, GetAbsOrigin() + Vector( 0, 0, 80 ), vec3_angle );
		DispatchParticleEffect( pszParticle, PATTACH_POINT_FOLLOW, this, "mediccall" );
		EmitSound( "Medic.AutoCallerAnnounce" );
	}

	// Close revive prompt if no longer being revived
	if ( HasOwnerBeenPrompted() && !IsReviveInProgress() )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "revive_player_stopped" );
		if ( event )
		{
			event->SetInt( "entindex", m_hOwner->entindex() );
			gameeventmanager->FireEvent( event );

			SetOwnerHasBeenPrompted( false );
		}
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CTFReviveMarker::SetOwner( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	if ( !pPlayer )
		return;

	m_hOwner = pPlayer;
	ChangeTeam( m_hOwner->GetTeamNumber() );

	// Determine bodygroup based on class
	SetBodygroup( 1, m_hOwner->GetPlayerClass()->GetClassIndex() - 1 );
	
	SetAbsAngles( m_hOwner->GetAbsAngles() );
#endif // GAME_DLL
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CTFReviveMarker::AddMarkerHealth( float flAmount )
{
	CTFPlayer *pReviver = GetReviver();
	if ( !pReviver )
		return;

	CTFPlayer *pOwner = GetOwner();
	if ( !pOwner )
		return;

	if ( !GetMaxHealth() )
		return;

	HandleRageGain( pReviver, kRageBuffFlag_OnHeal, flAmount * 2, 1.f );

	m_flHealAccumulator += flAmount;
	if ( m_flHealAccumulator >= 1.f )
	{
		float flHealthToAdd = floor( m_flHealAccumulator );
		m_flHealAccumulator -= flHealthToAdd;
		m_iHealth += flHealthToAdd;
		m_flLastHealTime = gpGlobals->curtime;
	}

	if ( m_iHealth >= GetMaxHealth() )
	{
		ReviveOwner();
	
		// Give points
		CTF_GameStats.Event_PlayerAwardBonusPoints( pReviver, pOwner, 50 );
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CTFReviveMarker::IsReviveInProgress( void )
{
	float flTimeSinceHeal = gpGlobals->curtime - m_flLastHealTime;
	return ( m_flLastHealTime && flTimeSinceHeal <= 2.f );
}

//-----------------------------------------------------------------------------
// Returns true if the player was spawned at their marker
//-----------------------------------------------------------------------------
bool CTFReviveMarker::ReviveOwner( void )
{
	if ( !m_hOwner )
		return false;

	m_hOwner->ForceRespawn();

	// Increment stat
	CTF_GameStats.Event_PlayerRevived( m_hOwner );

	// If the medic's gone, or dead, stay in the spawn room
	if ( !m_pReviver || !m_pReviver->IsAlive() )
		return false;

	// See if their marker is clear
	Vector vecTeleportPos = GetAbsOrigin();			
	trace_t tr;
	CTraceFilterIgnoreTeammatesAndTeamObjects filter( m_hOwner, COLLISION_GROUP_NONE, m_hOwner->GetTeamNumber() );
	UTIL_TraceHull( vecTeleportPos, vecTeleportPos, VEC_HULL_MIN_SCALED( m_hOwner ), VEC_HULL_MAX_SCALED( m_hOwner ), ( MASK_SOLID | CONTENTS_PLAYERCLIP ), &filter, &tr );
		
	// If not, try the medic's location
	if ( tr.fraction < 1.f )
	{
		if ( !m_pReviver )
			// They'll appear in their spawn room.
			return false;
			
		vecTeleportPos = m_pReviver->GetAbsOrigin();
	}
	else
	{
		// Use the angles that were stored when the marker was spawned
		m_hOwner->SetAbsAngles( GetAbsAngles() );
	}

	// Magic
	color32 fadeColor = { 50, 50, 50, 200 };
	UTIL_ScreenFade( m_hOwner, fadeColor, 0.5, 0.4, FFADE_IN );

	m_hOwner->Teleport( &vecTeleportPos, &m_hOwner->GetAbsAngles(), &vec3_origin  );
	m_hOwner->EmitSound( "MVM.PlayerRevived" );

	if ( m_pReviver )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "revive_player_complete" );
		if ( event )
		{
			event->SetInt( "entindex", m_pReviver->entindex() );
			gameeventmanager->FireEvent( event );
		}
	}

	m_hOwner->SpeakConceptIfAllowed( MP_CONCEPT_RESURRECTED );

	return true;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CTFReviveMarker::PromptOwner( void )
{
	if ( !m_hOwner )
	{
		UTIL_Remove( this );
		return;
	}

	if ( HasOwnerBeenPrompted() )
		return;

	IGameEvent *event = gameeventmanager->CreateEvent( "revive_player_notify" );
	if ( event )
	{
		event->SetInt( "entindex", m_hOwner->entindex() );
		event->SetInt( "marker_entindex", entindex() );
		gameeventmanager->FireEvent( event );

		SetOwnerHasBeenPrompted( true );
	}
}
#endif // GAME_DLL
