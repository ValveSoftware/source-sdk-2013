//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_buff_item.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_baseentity.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// Weapon Buff Item tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBuffItem, DT_TFWeaponBuffItem )

BEGIN_NETWORK_TABLE( CTFBuffItem, DT_TFWeaponBuffItem )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBuffItem )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_buff_item, CTFBuffItem );
PRECACHE_WEAPON_REGISTER( tf_weapon_buff_item );

// Models and Buffs need to be aligned
const char* BannerModels[] =
{
	"models/weapons/c_models/c_buffbanner/c_buffbanner.mdl",
	"models/workshop/weapons/c_models/c_battalion_buffbanner/c_battalion_buffbanner.mdl",
	"models/workshop_partner/weapons/c_models/c_shogun_warbanner/c_shogun_warbanner.mdl",
	"models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_parachute.mdl"
};

#define CLOSED_PARACHUTE_MDL "models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_pack.mdl"
#define OPEN_PARACHUTE_MDL "models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_pack_open.mdl"

COMPILE_TIME_ASSERT( ARRAYSIZE( BannerModels ) == NUM_BUFF_ITEM_TYPES );
//=============================================================================
//
// Weapon Buff Item functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFBuffItem::CTFBuffItem()
{
#ifdef CLIENT_DLL
	ListenForGameEvent( "deploy_buff_banner" );
	m_iBuffType = -1;
	m_hBannerEntity = NULL;
#endif
	m_bPlayingHorn = false;

	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFBuffItem::~CTFBuffItem()
{
#ifdef CLIENT_DLL
	if ( m_hBannerEntity )
	{
		m_hBannerEntity->Release();
		m_hBannerEntity = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBuffItem::Precache()
{
	// Need to make this dynamic, but the item data isn't available here?
	for ( int i=0; i<ARRAYSIZE(BannerModels); i++ )
	{
		PrecacheModel( BannerModels[i] );
	}

	PrecacheModel( "models/weapons/c_models/c_buffpack/c_buffpack.mdl" );
	PrecacheModel( "models/workshop/weapons/c_models/c_battalion_buffpack/c_battalion_buffpack.mdl" );
	PrecacheModel( "models/workshop_partner/weapons/c_models/c_shogun_warpack/c_shogun_warpack.mdl" );
	PrecacheModel( OPEN_PARACHUTE_MDL );
	PrecacheModel( CLOSED_PARACHUTE_MDL );

	PrecacheScriptSound( "Weapon_BuffBanner.HornRed" );
	PrecacheScriptSound( "Weapon_BuffBanner.HornBlue" );
	PrecacheScriptSound( "Weapon_BattalionsBackup.HornRed" );
	PrecacheScriptSound( "Weapon_BattalionsBackup.HornBlue" );
	PrecacheScriptSound( "Weapon_BuffBanner.Flag" );
	PrecacheScriptSound( "Samurai.Conch" );

	BaseClass::Precache();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFBuffItem::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	CTFPlayer* pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( pPlayer->m_Shared.GetRageMeter() < 100.f )
		return;

	if ( m_bPlayingHorn )
		return;

	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

	SetContextThink( &CTFBuffItem::BlowHorn, gpGlobals->curtime + 0.22f, "BlowHorn" );
}


//-----------------------------------------------------------------------------
// Purpose: Attaches the item to the player.
//-----------------------------------------------------------------------------
void CTFBuffItem::Equip( CBaseCombatCharacter* pOwner )
{
	BaseClass::Equip( pOwner );

	CTFPlayer *pTFPlayer = ToTFPlayer( pOwner );
	if ( pTFPlayer )
	{
		pTFPlayer->m_Shared.SetParachuteEquipped( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove item from the player.
//-----------------------------------------------------------------------------
void CTFBuffItem::Detach( void )
{
	CTFPlayer *pTFPlayer = GetTFPlayerOwner();
	if ( pTFPlayer )
	{
		pTFPlayer->m_Shared.SetParachuteEquipped( false );
	}

	BaseClass::Detach();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::BlowHorn( void )
{
	CTFPlayer* pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsAlive() )
		return;

#ifdef GAME_DLL
	// Use the buff type to try and figure out what sort of mesh we have and what
	// sort of sound to play. This isn't the cleanest thing in the world.
	int iBuffType = 0;
	CALL_ATTRIB_HOOK_INT( iBuffType, set_buff_type );

	// Samurai conch shell.
	if ( iBuffType == EConcheror )
	{
		pPlayer->EmitSound( "Samurai.Conch" );
	}
	// Bugle.
	else if ( iBuffType == EBuffBanner )
	{
		if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
		{
			pPlayer->EmitSound( "Weapon_BuffBanner.HornBlue" );
		}
		else
		{
			pPlayer->EmitSound( "Weapon_BuffBanner.HornRed" );
		}
	}
	else if ( iBuffType == EBattalion )
	{
		if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
		{
			pPlayer->EmitSound( "Weapon_BattalionsBackup.HornBlue" );
		}
		else
		{
			pPlayer->EmitSound( "Weapon_BattalionsBackup.HornRed" );
		}
	}

	// Strange Tracking
	if ( !pPlayer->IsBot() )
	{
		EconEntity_OnOwnerKillEaterEventNoPartner( dynamic_cast<CEconEntity *>( this ), pPlayer, kKillEaterEvent_BannersDeployed );
	}
#endif

	m_bPlayingHorn = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::RaiseFlag( void )
{
	CTFPlayer* pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsAlive() )
		return;

	m_bPlayingHorn = false;

	int iBuffType = 0;
	CALL_ATTRIB_HOOK_INT( iBuffType, set_buff_type );

#if GAME_DLL
	// this event needs to be sent before we call ActivateSoldierBuff() below
	IGameEvent* event = gameeventmanager->CreateEvent( "deploy_buff_banner" );
	if ( event )
	{
		event->SetInt( "buff_type", iBuffType );
		event->SetInt( "buff_owner", pPlayer->GetUserID() );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->EmitSound( "Weapon_BuffBanner.Flag" );
	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_BATTLECRY );
#endif

	pPlayer->m_Shared.ActivateRageBuff( this, iBuffType );
	pPlayer->SelectLastItem();

}

//-----------------------------------------------------------------------------
// Don't let them holster while they are blowing the horn.
//-----------------------------------------------------------------------------
bool CTFBuffItem::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_bPlayingHorn )
	{
		return false;
	}
	else
	{
		return BaseClass::Holster( pSwitchingTo );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::FireGameEvent( IGameEvent* event )
{
#ifdef CLIENT_DLL
	const char *name = event->GetName();

	if ( 0 == Q_strcmp( name, "deploy_buff_banner" ) )
	{
		CTFPlayer* pPlayer = ToTFPlayer( ClientEntityList().GetEnt( engine->GetPlayerForUserID( event->GetInt( "buff_owner" ) ) ) );
		if ( pPlayer == GetOwner() )
		{
			CreateBanner();
		}
	}

	BaseClass::FireGameEvent( event );
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFBuffItem::CreateBanner()
{
#ifdef CLIENT_DLL

	if ( m_hBannerEntity == NULL )
	{
		CTFPlayer* pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( !pPlayer || !pPlayer->IsAlive() )
			return;

		C_TFBuffBanner* pBanner = new C_TFBuffBanner;
		if ( !pBanner )
			return;

		//Assert( iBuffType > 0 );
		//Assert( iBuffType <= ARRAYSIZE(BannerModels) );
		pBanner->m_nSkin = 0;
		pBanner->InitializeAsClientEntity( BannerModels[GetBuffType()-1], RENDER_GROUP_OPAQUE_ENTITY );
		pBanner->SetBuffItem( this );
		pBanner->SetBuffType( GetBuffType() );
		pBanner->ForceClientSideAnimationOn();
		SetBanner( pBanner );
		int iSpine = pPlayer->LookupBone( "bip_spine_3" );
		Assert( iSpine != -1 );
		if ( iSpine != -1 )
		{
			pBanner->AttachEntityToBone( pPlayer, iSpine );
		}
	}
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
void CTFBuffItem::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	if ( state == SHOULDTRANSMIT_END )
	{
		// Kill the item - It will be recreated later
		if ( m_hBannerEntity )
		{
			m_hBannerEntity->Release();
			m_hBannerEntity = NULL;
		}
	}

	BaseClass::NotifyShouldTransmit( state );
}

void CTFBuffItem::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	BaseClass::OnDataChanged( updateType );
}

void CTFBuffItem::ClientThink( void )
{
	if ( m_iBuffType == -1 || m_iBuffType == 0 )
	{
		m_iBuffType = GetBuffType();

		if ( m_iBuffType != EParachute )
		{
			SetNextClientThink( CLIENT_THINK_NEVER );
		}
	}
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_bPlayingHorn = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CTFBuffItem::TranslateViewmodelHandActivityInternal( Activity actBase )
{
	Activity iActivity = actBase;
	switch ( iActivity )
	{
	case ACT_VM_DRAW:
		iActivity = ACT_ITEM1_VM_DRAW;
		break;
	case ACT_VM_HOLSTER:
		iActivity = ACT_ITEM1_VM_HOLSTER;
		break;
	case ACT_VM_IDLE:
		if ( m_bPlayingHorn )
		{
			RaiseFlag();
			iActivity = ACT_RESET;
		}
		else
		{
			iActivity = ACT_ITEM1_VM_IDLE;
		}
		break;
	case ACT_VM_PULLBACK:
		iActivity = ACT_ITEM1_VM_PULLBACK;
		break;
	case ACT_VM_PRIMARYATTACK:
		m_bPlayingHorn = true;
		iActivity = ACT_ITEM1_VM_PRIMARYATTACK;
		break;
	case ACT_VM_SECONDARYATTACK:
		m_bPlayingHorn = true;
		iActivity = ACT_ITEM1_VM_SECONDARYATTACK;
		break;
	case ACT_VM_IDLE_TO_LOWERED:
		iActivity = ACT_ITEM1_VM_IDLE_TO_LOWERED;
		break;
	case ACT_VM_IDLE_LOWERED:
		iActivity = ACT_ITEM1_VM_IDLE_LOWERED;
		break;
	case ACT_VM_LOWERED_TO_IDLE:
		iActivity = ACT_ITEM1_VM_LOWERED_TO_IDLE;
		break;
	default:
		break;
	}

	return BaseClass::TranslateViewmodelHandActivityInternal( iActivity );
}


bool CTFBuffItem::CanReload( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: UI Progress
//-----------------------------------------------------------------------------
float CTFBuffItem::GetProgress( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return 0.f;

	return pPlayer->m_Shared.GetRageMeter() / 100.0f;
}


//-----------------------------------------------------------------------------
// Purpose: UI Progress (same as GetProgress() without the division by 100.0f)
//-----------------------------------------------------------------------------
bool CTFBuffItem::IsFull( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	return ( pPlayer->m_Shared.GetRageMeter() >= 100.0f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFBuffItem::EffectMeterShouldFlash( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	if ( pPlayer && (IsFull() || pPlayer->m_Shared.IsRageDraining()) )
		return true;
	else
		return false;
}
