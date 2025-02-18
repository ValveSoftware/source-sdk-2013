//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

#include "c_tf_player.h"
#include "collisionutils.h"
#include "econ_item_inventory.h"
#include "iclientmode.h"
#include "tf_gcmessages.h"
#include "tf_gamerules.h"
#include "econ_notifications.h"
#include "rtime.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "achievements_tf.h"
#include "gc_clientsystem.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
EHalloweenMap GetHalloweenMap()
{
	if ( FStrEq( engine->GetLevelName(), "maps/cp_manor_event.bsp" ) )
		return kHalloweenMap_MannManor;

	if ( FStrEq( engine->GetLevelName(), "maps/koth_viaduct_event.bsp" ) )
		return kHalloweenMap_Viaduct;

	if ( FStrEq( engine->GetLevelName(), "maps/koth_lakeside_event.bsp" ) )
		return kHalloweenMap_Lakeside;

	if ( FStrEq( engine->GetLevelName(), "maps/plr_hightower_event.bsp" ) )
		return kHalloweenMap_Hightower;

	return kHalloweenMapCount;
}


//-----------------------------------------------------------------------------
// Created when the GC decides to give out an item.
// A player must intersect the item to claim it.
//-----------------------------------------------------------------------------
#define HALLOWEEN_ITEM_TIME_TO_READY 10.0f
class C_HalloweenItemPickup	: public CBaseAnimating
{
	DECLARE_CLASS( C_HalloweenItemPickup, CBaseAnimating );
public:
	C_HalloweenItemPickup()
		: m_bReadyForPickup( false )
		, m_bClaimed( false )
		, m_flTimeToReady( 0.0f )
	{
		AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );
	}

	virtual ~C_HalloweenItemPickup()
	{
	}

	bool Initialize()
	{
		const char *pszModelName = "models/props_halloween/halloween_gift.mdl";
		SetModelName( AllocPooledString( pszModelName ) );

		if ( InitializeAsClientEntity( STRING(GetModelName()), RENDER_GROUP_OPAQUE_ENTITY ) == false )
			return false;
				
		const model_t *mod = GetModel();
		if ( mod )
		{
			Vector mins, maxs;
			modelinfo->GetModelBounds( mod, mins, maxs );
			SetCollisionBounds( mins, maxs );
		}

		Spawn();

		// initialize as translucent		
		float alpha = 0.0f;
		SetRenderMode( kRenderTransTexture );		
		SetRenderColorA( alpha * 256 );
		m_flTimeToReady = gpGlobals->realtime + HALLOWEEN_ITEM_TIME_TO_READY;
		
		UpdatePartitionListEntry();
		
		SetBlocksLOS( false ); // this should be a small object
		
		// Set up shadows; do it here so that objects can change shadowcasting state
		CreateShadow();
		
		UpdateVisibility();
		
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		
		return true;
	}

	virtual void Spawn()
	{
		Precache();
		BaseClass::Spawn();
		SetSolid( SOLID_NONE );	
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetMoveType( MOVETYPE_NONE );
	}

	virtual void ClientThink()
	{
		if ( m_bReadyForPickup )
		{
			ClientThink_Active();
			return;
		}

		float flTimeDelta = m_flTimeToReady - gpGlobals->realtime;
		if ( flTimeDelta < 0 )
		{
			m_bReadyForPickup = true;
			ParticleProp()->Create( "halloween_pickup_active", PATTACH_ABSORIGIN_FOLLOW );
			SetRenderMode( kRenderNormal );
		}
		else
		{
			float alpha = 0.75f * ( ( HALLOWEEN_ITEM_TIME_TO_READY - flTimeDelta ) / HALLOWEEN_ITEM_TIME_TO_READY );
			SetRenderColorA( alpha * 256 );
		}		
	}

	void ClientThink_Active( void )
	{
		Vector vWorldMins = WorldAlignMins();
		Vector vWorldMaxs = WorldAlignMaxs();
		Vector vBoxMin1 = GetAbsOrigin() + vWorldMins;
		Vector vBoxMax1	= GetAbsOrigin() + vWorldMaxs;

		float flBestDistance2 = 0.0f;
		CSteamID bestSteamID;
		bool bBestHasNoclip = false;

#define CLIENT_HALLOWEEN_LOGIC_ENABLE_LOCAL_PLAYER_ONLY 1

#if !CLIENT_HALLOWEEN_LOGIC_ENABLE_LOCAL_PLAYER_ONLY
		for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
#else
		do
		{
			C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
#endif
			CSteamID steamID;
			if ( pPlayer == NULL || pPlayer->IsBot() == true || pPlayer->GetSteamID( &steamID ) == false || 
				 ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE ) ||
				 pPlayer->IsAlive() == false ||
				 pPlayer->GetObserverMode() != OBS_MODE_NONE )
			{
				continue;
			}
			
			Vector vPlayerMins = pPlayer->GetAbsOrigin() + pPlayer->WorldAlignMins();
			Vector vPlayerMaxs = pPlayer->GetAbsOrigin() + pPlayer->WorldAlignMaxs();
			bool bIntersecting = IsBoxIntersectingBox( vBoxMin1, vBoxMax1, vPlayerMins, vPlayerMaxs );
			float flDistance2 = ( pPlayer->GetAbsOrigin(), GetAbsOrigin() ).LengthSqr();
			if ( bIntersecting && ( bestSteamID.GetAccountID() == 0 || flDistance2 < flBestDistance2 ) )
			{
				bestSteamID = steamID;
				bBestHasNoclip = pPlayer->GetMoveType() == MOVETYPE_NOCLIP;
				flBestDistance2 = flDistance2;
			}
		}
#if CLIENT_HALLOWEEN_LOGIC_ENABLE_LOCAL_PLAYER_ONLY
		while ( false );
#endif

		if ( bestSteamID.GetAccountID() != 0 )
		{
			GCSDK::CProtoBufMsg<CMsgGC_Halloween_GrantItem> msg( k_EMsgGC_Halloween_GrantItem );
			msg.Body().set_recipient_account_id( bestSteamID.GetAccountID() );
			msg.Body().set_level_id( GetHalloweenMap() );
			msg.Body().set_flagged( bBestHasNoclip );
			GCClientSystem()->BSendMessage( msg );
			OnClaimed( true );
			return;
		}

		SetNextClientThink( gpGlobals->curtime + 0.33f );
	}

	void OnClaimed( bool bPlayAudio )
	{
		if ( m_bClaimed )
			return;

		m_bClaimed = true;
		// stop thinking and remove sparkle...
		ParticleProp()->StopParticlesNamed( "halloween_pickup_active", true );
		SetNextClientThink( CLIENT_THINK_NEVER );
		// throw up bday confetti
		DispatchParticleEffect( "halloween_gift_pickup", GetAbsOrigin(), vec3_angle );

		if ( bPlayAudio )
		{
			C_BaseEntity::EmitSound( "Game.HappyBirthday" );
		}
		SetRenderMode( kRenderNone );
		UpdateVisibility();
	}

	bool m_bReadyForPickup;
	bool m_bClaimed;
	float m_flTimeToReady;
};

LINK_ENTITY_TO_CLASS( tf_halloween_item_pickup, C_HalloweenItemPickup );
PRECACHE_REGISTER( tf_halloween_item_pickup );

static EHANDLE gHalloweenPickup;

#ifdef _DEBUG
CON_COMMAND( cl_halloween_test_cheating, "Test cheating the halloween pickup" )
{
	GCSDK::CProtoBufMsg< CMsgGC_Halloween_GrantItem > msg( k_EMsgGC_Halloween_GrantItem );
	msg.Body().set_recipient_account_id( steamapicontext->SteamUser()->GetSteamID().GetAccountID() );
	GCClientSystem()->BSendMessage( msg );
}

CON_COMMAND( cl_halloween_test_spawn_pickup, "Test spawning the pickup item" )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer == NULL )
		return;

	// Now create the pickup item
	C_HalloweenItemPickup *pEntity = new C_HalloweenItemPickup();
	if ( !pEntity )
		return;
	
	Vector vecTargetPoint;
	trace_t tr;
	Vector forward;
	pLocalPlayer->EyeVectors( &forward );
	UTIL_TraceLine( pLocalPlayer->EyePosition(),
					pLocalPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
					pLocalPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction != 1.0 )
	{
		vecTargetPoint = tr.endpos;
	}

	pEntity->SetAbsOrigin( vecTargetPoint );
	if ( !pEntity->Initialize() )
	{
		pEntity->Release();
	}
	else
	{
		if ( gHalloweenPickup.Get() )
			gHalloweenPickup->Release();
		gHalloweenPickup = pEntity;
	}
}
#endif

//-----------------------------------------------------------------------------
// GC has decided to drop a Halloween item
//-----------------------------------------------------------------------------
//class CGCHalloween_ReservedItem : public GCSDK::CGCClientJob
//{
//public:
//	CGCHalloween_ReservedItem( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
//
//	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
//	{
//		GCSDK::CProtoBufMsg<CMsgGC_Halloween_ReservedItem> msg( pNetPacket );
//
//		// Figure out which level we're on so we know how to handle notifications, etc.
//		EHalloweenMap eMap = GetHalloweenMap();
//		if ( eMap == kHalloweenMapCount )
//			return true;
//
//		// Sanity-check the message contents from the GC.
//		if ( msg.Body().x_size() != msg.Body().y_size() || msg.Body().y_size() != msg.Body().z_size() )
//			return true;
//
//		if ( msg.Body().x_size() <= eMap )
//			return true;
//
//		// Don't spawn gifts during startup.
//		if ( TFGameRules() == NULL 
//			|| TFGameRules()->State_Get() != GR_STATE_RND_RUNNING 
//			|| TFGameRules()->InSetup() 
//			|| TFGameRules()->IsInWaitingForPlayers()
//			|| TFGameRules()->ArePlayersInHell() )	// Dont spawn gifts if players are in 2013 Hell
//			return true;
//
//		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
//		if ( pLocalPlayer == NULL )
//			return true;
//
//		// If we don't already know about this gift pickup, create one.
//		if ( gHalloweenPickup.Get() == NULL )
//		{
//			// Now create the pickup item
//			C_HalloweenItemPickup *pEntity = new C_HalloweenItemPickup();
//			if ( !pEntity )
//				return true;
//		
//			Vector position( msg.Body().x( eMap ), msg.Body().y( eMap ), msg.Body().z( eMap ) );
//			pEntity->SetAbsOrigin( position );
//			if ( !pEntity->Initialize() )
//			{
//				pEntity->Release();
//			}
//			else
//			{
//				gHalloweenPickup = pEntity;
//			}
//		}
//
//		// Regardless of whether we created a new gift or whether this was a new notification about an old gift,
//		// display a UI notification for the user.
//		CEconNotification *pNotification = new CEconNotification();
//		pNotification->SetText( "#TF_HalloweenItem_Reserved" );
//		pNotification->SetLifetime( 15.0f );
//		pNotification->SetSoundFilename( "ui/halloween_loot_spawn.wav" );
//		NotificationQueue_Add( pNotification );
//
//		return true;
//	}
//};
//GC_REG_JOB( GCSDK::CGCClient, CGCHalloween_ReservedItem, "CGCHalloween_ReservedItem", k_EMsgGC_Halloween_ReservedItem, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------
// GC gave out the Halloween item to a player
//-----------------------------------------------------------------------------
//class CGCHalloween_GrantedItemResponse : public GCSDK::CGCClientJob
//{
//public:
//	CGCHalloween_GrantedItemResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
//
//	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
//	{
//		GCSDK::CProtoBufMsg<CMsgGC_Halloween_GrantItemResponse> msg( pNetPacket );
//
//		// which Steam universe are we in?
//		EUniverse eUniverse = steamapicontext && steamapicontext->SteamUtils()
//			? steamapicontext->SteamUtils()->GetConnectedUniverse()
//			: k_EUniverseInvalid;
//
//		CSteamID steamIDRecipient( msg.Body().recipient_account_id(), eUniverse, k_EAccountTypeIndividual );
//		bool bIsValidRecipient = steamIDRecipient.IsValid();
//
//		if ( gHalloweenPickup.Get() != NULL )
//		{
//			assert_cast<C_HalloweenItemPickup *>( gHalloweenPickup.Get() )->OnClaimed( bIsValidRecipient );
//			gHalloweenPickup->Release();
//			gHalloweenPickup = NULL;
//		}
//
//		// don't do any work if we're not on a Halloween map
//		EHalloweenMap eMap = GetHalloweenMap();
//		if ( eMap == kHalloweenMapCount )
//			return true;
//		
//		// add alert
//		const char* pPlayerName = InventoryManager()->PersonaName_Get( steamIDRecipient.GetAccountID() );
//		wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH] = L"";
//		if ( pPlayerName != NULL && FStrEq( pPlayerName, "" ) == false )
//		{
//			g_pVGuiLocalize->ConvertANSIToUnicode( pPlayerName, wszPlayerName, sizeof(wszPlayerName) );
//		}
//		CEconNotification *pNotification = new CEconNotification();
//		pNotification->SetLifetime( 15.0f );
//
//		if ( bIsValidRecipient )
//		{
//			pNotification->SetText( "#TF_HalloweenItem_Granted" );
//			pNotification->AddStringToken( "recipient", wszPlayerName );
//			pNotification->SetSteamID( steamIDRecipient );
//			pNotification->SetSoundFilename( "ui/halloween_loot_found.wav" );
//		}
//		else
//		{
//			pNotification->SetText( "#TF_HalloweenItem_GrantPickupFail" );
//			// pNotification->SetSoundFilename( "coach/coach_student_died.wav" );
//		}
//		
//		NotificationQueue_Add( pNotification );
//
//		// is this the local player? award the achievement...
//		if ( steamapicontext && steamapicontext->SteamUser() )
//		{
//			CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
//			if ( steamIDRecipient == localSteamID )
//			{
//				g_AchievementMgrTF.OnAchievementEvent( ACHIEVEMENT_TF_HALLOWEEN_COLLECT_GOODY_BAG );
//			}
//		}
//
//		return true;
//	}
//};
//GC_REG_JOB( GCSDK::CGCClient, CGCHalloween_GrantedItemResponse, "CGCHalloween_GrantedItemResponse", k_EMsgGC_Halloween_GrantItemResponse, GCSDK::k_EServerTypeGCClient );


//-----------------------------------------------------------------------------

void CL_Halloween_LevelShutdown()
{
	gHalloweenPickup = NULL;
}
