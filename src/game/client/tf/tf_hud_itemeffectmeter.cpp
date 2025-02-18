//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_hud_itemeffectmeter.h"
#include "tf_weapon_bat.h"
#include "tf_weapon_jar.h"
#include "tf_weapon_jar_gas.h"
#include "tf_weapon_sword.h"
#include "tf_weapon_buff_item.h"
#include "tf_weapon_lunchbox.h"
#include "tf_weapon_shotgun.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_weapon_rocketlauncher.h"
#include "tf_weapon_particle_cannon.h"
#include "tf_weapon_raygun.h"
#include "tf_weapon_flaregun.h"
#include "tf_weapon_revolver.h"
#include "tf_weapon_flamethrower.h"
#include "tf_weapon_knife.h"
#include "tf_item_powerup_bottle.h"
#include "tf_imagepanel.h"
#include "c_tf_weapon_builder.h"
#include "tf_weapon_minigun.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_throwable.h"
#include "tf_weapon_smg.h"
#include "halloween/tf_weapon_spellbook.h"
#include "tf_logic_halloween_2014.h"
#include <game/client/iviewport.h>
#include "tf_weapon_rocketpack.h"
#include "tf_weapon_bonesaw.h"
#include "tf_weapon_slap.h"

#include <vgui_controls/ImagePanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const char *GetDefaultMeterTextForLoadoutPosition( int iLoadout )
{
	const char *pszRetVal = "";
	switch ( iLoadout )
	{
	case LOADOUT_POSITION_PRIMARY:
		pszRetVal = "#TF_PrimaryMeter";
		break;
	case LOADOUT_POSITION_SECONDARY:
		pszRetVal = "#TF_SecondaryMeter";
		break;
	case LOADOUT_POSITION_MELEE:
		pszRetVal = "#TF_MeleeMeter";
		break;
	}

	return pszRetVal;
}

#define DECLARE_ITEM_EFFECT_METER( weaponClass, weaponType, beeps, resfile ) \
	hNewMeter = new CHudItemEffectMeter_Weapon< weaponClass >( pszElementName, pPlayer, weaponType, beeps, resfile ); \
	if ( hNewMeter ) \
	{ \
		gHUD.AddHudElement( hNewMeter ); \
		outMeters.AddToHead( hNewMeter ); \
		hNewMeter->SetVisible( false ); \
	}


extern ConVar tf_rocketpack_cost;


using namespace vgui;

IMPLEMENT_AUTO_LIST( IHudItemEffectMeterAutoList );
											
CItemEffectMeterManager g_ItemEffectMeterManager;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemEffectMeterManager::~CItemEffectMeterManager()
{
	ClearExistingMeters();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemEffectMeterManager::ClearExistingMeters()
{
	for ( int i=0; i<m_Meters.Count(); i++ )
	{
		gHUD.RemoveHudElement( m_Meters[i].Get() );
		delete m_Meters[i].Get();
	}
	m_Meters.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CItemEffectMeterManager::GetNumEnabled( void )
{
	int nCount = 0;

	for ( int i = 0; i < m_Meters.Count(); i++ )
	{
		if ( m_Meters[i] && m_Meters[i]->IsEnabled() )
		{
			nCount++;
		}
	}

	return nCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemEffectMeterManager::SetPlayer( C_TFPlayer* pPlayer )
{
	StopListeningForAllEvents();
	ListenForGameEvent( "post_inventory_application" );	
	ListenForGameEvent( "localplayer_pickup_weapon" );
	ListenForGameEvent( "localplayer_respawn" );

	ClearExistingMeters();

	if ( pPlayer )
	{
		CHudItemEffectMeter::CreateHudElementsForClass( pPlayer, m_Meters );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemEffectMeterManager::Update( C_TFPlayer* pPlayer )
{
	for ( int i=0; i<m_Meters.Count(); i++ )
	{
		if ( m_Meters[i] )
		{
			m_Meters[i]->Update( pPlayer );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemEffectMeterManager::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	bool bNeedsUpdate = false;

	if ( FStrEq( "localplayer_pickup_weapon", type ) ||
		 FStrEq( "localplayer_respawn", type ) )
	{
		bNeedsUpdate = true;
	}
	else if ( FStrEq( "post_inventory_application", type ) )
	{
		// Force a refresh. Our items may have changed causing us to now draw, etc.
		// This creates a new game logic object which will re-cache necessary item data.
		int iUserID = event->GetInt( "userid" );
		C_TFPlayer* pPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( pPlayer && pPlayer->GetUserID() == iUserID )
		{
			bNeedsUpdate = true;
		}
	}

	if ( bNeedsUpdate )
	{
		SetPlayer( C_TFPlayer::GetLocalTFPlayer() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudItemEffectMeter::CHudItemEffectMeter( const char *pszElementName, C_TFPlayer* pPlayer ) : CHudElement( pszElementName ), BaseClass( NULL, "HudItemEffectMeter" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pLabel = new Label( this, "ItemEffectMeterLabel", "" );

	SetHiddenBits( HIDEHUD_MISCSTATUS | HIDEHUD_CLOAK_AND_FEIGN );

	m_pPlayer		= pPlayer;
	m_bEnabled		= true;
	m_flOldProgress = 1.f;
	m_nState		= -1;

	m_pItemEffectIcon = NULL;

	RegisterForRenderGroup( "inspect_panel" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudItemEffectMeter::~CHudItemEffectMeter()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffectMeter::CreateHudElementsForClass( C_TFPlayer* pPlayer, CUtlVector< vgui::DHANDLE< CHudItemEffectMeter > >& outMeters )
{
	vgui::DHANDLE< CHudItemEffectMeter > hNewMeter;
	const char* pszElementName = "HudItemEffectMeter";
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

	static CSchemaAttributeDefHandle attrMeterType( "item_meter_charge_type" );

	auto lambdaAddItemEffectMeter = [&]( const char* pszItemClass, bool bBeep )
	{
		for ( int iLoadout = FIRST_LOADOUT_SLOT_WITH_CHARGE_METER; iLoadout<=LAST_LOADOUT_SLOT_WITH_CHARGE_METER; ++iLoadout )
		{
			CEconEntity *pEconItem = dynamic_cast<CEconEntity *>( pPlayer->GetEntityForLoadoutSlot( iLoadout, true ) );
			if ( !pEconItem )
				continue;

			CEconItemView* pItem = pEconItem->GetAttributeContainer()->GetItem();

			if ( !pItem || !pItem->GetStaticData() || !FStrEq( pItem->GetStaticData()->GetItemClass(), pszItemClass ) )
				continue;

			static CSchemaAttributeDefHandle attrMeterLabel( "meter_label" );
			attrib_value_t retval;

		
			if ( !FindAttribute( pItem, attrMeterType, &retval ) )
				return;

			if ( retval == ATTRIBUTE_METER_TYPE_NONE )
				return;
		
			const char *pszLabelText = "";
			CAttribute_String attrModule;
			if ( !pItem->FindAttribute( attrMeterLabel, &attrModule ) || !attrModule.has_value() )
			{
				pszLabelText = GetDefaultMeterTextForLoadoutPosition( iLoadout );
			}
			else
			{
				pszLabelText = attrModule.value().c_str();
			}

			hNewMeter = new CHudItemEffectMeter_ItemAttribute( pszElementName, pPlayer, (loadout_positions_t)iLoadout, pszLabelText, bBeep );
			if ( hNewMeter )
			{
				gHUD.AddHudElement( hNewMeter );
				outMeters.AddToHead( hNewMeter );
				hNewMeter->SetVisible( false );
			}

			return;
		}
	};

	switch ( iClass )
	{
	case TF_CLASS_SCOUT:
		DECLARE_ITEM_EFFECT_METER( CTFBat_Wood, TF_WEAPON_BAT_WOOD, true, NULL );
		DECLARE_ITEM_EFFECT_METER( CTFBat_Giftwrap, TF_WEAPON_BAT_GIFTWRAP, true, NULL );
		DECLARE_ITEM_EFFECT_METER( CTFLunchBox_Drink, TF_WEAPON_LUNCHBOX, true, "resource/UI/HudItemEffectMeter_Scout.res" );
		DECLARE_ITEM_EFFECT_METER( CTFJarMilk, TF_WEAPON_JAR_MILK, true, "resource/UI/HudItemEffectMeter_Scout.res" );
		DECLARE_ITEM_EFFECT_METER( CTFSodaPopper, TF_WEAPON_SODA_POPPER, true, "resource/UI/HudItemEffectMeter_SodaPopper.res" );
		DECLARE_ITEM_EFFECT_METER( CTFPEPBrawlerBlaster, TF_WEAPON_PEP_BRAWLER_BLASTER, true, "resource/UI/HudItemEffectMeter_SodaPopper.res" );
		DECLARE_ITEM_EFFECT_METER( CTFCleaver, TF_WEAPON_CLEAVER, true, "resource/UI/HudItemEffectMeter_Cleaver.res" );
		break;

	case TF_CLASS_HEAVYWEAPONS:
	{
		lambdaAddItemEffectMeter( "tf_weapon_lunchbox", true );
		DECLARE_ITEM_EFFECT_METER( CTFMinigun, TF_WEAPON_MINIGUN, true, "resource/UI/HudItemEffectMeter_Heavy.res" );
		break;
	}

	case TF_CLASS_SNIPER:
	{
		DECLARE_ITEM_EFFECT_METER( CTFJar, TF_WEAPON_JAR, true, NULL );
		DECLARE_ITEM_EFFECT_METER( CTFSniperRifleDecap, TF_WEAPON_SNIPERRIFLE_DECAP, false, "resource/UI/HudItemEffectMeter_Sniper.res" );
		DECLARE_ITEM_EFFECT_METER( CTFSniperRifle, TF_WEAPON_SNIPERRIFLE, true, "resource/UI/HudItemEffectMeter_SniperFocus.res" );
		DECLARE_ITEM_EFFECT_METER( CTFChargedSMG, TF_WEAPON_CHARGED_SMG, false, NULL );
		lambdaAddItemEffectMeter( "tf_wearable_razorback", true );
		break;
	}
	case TF_CLASS_DEMOMAN:
		DECLARE_ITEM_EFFECT_METER( CTFSword, TF_WEAPON_SWORD, false, "resource/UI/HudItemEffectMeter_Demoman.res" );
		break;

	case TF_CLASS_SOLDIER:
		DECLARE_ITEM_EFFECT_METER( CTFBuffItem, TF_WEAPON_BUFF_ITEM, true, NULL );
		DECLARE_ITEM_EFFECT_METER( CTFParticleCannon, TF_WEAPON_PARTICLE_CANNON, false, "resource/UI/HUDItemEffectMeter_ParticleCannon.res" );
		DECLARE_ITEM_EFFECT_METER( CTFRaygun, TF_WEAPON_RAYGUN, false, "resource/UI/HUDItemEffectMeter_Raygun.res" );
		DECLARE_ITEM_EFFECT_METER( CTFRocketLauncher_AirStrike, TF_WEAPON_ROCKETLAUNCHER, false, "resource/UI/HudItemEffectMeter_Demoman.res" );
		break;

	case TF_CLASS_SPY:
		DECLARE_ITEM_EFFECT_METER( CTFKnife, TF_WEAPON_KNIFE, true, "resource/UI/HUDItemEffectMeter_SpyKnife.res" );

		hNewMeter = new CHudItemEffectMeter( pszElementName, pPlayer );
		if ( hNewMeter )
		{
			gHUD.AddHudElement( hNewMeter );
			outMeters.AddToHead( hNewMeter );
			hNewMeter->SetVisible( false );
		}

		DECLARE_ITEM_EFFECT_METER( C_TFWeaponBuilder, TF_WEAPON_BUILDER, true, "resource/UI/HudItemEffectMeter_Sapper.res" );
		DECLARE_ITEM_EFFECT_METER( CTFRevolver, TF_WEAPON_REVOLVER, false, "resource/UI/HUDItemEffectMeter_Spy.res" );

		break;

	case TF_CLASS_ENGINEER:
		DECLARE_ITEM_EFFECT_METER( CTFShotgun_Revenge, TF_WEAPON_SENTRY_REVENGE, false, "resource/UI/HUDItemEffectMeter_Engineer.res" );
		DECLARE_ITEM_EFFECT_METER( CTFDRGPomson, TF_WEAPON_DRG_POMSON, false, "resource/UI/HUDItemEffectMeter_Pomson.res" );
		DECLARE_ITEM_EFFECT_METER( CTFRevolver, TF_WEAPON_REVOLVER, false, "resource/UI/HUDItemEffectMeter_Spy.res" );
		break;

	case TF_CLASS_PYRO:
	{
		DECLARE_ITEM_EFFECT_METER( CTFFlameThrower, TF_WEAPON_FLAMETHROWER, true, "resource/UI/HudItemEffectMeter_Pyro.res" );
		DECLARE_ITEM_EFFECT_METER( CTFFlareGun_Revenge, TF_WEAPON_FLAREGUN_REVENGE, false, "resource/UI/HUDItemEffectMeter_Engineer.res" );
		DECLARE_ITEM_EFFECT_METER( CTFRocketPack, TF_WEAPON_ROCKETPACK, false, "resource/UI/HudRocketPack.res" );
		lambdaAddItemEffectMeter( "tf_weapon_jar_gas", true );
		lambdaAddItemEffectMeter( "tf_weapon_rocketlauncher_fireball", false );
		break;
	}
	case TF_CLASS_MEDIC:
		DECLARE_ITEM_EFFECT_METER( CWeaponMedigun, TF_WEAPON_MEDIGUN, true, "resource/UI/HudItemEffectMeter_Scout.res" );
		DECLARE_ITEM_EFFECT_METER( CTFBonesaw, TF_WEAPON_BONESAW, false, "resource/UI/HUDItemEffectMeter_Organs.res" );
		break;
	}

	// ALL CLASS
	DECLARE_ITEM_EFFECT_METER( CTFThrowable, TF_WEAPON_THROWABLE, true, "resource/UI/HudItemEffectMeter_Action.res" );

	// Kill Streak
	DECLARE_ITEM_EFFECT_METER( CTFWeaponBase, TF_WEAPON_NONE, false, "resource/UI/HudItemEffectMeter_KillStreak.res" );

	DECLARE_ITEM_EFFECT_METER( CTFSpellBook, TF_WEAPON_SPELLBOOK, true, "resource/UI/HudItemEffectMeter_KartCharge.res" );
	/*hNewMeter = new CHudItemEffectMeter_HalloweenSouls( pszElementName, pPlayer );
	if ( hNewMeter )
	{
		gHUD.AddHudElement( hNewMeter );
		outMeters.AddToHead( hNewMeter );
		hNewMeter->SetVisible( false );
	}*/

	// Mvm canteen
	hNewMeter = new CHudItemEffectMeter_Weapon< CTFPowerupBottle >( pszElementName, pPlayer, TF_WEAPON_NONE, true, "resource/UI/HudItemEffectMeter_PowerupBottle.res" );
	if ( hNewMeter )
	{
		gHUD.AddHudElement( hNewMeter );
		outMeters.AddToHead( hNewMeter );
		hNewMeter->SetVisible( false );
	}

	hNewMeter = new CHudItemEffectMeter_Rune( pszElementName, pPlayer );
	if ( hNewMeter )
	{
		gHUD.AddHudElement( hNewMeter );
		outMeters.AddToHead( hNewMeter );
		hNewMeter->SetVisible( false );
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffectMeter::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( GetResFile() );

	BaseClass::ApplySchemeSettings( pScheme );

	SetLabelText();

	m_pItemEffectIcon = dynamic_cast< CTFImagePanel* >( FindChildByName( "ItemEffectIcon" ) );
	if ( m_pItemEffectIcon )
	{
		m_pItemEffectIcon->SetImage( GetIconName() );
	}

	m_vecProgressBars.Purge();
	for ( int i = 0; i < GetNumProgressBar(); ++i )
	{
		CFmtStr strEffectMeter;
		const char *pszProgressBarName = "ItemEffectMeter";
		if ( i != 0 )
		{
			strEffectMeter.sprintf( "ItemEffectMeter%d", i + 1 );
			pszProgressBarName = strEffectMeter;
		}
		ContinuousProgressBar *pProgressBar = dynamic_cast< ContinuousProgressBar* >( FindChildByName( pszProgressBarName ) );
		if ( pProgressBar )
		{
			m_vecProgressBars.AddToTail( pProgressBar );
		}
		else
		{
			Warning( "%s missing ContinuousProgressBar field \"%s\"\n", GetResFile(), pszProgressBarName );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffectMeter::PerformLayout()
{
	BaseClass::PerformLayout();

	// update icon
	if ( m_pItemEffectIcon )
	{
		m_pItemEffectIcon->SetImage( GetIconName() );
	}

	// update label text
	SetLabelText();

	if ( ShouldAutoAdjustPosition() )
	{
		// slide over by 1 for medic
		int iOffset = 0;
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer && pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC )
		{
			iOffset = 1;
		}

		if ( g_ItemEffectMeterManager.GetNumEnabled() + iOffset > 1 )
		{
			int xPos = 0, yPos = 0;
			GetPos( xPos, yPos );
			SetPos( xPos - m_iXOffset, yPos );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudItemEffectMeter::ShouldDraw( void )
{
	bool bShouldDraw = true;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->IsAlive() || pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
	{
		bShouldDraw = false;
	}
	else if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
	{
		bShouldDraw = false;
	}
	else if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
	{
		bShouldDraw = false;
	}
	else if ( IsEnabled() )
	{
		bShouldDraw = CHudElement::ShouldDraw();
	}
	else
	{
		bShouldDraw = false;
	}

	if ( IsVisible() != bShouldDraw )
	{
		SetVisible( bShouldDraw );

		if ( bShouldDraw )
		{
			// if we're going to be visible, redo our layout
			InvalidateLayout( false, true );
		}
	}

	return bShouldDraw;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffectMeter::Update( C_TFPlayer* pPlayer )
{
	if ( !IsEnabled() )
		return;

	if ( !pPlayer )
		return;

	// Progress counts override progress bars.
	int iCount = GetCount();
	if ( iCount >= 0 )
	{
		if ( ShowPercentSymbol() )
		{
			SetDialogVariable( "progresscount", VarArgs( "%d%%", iCount ) );
		}
		else
		{
			SetDialogVariable( "progresscount", iCount );
		}
	}

	float flProgress = GetProgress();

	// Play a sound if we are refreshed.
	if ( C_TFPlayer::GetLocalTFPlayer() && flProgress >= 1.f && m_flOldProgress < 1.f &&
		pPlayer->IsAlive() && ShouldBeep() )
	{
		m_flOldProgress = flProgress;
		C_TFPlayer::GetLocalTFPlayer()->EmitSound( GetBeepSound() );
	}
	else
	{
		m_flOldProgress = flProgress;
	}

	const float flMaxProgressPerBar = 1.f / GetNumProgressBar();

	// Update the meter GUI element.
	FOR_EACH_VEC( m_vecProgressBars, i )
	{
		float flCurrentBarProgress = Min( flProgress - flMaxProgressPerBar * i, flMaxProgressPerBar );
		m_vecProgressBars[i]->SetProgress( RemapValClamped( flCurrentBarProgress, 0.f, flMaxProgressPerBar, 0.f, 1.f ) );

		// Flash the bar if this class implementation requires it.
		if ( ShouldFlash() )
		{
			int color_offset = ( ( int )( gpGlobals->realtime * 10 ) ) % 10;
			int red = 160 + ( color_offset * 10 );
			m_vecProgressBars[i]->SetFgColor( Color( red, 0, 0, 255 ) );
		}
		else
		{
			m_vecProgressBars[i]->SetFgColor( GetProgressBarColor() );
		}
	}

	// update these when state changes
	int nCurrentState = GetState();
	if ( m_nState != nCurrentState )
	{
		InvalidateLayout();

		m_nState = nCurrentState;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char*	CHudItemEffectMeter::GetLabelText( void )
{
	if ( m_pPlayer && m_pPlayer->IsPlayerClass( TF_CLASS_SPY ) )
	{
		CTFWeaponInvis *pWpn = (CTFWeaponInvis *)m_pPlayer->Weapon_OwnsThisID( TF_WEAPON_INVIS );
		if ( pWpn )
		{
			if ( pWpn->HasFeignDeath() )
			{
				return "#TF_Feign";
			}
			else if ( pWpn->HasMotionCloak() )
			{
				return "#TF_CloakDagger";
			}
		}
	}

	return "#TF_CLOAK"; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CHudItemEffectMeter::GetProgress( void )
{
	if ( m_pPlayer )
		return m_pPlayer->m_Shared.GetSpyCloakMeter() / 100.0f;
	else
		return 1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffectMeter::SetLabelText( const char *pszText /*= NULL*/ )
{
	const char *pszLabel = ( pszText && pszText[0] ) ? pszText : GetLabelText();

	const wchar_t *pLocalized = g_pVGuiLocalize->Find( pszLabel );
	if ( pLocalized )
	{
		wchar_t wszLabel[128];
		V_wcsncpy( wszLabel, pLocalized, sizeof( wszLabel ) );

		wchar_t wszFinalLabel[128];
		UTIL_ReplaceKeyBindings( wszLabel, 0, wszFinalLabel, sizeof( wszFinalLabel ), ::input->IsSteamControllerActive() ? GAME_ACTION_SET_FPSCONTROLS : GAME_ACTION_SET_NONE );

		m_pLabel->SetText( wszFinalLabel, true );
	}
	else
	{
		m_pLabel->SetText( pszLabel );
	}

	m_pLabel->SetFgColor( GetLabelTextColor() );
}

//-----------------------------------------------------------------------------
// Purpose: Tracks the weapon's regen.
//-----------------------------------------------------------------------------
template <class T>
CHudItemEffectMeter_Weapon<T>::CHudItemEffectMeter_Weapon( const char* pszElementName, C_TFPlayer* pPlayer, int iWeaponID, bool bBeeps, const char* pszResFile )
	: CHudItemEffectMeter( pszElementName, pPlayer )
{
	m_iWeaponID = iWeaponID;
	m_bBeeps = bBeeps;
	m_pszResFile = pszResFile;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template <class T>
const char* CHudItemEffectMeter_Weapon<T>::GetResFile( void )
{
	if ( m_pszResFile )
	{
		return m_pszResFile;
	}
	else
	{
		return "resource/UI/HudItemEffectMeter.res";
	}
}

//-----------------------------------------------------------------------------
// Purpose: Caches the weapon reference.
//-----------------------------------------------------------------------------
template <class T>
T* CHudItemEffectMeter_Weapon<T>::GetWeapon( void )
{
	if ( m_bEnabled && m_pPlayer && !m_hWeapon )
	{
		m_hWeapon = dynamic_cast<T*>( m_pPlayer->Weapon_OwnsThisID( m_iWeaponID ) );
		if ( !m_hWeapon )
		{
			m_bEnabled = false;
		}
	}

	return m_hWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template <class T>
bool CHudItemEffectMeter_Weapon<T>::IsEnabled( void )
{
	T *pWeapon = GetWeapon();
	if ( pWeapon )
		return true;
	
	return CHudItemEffectMeter::IsEnabled();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template <class T>
float CHudItemEffectMeter_Weapon<T>::GetProgress( void )
{
	T *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->GetProgress();
	}
	else
	{
		return 0.f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template <class T>
void CHudItemEffectMeter_Weapon<T>::Update( C_TFPlayer* pPlayer )
{
	CHudItemEffectMeter::Update( pPlayer );
}

//-----------------------------------------------------------------------------
template <class T>
bool CHudItemEffectMeter_Weapon<T>::ShouldDraw( )
{
	return CHudItemEffectMeter::ShouldDraw();
}
//-----------------------------------------------------------------------------
// Specializations for meters that do unique things follow...
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFWeaponBase>::IsEnabled( void )
{
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pTFPlayer )
	{
		int iKillStreak = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFPlayer, iKillStreak, killstreak_tier );
		return iKillStreak != 0;
	}
	return false;
}

//-----------------------------------------------------------------------------
template <>
int CHudItemEffectMeter_Weapon<CTFWeaponBase>::GetCount( void )
{
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pTFPlayer )
	{
		return pTFPlayer->m_Shared.GetStreak( CTFPlayerShared::kTFStreak_Kills );
	}
	return 0;
}

//-----------------------------------------------------------------------------
template <>
const char*	CHudItemEffectMeter_Weapon<CTFWeaponBase>::GetLabelText( void )
{
	return "TF_KillStreak";
}

//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFWeaponBase>::IsKillstreakMeter( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Specialization for the sword (to differentiate it from other sword-type weapons)
//-----------------------------------------------------------------------------
template <>
CTFSword* CHudItemEffectMeter_Weapon<CTFSword>::GetWeapon( void )
{
	if ( m_bEnabled && m_pPlayer && !m_hWeapon )
	{
		m_hWeapon = dynamic_cast<CTFSword*>( m_pPlayer->Weapon_OwnsThisID( m_iWeaponID ) );

		if ( m_hWeapon && !m_hWeapon->CanDecapitate() )
			m_hWeapon = NULL;

		if ( !m_hWeapon )
		{
			m_bEnabled = false;
		}
	}

	return m_hWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Specialization for demoman head count.
//-----------------------------------------------------------------------------
template <>
int CHudItemEffectMeter_Weapon<CTFSword>::GetCount( void )
{
	CTFSword *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->GetCount();
	}
	else
	{
		return 0.f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Soldier buff charge bar specialization for flashing...
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFBuffItem>::ShouldFlash( void )
{
	CTFBuffItem *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->EffectMeterShouldFlash();
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFBuffItem>::ShouldDraw()
{
	if ( !m_pPlayer )
		return false;

	CTFBuffItem *pWeapon = GetWeapon();
	if ( !pWeapon )
		return false;

	// do not draw for the parachute
	if ( pWeapon->GetBuffType() == EParachute )
		return false;
	
	return CHudItemEffectMeter::ShouldDraw();
}

//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFFlameThrower>::IsEnabled( void )
{
	if ( !m_pPlayer )
		return false;

	CTFFlameThrower *pWeapon = GetWeapon();
	if ( !pWeapon )
		return false;

	return ( pWeapon->GetBuffType() > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Pyro rage bar specialization for flashing...
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFFlameThrower>::ShouldFlash( void )
{
	CTFFlameThrower *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->EffectMeterShouldFlash();
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Soda Popper Flash
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFSodaPopper>::ShouldFlash( void )
{
	if ( !m_pPlayer )
		return false;

	return m_pPlayer->m_Shared.GetScoutHypeMeter() >= 100.0f;
}

//-----------------------------------------------------------------------------
// Charged SMG Flash
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFChargedSMG>::ShouldFlash( void )
{
	if ( !m_pPlayer )
		return false;

	CTFChargedSMG* pWeapon = GetWeapon();
	if ( !pWeapon )
		return false;

	return pWeapon->ShouldFlashChargeBar();
}

//-----------------------------------------------------------------------------
// Purpose: Heavy rage bar in MvM
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon< CTFMinigun >::IsEnabled( void )
{
	if ( !m_pPlayer )
		return false;

	CTFMinigun *pWeapon = GetWeapon();
	if ( !pWeapon )
		return false;

	bool bVisible = false;


	int iRage = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pPlayer, iRage, generate_rage_on_dmg );
	if ( iRage )
	{
		bVisible = true;
	}

	return bVisible;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon< CTFMinigun >::ShouldFlash( void )
{
	CTFMinigun *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->EffectMeterShouldFlash();
	}
	else
	{
		return false;
	}
}

static const char *pszClassIcons[] = {
	"",
	"../hud/leaderboard_class_scout",
	"../hud/leaderboard_class_sniper",
	"../hud/leaderboard_class_soldier",
	"../hud/leaderboard_class_demo",
	"../hud/leaderboard_class_medic",
	"../hud/leaderboard_class_heavy",
	"../hud/leaderboard_class_pyro",
	"../hud/leaderboard_class_spy",
	"../hud/leaderboard_class_engineer",
};

template <>
void CHudItemEffectMeter_Weapon< CTFMinigun >::Update( C_TFPlayer* pPlayer )
{
	CTFMinigun *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		float fKillComboFireRateBoost = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, fKillComboFireRateBoost, kill_combo_fire_rate_boost );
		if ( fKillComboFireRateBoost > 0.0f )
		{
			SetControlVisible( "ItemEffectMeterLabel2", true );

			int nKillComboClass = pWeapon->GetKillComboClass();
			int nKillComboCount = pWeapon->GetKillComboCount();

			ImagePanel *( pImagePanel[3] ) =
			{
				assert_cast< ImagePanel* >( FindChildByName( "KillComboClassIcon1" ) ),
				assert_cast< ImagePanel* >( FindChildByName( "KillComboClassIcon2" ) ),
				assert_cast< ImagePanel* >( FindChildByName( "KillComboClassIcon3" ) )
			};

			for ( int i = 0; i < ARRAYSIZE( pImagePanel ); ++i )
			{
				if ( !pImagePanel[i] )
					continue;

				if ( nKillComboCount > i )
				{
					pImagePanel[i]->SetImage( pszClassIcons[nKillComboClass] );
					pImagePanel[i]->SetVisible( true );
				}
				else
				{
					pImagePanel[i]->SetVisible( false );
				}
			}
		}
		else
		{
			SetControlVisible( "ItemEffectMeterLabel2", false );
			SetControlVisible( "KillComboClassIcon1", false );
			SetControlVisible( "KillComboClassIcon2", false );
			SetControlVisible( "KillComboClassIcon3", false );
		}
	}
	CHudItemEffectMeter::Update( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Specialization for engineer revenge count.
//-----------------------------------------------------------------------------
template <>
int CHudItemEffectMeter_Weapon<CTFShotgun_Revenge>::GetCount( void )
{
	CTFShotgun_Revenge *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->GetCount();
	}
	else
	{
		return 0.f;
	}
}

template <>
bool CHudItemEffectMeter_Weapon<CTFFlareGun_Revenge>::IsEnabled( void )
{
	if ( !m_pPlayer )
		return false;

	CTFFlareGun_Revenge *pWeapon = GetWeapon();
	return pWeapon && pWeapon->IsActiveByLocalPlayer();
}

template <>
int CHudItemEffectMeter_Weapon<CTFFlareGun_Revenge>::GetCount( void )
{
	CTFFlareGun_Revenge *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->GetCount();
	}
	else
	{
		return 0.f;
	}
}

template <>
const char *CHudItemEffectMeter_Weapon<CTFRocketPack>::GetLabelText( void )
{
	CTFRocketPack *pRocketpack = GetWeapon();
	if ( pRocketpack )
	{
		return pRocketpack->IsEnabled() ? "#TF_RocketPack_Charges" : "#TF_RocketPack_Disabled";
	}

	return CHudItemEffectMeter::GetLabelText();
}

template <>
const char *CHudItemEffectMeter_Weapon<CTFRocketPack>::GetIconName( void )
{
	CTFRocketPack *pRocketpack = GetWeapon();
	if ( pRocketpack && pRocketpack->IsEnabled() )
	{

		// enabled
		return "../hud/pyro_jetpack";
	}

	// disabled
	return "../hud/pyro_jetpack_off2";
}

template <>
int CHudItemEffectMeter_Weapon<CTFRocketPack>::GetNumProgressBar( void ) const
{
	return 2;
}

template <>
Color CHudItemEffectMeter_Weapon<CTFRocketPack>::GetProgressBarColor( void )
{	
	if ( m_pPlayer )
	{
		if ( !m_pPlayer->m_Shared.IsRocketPackReady() )
		{
			return Color( 255, 0, 0, 255 );
		}
		else
		{
			return Color( 255, 255, 255, 255 );
		}
	}

	return CHudItemEffectMeter::GetProgressBarColor();
}

template <>
float CHudItemEffectMeter_Weapon<CTFRocketPack>::GetProgress( void )
{
	if ( m_pPlayer )
		return m_pPlayer->m_Shared.GetRocketPackCharge() / 100.0f;
	return 0;
}

template <>
Color CHudItemEffectMeter_Weapon<CTFRocketPack>::GetLabelTextColor( void )
{
	CTFRocketPack *pRocketpack = GetWeapon();
	if ( pRocketpack )
	{
		return pRocketpack->IsEnabled() ? Color( 235, 235, 235, 255 ) : Color( 178, 178, 178, 255 );
	}

	return CHudItemEffectMeter::GetLabelTextColor();
}

template <>
int CHudItemEffectMeter_Weapon<CTFRocketPack>::GetState( void )
{
	CTFRocketPack *pRocketpack = GetWeapon();
	if ( pRocketpack )
	{
		enum
		{
			ROCKETPACK_DISABLED = 0,
			ROCKETPACK_ENABLED,
			ROCKETPACK_WAITFORPASSENGER,
			ROCKETPACK_HASPASSENGER
		};

		if ( pRocketpack && pRocketpack->IsEnabled() )
		{

			return ROCKETPACK_ENABLED;
		}

		return ROCKETPACK_DISABLED;
	}

	return CHudItemEffectMeter::GetState();
}

template <>
bool CHudItemEffectMeter_Weapon<CTFRocketPack>::ShouldAutoAdjustPosition( void ) const
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <>
int CHudItemEffectMeter_Weapon<CTFSniperRifleDecap>::GetCount( void )
{
	CTFSniperRifleDecap *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->GetCount();
	}
	else
	{
		return 0.f;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------		    
template <>
Color CHudItemEffectMeter_Weapon<CTFParticleCannon>::GetProgressBarColor( void )
{
	CTFParticleCannon *pWeapon = GetWeapon();
	
	if ( pWeapon && pWeapon->CanChargeFire() )
		return Color( 255, 255, 255, 255 );
	else
		return Color( 255, 0, 0, 255 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <>
int CHudItemEffectMeter_Weapon<CTFRevolver>::GetCount( void )
{
	CTFRevolver *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->GetCount();
	}
	else
	{
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFRevolver>::IsEnabled( void )
{
	CTFRevolver *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		int iExtraDamageOnHit = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iExtraDamageOnHit, extra_damage_on_hit );
		return pWeapon->SapperKillsCollectCrits() || iExtraDamageOnHit;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFRevolver>::ShowPercentSymbol( void )
{
	CTFRevolver *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		int iExtraDamageOnHit = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iExtraDamageOnHit, extra_damage_on_hit );
		return iExtraDamageOnHit;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFKnife>::IsEnabled( void )
{
	CTFKnife *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->GetKnifeType() == KNIFE_ICICLE;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFSniperRifle>::IsEnabled( void )
{
	if ( !m_pPlayer )
		return false;

	CTFSniperRifle *pWeapon = GetWeapon();
	if ( !pWeapon )
		return false;

	return ( pWeapon->GetBuffType() > 0 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFSniperRifle>::ShouldFlash( void )
{
	CTFSniperRifle *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->EffectMeterShouldFlash();
	}
	else
	{
		return false;
	}
}

template <>
const char*	CHudItemEffectMeter_Weapon<CTFSniperRifle>::GetBeepSound( void )
{
	CTFSniperRifle *pWeapon = GetWeapon();
	if ( pWeapon && pWeapon->GetBuffType() > 0 )
	{
		return "Weapon_Bison.SingleCrit";
	}

	return CHudItemEffectMeter::GetBeepSound();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<C_TFWeaponBuilder>::IsEnabled( void )
{
	if ( !m_pPlayer )
		return false;

	return ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<C_TFWeaponBuilder>::ShouldBeep( void )
{
	if ( !m_pPlayer )
		return false;

	int iRoboSapper = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pPlayer, iRoboSapper, robo_sapper );

	return ( iRoboSapper > 0 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<C_TFWeaponBuilder>::ShouldFlash( void )
{
	if ( !m_pPlayer )
		return false;

	C_TFWeaponBuilder *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->EffectMeterShouldFlash();
	}
	else
	{
		return false;
	}
}

template <>
CTFPowerupBottle* CHudItemEffectMeter_Weapon<CTFPowerupBottle>::GetWeapon( void )
{
	if ( m_bEnabled && m_pPlayer && !m_hWeapon )
	{
		for ( int i = 0; i < m_pPlayer->GetNumWearables(); ++i )
		{
			m_hWeapon = dynamic_cast<CTFPowerupBottle*>( m_pPlayer->GetWearable( i ) );
			if ( m_hWeapon )
			{
				break;
			}
		}

		if ( !m_hWeapon )
		{
			m_bEnabled = false;
		}
	}

	return m_hWeapon;
}

template <>
bool CHudItemEffectMeter_Weapon<CTFPowerupBottle>::IsEnabled( void )
{
	if ( !m_pPlayer )
		return false;

	CTFPowerupBottle *pPowerupBottle = GetWeapon();
	if ( pPowerupBottle )
	{
		return ( pPowerupBottle->GetNumCharges() > 0 );
	}

	return false;
}

template <>
int CHudItemEffectMeter_Weapon<CTFPowerupBottle>::GetCount( void )
{
	CTFPowerupBottle *pPowerupBottle = GetWeapon();
	if ( pPowerupBottle )
	{
		return pPowerupBottle->GetNumCharges();
	}
	else
	{
		return 0;
	}
}

template <>
const char* CHudItemEffectMeter_Weapon<CTFPowerupBottle>::GetIconName( void )
{
	CTFPowerupBottle *pPowerupBottle = GetWeapon();
	if ( pPowerupBottle )
	{
		return pPowerupBottle->GetEffectIconName();
	}

	return CHudItemEffectMeter::GetIconName();
}

//-----------------------------------------------------------------------------
// Purpose: Medic "rage" bar in MvM
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon< CWeaponMedigun >::IsEnabled( void )
{
	if ( !m_pPlayer )
		return false;

	int iRage = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pPlayer, iRage, generate_rage_on_heal );
	if ( !iRage )
		return false;

	CWeaponMedigun *pWeapon = GetWeapon();
	if ( !pWeapon )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CWeaponMedigun>::ShouldFlash( void )
{
	CWeaponMedigun *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->EffectMeterShouldFlash();
	}
	else
	{
		return false;
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon< CTFLunchBox >::IsEnabled( void )
{
	return CHudItemEffectMeter::IsEnabled();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template <>
float CHudItemEffectMeter_Weapon< CTFLunchBox >::GetProgress( void )
{
	if ( m_pPlayer )
		return m_pPlayer->m_Shared.GetItemChargeMeter( LOADOUT_POSITION_SECONDARY ) / 100.f;
	
	return 0.f;
}

//-----------------------------------------------------------------------------
// CTFThrowable
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFThrowable>::IsEnabled( void )
{
	CTFThrowable *pWep = dynamic_cast<CTFThrowable*>( GetWeapon() );
	if ( pWep )
	{
		return pWep->ShowHudElement();
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudItemEffectMeter_Rune::CHudItemEffectMeter_Rune( const char *pszElementName, C_TFPlayer* pPlayer ) : CHudItemEffectMeter( pszElementName, pPlayer )
{

}

//-----------------------------------------------------------------------------
bool CHudItemEffectMeter_Rune::IsEnabled( void )
{
	return m_pPlayer && m_pPlayer->m_Shared.CanRuneCharge();
}

//-----------------------------------------------------------------------------
float CHudItemEffectMeter_Rune::GetProgress( void )
{
	if ( m_pPlayer )
		return m_pPlayer->m_Shared.GetRuneCharge() / 100.0f;
	return 0;
}

//-----------------------------------------------------------------------------
bool CHudItemEffectMeter_Rune::ShouldFlash( void )
{
	if ( m_pPlayer )
		return m_pPlayer->m_Shared.IsRuneCharged();
	return false;
}

//-----------------------------------------------------------------------------
bool CHudItemEffectMeter_Rune::ShouldDraw( void )
{
	return m_pPlayer && m_pPlayer->m_Shared.CanRuneCharge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon< CTFBonesaw >::IsEnabled( void )
{
	if ( !m_pPlayer )
		return false;

	float flPreserveUber = 0.f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pPlayer, flPreserveUber, ubercharge_preserved_on_spawn_max );
	if ( !flPreserveUber )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template <>
int CHudItemEffectMeter_Weapon< CTFBonesaw >::GetCount( void )
{
	if ( !m_pPlayer )
		return 0;

	return m_pPlayer->m_Shared.GetDecapitations();
}


//-----------------------------------------------------------------------------
// Rocket Launcher AirStrike Headcounter
//-----------------------------------------------------------------------------
template <>
int CHudItemEffectMeter_Weapon<CTFRocketLauncher_AirStrike>::GetCount( void )
{
	CTFRocketLauncher_AirStrike *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		return pWeapon->GetCount();
	}
	else
	{
		return 0;
	}
}

//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFSpellBook>::IsEnabled( void )
{
	if ( !m_pPlayer )
		return false;

	if ( !m_pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return false;

	CTFSpellBook *pWep = dynamic_cast<CTFSpellBook*>( GetWeapon() );
	if ( pWep )
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
template <>
float CHudItemEffectMeter_Weapon<CTFSpellBook>::GetProgress( void )
{
	if ( !m_pPlayer )
		return 0;

	if ( !m_pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return 0;

	return m_pPlayer->GetKartSpeedBoost();
}

//-----------------------------------------------------------------------------
template <>
int CHudItemEffectMeter_Weapon<CTFSpellBook>::GetCount( void )
{
	if ( !m_pPlayer )
		return 0;

	if ( !m_pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return 0;

	return m_pPlayer->GetKartHealth();
}
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFSpellBook>::ShowPercentSymbol( void )
{
	return true;
}
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFSpellBook>::ShouldDraw( void )
{
	if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
	{
		if ( m_pPlayer && m_pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
			return false;

		IViewPortPanel *scoreboard = gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD );
		if ( scoreboard && scoreboard->IsVisible() )
			return false;

		if ( TFGameRules() && ( TFGameRules()->State_Get() != GR_STATE_RND_RUNNING ) )
			return false;

		return true;
	}
	
	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
	{
		return false;
	}

	return CHudItemEffectMeter::ShouldDraw();
}


//-----------------------------------------------------------------------------
CHudItemEffectMeter_ItemAttribute::CHudItemEffectMeter_ItemAttribute( const char *pszElementName, C_TFPlayer *pPlayer, loadout_positions_t iLoadoutSlot, const char *pszLabelText /*= NULL*/, bool bBeeps /*= true*/ )
	: CHudItemEffectMeter( pszElementName, pPlayer )
	, m_pMeterEntity( NULL )
{
	m_iLoadoutSlot = iLoadoutSlot;
	CBaseEntity* pEntity = pPlayer->GetEntityForLoadoutSlot( m_iLoadoutSlot, true );
	IHasGenericMeter* pMeter = dynamic_cast< IHasGenericMeter* >( pEntity );
	if ( pMeter )
	{
		m_hEntity = pEntity;
		m_pMeterEntity = pMeter;
	}
	else
	{
		Assert( false );
	}

	m_bBeeps = bBeeps;
	m_strLabelText = pszLabelText;

	if ( !m_hEntity )
	{
		m_bEnabled = false;
	}

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 ); 
}


const IHasGenericMeter *CHudItemEffectMeter_ItemAttribute::GetItem()
{
	return m_hEntity.Get() ? m_pMeterEntity : NULL;
}

//-----------------------------------------------------------------------------
float CHudItemEffectMeter_ItemAttribute::GetProgress( void )
{
	if ( !GetItem() || !m_pPlayer )
		return 0.f;

	return m_pPlayer->m_Shared.GetItemChargeMeter( m_iLoadoutSlot ) / 100.f;
}

//-----------------------------------------------------------------------------
bool CHudItemEffectMeter_ItemAttribute::ShouldDraw( void )
{
	if ( !GetItem() || !GetItem()->ShouldDrawMeter() )
		return false;

	return CHudItemEffectMeter::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffectMeter_ItemAttribute::OnTick( void )
{
	// if the handle for the item is no longer valid, look for it again
	if ( !GetItem() )
	{
		CBaseEntity* pEntity = m_pPlayer->GetEntityForLoadoutSlot( m_iLoadoutSlot, true );
		if ( pEntity )
		{
			m_hEntity = pEntity;
			m_pMeterEntity = dynamic_cast< IHasGenericMeter* >( pEntity );
		}
	}

	CHudItemEffectMeter::OnTick();
}
