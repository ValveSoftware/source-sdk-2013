//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_freezepanel.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "c_tf_player.h"
#include "c_tf_playerresource.h"
#include <vgui_controls/Label.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IInput.h>
#include "c_baseobject.h"
#include "fmtstr.h"
#include "tf_gamerules.h"
#include "tf_hud_statpanel.h"
#include "view.h"
#include "ivieweffects.h"
#include "viewrender.h"
#include "c_obj_sentrygun.h"
#include "NextBot/C_NextBot.h"
#include "halloween/c_headless_hatman.h"
#include "halloween/c_eyeball_boss.h"
#include "halloween/c_merasmus.h"
#include "tf_wardata.h"

#if defined( REPLAY_ENABLED )
#include "replay/ireplaysystem.h"
#include "replay/ireplaymanager.h"
#include "replay/replay.h"
#include "replay/screenshot.h"
#include "replay/ireplayscreenshotmanager.h"
#include "replay/vgui/replayreminderpanel.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT_DEPTH( CTFFreezePanel, 1 );

#define CALLOUT_WIDE		(XRES(100))
#define CALLOUT_TALL		(XRES(50))

extern float g_flFreezeFlash;

#define FREEZECAM_SCREENSHOT_STRING "is looking good!"

extern ConVar hud_freezecamhide;

bool IsTakingAFreezecamScreenshot( void )
{
	// Don't draw in freezecam, or when the game's not running
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	bool bInFreezeCam = ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM );

	if ( bInFreezeCam == true && engine->IsTakingScreenshot() )
		return true;

	CTFFreezePanel *pFreezePanel = CTFFreezePanel::Instance();
	if ( pFreezePanel )
	{
		if ( pFreezePanel->IsHoldingAfterScreenShot() )
			return true;
	}

	return false;
}

DECLARE_BUILD_FACTORY( CTFFreezePanelHealth );

//-----------------------------------------------------------------------------

CTFFreezePanel *CTFFreezePanel::s_pFreezePanel = NULL;

//-----------------------------------------------------------------------------

CTFFreezePanel *CTFFreezePanel::Instance()
{
	return s_pFreezePanel;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFFreezePanel::CTFFreezePanel( const char *pElementName )
	: EditablePanel( NULL, "FreezePanel" ), CHudElement( pElementName )
{
	AssertMsg( !s_pFreezePanel, "There can be only one." );
	s_pFreezePanel = this;

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetVisible( false );
	SetScheme( "ClientScheme" );

	m_iKillerIndex = 0;
	m_iShowNemesisPanel = SHOW_NO_NEMESIS;
	m_iYBase = -1;
	m_flShowCalloutsAt = 0;

	m_iBasePanelOriginalX = -1;
	m_iBasePanelOriginalY = -1;

	m_pItemPanel = new CItemModelPanel( this, "itempanel" ) ;
	m_iItemPanelOriginalX = -1;
	m_iItemPanelOriginalY = -1;

#if defined( REPLAY_ENABLED )
	m_pSaveReplayPanel = GET_HUDELEMENT( CReplayReminderPanel );	// Use the HUD's instance
#endif

	m_strCurrentFreezeCamResFile = GetResFilename();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::Reset()
{
	Hide();

	if ( m_pKillerHealth )
	{
		m_pKillerHealth->Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::Init()
{
	// listen for events
	ListenForGameEvent( "show_freezepanel" );
	ListenForGameEvent( "hide_freezepanel" );
	ListenForGameEvent( "freezecam_started" );
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "teamplay_win_panel" );
	ListenForGameEvent( "training_complete" );
	
	Hide();

	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::SendTauntAcknowledgement( const char *pszCommand, int iGibs )
{
	C_TFPlayer *pKiller = ToTFPlayer( UTIL_PlayerByIndex( GetSpectatorTarget() ) );
	if ( pKiller && pKiller->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer )
		{
			KeyValues *kv = new KeyValues( "FreezeCamTaunt" );
			kv->SetInt( "achiever", pKiller->GetUserID() );
			kv->SetString( "command", pszCommand );
			kv->SetInt( "gibs", iGibs );
			engine->ServerCmdKeyValues( kv );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFFreezePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	Assert( !m_strCurrentFreezeCamResFile.IsEmpty() );
	LoadControlSettings( m_strCurrentFreezeCamResFile.String() );

	m_pBasePanel = dynamic_cast<EditablePanel *>( FindChildByName("FreezePanelBase") );

	Assert( m_pBasePanel );

	if ( m_pBasePanel )
	{
		m_pFreezeLabel = dynamic_cast<Label *>( m_pBasePanel->FindChildByName("FreezeLabel") );
		m_pKillerLabel = dynamic_cast<Label *>( m_pBasePanel->FindChildByName("FreezeLabelKiller") );
		m_pFreezePanelBG = dynamic_cast<CTFImagePanel *>( m_pBasePanel->FindChildByName( "FreezePanelBG" ) );
		m_pNemesisSubPanel = dynamic_cast<EditablePanel *>( m_pBasePanel->FindChildByName( "NemesisSubPanel" ) );
		m_pKillerHealth	= dynamic_cast<CTFFreezePanelHealth *>( m_pBasePanel->FindChildByName( "FreezePanelHealth" ) );
		m_pAvatar = dynamic_cast<CAvatarImagePanel *>( m_pBasePanel->FindChildByName("AvatarImage") );
		m_pFreezeTeamIcon = dynamic_cast<CTFImagePanel *>( m_pBasePanel->FindChildByName( "FreezeTeamIcon" ) );

		if ( m_pAvatar )
		{
			m_pAvatar->SetShouldScaleImage( true );
			m_pAvatar->SetShouldDrawFriendIcon( false );
		}
	}
		
	m_pScreenshotPanel = dynamic_cast<EditablePanel *>( FindChildByName( "ScreenshotPanel" ) );
	Assert( m_pScreenshotPanel );

	// Move killer panels when the win panel is up
	int xp,yp;
	GetPos( xp, yp );
	m_iYBase = yp;

	int w = 0 , h = 0;
	if ( m_pBasePanel )
		m_pBasePanel->GetBounds( m_iBasePanelOriginalX, m_iBasePanelOriginalY, w, h );
	m_pItemPanel->GetBounds( m_iItemPanelOriginalX, m_iItemPanelOriginalY, w, h );
	if ( m_pKillerLabel )
	{
		m_pKillerLabel->GetPos( m_iKillerOriginalX, m_iKillerOriginalY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "player_death", pEventName ) == 0 )
	{
		// see if the local player died
		int iPlayerIndexVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer && iPlayerIndexVictim == pLocalPlayer->entindex() )
		{
			// the local player is dead, see if this is a new nemesis or a revenge
			if (event->GetInt( "death_flags" ) & TF_DEATH_DOMINATION )
			{
				m_iShowNemesisPanel = SHOW_NEW_NEMESIS;
			}
			else if ( event->GetInt( "death_flags" ) & TF_DEATH_REVENGE )
			{
				m_iShowNemesisPanel = SHOW_REVENGE;
			}
			else
			{
				m_iShowNemesisPanel = SHOW_NO_NEMESIS;
			}
		}		
	}
	else if ( Q_strcmp( "hide_freezepanel", pEventName ) == 0 )
	{
		Hide();
	}
	else if ( Q_strcmp( "freezecam_started", pEventName ) == 0 )
	{
		ShowCalloutsIn( 1.0 );
		ShowSnapshotPanelIn( 1.25 );

#if defined( REPLAY_ENABLED )
		// If Replay is enabled on the server, show the replay download reminder.  If GetPendingReplay()
		// returns NULL, we know we've already saved the replay.
		CReplay *pCurLifeReplay = ( g_pReplayManager ) ? g_pReplayManager->GetReplayForCurrentLife() : NULL;
		if ( g_pReplay->IsRecording() && ( pCurLifeReplay && !pCurLifeReplay->m_bRequestedByUser && !pCurLifeReplay->m_bSaved ) )
		{
			ShowSaveReplayPanelIn( 1.25 );
		}

		// Save the freezeframe for the replay browser
		if ( g_pReplay->IsRecording() )
		{
			// Capture the freezecam in half a second
			CaptureScreenshotParams_t params;
			V_memset( &params, 0, sizeof( params ) );
			params.m_flDelay = 0.0f;
			params.m_bIgnoreMinTimeBetweenScreenshots = true;
			g_pReplayScreenshotManager->CaptureScreenshot( params );
		}
#endif

		CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer )
		{
			SendTauntAcknowledgement( "freezecam_taunt" );
		}
	}
	else if ( Q_strcmp( "teamplay_win_panel", pEventName ) == 0 )
	{
		Hide();
	}
	else if ( Q_strcmp( "training_complete", pEventName ) == 0 )
	{
		Hide();
	}
	else if ( Q_strcmp( "show_freezepanel", pEventName ) == 0 )
	{
		// Get the entity who killed us
		m_iKillerIndex = event->GetInt( "killer" );
		C_BaseEntity *pKiller =  ClientEntityList().GetBaseEntity( m_iKillerIndex );
		CTFPlayer *pTFPlayerKiller = NULL;
		if ( pKiller )
		{
			if ( pKiller->IsPlayer() )
			{
				pTFPlayerKiller = ToTFPlayer( pKiller );
			}
			else if ( pKiller->IsBaseObject() )
			{
				C_BaseObject *pObj = assert_cast<C_BaseObject *>( pKiller );
				pTFPlayerKiller = pObj->GetOwner();
			}
		}

		// Do we need to invalidate a new res file?
		const char *pszNewResFile = GetResFilename( pTFPlayerKiller );
		if ( V_stricmp( m_strCurrentFreezeCamResFile.String(), pszNewResFile ) != 0 )
		{
			m_strCurrentFreezeCamResFile = pszNewResFile;
			InvalidateLayout( true, true );
		}

		if ( !g_TF_PR )
		{
			if ( m_pNemesisSubPanel )
				m_pNemesisSubPanel->SetDialogVariable( "nemesisname", (const char *)nullptr );
			return;
		}

		Show();

		ShowSnapshotPanel( false );
		ShowSaveReplayPanel( false );
		m_bHoldingAfterScreenshot = false;

		if ( m_iBasePanelOriginalX > -1 && m_iBasePanelOriginalY > -1 )
		{
			m_pBasePanel->SetPos( m_iBasePanelOriginalX, m_iBasePanelOriginalY );
		}
		if ( m_iItemPanelOriginalX > -1 && m_iItemPanelOriginalY > -1 )
		{
			m_pItemPanel->SetPos( m_iItemPanelOriginalX, m_iItemPanelOriginalY );
		}

		int xp,yp;
		GetPos( xp, yp );
		if ( TFGameRules()->RoundHasBeenWon() )
		{
			SetPos( xp, m_iYBase - YRES(50) );
		}
		else
		{
			SetPos( xp, m_iYBase );
		}

		if ( pKiller )
		{
			int iMaxBuffedHealth = 0;

			if ( pTFPlayerKiller )
			{
				iMaxBuffedHealth = pTFPlayerKiller->m_Shared.GetMaxBuffedHealth();
			}

			int iKillerHealth = pKiller->GetHealth();
			if ( !pKiller->IsAlive() )
			{
				iKillerHealth = 0;
			}

			m_pKillerHealth->SetBuilding( pKiller->IsBaseObject() );
			m_pKillerHealth->SetHealth( iKillerHealth, pKiller->GetMaxHealth(), iMaxBuffedHealth );

			if ( m_pItemPanel )
			{
				m_pItemPanel->SetVisible( false );
			}

			if ( pKiller->IsPlayer() )
			{
				C_TFPlayer *pVictim = C_TFPlayer::GetLocalTFPlayer();

				//If this was just a regular kill but this guy is our nemesis then just show it.
				if ( pVictim && pTFPlayerKiller->m_Shared.IsPlayerDominated( pVictim->entindex() ) )
				{
					if ( !pKiller->IsAlive() )
					{
						m_pFreezeLabel->SetText( "#FreezePanel_Nemesis_Dead" );
					}
					else
					{
						m_pFreezeLabel->SetText( "#FreezePanel_Nemesis" );
					}
				}
				else
				{
					if ( !pKiller->IsAlive() )
					{
						m_pFreezeLabel->SetText( "#FreezePanel_Killer_Dead" );
					}
					else
					{
						m_pFreezeLabel->SetText( "#FreezePanel_Killer" );
					}
				}

				m_pBasePanel->SetDialogVariable( "killername", g_PR->GetPlayerName( m_iKillerIndex ) );

				if ( m_pAvatar )
				{
					m_pAvatar->SetPlayer( (C_BasePlayer*)pKiller );
				}

				// If our killer is using a powerup, show the details of that powerup
				if ( pTFPlayerKiller && pTFPlayerKiller->m_Shared.IsCarryingRune() )
				{
					static CSchemaItemDefHandle rgPowerupItems [] =  { CSchemaItemDefHandle( "Powerup Strength" )
																	 , CSchemaItemDefHandle( "Powerup Haste" )
																	 , CSchemaItemDefHandle( "Powerup Regen" )
																	 , CSchemaItemDefHandle( "Powerup Resist" )
																	 , CSchemaItemDefHandle( "Powerup Vampire" )
																	 , CSchemaItemDefHandle( "Powerup Reflect" )
																	 , CSchemaItemDefHandle( "Powerup Precision" )
																	 , CSchemaItemDefHandle( "Powerup Agility" )
																	 , CSchemaItemDefHandle( "Powerup Knockout" )
																	 , CSchemaItemDefHandle( "Powerup King" ) 
																	 , CSchemaItemDefHandle( "Powerup Plague" ) 
																	 , CSchemaItemDefHandle( "Powerup Supernova" ) };

					COMPILE_TIME_ASSERT( ARRAYSIZE( rgPowerupItems ) == RUNE_TYPES_MAX );

					// Get the item 
					const CSchemaItemDefHandle& itemDef = rgPowerupItems[pTFPlayerKiller->m_Shared.GetCarryingRuneType()];

					// Create a fake, temp item to show the powerup
					CEconItemView item;
					item.SetItemDefIndex( itemDef->GetDefinitionIndex() );
					item.SetItemQuality( AE_UNIQUE );	// Unique by default
					item.SetItemLevel( 0 ); // Hide this?
					item.SetInitialized( true );
					item.SetItemOriginOverride( kEconItemOrigin_Invalid );

					m_pItemPanel->SetDialogVariable( "killername", g_PR->GetPlayerName( m_iKillerIndex ) );
					m_pItemPanel->SetItem( &item );
					m_pItemPanel->SetVisible( true );
				}
				else
				{
					// If our killer is using an item, display its stats.
					CTFWeaponBase *pWeapon = pTFPlayerKiller ? pTFPlayerKiller->GetActiveTFWeapon() : NULL;
					bool bShowItem = false;
					if ( pWeapon )
					{
						bShowItem = pWeapon->GetAttributeContainer()->GetItem()->GetItemQuality() != AE_NORMAL;
						if ( bShowItem )
						{
							CTFStatPanel *pStatPanel = GET_HUDELEMENT( CTFStatPanel );
							if ( pStatPanel && pStatPanel->IsVisible() )
							{
								// Stat panel overrides.
								bShowItem = false;
							}
						}
					}

					if ( bShowItem )
					{
						Label* pItemLabel = m_pItemPanel->FindControl<Label>( "ItemLabel" );
						CEconItemView *pItemToShow = pWeapon->GetAttributeContainer()->GetItem();

						if ( pItemToShow && !pItemToShow->IsUndefined() )
						{
							if ( pItemLabel )
							{
								// Change the label text depending on if they're holding someone else's item
								CBasePlayer *pOriginalOwner = GetPlayerByAccountID( pItemToShow->GetAccountID() );
								bool bOriginalOwner = !pOriginalOwner || pOriginalOwner == pKiller;
								pItemLabel->SetText( bOriginalOwner ? "#FreezePanel_Item" : "#FreezePanel_ItemOtherOwner" );
								m_pItemPanel->SetDialogVariable( "ownername", pOriginalOwner ? g_PR->GetPlayerName( pOriginalOwner->entindex() ) : "" );
							}

							m_pItemPanel->SetDialogVariable( "killername", g_PR->GetPlayerName( m_iKillerIndex ) );
							m_pItemPanel->SetItem( pItemToShow );
							m_pItemPanel->SetVisible( true );
						}
					}
				}
				if ( m_pItemPanel && m_pItemPanel->IsVisible() )
				{
					int x, y;
					m_pItemPanel->GetPos( x, y );
					m_pItemPanel->SetPos( x, ScreenHeight() - YRES( 12 ) - m_pItemPanel->GetTall() );
				}
			}
			else if ( pKiller->IsBaseObject() )
			{
				C_BaseObject *pObj = assert_cast<C_BaseObject *>( pKiller );
				//Assert( pTFPlayerKiller && "Why does this object not have an owner?" );
				if ( pTFPlayerKiller )
				{
					m_iKillerIndex = pTFPlayerKiller->entindex();

					m_pBasePanel->SetDialogVariable( "killername", g_PR->GetPlayerName( m_iKillerIndex ) );

					if ( m_pAvatar )
					{
						m_pAvatar->SetPlayer( pTFPlayerKiller );
						m_pAvatar->SetVisible( true );
					}

					pKiller = pTFPlayerKiller;
				}
				else
				{
					if ( m_pAvatar )
					{
						m_pAvatar->SetVisible( false );
					}
				}

				if ( m_pFreezeLabel )
				{
					if ( pKiller && !pKiller->IsAlive() )
					{
						m_pFreezeLabel->SetText( "#FreezePanel_KillerObject_Dead" );
					}
					else
					{
						m_pFreezeLabel->SetText( "#FreezePanel_KillerObject" );
					}
				}
				const char *pszStatusName = pObj->GetStatusName();
				wchar_t *wszLocalized = g_pVGuiLocalize->Find( pszStatusName );

				if ( !wszLocalized )
				{
					m_pBasePanel->SetDialogVariable( "objectkiller", pszStatusName );
				}
				else
				{
					m_pBasePanel->SetDialogVariable( "objectkiller", wszLocalized );
				}
			}
			else if ( dynamic_cast< C_HeadlessHatman * >( pKiller ) != NULL )
			{
				m_pBasePanel->SetDialogVariable( "killername", g_pVGuiLocalize->Find( "#TF_HALLOWEEN_BOSS_DEATHCAM_NAME" ) );

				if ( m_pAvatar )
				{
					m_pAvatar->SetVisible( false );
				}
			}
			else if ( dynamic_cast< C_EyeballBoss * >( pKiller ) != NULL )
			{
				m_pBasePanel->SetDialogVariable( "killername", g_pVGuiLocalize->Find( "#TF_HALLOWEEN_EYEBALL_BOSS_DEATHCAM_NAME" ) );

				if ( m_pAvatar )
				{
					m_pAvatar->SetVisible( false );
				}
			}
			else if ( dynamic_cast< C_Merasmus * >( pKiller ) != NULL )
			{
				m_pBasePanel->SetDialogVariable( "killername", g_pVGuiLocalize->Find( "#TF_HALLOWEEN_MERASMUS_DEATHCAM_NAME" ) );

				if ( m_pAvatar )
				{
					m_pAvatar->SetVisible( false );
				}
			}
			else if ( m_pFreezeLabel )
			{
				if ( !pKiller->IsAlive() )
				{
					m_pFreezeLabel->SetText( "#FreezePanel_Killer_Dead" );
				}
				else
				{
					m_pFreezeLabel->SetText( "#FreezePanel_Killer" );
				}
			}
			
			if ( m_pFreezePanelBG )
			{
				// use the killer's team for the background color
				m_pFreezePanelBG->SetImage( pKiller->GetTeamNumber() == TF_TEAM_BLUE ? "../hud/color_panel_blu" : "../hud/color_panel_red" );
			}

			if ( m_pAvatar )
			{
				int iAvX, iAvY;
				m_pAvatar->GetPos( iAvX, iAvY );
				if ( m_pAvatar->IsVisible() && m_pAvatar->IsValid() )
				{
					m_pKillerLabel->SetPos( iAvX + m_pAvatar->GetWide() + XRES(2), m_iKillerOriginalY );
				}
				else
				{
					m_pKillerLabel->SetPos( iAvX, m_iKillerOriginalY );
				}
			}
		}
		
		// see if we should show nemesis panel
		bool bAdvice = false;
		const wchar_t *pchNemesisText = NULL;
		switch ( m_iShowNemesisPanel )
		{
		case SHOW_NO_NEMESIS:
			{
				C_TFPlayer *pVictim = C_TFPlayer::GetLocalTFPlayer();
				CTFPlayer *pTFKiller = ToTFPlayer( pKiller );
			
				//If this was just a regular kill but this guy is our nemesis then just show it.
				if ( pTFKiller && pTFKiller->m_Shared.IsPlayerDominated( pVictim->entindex() ) )
				{					
					pchNemesisText = g_pVGuiLocalize->Find( "#TF_FreezeNemesis" );
				}
				// UNDONE: We're not shipping this for now
				/*else if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && pTFKiller && pTFKiller->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
				{
					const wchar_t *pwchHint = g_pVGuiLocalize->Find( VarArgs( "#TF_PVE_FreezePanelHint_%s", pTFKiller->GetPlayerClass()->GetClassIconName() ) );
					if ( pwchHint && pwchHint[ 0 ] != L'\0' )
					{
						pchNemesisText = pwchHint;
						bAdvice = true;
					}
				}*/
			}
			break;
		case SHOW_NEW_NEMESIS:
			{
				C_TFPlayer *pVictim = C_TFPlayer::GetLocalTFPlayer();
				CTFPlayer *pTFKiller = ToTFPlayer( pKiller );
				// check to see if killer is still the nemesis of victim; victim may have managed to kill him after victim's
				// death by grenade or some such, extracting revenge and clearing nemesis condition
				if ( pTFKiller && pTFKiller->m_Shared.IsPlayerDominated( pVictim->entindex() ) )
				{					
					pchNemesisText = g_pVGuiLocalize->Find( "#TF_NewNemesis" );
				}			
			}
			break;
		case SHOW_REVENGE:
			pchNemesisText = g_pVGuiLocalize->Find( "#TF_GotRevenge" );
			break;
		default:
			Assert( false );	// invalid value
			break;
		}

		if ( m_pNemesisSubPanel )
		{
			if ( !bAdvice )
			{
				m_pNemesisSubPanel->SetDialogVariable( "nemesisname", pchNemesisText );
				m_pNemesisSubPanel->SetControlVisible( "NemesisLabel2", false );
			}
			else
			{
				m_pNemesisSubPanel->SetDialogVariable( "nemesisname", "" );
				m_pNemesisSubPanel->SetControlVisible( "NemesisLabel2", true );
				m_pNemesisSubPanel->SetDialogVariable( "nemesisadvice", pchNemesisText );
			}
		}

		ShowNemesisPanel( pchNemesisText != NULL );
		m_iShowNemesisPanel = SHOW_NO_NEMESIS;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFFreezePanel::GetResFilename( C_TFPlayer *pTFPlayer /*= NULL*/ ) const
{
	return "resource/UI/FreezePanel_Basic.res";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::ShowCalloutsIn( float flTime )
{
	m_flShowCalloutsAt = gpGlobals->curtime + flTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFreezePanelCallout *CTFFreezePanel::TestAndAddCallout( Vector &origin, Vector &vMins, Vector &vMaxs, CUtlVector<Vector> *vecCalloutsTL, 
			CUtlVector<Vector> *vecCalloutsBR, Vector &vecFreezeTL, Vector &vecFreezeBR, Vector &vecStatTL, Vector &vecStatBR, int *iX, int *iY )
{
	// This is the offset from the topleft of the callout to the arrow tip
	const int iXOffset = XRES(25);
	const int iYOffset = YRES(50);

	//if ( engine->IsBoxInViewCluster( vMins + origin, vMaxs + origin) && !engine->CullBox( vMins + origin, vMaxs + origin ) )
	{
		if ( GetVectorInHudSpace( origin, *iX, *iY ) )				// TODO: GetVectorInHudSpace or GetVectorInScreenSpace?
		{
			*iX -= iXOffset;
			*iY -= iYOffset;
			int iRight = *iX + CALLOUT_WIDE;
			int iBottom = *iY + CALLOUT_TALL;
			if ( *iX > 0 && *iY > 0 && (iRight < ScreenWidth()) && (iBottom < (ScreenHeight()-YRES(40))) )
			{
				// Make sure it wouldn't be over the top of the freezepanel or statpanel
				Vector vecCalloutTL( *iX, *iY, 0 );
				Vector vecCalloutBR( iRight, iBottom, 1 );
				if ( !QuickBoxIntersectTest( vecCalloutTL, vecCalloutBR, vecFreezeTL, vecFreezeBR ) &&
					 !QuickBoxIntersectTest( vecCalloutTL, vecCalloutBR, vecStatTL, vecStatBR ) )
				{
					// Make sure it doesn't intersect any other callouts
					bool bClear = true;
					for ( int iCall = 0; iCall < vecCalloutsTL->Count(); iCall++ )
					{
						if ( QuickBoxIntersectTest( vecCalloutTL, vecCalloutBR, vecCalloutsTL->Element(iCall), vecCalloutsBR->Element(iCall) ) )
						{
							bClear = false;
							break;
						}
					}

					if ( bClear )
					{
						// Verify that we have LOS to the gib
						trace_t	tr;
						UTIL_TraceLine( origin, MainViewOrigin(), MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr );
						bClear = ( tr.fraction >= 1.0f );
					}

					if ( bClear )
					{
						CTFFreezePanelCallout *pCallout = new CTFFreezePanelCallout( g_pClientMode->GetViewport(), "FreezePanelCallout" );
						m_pCalloutPanels.AddToTail( vgui::SETUP_PANEL(pCallout) );
						vecCalloutsTL->AddToTail( vecCalloutTL );
						vecCalloutsBR->AddToTail( vecCalloutBR );
						pCallout->SetVisible( true );
						pCallout->SetBounds( *iX, *iY, CALLOUT_WIDE, CALLOUT_TALL );
						return pCallout;
					}
				}
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::UpdateCallout( void )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	// Abort early if we have no gibs or ragdoll
	CUtlVector<EHANDLE>	*pGibList = pPlayer->GetSpawnedGibs();
	IRagdoll *pRagdoll = pPlayer->GetRepresentativeRagdoll();
	if ( (!pGibList || pGibList->Count() == 0) && !pRagdoll )
		return;

	if ( m_pFreezePanelBG == NULL )
		return;

	// Precalc the vectors of the freezepanel & statpanel
	int iX, iY;
	m_pFreezePanelBG->GetPos( iX, iY );
	Vector vecFreezeTL( iX, iY, 0 );
	Vector vecFreezeBR( iX + m_pFreezePanelBG->GetWide(), iY + m_pFreezePanelBG->GetTall(), 1 );

	CUtlVector<Vector> vecCalloutsTL;
	CUtlVector<Vector> vecCalloutsBR;

	Vector vecStatTL(0,0,0);
	Vector vecStatBR(0,0,1);
	CTFStatPanel *pStatPanel = GET_HUDELEMENT( CTFStatPanel );
	if ( pStatPanel && pStatPanel->IsVisible() )
	{
		pStatPanel->GetPos( iX, iY );
		vecStatTL.x = iX;
		vecStatTL.y = iY;
		vecStatBR.x = vecStatTL.x + pStatPanel->GetWide();
		vecStatBR.y = vecStatTL.y + pStatPanel->GetTall();
	}

	Vector vMins, vMaxs;

	// Check gibs
	if ( pGibList && pGibList->Count() )
	{
		int iCount = 0;
		for ( int i = 0; i < pGibList->Count(); i++ )
		{
			CBaseEntity *pGib = pGibList->Element(i);
			if ( pGib )
			{
				Vector origin = pGib->GetRenderOrigin();
				IPhysicsObject *pPhysicsObject = pGib->VPhysicsGetObject();
				if( pPhysicsObject )
				{
					Vector vecMassCenter = pPhysicsObject->GetMassCenterLocalSpace();
					pGib->CollisionProp()->CollisionToWorldSpace( vecMassCenter, &origin );
				}
				pGib->GetRenderBounds( vMins, vMaxs );

				// Try and add the callout
				CTFFreezePanelCallout *pCallout = TestAndAddCallout( origin, vMins, vMaxs, &vecCalloutsTL, &vecCalloutsBR, 
					vecFreezeTL, vecFreezeBR, vecStatTL, vecStatBR, &iX, &iY );
				if ( pCallout )
				{
					pCallout->UpdateForGib( i, iCount );
					iCount++;
				}
			}
		}

		C_ObjectSentrygun *pSentry = dynamic_cast<C_ObjectSentrygun*>( ClientEntityList().GetEnt( GetSpectatorTarget() ) );
		if ( pSentry )
		{
			// A sentry was the killer...check and see if the builder is on screen.
			CTFPlayer *pBuilder = pSentry->GetBuilder();
			if ( pBuilder && GetVectorInHudSpace( pBuilder->GetRenderOrigin(), iX, iY ) )				// TODO: GetVectorInHudSpace or GetVectorInScreenSpace?
			{
				KeyValues *kv = new KeyValues( "FreezeCamTaunt" );
				kv->SetInt( "achiever", pBuilder->GetUserID() );
				kv->SetString( "command", "freezecam_tauntsentry" );
				engine->ServerCmdKeyValues( kv );
			}
		}

		// Tell the server that we saw some gibs onscreen
		if ( iCount > 0 )
		{
			SendTauntAcknowledgement( "freezecam_tauntgibs", iCount );
		}
	}

	// Check for a ragdoll as well. Dying characters that ragdoll can also drop wearable items as gibs
	if ( pRagdoll )
	{
		Vector origin = pRagdoll->GetRagdollOrigin();
		pRagdoll->GetRagdollBounds( vMins, vMaxs );

		// Try and add the callout
		CTFFreezePanelCallout *pCallout = TestAndAddCallout( origin, vMins, vMaxs, &vecCalloutsTL, &vecCalloutsBR, 
															 vecFreezeTL, vecFreezeBR, vecStatTL, vecStatBR, &iX, &iY );
		if ( pCallout )
		{
			pCallout->UpdateForRagdoll();
		}

		// even if the callout failed, check that our ragdoll is onscreen and our killer is taunting us (for an achievement)
		if ( GetVectorInHudSpace( origin, iX, iY ) )				// TODO: GetVectorInHudSpace or GetVectorInScreenSpace?
		{
			SendTauntAcknowledgement( "freezecam_tauntrag" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::Show()
{
	// Josh:
	// When the freeze panel is first shown( after we have done all the setup of setting strings, dialog vars, etc ),
	// due to some jank modern TF does with HUD setup, it ends up re - creating all the elements and calling ApplySchemeSettings
	// which calls LoadControlSettings and such again, which invalidates all of our previous setup!
	MakeReadyForUse();

	m_flShowCalloutsAt = 0;
	SetVisible( true );
}

void CTFFreezePanel::DeleteCalloutPanels()
{
	for ( int i = m_pCalloutPanels.Count()-1; i >= 0; i-- )
	{
		m_pCalloutPanels[i]->MarkForDeletion();
	}
	m_pCalloutPanels.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::Hide()
{
	SetVisible( false );
	m_bHoldingAfterScreenshot = false;

	// Delete all our callout panels
	DeleteCalloutPanels();

#if defined( REPLAY_ENABLED )
	// Explicitly set the replay reminder's visibility, which is not parented
	// to the freeze panel.
	if ( m_pSaveReplayPanel )
	{
		m_pSaveReplayPanel->SetVisible( false );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFreezePanel::ShouldDraw( void )
{
	return ( IsVisible() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::OnThink( void )
{
	BaseClass::OnThink();

	if ( m_pItemPanel && m_pItemPanel->IsVisible() )
	{
		CTFStatPanel *pStatPanel = GET_HUDELEMENT( CTFStatPanel );
		if ( pStatPanel && pStatPanel->IsVisible() )
		{
			m_pItemPanel->SetVisible( false );
		}
	}

	if ( m_flShowCalloutsAt && m_flShowCalloutsAt < gpGlobals->curtime )
	{
		if ( ShouldDraw() )
		{
			UpdateCallout();
		}
		m_flShowCalloutsAt = 0;
	}

	if ( m_flShowSnapshotReminderAt && m_flShowSnapshotReminderAt < gpGlobals->curtime )
	{
		if ( ShouldDraw() )
		{
			// For now don't do this in Steam Controller mode, because there's no easy way for a SC user to deal with this
			if ( !::input->IsSteamControllerActive() )
			{
				ShowSnapshotPanel( true );
			}
		}
		m_flShowSnapshotReminderAt = 0;
	}

	if ( m_flShowReplayReminderAt && m_flShowReplayReminderAt < gpGlobals->curtime )
	{
		if ( ShouldDraw() )
		{
			// For now don't do this in Steam Controller mode, because there's no easy way for a SC user to deal with this
			if ( !::input->IsSteamControllerActive() )
			{
				ShowSaveReplayPanel( true );
			}
		}
		m_flShowReplayReminderAt = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::ShowSnapshotPanelIn( float flTime )
{
#if defined (_X360 )
	return;
#endif

	m_flShowSnapshotReminderAt = gpGlobals->curtime + flTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::ShowSaveReplayPanelIn( float flTime )
{
#if defined (_X360 )
	return;
#endif
	m_flShowReplayReminderAt = gpGlobals->curtime + flTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::ShowSnapshotPanel( bool bShow )
{
	if ( !m_pScreenshotPanel )
		return;

	const char *key = engine->Key_LookupBinding( "screenshot" );

	if ( key == NULL || FStrEq( key, "(null)" ) )
	{
		bShow = false;
		key = " ";
	}

	if ( bShow )
	{
		char szKey[16];
		Q_snprintf( szKey, sizeof(szKey), "%s", key );
		wchar_t wKey[16];
		wchar_t wLabel[256];

		g_pVGuiLocalize->ConvertANSIToUnicode(szKey, wKey, sizeof(wKey));
		g_pVGuiLocalize->ConstructString_safe( wLabel, g_pVGuiLocalize->Find("#TF_freezecam_snapshot" ), 1, wKey );

		m_pScreenshotPanel->SetDialogVariable( "text", wLabel );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudSnapShotReminderIn" );
	}

	m_pScreenshotPanel->SetVisible( bShow );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::ShowSaveReplayPanel( bool bShow )
{
#if defined( REPLAY_ENABLED )
	// Make sure ptr's ok
	if ( !m_pSaveReplayPanel )
		return;

	// Don't do this for Steam Controller users
	if ( ::input->IsSteamControllerActive() )
		return;

	// Make sure we're recording
	if ( !g_pReplay->IsRecording() )
		return;
	
	// Start animation if necessary
	if ( bShow )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pSaveReplayPanel->GetParent(), "HudReplayReminderIn2" );
	}

	// Setup visibility
	m_pSaveReplayPanel->SetVisible( bShow );
#endif
}

const char *CTFFreezePanel::GetFilesafePlayerName( const char *pszOldName )
{
	if ( !pszOldName )
		return "";

	static char szSafeName[ MAX_PLAYER_NAME_LENGTH ];
	int nSafeNameBufSize = sizeof( szSafeName );
	int nNewPos = 0;
	
	for( const char *p = pszOldName; *p != 0 && nNewPos < nSafeNameBufSize-1; p++ )
	{
		if( *p == '.' )
		{
			szSafeName[ nNewPos ] = '-';
		}
		else if( *p == '/' )
		{
			szSafeName[ nNewPos ] = '-';
		}
		else if( *p == '\\' )
		{
			szSafeName[ nNewPos ] = '-';
		}
		else if( *p == ':' )
		{
			szSafeName[ nNewPos ] = '-';
		}
		else
		{
			szSafeName[ nNewPos ] = *p;
		}

		nNewPos++;
	}

	szSafeName[ nNewPos ] = 0;

	return szSafeName;
}

int	CTFFreezePanel::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( ShouldDraw() && pszCurrentBinding )
	{
		if ( FStrEq( pszCurrentBinding, "screenshot" ) || FStrEq( pszCurrentBinding, "jpeg" ) )
		{
			// move the target id to the corner
			if ( m_pBasePanel && m_bShouldScreenshotMovePanelToCorner )
			{
				int w, h;
				m_pBasePanel->GetSize( w, h );

				if ( m_pItemPanel && m_pItemPanel->IsVisible() )
				{
					int iw,ih;
					m_pItemPanel->GetSize( iw, ih );
					m_pItemPanel->SetPos( ScreenWidth() - iw, ScreenHeight() - ih );
					m_pBasePanel->SetPos( ScreenWidth() - w, ScreenHeight() - ih - h );
				}
				else
				{
					m_pBasePanel->SetPos( ScreenWidth() - w, ScreenHeight() - h );
				}
			}

			// Get the local player.
			C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
			if ( pPlayer )
			{
				//Do effects
				g_flFreezeFlash = gpGlobals->curtime + 0.75f;
				pPlayer->EmitSound( "Camera.SnapShot" );

				//Extend Freezecam by a couple more seconds.
				engine->ClientCmd( "extendfreeze" );
				view->FreezeFrame( 3.0f );

				//Hide the reminder panel
				m_flShowSnapshotReminderAt = 0;
				ShowSnapshotPanel( false );

				// Hide replay reminder panel
				m_flShowReplayReminderAt = 0;
				ShowSaveReplayPanel( false );

				m_bHoldingAfterScreenshot = true;

				// Hide everything?
				if ( hud_freezecamhide.GetBool() )
				{
					SetVisible( false );
					DeleteCalloutPanels();
				}

				//Set the screenshot name
				if ( m_iKillerIndex <= MAX_PLAYERS )
				{
					const char *pszKillerName = g_PR->GetPlayerName( m_iKillerIndex );

					if ( pszKillerName )
					{
						ConVarRef cl_screenshotname( "cl_screenshotname" );

						if ( cl_screenshotname.IsValid() )
						{
							char szScreenShotName[512];

							Q_snprintf( szScreenShotName, sizeof( szScreenShotName ), "%s %s", GetFilesafePlayerName( pszKillerName ), FREEZECAM_SCREENSHOT_STRING );

							cl_screenshotname.SetValue( szScreenShotName );
						}
					}

					C_TFPlayer *pKiller = ToTFPlayer( UTIL_PlayerByIndex( m_iKillerIndex ) );
					if ( pKiller )
					{
						CSteamID steamID;
						if ( pKiller->GetSteamID( &steamID ) )
						{
							ConVarRef cl_screenshotusertag( "cl_screenshotusertag" );
							if ( cl_screenshotusertag.IsValid() )
							{
								cl_screenshotusertag.SetValue( (int)steamID.GetAccountID() );
							}
						}
					}
				}
			}
		}
#if defined( REPLAY_ENABLED )
		else if ( FStrEq (pszCurrentBinding, "save_replay" ) )
		{
			m_flShowReplayReminderAt = 0;
			ShowSaveReplayPanel( false );
		}
#endif
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Shows or hides the nemesis part of the panel
//-----------------------------------------------------------------------------
void CTFFreezePanel::ShowNemesisPanel( bool bShow )
{
	if ( !m_pNemesisSubPanel )
		return;

	m_pNemesisSubPanel->SetVisible( bShow );

	if ( bShow )
	{
		vgui::Label *pLabel = dynamic_cast< vgui::Label *>( m_pNemesisSubPanel->FindChildByName( "NemesisLabel" ) );
		vgui::Label *pLabel2 = dynamic_cast< vgui::Label *>( m_pNemesisSubPanel->FindChildByName( "NemesisLabel2" ) );
		vgui::Panel *pBG = m_pNemesisSubPanel->FindChildByName( "NemesisPanelBG" );
		vgui::ImagePanel *pIcon = dynamic_cast< vgui::ImagePanel *>( m_pNemesisSubPanel->FindChildByName( "NemesisIcon" ) );

		// check that our Nemesis panel and resize it to the length of the string (the right side is pinned and doesn't move)
		if ( pLabel && pLabel2 && pBG && pIcon )
		{
			int nDiffX, nDiffY;
			int wide, tall;

			if ( !pLabel2->IsVisible() )
			{
				pLabel->GetContentSize( wide, tall );
				nDiffX = wide - pLabel->GetWide();
				nDiffY = tall - pLabel->GetTall();
			}
			else
			{
				pLabel2->GetContentSize( wide, tall );
				nDiffX = wide - pLabel2->GetWide();
				nDiffY = tall - pLabel2->GetTall();
			}

			if ( nDiffX != 0 || nDiffY != 0 )
			{
				int x, y, w, t;

				// move the icon
				pIcon->GetBounds( x, y, w, t );
				pIcon->SetBounds( x - nDiffX, y - nDiffY, w, t );

				pLabel->GetBounds( x, y, w, t );
				pLabel->SetBounds( x - nDiffX, y - nDiffY, w + nDiffX, t + nDiffY );

				pLabel2->GetBounds( x, y, w, t );
				pLabel2->SetBounds( x - nDiffX, y - nDiffY, w + nDiffX, t + nDiffY );

				// move/resize the background
				pBG->GetBounds( x, y, w, t );
				pBG->SetBounds( x - nDiffX, y - nDiffY, w + nDiffX, t + nDiffY );

				m_pNemesisSubPanel->GetBounds( x, y, w, t );
				m_pNemesisSubPanel->SetBounds( x, y - nDiffY, w, t + nDiffY );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFreezePanelCallout::CTFFreezePanelCallout( Panel *parent, const char *name ) : EditablePanel(parent,name)
{
	m_pGibLabel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFFreezePanelCallout::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/FreezePanelCallout.res" );

	m_pGibLabel = dynamic_cast<Label *>( FindChildByName("CalloutLabel") );
}

const char *pszCalloutGibNames[] =
{
	"#Callout_Head",
	"#Callout_Foot",
	"#Callout_Hand",
	"#Callout_Torso",
	NULL,	// Random
};
const char *pszCalloutRandomGibNames[] =
{
	"#Callout_Organ2",
	"#Callout_Organ3",
	"#Callout_Organ4",
	"#Callout_Organ5",
	"#Callout_Organ6",
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanelCallout::UpdateForGib( int iGib, int iCount )
{
	if ( !m_pGibLabel )
		return;

	if ( iGib < ARRAYSIZE(pszCalloutGibNames) )
	{
		if ( pszCalloutGibNames[iGib] )
		{
			m_pGibLabel->SetText( pszCalloutGibNames[iGib] );
		}
		else
		{
			m_pGibLabel->SetText( pszCalloutRandomGibNames[ RandomInt(0,ARRAYSIZE(pszCalloutRandomGibNames)-1) ] );
		}
	}
	else
	{
		if ( iCount > 1 )
		{
			m_pGibLabel->SetText( "#FreezePanel_Callout3" );
		}
		else if ( iCount == 1 )
		{
			m_pGibLabel->SetText( "#FreezePanel_Callout2" );
		}
		else
		{
			m_pGibLabel->SetText( "#FreezePanel_Callout" );
		}
	}
	
#ifndef _X360
	int wide, tall;
	m_pGibLabel->GetContentSize( wide, tall );

	// is the text wider than the label?
	if ( wide > m_pGibLabel->GetWide() )
	{
		int nDiff = wide - m_pGibLabel->GetWide();
		int x, y, w, t;

		// make the label wider
		m_pGibLabel->GetBounds( x, y, w, t );
		m_pGibLabel->SetBounds( x, y, w + nDiff, t );

		vgui::Panel *pBackground = FindChildByName( "CalloutBG" );
		if ( pBackground )
		{
			// also adjust the background image
			pBackground->GetBounds( x, y, w, t );
			pBackground->SetBounds( x, y, w + nDiff, t );
		}

		// make ourselves bigger to accommodate the wider children
		GetBounds( x, y, w, t );
		SetBounds( x, y, w + nDiff, t );

		// check that we haven't run off the right side of the screen
		if ( x + GetWide() > ScreenWidth() )
		{
			// push ourselves to the left to fit on the screen
			nDiff = ( x + GetWide() ) - ScreenWidth();
			SetPos( x - nDiff, y );

			// push the arrow to the right to offset moving ourselves to the left
			vgui::ImagePanel *pArrow = dynamic_cast<ImagePanel *>( FindChildByName( "ArrowIcon" ) );
			if ( pArrow )
			{
				pArrow->GetBounds( x, y, w, t );
				pArrow->SetBounds( x + nDiff, y, w, t );
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanelCallout::UpdateForRagdoll( void )
{
	if ( !m_pGibLabel )
		return;

	m_pGibLabel->SetText( "#Callout_Ragdoll" );	
}
