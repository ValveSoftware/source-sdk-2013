//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#include "cbase.h"

#include "tf_halloween_souls_pickup.h"	

#ifdef CLIENT_DLL
	#include "tf_shareddefs.h"
#else
	#include "tf_gamerules.h"
	#include "tf_player.h"
	#include "particle_parse.h"
#endif

#define TF_SOULS_TIMEOUT		10.0f		// How long before dropped souls disappear

LINK_ENTITY_TO_CLASS( halloween_souls_pack, CHalloweenSoulPack );
PRECACHE_REGISTER( halloween_souls_pack );

IMPLEMENT_NETWORKCLASS_ALIASED( HalloweenSoulPack, DT_HalloweenSoulPack )

BEGIN_NETWORK_TABLE( CHalloweenSoulPack, DT_HalloweenSoulPack )
#ifdef GAME_DLL
	SendPropEHandle( SENDINFO( m_hTarget ) ),
	SendPropVector( SENDINFO( m_vecPreCurvePos ) ),
	SendPropVector( SENDINFO( m_vecStartCurvePos ) ),
	SendPropFloat( SENDINFO( m_flDuration ) ),
#else
	RecvPropEHandle( RECVINFO( m_hTarget) ),
	RecvPropVector( RECVINFO( m_vecPreCurvePos) ),
	RecvPropVector( RECVINFO( m_vecStartCurvePos) ),
	RecvPropFloat( RECVINFO( m_flDuration ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CHalloweenSoulPack )
END_PREDICTION_DATA()

#define SOUND_PLAYER_COLLECT_SOULS "Player.ReceiveSouls"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHalloweenSoulPack::CHalloweenSoulPack()
	: m_flCreationTime( 0.f )
#ifdef GAME_DLL
	, m_nAmount( 1 )
#endif
{
	m_hTarget = NULL;
	m_flDuration = 2.f;
}


CHalloweenSoulPack::~CHalloweenSoulPack()
{}


void CHalloweenSoulPack::Spawn()
{
	BaseClass::Spawn();

	SetMoveType( MOVETYPE_NOCLIP, MOVECOLLIDE_DEFAULT );
	SetSolid( SOLID_BBOX );
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	SetSolidFlags( FSOLID_TRIGGER );
#ifdef GAME_DLL
	InitSplineData();
	SetTouch(&CHalloweenSoulPack::ItemTouch);
#else
	SetNextClientThink( CLIENT_THINK_ALWAYS );
#endif
}

void CHalloweenSoulPack::Precache()
{
	PrecacheScriptSound( SOUND_PLAYER_COLLECT_SOULS );
}

void CHalloweenSoulPack::FlyThink( void )
{
	FlyTowardsTargetEntity();
}

#ifdef CLIENT_DLL
void CHalloweenSoulPack::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		switch( GetTeamNumber() )
		{
			case TF_TEAM_RED:
				ParticleProp()->Create( "halloween_pickup_active_red", PATTACH_ABSORIGIN_FOLLOW );
				break;
			case TF_TEAM_BLUE:
				ParticleProp()->Create( "halloween_pickup_active", PATTACH_ABSORIGIN_FOLLOW );
				break;
			case TEAM_SPECTATOR:
				ParticleProp()->Create( "halloween_pickup_active_green", PATTACH_ABSORIGIN_FOLLOW );
				break;
			default:
				Assert( false );
		}
		ParticleProp()->Create( "eb_beam_angry_ring01", PATTACH_ABSORIGIN_FOLLOW );
		ParticleProp()->Create( "soul_trail", PATTACH_ABSORIGIN_FOLLOW );
		InitSplineData();
	}
}

void CHalloweenSoulPack::ClientThink()
{
	FlyTowardsTargetEntity();
}
#endif

#ifdef GAME_DLL
int CHalloweenSoulPack::UpdateTransmitState()
{ 
	return SetTransmitState( FL_EDICT_ALWAYS );
}


void CHalloweenSoulPack::ItemTouch( CBaseEntity *pOther ) 
{
	// Only allow our target to pick us up, if we have one
	if ( pOther != m_hTarget && m_hTarget != NULL )
		return;

	// Only allow to be picked up when done travelling
	float flT = ( gpGlobals->curtime - m_flCreationTime ) / m_flDuration;
	if ( flT < 1.f )
		return;

	CTFPlayer * pPlayer = ToTFPlayer( pOther );
	if ( pPlayer )
	{
		CTFPlayer *pTargetPlayer = ToTFPlayer( m_hTarget );
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "halloween_soul_collected" );
		if ( pEvent )
		{
			pEvent->SetInt( "intended_target", pTargetPlayer ? pTargetPlayer->GetUserID() : -1 );
			pEvent->SetInt( "collecting_player", pPlayer->GetUserID() );
			pEvent->SetInt( "soul_count", m_nAmount );
			gameeventmanager->FireEvent( pEvent, true );
		}

		// Strange Tracking
		static CSchemaItemDefHandle hItemDef( "Activated Halloween Pass");
		kill_eater_event_t eEventType = kKillEaterEvent_HalloweenSouls;
		EconEntity_NonEquippedItemKillTracking_NoPartnerBatched( pPlayer, hItemDef->GetDefinitionIndex(), eEventType, m_nAmount );

		// Play a spooky sound in their ears
		CSingleUserRecipientFilter filter( pPlayer );
		EmitSound_t params;
		params.m_nChannel = CHAN_STATIC;
		params.m_pSoundName = SOUND_PLAYER_COLLECT_SOULS;
		EmitSound( filter, pPlayer->entindex(), params );
	}

	if ( pOther == m_hTarget || ( !m_hTarget && pOther->IsPlayer() ) )
	{
		UTIL_Remove( this );
	}
}
#endif

void CHalloweenSoulPack::FlyTowardsTargetEntity( void )
{
	CBaseEntity* pEntity = m_hTarget.Get();
	float flT = ( gpGlobals->curtime - m_flCreationTime ) / m_flDuration;
#ifdef CLIENT_DLL
	// Client doesn't need to do anything if there's no target
	if ( !pEntity )
	{
		return;
	}
#else
	bool bIsAGhost = false;
	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer* pTFPlayerTarget = assert_cast< CTFPlayer* >( pEntity );
		bIsAGhost = pTFPlayerTarget->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE );
	}

	// If flT > 2.f something has gone terribly wrong, just give up.
	// Also give up if our entity is gone, dead or now a ghost
	if( flT > 2.f || pEntity == NULL || !pEntity->IsAlive() || bIsAGhost )
	{
		m_hTarget = NULL;
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
		SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + TF_SOULS_TIMEOUT, "RemoveThink" );
		return;
	}
#endif
	// Clamp
	const float flBiasAmt = 0.2f;
	flT = clamp( Bias( flT, flBiasAmt ), 0.f, 1.f );

	// We want to fly through the front of their chest so they get a good show
	QAngle eyeAngles = pEntity->EyeAngles();
	Vector vecBehindChest;
	AngleVectors( eyeAngles, &vecBehindChest );
	vecBehindChest *= -2000;

	Vector vecNextCuvePos = pEntity->WorldSpaceCenter() + vecBehindChest;
	Vector vecOutput;
	Catmull_Rom_Spline( m_vecPreCurvePos, m_vecStartCurvePos, pEntity->WorldSpaceCenter(), vecNextCuvePos, flT, vecOutput );

	SetAbsOrigin( vecOutput );

	SetContextThink( &CHalloweenSoulPack::FlyThink, gpGlobals->curtime, "HalloweenSoulPackThink" );
}

void CHalloweenSoulPack::InitSplineData( void )
{
	m_flCreationTime = gpGlobals->curtime;
#ifdef GAME_DLL
	m_vecStartCurvePos = GetAbsOrigin();
	m_vecPreCurvePos = m_vecStartCurvePos + RandomVector( -2000, 2000 );
	m_vecPreCurvePos.SetZ( Min( m_vecPreCurvePos.GetZ(), 0.f ) );
	m_flDuration = RandomFloat( 1.f, 3.f );
#endif
	SetContextThink( &CHalloweenSoulPack::FlyThink, gpGlobals->curtime, "HalloweenSoulPackThink" );
}
