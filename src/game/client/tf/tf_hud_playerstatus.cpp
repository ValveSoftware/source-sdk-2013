//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "c_tf_playerresource.h"
#include "tf_playermodelpanel.h"
#include "econ_item_description.h"

#include "hud_numericdisplay.h"
#include "c_team.h"
#include "c_tf_player.h"
#include "tf_shareddefs.h"
#include "tf_hud_playerstatus.h"
#include "tf_gamerules.h"
#include "tf_logic_halloween_2014.h"
#include "tf_logic_player_destruction.h"

#include "tf_wheel_of_doom.h"

#include "confirm_dialog.h"

using namespace vgui;

ConVar cl_hud_playerclass_use_playermodel( "cl_hud_playerclass_use_playermodel", "1", FCVAR_ARCHIVE, "Use player model in player class HUD." );


ConVar cl_hud_playerclass_playermodel_showed_confirm_dialog( "cl_hud_playerclass_playermodel_showed_confirm_dialog", "0", FCVAR_ARCHIVE | FCVAR_HIDDEN );

extern ConVar tf_max_health_boost;


static const char *g_szBlueClassImages[] = 
{ 
	"",
	"../hud/class_scoutblue", 
	"../hud/class_sniperblue",
	"../hud/class_soldierblue",
	"../hud/class_demoblue",
	"../hud/class_medicblue",
	"../hud/class_heavyblue",
	"../hud/class_pyroblue",
	"../hud/class_spyblue",
	"../hud/class_engiblue",
	"../hud/class_scoutblue",
};

static const char *g_szRedClassImages[] = 
{ 
	"",
	"../hud/class_scoutred", 
	"../hud/class_sniperred",
	"../hud/class_soldierred",
	"../hud/class_demored",
	"../hud/class_medicred",
	"../hud/class_heavyred",
	"../hud/class_pyrored",
	"../hud/class_spyred",
	"../hud/class_engired",
	"../hud/class_scoutred",
};

enum
{
	HUD_HEALTH_NO_ANIM = 0,
	HUD_HEALTH_BONUS_ANIM,
	HUD_HEALTH_DYING_ANIM,
};

DECLARE_BUILD_FACTORY( CTFClassImage );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudPlayerClass::CTFHudPlayerClass( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pClassImage = NULL;
	m_pClassImageBG = NULL;
	m_pSpyImage = NULL;
	m_pSpyOutlineImage = NULL;
	m_pPlayerModelPanel = NULL;
	m_pPlayerModelPanelBG = NULL;
	m_pCarryingWeaponPanel = NULL;
	m_pCarryingLabel = NULL;
	m_pCarryingOwnerLabel = NULL;
	m_pCarryingBG = NULL;

	m_nTeam = TEAM_UNASSIGNED;
	m_nClass = TF_CLASS_UNDEFINED;
	m_nDisguiseTeam = TEAM_UNASSIGNED;
	m_nDisguiseClass = TF_CLASS_UNDEFINED;
	m_hDisguiseWeapon = NULL;
	m_flNextThink = 0.0f;
	m_nKillStreak = 0;

	m_bUsePlayerModel = cl_hud_playerclass_use_playermodel.GetBool();

	ListenForGameEvent( "localplayer_changedisguise" );
	ListenForGameEvent( "post_inventory_application" );
	ListenForGameEvent( "localplayer_pickup_weapon" );

	for ( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
	{
		// The materials are given to vgui via the SetImage() function, which prepends 
		// the "vgui/", so we need to precache them with the same.
		if ( g_szBlueClassImages[i] && g_szBlueClassImages[i][0] )
		{
			PrecacheMaterial( VarArgs( "vgui/%s", g_szBlueClassImages[i] ) );
			PrecacheMaterial( VarArgs( "vgui/%s_cloak", g_szBlueClassImages[i] ) );
			PrecacheMaterial( VarArgs( "vgui/%s_halfcloak", g_szBlueClassImages[i] ) );
		}
		if ( g_szRedClassImages[i] && g_szRedClassImages[i][0] )
		{
			PrecacheMaterial( VarArgs( "vgui/%s", g_szRedClassImages[i] ) );
			PrecacheMaterial( VarArgs( "vgui/%s_cloak", g_szRedClassImages[i] ) );
			PrecacheMaterial( VarArgs( "vgui/%s_halfcloak", g_szRedClassImages[i] ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerClass::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudSpyDisguiseHide" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerClass::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudPlayerClass.res" );

	m_nTeam = TEAM_UNASSIGNED;
	m_nClass = TF_CLASS_UNDEFINED;
	m_nDisguiseTeam = TEAM_UNASSIGNED;
	m_nDisguiseClass = TF_CLASS_UNDEFINED;
	m_hDisguiseWeapon = NULL;
	m_flNextThink = 0.0f;
	m_nCloakLevel = 0;
	m_nLoadoutPosition = LOADOUT_POSITION_PRIMARY;

	m_pClassImage = FindControl<CTFClassImage>( "PlayerStatusClassImage", false );
	m_pClassImageBG = FindControl<CTFImagePanel>( "PlayerStatusClassImageBG", false );
	m_pSpyImage = FindControl<CTFImagePanel>( "PlayerStatusSpyImage", false );
	m_pSpyOutlineImage = FindControl<CTFImagePanel>( "PlayerStatusSpyOutlineImage", false );

	m_pPlayerModelPanel = FindControl<CTFPlayerModelPanel>( "classmodelpanel", false );
	m_pPlayerModelPanelBG = FindControl<CTFImagePanel>( "classmodelpanelBG", false );

	m_pCarryingWeaponPanel = FindControl< EditablePanel >( "CarryingWeapon", false );
	if ( m_pCarryingWeaponPanel )
	{
		m_pCarryingLabel = m_pCarryingWeaponPanel->FindControl< CExLabel >( "CarryingLabel" );
		m_pCarryingOwnerLabel = m_pCarryingWeaponPanel->FindControl< Label >( "OwnerLabel" );
		m_pCarryingBG = m_pCarryingWeaponPanel->FindControl< CTFImagePanel >( "CarryingBackground" );
	}

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerClass::OnThink()
{
	if ( m_flNextThink > gpGlobals->curtime )
		return;

	m_flNextThink = gpGlobals->curtime + 0.5f;
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	bool bTeamChange = false;
	// set our background colors
	if ( m_nTeam != pPlayer->GetTeamNumber() )
	{
		bTeamChange = true;
		m_nTeam = pPlayer->GetTeamNumber();
	}

	int nCloakLevel = 0;
	bool bCloakChange = false;
	float flInvis = pPlayer->GetPercentInvisible();

	if ( flInvis > 0.9 )
	{
		nCloakLevel = 2;
	}
	else if ( flInvis > 0.1 )
	{
		nCloakLevel = 1;
	}

	if ( nCloakLevel != m_nCloakLevel )
	{
		m_nCloakLevel = nCloakLevel;
		bCloakChange = true;
	}

	bool bLoadoutPositionChange = false;
	int nLoadoutSlot = pPlayer->GetActiveTFWeapon() ? pPlayer->GetActiveTFWeapon()->GetAttributeContainer()->GetItem()->GetStaticData()->GetLoadoutSlot( m_nClass ) : LOADOUT_POSITION_PRIMARY;
	if ( m_nLoadoutPosition != nLoadoutSlot )
	{
		m_nLoadoutPosition = nLoadoutSlot;
		bLoadoutPositionChange = true;
	}

	bool bPlayerClassModeChange = false;
	if ( m_bUsePlayerModel != cl_hud_playerclass_use_playermodel.GetBool() )
	{
		m_bUsePlayerModel = cl_hud_playerclass_use_playermodel.GetBool();
		bPlayerClassModeChange = true;
	}


	bool bForceEyeUpdate = false;
	// set our class image
	if (	m_nClass != pPlayer->GetPlayerClass()->GetClassIndex() || bTeamChange || bCloakChange || bLoadoutPositionChange || bPlayerClassModeChange ||
			(
				m_nClass == TF_CLASS_SPY &&
				(
					m_nDisguiseClass != pPlayer->m_Shared.GetDisguiseClass() ||
					m_nDisguiseTeam != pPlayer->m_Shared.GetDisguiseTeam() ||
					m_hDisguiseWeapon != pPlayer->m_Shared.GetDisguiseWeapon()
				)
			)
		)
	{
		bForceEyeUpdate = true;
		m_nClass = pPlayer->GetPlayerClass()->GetClassIndex();

		if ( m_nClass == TF_CLASS_SPY && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			if ( !pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
			{
				m_nDisguiseTeam = pPlayer->m_Shared.GetDisguiseTeam();
				m_nDisguiseClass = pPlayer->m_Shared.GetDisguiseClass();
				m_hDisguiseWeapon = pPlayer->m_Shared.GetDisguiseWeapon();
			}
		}
		else
		{
			m_nDisguiseTeam = TEAM_UNASSIGNED;
			m_nDisguiseClass = TF_CLASS_UNDEFINED;
			m_hDisguiseWeapon = NULL;
		}

		if ( m_bUsePlayerModel && m_pPlayerModelPanel && m_pPlayerModelPanelBG )
		{
			m_pPlayerModelPanel->SetVisible( true );
			m_pPlayerModelPanelBG->SetVisible( true );

			UpdateModelPanel();
		}
		else if ( m_pClassImage && m_pSpyImage )
		{
			if ( m_pPlayerModelPanel )
				m_pPlayerModelPanel->SetVisible( false );
			if ( m_pPlayerModelPanelBG )
				m_pPlayerModelPanelBG->SetVisible( false );

			m_pClassImage->SetVisible( true );
			m_pClassImageBG->SetVisible( true );

			int iCloakState = 0;
			if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) )
			{
				iCloakState = m_nCloakLevel;
			}

			if ( m_nDisguiseTeam != TEAM_UNASSIGNED || m_nDisguiseClass != TF_CLASS_UNDEFINED )
			{
				m_pSpyImage->SetVisible( true );
				m_pClassImage->SetClass( m_nDisguiseTeam, m_nDisguiseClass, iCloakState );
			}
			else
			{
				m_pSpyImage->SetVisible( false );
				m_pClassImage->SetClass( m_nTeam, m_nClass, iCloakState );
			}
		}
	}

	if ( m_pCarryingWeaponPanel )
	{
		// Don't show if we're disguised (the panels overlap)
		bool bShowCarryingWeaponPanel = m_nDisguiseClass == TF_CLASS_UNDEFINED;

		if ( pPlayer->GetActiveTFWeapon() && pPlayer->GetActiveTFWeapon()->GetAttributeContainer() )
		{
			CEconItemView* pItem = pPlayer->GetActiveTFWeapon()->GetAttributeContainer()->GetItem();
			if ( pItem )
			{
				CSteamID playerSteamID;
				pPlayer->GetSteamID( &playerSteamID );
				// We're holding a weapon we dont own!
				if ( playerSteamID.GetAccountID() != pItem->GetAccountID() && m_pCarryingLabel )
				{
					locchar_t wszLocString [128];

					// Construct and set the weapon's name
					g_pVGuiLocalize->ConstructString_safe( wszLocString, L"%s1", 1, CEconItemLocalizedFullNameGenerator( GLocalizationProvider(), pItem->GetItemDefinition(), pItem->GetItemQuality() ).GetFullName() );
					m_pCarryingWeaponPanel->SetDialogVariable( "carrying", wszLocString );

					// Get and set the rarity color of the weapon
					const char* pszColorName = GetItemSchema()->GetRarityColor( pItem->GetItemDefinition()->GetRarity() );
					pszColorName = pszColorName ? pszColorName : "TanLight";
					if ( pszColorName )
					{
						m_pCarryingLabel->SetColorStr( pszColorName );
					}

					bool bHasOwner = false;
					locchar_t wszPlayerName [128];
					CBasePlayer *pOwner = GetPlayerByAccountID( pItem->GetAccountID() );
					// Bots will not work here, so don't fill this out if there's no owner
					if ( pOwner )
					{
						// Fill out the actual owner's name
						locchar_t wszStolenString[128];
						g_pVGuiLocalize->ConvertANSIToUnicode( pOwner->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName) );
						g_pVGuiLocalize->ConstructString_safe( wszStolenString, g_pVGuiLocalize->Find( "TF_WhoDropped" ), 1, wszPlayerName );
						m_pCarryingOwnerLabel->SetText( wszStolenString );
						bHasOwner = true;
					}
					else
					{
						m_pCarryingOwnerLabel->SetText( "" );
					}
					
					int nMaxWide = 0, nMaxTall = 0;
					// Resize the panel to just be the width of whichever label is longer
					int nTall, nWide;
					m_pCarryingLabel->SizeToContents();
					m_pCarryingLabel->GetContentSize( nWide, nTall );
					nMaxWide = Max( nMaxWide, nWide );
					nMaxTall = Max( nMaxTall, nTall );

					m_pCarryingOwnerLabel->SizeToContents();
					m_pCarryingOwnerLabel->GetContentSize( nWide, nTall );
					nMaxWide = Max( nMaxWide, nWide );
					nMaxTall = Max( nMaxTall, nTall );

					m_pCarryingBG->SetWide( nMaxWide + ( m_pCarryingLabel->GetXPos() * 2 ) );
					m_pCarryingBG->SetTall( bHasOwner ? m_pCarryingOwnerLabel->GetYPos() + m_pCarryingOwnerLabel->GetTall() + YRES( 2 )
													  : m_pCarryingLabel->GetYPos() + m_pCarryingLabel->GetTall() + YRES( 2 ) );
				}
				else
				{
					bShowCarryingWeaponPanel = false;	
				}
			}
		}
		else
		{
			bShowCarryingWeaponPanel = false;
		}

		if ( CTFPlayerDestructionLogic::GetRobotDestructionLogic() && ( CTFPlayerDestructionLogic::GetRobotDestructionLogic()->GetType() == CTFPlayerDestructionLogic::TYPE_PLAYER_DESTRUCTION ) )
		{
			if ( pPlayer->HasTheFlag() )
			{
				bShowCarryingWeaponPanel = false;			
			}
		}

		m_pCarryingWeaponPanel->SetVisible( bShowCarryingWeaponPanel );
	}

	if ( m_bUsePlayerModel && m_pPlayerModelPanel )
	{
		bool bPlaySparks = false;
		int iKillStreak = pPlayer->m_Shared.GetStreak( CTFPlayerShared::kTFStreak_Kills );
		if ( iKillStreak != m_nKillStreak && iKillStreak > 0 )
		{
			bPlaySparks = true;
		}
		m_nKillStreak = iKillStreak;
		m_pPlayerModelPanel->SetEyeGlowEffect( pPlayer->GetEyeGlowEffect(), pPlayer->GetEyeGlowColor( false ), pPlayer->GetEyeGlowColor( true ), bForceEyeUpdate, bPlaySparks );
	}
}

static void HudPlayerClassUsePlayerModelDialogCallback( bool bConfirmed, void *pContext )
{
	cl_hud_playerclass_use_playermodel.SetValue( bConfirmed );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerClass::UpdateModelPanel()
{
	if ( !m_bUsePlayerModel )
	{
		return;
	}

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->IsAlive() )
	{
		return;
	}

	if ( !cl_hud_playerclass_playermodel_showed_confirm_dialog.GetBool() )
	{
		// only show this message one time
		ShowConfirmDialog(	"#GameUI_HudPlayerClassUsePlayerModelDialogTitle",
			"#GameUI_HudPlayerClassUsePlayerModelDialogMessage",
			"#GameUI_HudPlayerClassUsePlayerModelDialogConfirm", 
			"#GameUI_HudPlayerClassUsePlayerModelDialogCancel",
			&HudPlayerClassUsePlayerModelDialogCallback );
		cl_hud_playerclass_playermodel_showed_confirm_dialog.SetValue( true );
	}

	// hide old UI
	if ( m_pSpyImage )
		m_pSpyImage->SetVisible( false );
	if ( m_pClassImage )
		m_pClassImage->SetVisible( false );
	if ( m_pClassImageBG )
		m_pClassImageBG->SetVisible( false );

	if ( m_pPlayerModelPanel && m_pPlayerModelPanel->IsVisible() )
	{
		int nClass;
		int nTeam;
		int nItemSlot = m_nLoadoutPosition;
		CEconItemView *pWeapon = NULL;

		bool bDisguised = pPlayer->m_Shared.InCond( TF_COND_DISGUISED );
		if ( bDisguised )
		{
			nClass = pPlayer->m_Shared.GetDisguiseClass();
			nTeam = pPlayer->m_Shared.GetDisguiseTeam();

			if ( pPlayer->m_Shared.GetDisguiseWeapon() )
			{
				CAttributeContainer *pCont = pPlayer->m_Shared.GetDisguiseWeapon()->GetAttributeContainer();
				pWeapon = pCont ? pCont->GetItem() : NULL;
				if ( pWeapon )
				{
					nItemSlot = pWeapon->GetStaticData()->GetLoadoutSlot( nClass );
				}
			}
		}
		else
		{
			nClass = pPlayer->GetPlayerClass()->GetClassIndex();
			nTeam = pPlayer->GetTeamNumber();

			CTFWeaponBase *pEnt = dynamic_cast< CTFWeaponBase* >( pPlayer->GetEntityForLoadoutSlot( nItemSlot ) );
			if ( pEnt )
			{
				pWeapon = pEnt->GetAttributeContainer()->GetItem();
			}
		}

		m_pPlayerModelPanel->ClearCarriedItems();
		m_pPlayerModelPanel->SetToPlayerClass( nClass );
		m_pPlayerModelPanel->SetTeam( nTeam );

		if ( pWeapon )
		{
			m_pPlayerModelPanel->AddCarriedItem( pWeapon );
		}

		for ( int wbl = pPlayer->GetNumWearables()-1; wbl >= 0; wbl-- )
		{
			C_TFWearable *pItem = dynamic_cast<C_TFWearable*>( pPlayer->GetWearable( wbl ) );
			if ( !pItem )
				continue;

			if ( pItem->IsViewModelWearable() )
				continue;

			if ( pItem->IsDisguiseWearable() && !bDisguised )
				continue;

			if ( !pItem->IsDisguiseWearable() && bDisguised )
				continue;

			CAttributeContainer *pCont		   = pItem->GetAttributeContainer();
			CEconItemView		*pEconItemView = pCont ? pCont->GetItem() : NULL;

			if ( pEconItemView && pEconItemView->IsValid() )
			{
				m_pPlayerModelPanel->AddCarriedItem( pEconItemView );
			}
		}

		m_pPlayerModelPanel->HoldItemInSlot( nItemSlot );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerClass::FireGameEvent( IGameEvent * event )
{
	const char* pszEventName = event->GetName();

	if ( FStrEq( "localplayer_changedisguise", pszEventName ) )
	{
		if ( m_pSpyImage && m_pSpyOutlineImage )
		{
			bool bFadeIn = event->GetBool( "disguised", false );

			if ( bFadeIn )
			{
				m_pSpyImage->SetAlpha( 0 );
			}
			else
			{
				m_pSpyImage->SetAlpha( 255 );
			}

			m_pSpyOutlineImage->SetAlpha( 0 );
			
			m_pSpyImage->SetVisible( true );
			m_pSpyOutlineImage->SetVisible( true );

			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( bFadeIn ? "HudSpyDisguiseFadeIn" : "HudSpyDisguiseFadeOut" );
		}

		UpdateModelPanel();
	}
	else if ( FStrEq( "post_inventory_application", pszEventName ) )
	{
		// Force a refresh. if this is for the local player
		int iUserID = event->GetInt( "userid" );
		C_TFPlayer* pPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( pPlayer && pPlayer->GetUserID() == iUserID )
		{
			UpdateModelPanel();
		}
	}
	else if ( FStrEq( "localplayer_pickup_weapon", pszEventName ) )
	{
		UpdateModelPanel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHealthPanel::CTFHealthPanel( Panel *parent, const char *name ) : vgui::Panel( parent, name )
{
	m_flHealth = 1.0f;

	m_iMaterialIndex = surface()->DrawGetTextureId( "hud/health_color" );
	if ( m_iMaterialIndex == -1 ) // we didn't find it, so create a new one
	{
		m_iMaterialIndex = surface()->CreateNewTextureID();	
		surface()->DrawSetTextureFile( m_iMaterialIndex, "hud/health_color", true, false );
	}

	m_iDeadMaterialIndex = surface()->DrawGetTextureId( "hud/health_dead" );
	if ( m_iDeadMaterialIndex == -1 ) // we didn't find it, so create a new one
	{
		m_iDeadMaterialIndex = surface()->CreateNewTextureID();	
		surface()->DrawSetTextureFile( m_iDeadMaterialIndex, "hud/health_dead", true, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHealthPanel::Paint()
{
	BaseClass::Paint();

	int x, y, w, h;
	GetBounds( x, y, w, h );

	Vertex_t vert[4];	
	float uv1 = 0.0f;
	float uv2 = 1.0f;
	int xpos = 0, ypos = 0;

	if ( m_flHealth <= 0 )
	{
		// Draw the dead material
		surface()->DrawSetTexture( m_iDeadMaterialIndex );
		
		vert[0].Init( Vector2D( xpos, ypos ), Vector2D( uv1, uv1 ) );
		vert[1].Init( Vector2D( xpos + w, ypos ), Vector2D( uv2, uv1 ) );
		vert[2].Init( Vector2D( xpos + w, ypos + h ), Vector2D( uv2, uv2 ) );				
		vert[3].Init( Vector2D( xpos, ypos + h ), Vector2D( uv1, uv2 ) );

		surface()->DrawSetColor( Color(255,255,255,255) );
	}
	else
	{
		float flDamageY = h * ( 1.0f - m_flHealth );

		// blend in the red "damage" part
		surface()->DrawSetTexture( m_iMaterialIndex );

		Vector2D uv11( uv1, uv2 - m_flHealth );
		Vector2D uv21( uv2, uv2 - m_flHealth );
		Vector2D uv22( uv2, uv2 );
		Vector2D uv12( uv1, uv2 );

		vert[0].Init( Vector2D( xpos, flDamageY ), uv11 );
		vert[1].Init( Vector2D( xpos + w, flDamageY ), uv21 );
		vert[2].Init( Vector2D( xpos + w, ypos + h ), uv22 );				
		vert[3].Init( Vector2D( xpos, ypos + h ), uv12 );

		surface()->DrawSetColor( GetFgColor() );
	}

	surface()->DrawTexturedPolygon( 4, vert );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudPlayerHealth::CTFHudPlayerHealth( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pHealthImage = new CTFHealthPanel( this, "PlayerStatusHealthImage" );	
	m_pHealthImageBG = new ImagePanel( this, "PlayerStatusHealthImageBG" );
	m_pHealthBonusImage = new ImagePanel( this, "PlayerStatusHealthBonusImage" );
	m_pBuildingHealthImageBG = new ImagePanel( this, "BuildingStatusHealthImageBG" );
	m_pBleedImage = new ImagePanel( this, "PlayerStatusBleedImage" );
	m_pHookBleedImage = new ImagePanel( this, "PlayerStatusHookBleedImage" );
	m_pMarkedForDeathImage = new ImagePanel( this, "PlayerStatusMarkedForDeathImage" );
	m_pMarkedForDeathImageSilent = new ImagePanel( this, "PlayerStatusMarkedForDeathSilentImage" );
	m_pMilkImage = new ImagePanel( this, "PlayerStatusMilkImage" );
	m_pGasImage = new ImagePanel( this, "PlayerStatusGasImage" );
	m_pSlowedImage = new ImagePanel( this, "PlayerStatusSlowed" );

	m_pWheelOfDoomImage = new ImagePanel( this, "PlayerStatus_WheelOfDoom" );

	m_flNextThink = 0.0f;

	m_nBonusHealthOrigX = -1;
	m_nBonusHealthOrigY = -1;
	m_nBonusHealthOrigW = -1;
	m_nBonusHealthOrigH = -1;

	// Vaccinator
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_MEDIGUN_UBER_BULLET_RESIST, BUFF_CLASS_BULLET_RESIST, new ImagePanel( this, "PlayerStatus_MedicUberBulletResistImage" ),	"../HUD/defense_buff_bullet_blue",		"../HUD/defense_buff_bullet_red"  ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_MEDIGUN_UBER_BLAST_RESIST, BUFF_CLASS_BLAST_RESIST, new ImagePanel( this, "PlayerStatus_MedicUberBlastResistImage" ),		"../HUD/defense_buff_explosion_blue",	"../HUD/defense_buff_explosion_red" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_MEDIGUN_UBER_FIRE_RESIST, BUFF_CLASS_FIRE_RESIST, new ImagePanel( this, "PlayerStatus_MedicUberFireResistImage" ),		"../HUD/defense_buff_fire_blue",		"../HUD/defense_buff_fire_red" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_MEDIGUN_SMALL_BULLET_RESIST, BUFF_CLASS_BULLET_RESIST, new ImagePanel( this, "PlayerStatus_MedicSmallBulletResistImage" ),	"../HUD/defense_buff_bullet_blue",		"../HUD/defense_buff_bullet_red"  ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_MEDIGUN_SMALL_BLAST_RESIST, BUFF_CLASS_BLAST_RESIST, new ImagePanel( this, "PlayerStatus_MedicSmallBlastResistImage" ),	"../HUD/defense_buff_explosion_blue",	"../HUD/defense_buff_explosion_red" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_MEDIGUN_SMALL_FIRE_RESIST, BUFF_CLASS_FIRE_RESIST, new ImagePanel( this, "PlayerStatus_MedicSmallFireResistImage" ),		"../HUD/defense_buff_fire_blue",		"../HUD/defense_buff_fire_red" ) );
	// Soldier buffs
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_OFFENSEBUFF, BUFF_CLASS_SOLDIER_OFFENSE, new ImagePanel( this, "PlayerStatus_SoldierOffenseBuff" ),						"../Effects/soldier_buff_offense_blue",		"../Effects/soldier_buff_offense_red" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_DEFENSEBUFF, BUFF_CLASS_SOLDIER_DEFENSE, new ImagePanel( this, "PlayerStatus_SoldierDefenseBuff" ),						"../Effects/soldier_buff_defense_blue",		"../Effects/soldier_buff_defense_red" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_REGENONDAMAGEBUFF, BUFF_CLASS_SOLDIER_HEALTHONHIT, new ImagePanel( this, "PlayerStatus_SoldierHealOnHitBuff" ),			"../Effects/soldier_buff_healonhit_blue",	"../Effects/soldier_buff_healonhit_red" ) );
	// Powerup Rune status
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_RUNE_STRENGTH, RUNE_CLASS_STRENGTH, new ImagePanel( this, "PlayerStatus_RuneStrength" ), "../Effects/powerup_strength_hud", "../Effects/powerup_strength_hud" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_RUNE_HASTE, RUNE_CLASS_HASTE, new ImagePanel( this, "PlayerStatus_RuneHaste" ), "../Effects/powerup_haste_hud", "../Effects/powerup_haste_hud" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_RUNE_REGEN, RUNE_CLASS_REGEN, new ImagePanel( this, "PlayerStatus_RuneRegen" ), "../Effects/powerup_regen_hud", "../Effects/powerup_regen_hud" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_RUNE_RESIST, RUNE_CLASS_RESIST, new ImagePanel( this, "PlayerStatus_RuneResist" ), "../Effects/powerup_resist_hud", "../Effects/powerup_resist_hud" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_RUNE_VAMPIRE, RUNE_CLASS_VAMPIRE, new ImagePanel( this, "PlayerStatus_RuneVampire" ), "../Effects/powerup_vampire_hud", "../Effects/powerup_vampire_hud" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_RUNE_REFLECT, RUNE_CLASS_REFLECT, new ImagePanel( this, "PlayerStatus_RuneReflect" ), "../Effects/powerup_reflect_hud", "../Effects/powerup_reflect_hud" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_RUNE_PRECISION, RUNE_CLASS_PRECISION, new ImagePanel( this, "PlayerStatus_RunePrecision" ), "../Effects/powerup_precision_hud", "../Effects/powerup_precision_hud" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_RUNE_AGILITY, RUNE_CLASS_AGILITY, new ImagePanel( this, "PlayerStatus_RuneAgility" ), "../Effects/powerup_agility_hud", "../Effects/powerup_agility_hud" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_RUNE_KNOCKOUT, RUNE_CLASS_KNOCKOUT, new ImagePanel( this, "PlayerStatus_RuneKnockout" ), "../Effects/powerup_knockout_hud", "../Effects/powerup_knockout_hud" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_RUNE_KING, RUNE_CLASS_KING, new ImagePanel( this, "PlayerStatus_RuneKing" ), "../Effects/powerup_king_hud", "../Effects/powerup_king_hud" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_RUNE_PLAGUE, RUNE_CLASS_PLAGUE, new ImagePanel( this, "PlayerStatus_RunePlague" ), "../Effects/powerup_plague_hud", "../Effects/powerup_plague_hud" ) );
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_RUNE_SUPERNOVA, RUNE_CLASS_SUPERNOVA, new ImagePanel( this, "PlayerStatus_RuneSupernova" ), "../Effects/powerup_supernova_hud", "../Effects/powerup_supernova_hud" ) );

	// Parachute
	m_vecBuffInfo.AddToTail( new CTFBuffInfo( TF_COND_PARACHUTE_ACTIVE, BUFF_CLASS_PARACHUTE, new ImagePanel( this, "PlayerStatus_Parachute" ), "../HUD/hud_parachute_active", "../HUD/hud_parachute_active" ) );

	m_iAnimState = HUD_HEALTH_NO_ANIM;
	m_bAnimate = true;
}

CTFHudPlayerHealth::~CTFHudPlayerHealth()
{
	m_vecBuffInfo.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerHealth::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
	m_nHealth = -1;
	m_bBuilding = false;

	m_iAnimState = HUD_HEALTH_NO_ANIM;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerHealth::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( GetResFilename() );

	if ( m_pHealthBonusImage )
	{
		m_pHealthBonusImage->GetBounds( m_nBonusHealthOrigX, m_nBonusHealthOrigY, m_nBonusHealthOrigW, m_nBonusHealthOrigH );
	}

	m_flNextThink = 0.0f;

	BaseClass::ApplySchemeSettings( pScheme );

	m_pBuildingHealthImageBG->SetVisible( m_bBuilding );

	m_pPlayerLevelLabel = dynamic_cast<CExLabel*>( FindChildByName( "PlayerStatusPlayerLevel" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerHealth::SetHealth( int iNewHealth, int iMaxHealth, int	iMaxBuffedHealth )
{
	// set our health
	m_nHealth = iNewHealth;
	m_nMaxHealth = iMaxHealth;
	m_pHealthImage->SetHealth( (float)(m_nHealth) / (float)(m_nMaxHealth) );

	if ( m_pHealthImage )
	{
		m_pHealthImage->SetFgColor( Color( 255, 255, 255, 255 ) );
	}

	if ( m_nHealth <= 0 )
	{
		if ( m_pHealthImageBG->IsVisible() )
		{
			m_pHealthImageBG->SetVisible( false );
		}
		if ( m_pBuildingHealthImageBG->IsVisible() )
		{
			m_pBuildingHealthImageBG->SetVisible( false );
		}
		HideHealthBonusImage();
	}
	else
	{
		if ( !m_pHealthImageBG->IsVisible() )
		{
			m_pHealthImageBG->SetVisible( true );
		}
		m_pBuildingHealthImageBG->SetVisible( m_bBuilding );

		// are we getting a health bonus?
		if ( m_nHealth > m_nMaxHealth )
		{
			if ( m_pHealthBonusImage && m_nBonusHealthOrigW != -1 )
			{
				if ( !m_pHealthBonusImage->IsVisible() )
				{
					m_pHealthBonusImage->SetVisible( true );
				}

				if ( m_bAnimate && m_iAnimState != HUD_HEALTH_BONUS_ANIM )
				{
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthDyingPulseStop" );
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthBonusPulse" );

					m_iAnimState = HUD_HEALTH_BONUS_ANIM;
				}

				m_pHealthBonusImage->SetDrawColor( Color( 255, 255, 255, 255 ) );

				// scale the flashing image based on how much health bonus we currently have
				float flBoostMaxAmount = ( iMaxBuffedHealth ) - m_nMaxHealth;
				float flPercent = MIN( ( m_nHealth - m_nMaxHealth ) / flBoostMaxAmount, 1.0f );

				int nPosAdj = RoundFloatToInt( flPercent * m_nHealthBonusPosAdj );
				int nSizeAdj = 2 * nPosAdj;

				m_pHealthBonusImage->SetBounds( m_nBonusHealthOrigX - nPosAdj, 
					m_nBonusHealthOrigY - nPosAdj, 
					m_nBonusHealthOrigW + nSizeAdj,
					m_nBonusHealthOrigH + nSizeAdj );
			}
		}
		// are we close to dying?
		else if ( m_nHealth < m_nMaxHealth * m_flHealthDeathWarning )
		{
			if ( m_pHealthBonusImage && m_nBonusHealthOrigW != -1 )
			{
				if ( !m_pHealthBonusImage->IsVisible() )
				{
					m_pHealthBonusImage->SetVisible( true );
				}

				if ( m_bAnimate && m_iAnimState != HUD_HEALTH_DYING_ANIM )
				{
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthBonusPulseStop" );
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthDyingPulse" );

					m_iAnimState = HUD_HEALTH_DYING_ANIM;
				}

				m_pHealthBonusImage->SetDrawColor( m_clrHealthDeathWarningColor );

				// scale the flashing image based on how much health bonus we currently have
				float flBoostMaxAmount = m_nMaxHealth * m_flHealthDeathWarning;
				float flPercent = ( flBoostMaxAmount - m_nHealth ) / flBoostMaxAmount;

				int nPosAdj = RoundFloatToInt( flPercent * m_nHealthBonusPosAdj );
				int nSizeAdj = 2 * nPosAdj;

				m_pHealthBonusImage->SetBounds( m_nBonusHealthOrigX - nPosAdj, 
					m_nBonusHealthOrigY - nPosAdj, 
					m_nBonusHealthOrigW + nSizeAdj,
					m_nBonusHealthOrigH + nSizeAdj );
			}

			if ( m_pHealthImage )
			{
				m_pHealthImage->SetFgColor( m_clrHealthDeathWarningColor );
			}
		}
		// turn it off
		else
		{
			HideHealthBonusImage();
		}
	}

	// set our health display value
	if ( m_nHealth > 0 )
	{
		SetDialogVariable( "Health", m_nHealth );

		if ( m_nMaxHealth - m_nHealth >= 5 )
		{
			SetDialogVariable( "MaxHealth", m_nMaxHealth );
		}
		else
		{
			SetDialogVariable( "MaxHealth", "" );
		}
	}
	else
	{
		SetDialogVariable( "Health", "" );
		SetDialogVariable( "MaxHealth", "" );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerHealth::SetLevel( int nLevel )
{
	if ( m_pPlayerLevelLabel )
	{
		bool bVisible = ( nLevel >= 0 ) ? true : false;
		if ( bVisible )
		{
			m_pPlayerLevelLabel->SetText( CFmtStr( "%d", nLevel ) );
		}

		if ( m_pPlayerLevelLabel->IsVisible() != bVisible )
		{
			m_pPlayerLevelLabel->SetVisible( bVisible );
		}
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerHealth::HideHealthBonusImage( void )
{
	if ( m_pHealthBonusImage && m_pHealthBonusImage->IsVisible() )
	{
		if ( m_nBonusHealthOrigW != -1 )
		{
			m_pHealthBonusImage->SetBounds( m_nBonusHealthOrigX, m_nBonusHealthOrigY, m_nBonusHealthOrigW, m_nBonusHealthOrigH );
		}
		m_pHealthBonusImage->SetVisible( false );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthBonusPulseStop" );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthDyingPulseStop" );

		m_iAnimState = HUD_HEALTH_NO_ANIM;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void SetPlayerHealthImagePanelVisibility( CTFPlayer *pPlayer, ETFCond eCond, vgui::ImagePanel *pImagePanel, int& nXOffset, const Color& colorIfVisible )
{
	Assert( pImagePanel != NULL );

	if ( pPlayer->m_Shared.InCond( eCond ) && !pImagePanel->IsVisible() )
	{
		pImagePanel->SetVisible( true );
		pImagePanel->SetDrawColor( colorIfVisible );
		
		// Reposition ourselves and increase the offset if we are active
		int x,y;
		pImagePanel->GetPos( x, y );
		pImagePanel->SetPos( nXOffset, y );
		nXOffset += 100.f;
	}
}

void CTFBuffInfo::Update( CTFPlayer *pPlayer )
{
	Assert( m_pImagePanel != NULL && pPlayer != NULL );

	if ( pPlayer->m_Shared.InCond( m_eCond ) )
	{
		if( m_pzsBlueImage && m_pzsBlueImage[0] && m_pzsRedImage && m_pzsRedImage[0] )
		{
			if( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
				m_pImagePanel->SetImage( m_pzsBlueImage );
			else
				m_pImagePanel->SetImage( m_pzsRedImage );
		}
	}
}

void CTFHudPlayerHealth::OnThink()
{
	if ( m_flNextThink < gpGlobals->curtime )
	{
		C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );

		if ( pPlayer )
		{
			SetHealth( pPlayer->GetHealth(), pPlayer->GetMaxHealth(), pPlayer->m_Shared.GetMaxBuffedHealth() );

			int color_offset = ((int)(gpGlobals->realtime*10)) % 5;
			int color_fade	 = 160 + (color_offset*10);

			// Find our starting point, just above the health '+'
			int nXOffset,y;
			m_pHealthImage->GetPos( nXOffset, y );
			// Nudge over a bit to get centered
			nXOffset += 25;

			// Turn all the panels off, and below conditionally turn them on
			FOR_EACH_VEC( m_vecBuffInfo, i )
			{
				m_vecBuffInfo[ i ]->m_pImagePanel->SetVisible( false );
			}

			CUtlVector<BuffClass_t> m_vecActiveClasses;
			// Cycle through all the buffs and update them
			FOR_EACH_VEC( m_vecBuffInfo, i )
			{
				// Skip if this class of buff is already being drawn
				if( m_vecActiveClasses.Find( m_vecBuffInfo[i]->m_eClass ) != m_vecActiveClasses.InvalidIndex() )
					continue;

				m_vecBuffInfo[i]->Update( pPlayer );
				SetPlayerHealthImagePanelVisibility( pPlayer, m_vecBuffInfo[i]->m_eCond, m_vecBuffInfo[i]->m_pImagePanel, nXOffset, Color( 255, 255, 255, color_fade ) );

				// This class of buff is now active.
				if( m_vecBuffInfo[i]->m_pImagePanel->IsVisible() )
				{
					m_vecActiveClasses.AddToTail( m_vecBuffInfo[i]->m_eClass );
				}
			}

			// Turn all the panels off, and below conditionally turn them on
			m_pBleedImage->SetVisible( false );
			m_pHookBleedImage->SetVisible( false );
			m_pMilkImage->SetVisible( false );
			m_pGasImage->SetVisible( false );
			m_pMarkedForDeathImage->SetVisible( false );
			m_pMarkedForDeathImageSilent->SetVisible( false );
			m_pSlowedImage->SetVisible( false );
			
			// Old method for goofy color manipulation
			int nBloodX = nXOffset;
			SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_BLEEDING,					m_pBleedImage,					nXOffset,	Color( color_fade, 0, 0, 255 ) );
			SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_GRAPPLINGHOOK_BLEEDING,	m_pHookBleedImage,				nBloodX,	Color( 255, 255, 255, 255 ) ); // draw this on top of bleeding
			SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_MAD_MILK,					m_pMilkImage,					nXOffset,	Color( color_fade, color_fade, color_fade, 255 ) );
			SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_MARKEDFORDEATH,			m_pMarkedForDeathImage,			nXOffset,	Color( 255 - color_fade, 245 - color_fade, 245 - color_fade, 255 ) );
			SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_MARKEDFORDEATH_SILENT,	m_pMarkedForDeathImageSilent,	nXOffset,	Color( 125 - color_fade, 255 - color_fade, 255 - color_fade, 255 ) );
			SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_PASSTIME_PENALTY_DEBUFF,	m_pMarkedForDeathImageSilent,	nXOffset,	Color( 125 - color_fade, 255 - color_fade, 255 - color_fade, 255 ) );
			SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_STUNNED,					m_pSlowedImage,					nXOffset,	Color( color_fade, color_fade, 0, 255 ) );
			SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_GAS,						m_pGasImage,					nXOffset,	Color( color_fade, color_fade, color_fade, 255 ) );
			
			UpdateHalloweenStatus();
		}

		m_flNextThink = gpGlobals->curtime + 0.05f;
	}
}


void CTFHudPlayerHealth::UpdateHalloweenStatus( void )
{
	if ( TFGameRules()->IsHalloweenEffectStatusActive() )
	{
		int status = TFGameRules()->GetHalloweenEffectStatus();

		if ( status == EFFECT_WHAMMY )
		{
			m_pWheelOfDoomImage->SetImage( "..\\HUD\\death_wheel_whammy" );
		}
		else
		{
			m_pWheelOfDoomImage->SetImage( VarArgs( "..\\HUD\\death_wheel_%d", status - 1 ) );

		}

		float timeLeft = TFGameRules()->GetHalloweenEffectTimeLeft();

		const float warnExpireTime = 3.0f;
		const float blinkInterval = 0.25f;

		if ( timeLeft < warnExpireTime )
		{
			int blink = (int)( timeLeft / blinkInterval );

			m_pWheelOfDoomImage->SetVisible( blink & 0x1 );
		}
		else
		{
			m_pWheelOfDoomImage->SetVisible( true );
		}
	}
	else
	{
		m_pWheelOfDoomImage->SetVisible( false );
	}
}


DECLARE_HUDELEMENT( CTFHudPlayerStatus );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudPlayerStatus::CTFHudPlayerStatus( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudPlayerStatus" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pHudPlayerClass = new CTFHudPlayerClass( this, "HudPlayerClass" );
	m_pHudPlayerHealth = new CTFHudPlayerHealth( this, "HudPlayerHealth" );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// HACK: Work around the scheme application order failing
	// to reload the player class hud element's scheme in minmode.
	static ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
	if ( cl_hud_minmode.IsValid() && cl_hud_minmode.GetBool() )
	{
		m_pHudPlayerClass->InvalidateLayout( false, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudPlayerStatus::ShouldDraw( void )
{
	CTFPlayer *pTFPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		return false;

	if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
		return false;

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPlayerStatus::Reset()
{
	if ( m_pHudPlayerClass )
	{
		m_pHudPlayerClass->Reset();
	}

	if ( m_pHudPlayerHealth )
	{
		m_pHudPlayerHealth->Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassImage::SetClass( int iTeam, int iClass, int iCloakstate )
{
	char szImage[128];
	szImage[0] = '\0';

	if ( iTeam == TF_TEAM_BLUE )
	{
		Q_strncpy( szImage, g_szBlueClassImages[ iClass ], sizeof(szImage) );
	}
	else
	{
		Q_strncpy( szImage, g_szRedClassImages[ iClass ], sizeof(szImage) );
	}

	switch( iCloakstate )
	{
	case 2:
		Q_strncat( szImage, "_cloak", sizeof(szImage), COPY_ALL_CHARACTERS );
		break;
	case 1:
		Q_strncat( szImage, "_halfcloak", sizeof(szImage), COPY_ALL_CHARACTERS );
		break;
	default:
		break;
	}

	if ( Q_strlen( szImage ) > 0 )
	{
		SetImage( szImage );
	}
}
