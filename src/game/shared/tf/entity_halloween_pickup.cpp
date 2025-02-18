//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#include "cbase.h"
#include "entity_halloween_pickup.h"

#ifdef GAME_DLL
#include "items.h"
#include "tf_gamerules.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_halloween_pickup.h"
#include "tf_fx.h"
#include "tf_logic_halloween_2014.h"
#endif // GAME_DLL

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#endif 

#include "tf_shareddefs.h"
#include "tf_duckleaderboard.h"


#define TF_HALLOWEEN_PICKUP_RETURN_DELAY	10

#ifdef GAME_DLL
IMPLEMENT_AUTO_LIST( IHalloweenGiftSpawnAutoList );
#endif // GAME_DLL

//=============================================================================
//
// CTF Halloween Pickup defines.

IMPLEMENT_NETWORKCLASS_ALIASED( HalloweenPickup, DT_CHalloweenPickup )

BEGIN_NETWORK_TABLE( CHalloweenPickup, DT_CHalloweenPickup )
END_NETWORK_TABLE()

BEGIN_DATADESC( CHalloweenPickup )
	DEFINE_KEYFIELD( m_iszSound, FIELD_STRING, "pickup_sound" ),
	DEFINE_KEYFIELD( m_iszParticle, FIELD_STRING, "pickup_particle" ),

#ifdef GAME_DLL
	DEFINE_OUTPUT( m_OnRedPickup, "OnRedPickup" ),
	DEFINE_OUTPUT( m_OnBluePickup, "OnBluePickup" ),
#endif
END_DATADESC();
										 
LINK_ENTITY_TO_CLASS( tf_halloween_pickup, CHalloweenPickup );

// ************************************************************************************
BEGIN_DATADESC( CBonusDuckPickup )
//	DEFINE_KEYFIELD( m_iszSound, FIELD_STRING, "pickup_sound" ),
//	DEFINE_KEYFIELD( m_iszParticle, FIELD_STRING, "pickup_particle" ),
END_DATADESC();

IMPLEMENT_NETWORKCLASS_ALIASED( BonusDuckPickup, DT_CBonusDuckPickup )

BEGIN_NETWORK_TABLE( CBonusDuckPickup, DT_CBonusDuckPickup )
#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bSpecial ) ),
#else
	RecvPropBool( RECVINFO( m_bSpecial ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_bonus_duck_pickup, CBonusDuckPickup );
// ************************************************************************************
#ifdef GAME_DLL
LINK_ENTITY_TO_CLASS( tf_halloween_gift_spawn_location, CHalloweenGiftSpawnLocation );
#endif
// ************************************************************************************
IMPLEMENT_NETWORKCLASS_ALIASED( HalloweenGiftPickup, DT_CHalloweenGiftPickup )

BEGIN_NETWORK_TABLE( CHalloweenGiftPickup, DT_CHalloweenGiftPickup )
#ifdef CLIENT_DLL
RecvPropEHandle( RECVINFO( m_hTargetPlayer ) ),
#else
SendPropEHandle( SENDINFO( m_hTargetPlayer ) ),
#endif
END_NETWORK_TABLE()

BEGIN_DATADESC( CHalloweenGiftPickup )
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_halloween_gift_pickup, CHalloweenGiftPickup );
// ************************************************************************************

// ************************************************************************************


ConVar tf_halloween_gift_lifetime( "tf_halloween_gift_lifetime", "240", FCVAR_CHEAT | FCVAR_REPLICATED );


//=============================================================================
//
// CTF Halloween Pickup functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHalloweenPickup::CHalloweenPickup()
{
#ifdef GAME_DLL
	ChangeTeam( TEAM_UNASSIGNED );
#endif

	m_iszSound = MAKE_STRING( "Halloween.Quack" );
	m_iszParticle = MAKE_STRING( "halloween_explosion" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHalloweenPickup::~CHalloweenPickup()
{
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the pickup
//-----------------------------------------------------------------------------
void CHalloweenPickup::Precache( void )
{
	// We deliberately allow late precaches here
	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );
	PrecacheScriptSound( TF_HALLOWEEN_PICKUP_DEFAULT_SOUND );
	if ( m_iszSound != NULL_STRING )
	{
		PrecacheScriptSound( STRING( m_iszSound ) );
	}
	if ( m_iszParticle != NULL_STRING )
	{
		PrecacheParticleSystem( STRING( m_iszParticle ) );
	}
	BaseClass::Precache();
	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CHalloweenPickup::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CHalloweenPickup::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	return FL_EDICT_ALWAYS;
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the pickup
//-----------------------------------------------------------------------------
bool CHalloweenPickup::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		bSuccess = true;

		switch( pPlayer->GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			m_OnBluePickup.FireOutput( this, this );
			break;
		case TF_TEAM_RED:
			m_OnRedPickup.FireOutput( this, this );
			break;
		}

		Vector vecOrigin = GetAbsOrigin() + Vector( 0, 0, 32 );
		CPVSFilter filter( vecOrigin );
		if ( m_iszSound != NULL_STRING )
		{
			EmitSound( filter, entindex(), STRING( m_iszSound ) );
		}
		else
		{
			EmitSound( filter, entindex(), TF_HALLOWEEN_PICKUP_DEFAULT_SOUND );
		}

		if ( m_iszParticle != NULL_STRING )
		{
			TE_TFParticleEffect( filter, 0.0, STRING( m_iszParticle ), vecOrigin, vec3_angle );
		}

		// Increment score directly during 2014 halloween
		if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
		{
			inputdata_t inputdata;

			inputdata.pActivator = NULL;
			inputdata.pCaller = NULL;
			inputdata.value.SetInt( 1 );
			inputdata.nOutputID = 0;

			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
			{
				CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame()->InputScoreTeamRed( inputdata );
			}
			else
			{
				CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame()->InputScoreTeamBlue( inputdata );
			}
		}

		if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
			if ( pTFPlayer )
			{
				pTFPlayer->AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_COLLECT_DUCKS );

				IGameEvent *pEvent = gameeventmanager->CreateEvent( "halloween_duck_collected" );
				if ( pEvent )
				{
					pEvent->SetInt( "collector", pTFPlayer->GetUserID() );
					gameeventmanager->FireEvent( pEvent, true );
				}
			}
		}
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHalloweenPickup::ValidTouch( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		return false;

	return BaseClass::ValidTouch( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CHalloweenPickup::GetRespawnDelay( void )
{
	return TF_HALLOWEEN_PICKUP_RETURN_DELAY;
}

//-----------------------------------------------------------------------------
// Purpose: Do everything that our base does, but don't change our origin
//-----------------------------------------------------------------------------
CBaseEntity* CHalloweenPickup::Respawn( void )
{
	SetTouch( NULL );
	AddEffects( EF_NODRAW );

	VPhysicsDestroyObject();

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_TRIGGER );

	m_bRespawning = true;

	//UTIL_SetOrigin( this, g_pGameRules->VecItemRespawnSpot( this ) );// blip to whereever you should respawn.
	SetAbsAngles( g_pGameRules->VecItemRespawnAngles( this ) );// set the angles.

#if !defined( TF_DLL )
	UTIL_DropToFloor( this, MASK_SOLID );
#endif

	RemoveAllDecals(); //remove any decals

	SetThink ( &CItem::Materialize );
	SetNextThink( gpGlobals->curtime + GetRespawnDelay() );
	return this;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHalloweenPickup::ItemCanBeTouchedByPlayer( CBasePlayer *pPlayer )
{
	if ( m_flThrowerTouchTime > 0.f && gpGlobals->curtime < m_flThrowerTouchTime )
	{
		return false;
	}

	return BaseClass::ItemCanBeTouchedByPlayer( pPlayer );
}

#endif // GAME_DLL

// ***********************************************************************************************
ConVar tf_duck_allow_team_pickup( "tf_duck_allow_team_pickup", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
CBonusDuckPickup::CBonusDuckPickup()
{
#ifdef GAME_DLL
	ChangeTeam( TEAM_UNASSIGNED );

	m_iCreatorId = -1;
	m_iVictimId = -1;
	m_iAssisterId = -1;
	m_iFlags = 0;
#else
	pGlowEffect = NULL;
#endif

	m_bSpecial = false;
	m_iszSound = MAKE_STRING( BONUS_DUCK_CREATED_SOUND );
	m_iszParticle = MAKE_STRING( "duck_pickup" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBonusDuckPickup::~CBonusDuckPickup()
{
#ifdef GAME_DLL
	m_flLifeTime = 0;
#else
	if ( pGlowEffect )
	{
		ParticleProp()->StopEmission( pGlowEffect );
		pGlowEffect = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
void CBonusDuckPickup::Precache( void )
{
	// We deliberately allow late precaches here
	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );
	PrecacheParticleSystem( BONUS_DUCK_GLOW );
	PrecacheParticleSystem( BONUS_DUCK_TRAIL_RED );
	PrecacheParticleSystem( BONUS_DUCK_TRAIL_BLUE );
	PrecacheParticleSystem( BONUS_DUCK_TRAIL_SPECIAL_RED );
	PrecacheParticleSystem( BONUS_DUCK_TRAIL_SPECIAL_BLUE );
	PrecacheScriptSound( BONUS_DUCK_CREATED_SOUND );
	BaseClass::Precache();
	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBonusDuckPickup::ValidTouch( CBasePlayer *pPlayer )
{
	// Is the item enabled?
	if ( IsDisabled() )
		return false;

	// Only touch a live player.
	if ( !pPlayer || !pPlayer->IsPlayer() || !pPlayer->IsAlive() )
		return false;

	if ( ( GetTeamNumber() >= FIRST_GAME_TEAM ) && ( pPlayer->GetTeamNumber() == GetTeamNumber() ) )
		return false;

	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		return false;

	return true;
}
//-----------------------------------------------------------------------------
#define DUCK_BLINK_TIME 3.0f
void CBonusDuckPickup::Spawn( void )
{
	BaseClass::Spawn();
	//SetCycle( RandomFloat(0, 60.0f) );

	//align to the ground so we're not standing on end
	QAngle angle = vec3_angle;
	// rotate randomly in yaw
	angle[1] = random->RandomFloat( 0, 360 );
	SetAbsAngles( angle );

	float flLifeTime = GetLifeTime();
	m_flKillTime = gpGlobals->curtime + flLifeTime;
	m_nBlinkCount = 0;
	SetContextThink( &CBonusDuckPickup::BlinkThink,  gpGlobals->curtime + flLifeTime - DUCK_BLINK_TIME, "BonusDuckBlinkThink" );
	SetContextThink( &CBonusDuckPickup::UpdateCollisionBounds, gpGlobals->curtime + 2.0f, "UpdateCollisionBoundsThink" );
}
//-----------------------------------------------------------------------------
bool CBonusDuckPickup::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( tf_duck_allow_team_pickup.GetBool() || ValidTouch( pPlayer ) )
	{
		bSuccess = true;

		Vector vecOrigin = GetAbsOrigin();
		CPVSFilter pvsFilter( vecOrigin );
		if ( m_iszSound != NULL_STRING )
		{
			EmitSound( pvsFilter, entindex(), STRING( m_iszSound ) );
		}
		else
		{
			EmitSound( pvsFilter, entindex(), TF_HALLOWEEN_PICKUP_DEFAULT_SOUND );
		}

		if ( m_iszParticle != NULL_STRING )
		{
			TE_TFParticleEffect( pvsFilter, 0.0, STRING( m_iszParticle ), vecOrigin, vec3_angle );
		}

		if ( m_bSpecial )
		{
			CSingleUserRecipientFilter userfilter( pPlayer );
			UserMessageBegin( userfilter, "BonusDucks" );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_BYTE( true );
			MessageEnd();
		}

		// Notify User that they picked up a EOTL duck if the holiday is active
		if ( pPlayer && TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_EOTL ) && !TFGameRules()->HaveCheatsBeenEnabledDuringLevel() )
		{
			int iFlags = m_iFlags;
			if ( m_bSpecial )
			{
				iFlags |= DUCK_FLAG_BONUS;
			}

			// Send Message to Toucher and Creator if Creator is same team as toucher
			// Tell your team you picked up a duck
			// IsCreated, ID of Creator, ID of Victim, Count, IsGolden

			// Message to Toucher
			{
				CSingleUserRecipientFilter userfilter( pPlayer );
				UserMessageBegin( userfilter, "EOTLDuckEvent" );
				WRITE_BYTE( false );
				WRITE_BYTE( m_iCreatorId );
				WRITE_BYTE( m_iVictimId );
				WRITE_BYTE( pPlayer->entindex() );
				WRITE_BYTE( GetTeamNumber() );
				WRITE_BYTE( 1 );
				WRITE_BYTE( iFlags );
				MessageEnd();
			}

			// Notify Creator
			if ( m_iCreatorId != pPlayer->entindex() )
			{
				CBasePlayer *pCreator = UTIL_PlayerByIndex( m_iCreatorId );
				if ( pCreator && pCreator->InSameTeam( pPlayer ) )
				{
					CSingleUserRecipientFilter userfilter( pCreator );
					UserMessageBegin( userfilter, "EOTLDuckEvent" );
					WRITE_BYTE( false );
					WRITE_BYTE( m_iCreatorId );
					WRITE_BYTE( m_iVictimId );
					WRITE_BYTE( pPlayer->entindex() );
					WRITE_BYTE( GetTeamNumber() );
					WRITE_BYTE( 1 );
					WRITE_BYTE( iFlags );
					MessageEnd();
				}
			}

			// Notify Assister someone picked up their duck as well
			if ( m_iAssisterId != -1 && m_iAssisterId != pPlayer->entindex() )
			{
				CBasePlayer *pAssister = UTIL_PlayerByIndex( m_iAssisterId );
				if ( pAssister && pAssister->InSameTeam( pPlayer ) )
				{
					CSingleUserRecipientFilter userfilter( pAssister );
					UserMessageBegin( userfilter, "EOTLDuckEvent" );
					WRITE_BYTE( false );
					WRITE_BYTE( m_iAssisterId );
					WRITE_BYTE( m_iVictimId );
					WRITE_BYTE( pPlayer->entindex() );
					WRITE_BYTE( GetTeamNumber() );
					WRITE_BYTE( 1 );
					WRITE_BYTE( iFlags );
					MessageEnd();
				}
			}
		}
	}

	return bSuccess;
}
//-----------------------------------------------------------------------------
void CBonusDuckPickup::DropSingleInstance( Vector &vecLaunchVel, CBaseCombatCharacter *pThrower, float flThrowerTouchDelay, float flResetTime /*= 0.1f*/ )
{
	// Remove ourselves after some time
	SetContextThink( &CBonusDuckPickup::NotifyFadeOut, gpGlobals->curtime + GetLifeTime(), "CBonusDuckPreRemoveThink" );

	BaseClass::DropSingleInstance( vecLaunchVel, pThrower, flThrowerTouchDelay, flResetTime );
}
//-----------------------------------------------------------------------------
void CBonusDuckPickup::NotifyFadeOut( void )
{
	//// Notify User that they picked up a EOTL duck if the holiday is active
	//if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_EOTL ) )
	//{
	//	int iFlags = 0;
	//	if ( m_bSpecial )
	//	{
	//		iFlags |= DUCK_FLAG_BONUS;
	//	}
	//	// Tell your team you picked up a duck
	//	// IsCreated, ID of Creator, ID of Victim, Count, IsGolden
	//	CTeamRecipientFilter userfilter( GetTeamNumber(), true );
	//	UserMessageBegin( userfilter, "EOTLDuckEvent" );
	//		WRITE_BYTE( false );
	//		WRITE_BYTE( m_iCreatorId );
	//		WRITE_BYTE( m_iVictimId );
	//		WRITE_BYTE( 0 );
	//		WRITE_BYTE( GetTeamNumber() );
	//		WRITE_BYTE( 1 );
	//		WRITE_BYTE( iFlags );
	//	MessageEnd();
	//}
}
//-----------------------------------------------------------------------------
void CBonusDuckPickup::UpdateCollisionBounds()
{
	CollisionProp()->SetCollisionBounds( Vector( -50, -50, -50 ), Vector( 50, 50, 50 ) );
}
//-----------------------------------------------------------------------------
void CBonusDuckPickup::BlinkThink()
{
	float flTimeToKill = m_flKillTime - gpGlobals->curtime;
	float flNextBlink = RemapValClamped( flTimeToKill, DUCK_BLINK_TIME, 0.f, 0.3f, 0.05f );

	SetContextThink( &CBonusDuckPickup::BlinkThink, gpGlobals->curtime + flNextBlink, "BonusDuckBlinkThink" );

	SetRenderMode( kRenderTransAlpha );

	++m_nBlinkCount;
	if ( m_nBlinkCount % 2 == 0 )
	{
		SetRenderColorA( 50 );
	}
	else
	{
		SetRenderColorA( 255 );
	}
}
#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBonusDuckPickup::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( !IsDormant() )
		{

			if ( pGlowEffect )
			{
				ParticleProp()->StopEmission( pGlowEffect );
				pGlowEffect = NULL;
			}

			if ( m_bSpecial )
			{
				pGlowEffect = ParticleProp()->Create( BONUS_DUCK_GLOW, PATTACH_ABSORIGIN_FOLLOW, 0, Vector( 0, 0, 10 ) );

				// these are fire and forget
				ParticleProp()->Create( ( GetTeamNumber() == TF_TEAM_RED ) ? BONUS_DUCK_TRAIL_SPECIAL_RED : BONUS_DUCK_TRAIL_SPECIAL_BLUE, PATTACH_ABSORIGIN_FOLLOW );
			}
			else
			{
				// these are fire and forget
				ParticleProp()->Create( ( GetTeamNumber() == TF_TEAM_RED ) ? BONUS_DUCK_TRAIL_RED : BONUS_DUCK_TRAIL_BLUE, PATTACH_ABSORIGIN_FOLLOW );
			}

			CPVSFilter filter( GetAbsOrigin() );
			EmitSound( filter, entindex(), BONUS_DUCK_CREATED_SOUND );
		}
	}
}

#endif // GAME_DLL


//-----------------------------------------------------------------------------
// Purpose: Halloween Gift Spawn
//-----------------------------------------------------------------------------
#ifdef GAME_DLL
CHalloweenGiftSpawnLocation::CHalloweenGiftSpawnLocation()
{
}
#endif
//-----------------------------------------------------------------------------
CHalloweenGiftPickup::CHalloweenGiftPickup()
{
	m_hTargetPlayer = NULL;
#ifdef CLIENT_DLL
	m_pPreviousTargetPlayer = NULL;
#endif
}
//-----------------------------------------------------------------------------
void CHalloweenGiftPickup::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "sf15.Merasmus.Gargoyle.Spawn" );
	PrecacheScriptSound( "sf15.Merasmus.Gargoyle.Gone" );
	PrecacheScriptSound( "sf15.Merasmus.Gargoyle.Got" );
}

//------------------------------------------------------------------------
void CHalloweenGiftPickup::Spawn( void )
{
	BaseClass::Spawn();
#ifdef GAME_DLL
	// Set a timer
	SetContextThink( &CHalloweenGiftPickup::DespawnGift, gpGlobals->curtime + tf_halloween_gift_lifetime.GetInt(), "DespawnGift" );
	AddSpawnFlags( SF_NORESPAWN );
#endif // CLIENT_DLL
}

#ifdef GAME_DLL
//------------------------------------------------------------------------
// Despawn (and notify client) and then remove
//------------------------------------------------------------------------
void CHalloweenGiftPickup::DespawnGift()
{
	SetTargetPlayer( NULL );
	SetContextThink( &CHalloweenGiftPickup::RemoveGift, gpGlobals->curtime + 1.0, "RemoveGift" );
}
//------------------------------------------------------------------------
void CHalloweenGiftPickup::RemoveGift()
{
	UTIL_Remove( this );
}

//------------------------------------------------------------------------
void CHalloweenGiftPickup::SetTargetPlayer( CTFPlayer *pTarget )
{
	m_hTargetPlayer = pTarget; 
}
//------------------------------------------------------------------------
bool CHalloweenGiftPickup::ValidTouch( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer && pTFPlayer != m_hTargetPlayer.Get() )
		return false;

	return true;
}
//------------------------------------------------------------------------
bool CHalloweenGiftPickup::MyTouch( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer && pTFPlayer != m_hTargetPlayer.Get() )
		return false;

	// TODO: Give contract points

	// Visual effects
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );

	TE_TFParticleEffect( filter, 0.0, "duck_collect_green", vecOrigin, vec3_angle );

	// Sound effects
	CSingleUserRecipientFilter touchingFilter( pPlayer );
	EmitSound( touchingFilter, entindex(), "Halloween.PumpkinPickup" );
	EmitSound( touchingFilter, entindex(), "sf15.Merasmus.Gargoyle.Got" );

	// Give souls to the collecting player
	for( int i=0; i<10; ++i )
	{
		TFGameRules()->DropHalloweenSoulPack( 1, vecOrigin, pPlayer, TEAM_SPECTATOR );
	}

	// Achievement
	if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_MANN_MANOR ) )
	{
		pTFPlayer->AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_COLLECT_GOODY_BAG );
	}
	return true;
}

#endif // GAME_DLL

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
void CHalloweenGiftPickup::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_DATATABLE_CHANGED )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			// Gift Added
			if ( m_hTargetPlayer.Get() != NULL && m_pPreviousTargetPlayer == NULL && m_hTargetPlayer.Get() == pLocalPlayer  )
			{
				// Notification
				CEconNotification *pNotification = new CEconNotification();
				pNotification->SetText( "#TF_HalloweenItem_SoulAppeared" );
				pNotification->SetLifetime( 5.0f );
				pNotification->SetSoundFilename( "ui/halloween_loot_spawn.wav" );
				NotificationQueue_Add( pNotification );
				pLocalPlayer->EmitSound( "sf15.Merasmus.Gargoyle.Spawn" );
			}
			// Gift Despawned
			if ( m_hTargetPlayer.Get() == NULL && m_pPreviousTargetPlayer != NULL && m_pPreviousTargetPlayer == pLocalPlayer )
			{
				// Notification
				CEconNotification *pNotification = new CEconNotification();
				pNotification->SetText( "#TF_HalloweenItem_SoulDisappeared" );
				pNotification->SetLifetime( 5.0f );
				pNotification->SetSoundFilename( "ui/halloween_loot_found.wav" );
				NotificationQueue_Add( pNotification );
				pLocalPlayer->EmitSound( "sf15.Merasmus.Gargoyle.Gone" );
			}

			m_pPreviousTargetPlayer = m_hTargetPlayer.Get();
		}
	}

}
//------------------------------------------------------------------------
bool CHalloweenGiftPickup::ShouldDraw()
{
	CTFPlayer *pOwner = m_hTargetPlayer.Get();
	if ( pOwner != C_TFPlayer::GetLocalTFPlayer() )
		return false;

	return BaseClass::ShouldDraw();
}
#endif


