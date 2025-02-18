//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/QueryBox.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include <game/client/iviewport.h>
#include "tf_tips.h"
#include "tf_mapinfo.h"
#include "vgui_avatarimage.h"
#include "VGuiMatSurface/IMatSystemSurface.h"

#include "tf_statsummary.h"
#include <convar.h>
#include "fmtstr.h"
#include "tf_gamerules.h"
#include "tf_gc_client.h"

using namespace vgui;

#if defined( REPLAY_ENABLED )
extern bool g_bIsReplayRewinding;
#else
bool g_bIsReplayRewinding = false;
#endif

const char *g_pszTipsClassImages[] =
{
	"",						// TF_CLASS_UNDEFINED = 0,
	"class_portraits/scout",	// TF_CLASS_SCOUT,			
	"class_portraits/sniper",// TF_CLASS_SNIPER,
	"class_portraits/soldier",		// TF_CLASS_SOLDIER,
	"class_portraits/demoman",		// TF_CLASS_DEMOMAN,
	"class_portraits/medic",		// TF_CLASS_MEDIC,
	"class_portraits/heavy",	// TF_CLASS_HEAVYWEAPONS,
	"class_portraits/pyro",	// TF_CLASS_PYRO,
	"class_portraits/spy",		// TF_CLASS_SPY,
	"class_portraits/engineer",		// TF_CLASS_ENGINEER,		
};

ClassDetails_t g_PerClassStatDetails[15] =
{
	{ TFSTAT_POINTSSCORED,			ALL_CLASSES,					"#TF_ClassRecord_MostPoints", "#TF_ClassRecord_Alt_MostPoints" },
	{ TFSTAT_KILLS,					ALL_CLASSES,					"#TF_ClassRecord_MostKills", "#TF_ClassRecord_Alt_MostKills" },
	{ TFSTAT_KILLASSISTS,			ALL_CLASSES,					"#TF_ClassRecord_MostAssists", "#TF_ClassRecord_Alt_MostAssists" },
	{ TFSTAT_CAPTURES,				ALL_CLASSES,					"#TF_ClassRecord_MostCaptures", "#TF_ClassRecord_Alt_MostCaptures" },
	{ TFSTAT_DEFENSES,				ALL_CLASSES,					"#TF_ClassRecord_MostDefenses", "#TF_ClassRecord_Alt_MostDefenses" },
	{ TFSTAT_DAMAGE,				ALL_CLASSES,					"#TF_ClassRecord_MostDamage", "#TF_ClassRecord_Alt_MostDamage" },
	{ TFSTAT_BUILDINGSDESTROYED,	ALL_CLASSES,					"#TF_ClassRecord_MostDestruction", "#TF_ClassRecord_Alt_MostDestruction" },
	{ TFSTAT_DOMINATIONS,			ALL_CLASSES,					"#TF_ClassRecord_MostDominations", "#TF_ClassRecord_Alt_MostDominations" },
	{ TFSTAT_PLAYTIME,				ALL_CLASSES,					"#TF_ClassRecord_LongestLife", "#TF_ClassRecord_Alt_LongestLife" },
	{ TFSTAT_HEALING,				MAKESTATFLAG(TF_CLASS_MEDIC) | MAKESTATFLAG(TF_CLASS_ENGINEER) | MAKESTATFLAG(TF_CLASS_HEAVYWEAPONS),	"#TF_ClassRecord_MostHealing", "#TF_ClassRecord_Alt_MostHealing" },
	{ TFSTAT_INVULNS,				MAKESTATFLAG(TF_CLASS_MEDIC),		"#TF_ClassRecord_MostInvulns", "#TF_ClassRecord_Alt_MostInvulns" },
	{ TFSTAT_MAXSENTRYKILLS,		MAKESTATFLAG(TF_CLASS_ENGINEER),	"#TF_ClassRecord_MostSentryKills", "#TF_ClassRecord_Alt_MostSentryKills" },
	{ TFSTAT_TELEPORTS,				MAKESTATFLAG(TF_CLASS_ENGINEER),	"#TF_ClassRecord_MostTeleports", "#TF_ClassRecord_Alt_MostTeleports" },
	{ TFSTAT_HEADSHOTS,				MAKESTATFLAG(TF_CLASS_SNIPER) | MAKESTATFLAG(TF_CLASS_SPY),		"#TF_ClassRecord_MostHeadshots", "#TF_ClassRecord_Alt_MostHeadshots" },
	{ TFSTAT_BACKSTABS,				MAKESTATFLAG(TF_CLASS_SPY),			"#TF_ClassRecord_MostBackstabs", "#TF_ClassRecord_Alt_MostBackstabs" },
};

ClassDetails_t g_PerClassMVMStatDetails[12] =
{
	{ TFSTAT_POINTSSCORED,			ALL_CLASSES,					"#TF_ClassRecord_MostPoints", "#TF_ClassRecord_Alt_MostPoints" },
	{ TFSTAT_KILLS,					ALL_CLASSES,					"#TF_ClassRecord_MostKills", "#TF_ClassRecord_Alt_MostKills" },
	{ TFSTAT_KILLASSISTS,			ALL_CLASSES,					"#TF_ClassRecord_MostAssists", "#TF_ClassRecord_Alt_MostAssists" },
	{ TFSTAT_DEFENSES,				ALL_CLASSES,					"#TF_ClassRecord_MostDefenses", "#TF_ClassRecord_Alt_MostDefenses" },
	{ TFSTAT_DAMAGE,				ALL_CLASSES,					"#TF_ClassRecord_MostDamage", "#TF_ClassRecord_Alt_MostDamage" },
	{ TFSTAT_PLAYTIME,				ALL_CLASSES,					"#TF_ClassRecord_LongestLife", "#TF_ClassRecord_Alt_LongestLife" },
	{ TFSTAT_HEALING,				MAKESTATFLAG(TF_CLASS_MEDIC) | MAKESTATFLAG(TF_CLASS_ENGINEER) | MAKESTATFLAG(TF_CLASS_HEAVYWEAPONS),	"#TF_ClassRecord_MostHealing", "#TF_ClassRecord_Alt_MostHealing" },
	{ TFSTAT_INVULNS,				MAKESTATFLAG(TF_CLASS_MEDIC),		"#TF_ClassRecord_MostInvulns", "#TF_ClassRecord_Alt_MostInvulns" },
	{ TFSTAT_MAXSENTRYKILLS,		MAKESTATFLAG(TF_CLASS_ENGINEER),	"#TF_ClassRecord_MostSentryKills", "#TF_ClassRecord_Alt_MostSentryKills" },
	{ TFSTAT_TELEPORTS,				MAKESTATFLAG(TF_CLASS_ENGINEER),	"#TF_ClassRecord_MostTeleports", "#TF_ClassRecord_Alt_MostTeleports" },
	{ TFSTAT_HEADSHOTS,				MAKESTATFLAG(TF_CLASS_SNIPER) | MAKESTATFLAG(TF_CLASS_SPY),		"#TF_ClassRecord_MostHeadshots", "#TF_ClassRecord_Alt_MostHeadshots" },
	{ TFSTAT_BACKSTABS,				MAKESTATFLAG(TF_CLASS_SPY),			"#TF_ClassRecord_MostBackstabs", "#TF_ClassRecord_Alt_MostBackstabs" },
};

CTFStatsSummaryPanel *g_pTFStatsSummaryPanel = NULL;

CUtlVector<CTFStatsSummaryPanel *> g_vecStatPanels;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UpdateStatSummaryPanels( CUtlVector<ClassStats_t> &vecClassStats )
{
	for ( int i = 0; i < g_vecStatPanels.Count(); i++ )
	{
		g_vecStatPanels[i]->SetStats( vecClassStats );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the global stats summary panel
//-----------------------------------------------------------------------------
CTFStatsSummaryPanel *GStatsSummaryPanel()
{
	if ( NULL == g_pTFStatsSummaryPanel )
	{
		g_pTFStatsSummaryPanel = new CTFStatsSummaryPanel();
	}
	return g_pTFStatsSummaryPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Destroys the global stats summary panel
//-----------------------------------------------------------------------------
void DestroyStatsSummaryPanel()
{
	if ( NULL != g_pTFStatsSummaryPanel )
	{
		g_pTFStatsSummaryPanel->MarkForDeletion();
		g_pTFStatsSummaryPanel = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFStatsSummaryPanel::CTFStatsSummaryPanel() 
  : BaseClass( NULL, "TFStatsSummary", vgui::scheme()->LoadSchemeFromFile( "Resource/ClientScheme.res", "ClientScheme" ) )
  ,	m_bShowingLeaderboard( false )
  , m_bLoadingCommunityMap( false )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::Init( void )
{
	m_bControlsLoaded = false;
	m_bInteractive = false;
	m_bEmbedded = false;
	m_xStartLHBar = 0;
	m_xStartRHBar = 0;
	m_iBarHeight = 1;
	m_iBarMaxWidth = 1;

	m_pPlayerData = new vgui::EditablePanel( this, "statdata" );
	m_pInteractiveHeaders = new vgui::EditablePanel( m_pPlayerData, "InteractiveHeaders" );
	m_pNonInteractiveHeaders = new vgui::EditablePanel( m_pPlayerData, "NonInteractiveHeaders" );
	m_pBarChartComboBoxA = new vgui::ComboBox( m_pInteractiveHeaders, "BarChartComboA", 10, false );
	m_pBarChartComboBoxB = new vgui::ComboBox( m_pInteractiveHeaders, "BarChartComboB", 10, false );
	m_pClassComboBox = new vgui::ComboBox( m_pInteractiveHeaders, "ClassCombo", 10, false );	
	m_pTipImage = new CTFImagePanel( this, "TipImage" );
	m_pTipText = new vgui::Label( this, "TipText", "" );
	m_pMapInfoPanel = NULL;
	m_pMainBackground = NULL;
	m_pLeaderboardTitle = NULL;
	m_pContributedPanel = NULL;

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
	m_bShowBackButton = false;
#else
	m_pNextTipButton = new vgui::Button( this, "NextTipButton", "" );	
	m_pResetStatsButton = new vgui::Button( this, "ResetStatsButton", "" );
	m_pCloseButton = new vgui::Button( this, "CloseButton", "" );	
#endif

	m_pBarChartComboBoxA->AddActionSignalTarget( this );
	m_pBarChartComboBoxB->AddActionSignalTarget( this );
	m_pClassComboBox->AddActionSignalTarget( this );

	ListenForGameEvent( "server_spawn" );

	Reset();

	g_vecStatPanels.AddToTail( this );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFStatsSummaryPanel::CTFStatsSummaryPanel( vgui::Panel *parent ) : BaseClass( parent, "TFStatsSummary", 
	vgui::scheme()->LoadSchemeFromFile( "Resource/ClientScheme.res", "ClientScheme" ) )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStatsSummaryPanel::~CTFStatsSummaryPanel()
{
	g_vecStatPanels.FindAndRemove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Shows this dialog as a modal dialog
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::ShowModal()
{
#ifdef _X360
	m_bInteractive = false;
	m_bShowBackButton = true;
#else
	// we are in interactive mode, enable controls
	m_bInteractive = true;
#endif

	SetParent( enginevgui->GetPanel( PANEL_GAMEUIDLL ) );
	UpdateDialog();
	SetVisible( true );
	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::SetupForEmbedded( void )
{
	m_bInteractive = true;
	m_bEmbedded = true;

	UpdateDialog();

	InvalidateLayout( true, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::PerformLayout()
{
	BaseClass::PerformLayout();

#ifndef _X360
	if ( m_pTipImage && m_pTipText )
	{
		int iX,iY;
		m_pTipImage->GetPos(iX,iY);
		int iTX, iTY;
		m_pTipText->GetPos(iTX, iTY);
		m_pTipText->SetPos( iX + m_pTipImage->GetWide() + XRES(8), iTY );
	}

	if ( m_pNextTipButton )
	{
		m_pNextTipButton->SizeToContents();
	}

	if ( m_pResetStatsButton )
	{
		m_pResetStatsButton->SizeToContents();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnThink()
{
	BaseClass::OnThink();

	if ( m_bShowingLeaderboard )
	{
		UpdateLeaderboard();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnCommand( const char *command )
{
	if ( 0 == Q_stricmp( command, "vguicancel" ) )
	{
		m_bInteractive = false;
		UpdateDialog();
		SetVisible( false );
		SetParent( (VPANEL) NULL );

#ifdef _X360
		SetDefaultSelections();
		m_bShowBackButton = true;
#endif
	}
#ifndef _X360
	else if ( 0 == Q_stricmp( command, "resetstatsbutton" ) )
	{
		QueryBox *qb = new QueryBox( "#GameUI_Confirm", "#TF_ConfirmResetStats" );
		if (qb != NULL)
		{
			qb->SetOKCommand(new KeyValues("DoResetStats") );
			qb->AddActionSignalTarget(this);
			qb->MoveToFront();
			qb->DoModal();
		}
	}
#endif
	else if ( 0 == Q_stricmp( command, "nexttip" ) )
	{
		UpdateTip();
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: Resets the dialog
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::Reset()
{
	m_aClassStats.RemoveAll();

	SetDefaultSelections();
}

//-----------------------------------------------------------------------------
// Purpose: Sets all user-controllable dialog settings to default values
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::SetDefaultSelections()
{
	m_iSelectedClass = TF_CLASS_UNDEFINED;
	m_statBarGraph[0] = TFSTAT_POINTSSCORED;
	m_displayBarGraph[0]= SHOW_MAX;
	m_statBarGraph[1] = TFSTAT_PLAYTIME;
	m_displayBarGraph[1] = SHOW_TOTAL;

	m_pBarChartComboBoxA->ActivateItemByRow( 0 );
	m_pBarChartComboBoxB->ActivateItemByRow( 10 );
}


//-----------------------------------------------------------------------------
// Purpose: Set the background image based on the current mode
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::UpdateMainBackground( void )
{
	if ( IsPC() )
	{
		m_pMainBackground = dynamic_cast<ImagePanel *>( FindChildByName( "MainBackground" ) );
		if ( m_pMainBackground )
		{
			const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GTFGCClientSystem()->GetLiveMatchGroup() );

			// determine if we're in widescreen or not and select the appropriate image
			int screenWide, screenTall;
			surface()->GetScreenSize( screenWide, screenTall );
			float aspectRatio = (float)screenWide/(float)screenTall;
			bool bIsWidescreen = aspectRatio >= 1.5999f;

			if ( g_bIsReplayRewinding )
			{
				m_pMainBackground->SetImage( bIsWidescreen ? "../console/rewind_background_widescreen" : "../console/rewind_background" );
			}
			else if ( engine->IsLoadingDemo() || engine->IsPlayingDemo() || engine->IsSkippingPlayback() )
			{
				m_pMainBackground->SetImage( bIsWidescreen ? "../console/replay_loading_widescreen" : "../console/replay_loading" );
			}
			else if ( pMatchDesc && pMatchDesc->GetMapLoadBackgroundOverride( bIsWidescreen ) ) // Use match override if we have one
			{
				m_pMainBackground->SetImage( pMatchDesc->GetMapLoadBackgroundOverride( bIsWidescreen ) );
			}
			else
			{
				m_pMainBackground->SetImage( bIsWidescreen ? "../console/background01_widescreen" : "../console/background01" );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetProportional( true );

	if ( m_bEmbedded )
	{
		LoadControlSettings( "Resource/UI/StatSummary_Embedded.res" );
	}
	else
	{
		LoadControlSettings( "Resource/UI/StatSummary.res" );
	}
	m_bControlsLoaded = true;

	// set the background image
	UpdateMainBackground();

	m_pMapInfoPanel = dynamic_cast< EditablePanel *>( FindChildByName( "MapInfo" ) );
	m_vecLeaderboardEntries.RemoveAll();
	if ( m_pMapInfoPanel )
	{
		for ( int i = 0; i < 10; ++ i )
		{
			vgui::EditablePanel *pEntryUI = new vgui::EditablePanel( m_pMapInfoPanel, "LeaderboardEntry" );
			pEntryUI->ApplySchemeSettings( pScheme );
			pEntryUI->LoadControlSettings( "Resource/UI/LeaderboardEntry.res" );
			m_vecLeaderboardEntries.AddToTail( pEntryUI );
		}
	}

	// get the dimensions and position of a left-hand bar and a right-hand bar so we can do bar sizing later
	Panel *pLHBar = m_pPlayerData->FindChildByName( "ClassBar1A" );
	Panel *pRHBar = m_pPlayerData->FindChildByName( "ClassBar1B" );
	if ( pLHBar && pRHBar )
	{
		int y;
		pLHBar->GetBounds( m_xStartLHBar, y, m_iBarMaxWidth, m_iBarHeight );
		pRHBar->GetBounds( m_xStartRHBar, y, m_iBarMaxWidth, m_iBarHeight );
	}

	// fill the combo box selections appropriately
	InitBarChartComboBox( m_pBarChartComboBoxA );
	InitBarChartComboBox( m_pBarChartComboBoxB );

	// fill the class names in the class combo box
	HFont hFont = scheme()->GetIScheme( GetScheme() )->GetFont( "ScoreboardSmall", true );
	m_pClassComboBox->SetFont( hFont );
	m_pClassComboBox->RemoveAll();
	KeyValues *pKeyValues = new KeyValues( "data" );
	pKeyValues->SetInt( "class", TF_CLASS_UNDEFINED );
	m_pClassComboBox->AddItem( "#StatSummary_Label_AsAnyClass", pKeyValues );
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( iClass == TF_CLASS_CIVILIAN )
			continue;
		pKeyValues = new KeyValues( "data" );
		pKeyValues->SetInt( "class", iClass );
		m_pClassComboBox->AddItem( g_aPlayerClassNames[iClass], pKeyValues );
	}
	m_pClassComboBox->ActivateItemByRow( 0 );

	if ( m_pMapInfoPanel )
	{
		m_pContributedPanel = dynamic_cast< vgui::EditablePanel* >( m_pMapInfoPanel->FindChildByName( "ContributedLabel" ) );
	}

	SetDefaultSelections();
	UpdateDialog();

	if ( !m_bEmbedded )
	{
		SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnKeyCodePressed( KeyCode code )
{
	if ( IsX360() )
	{
		if ( code == KEY_XBUTTON_A || code == STEAMCONTROLLER_A )
		{
			OnCommand(  "nexttip" )	;
		}
		else if ( code == KEY_XBUTTON_B || code == STEAMCONTROLLER_B )
		{
			OnCommand( "vguicancel" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets stats to use
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::SetStats( CUtlVector<ClassStats_t> &vecClassStats ) 
{
	m_aClassStats = vecClassStats; 
	if ( m_bControlsLoaded )
	{
		UpdateDialog();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::ClearMapLabel()
{
	SetDialogVariable( "maplabel", "" );
	SetDialogVariable( "maptype", "" );

	vgui::Label *pLabel = dynamic_cast<Label *>( FindChildByName( "OnYourWayLabel" ) );
	if ( pLabel && pLabel->IsVisible() )
	{
		pLabel->SetVisible( false );
	}

	pLabel = dynamic_cast<Label *>( FindChildByName( "MapType" ) );
	if ( pLabel && pLabel->IsVisible() )
	{
		pLabel->SetVisible( false );
	}

	if ( m_pContributedPanel )
	{
		m_pContributedPanel->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::ShowMapInfo( bool bShowMapInfo, bool bIsMVM /*= false*/, bool bBackgroundOverride /*= false*/ )
{
	if ( m_pMainBackground )
	{
		m_pMainBackground->SetVisible( !bShowMapInfo );
	}
	m_pPlayerData->SetVisible( bIsMVM || !bShowMapInfo );
	m_pNextTipButton->SetVisible( m_bInteractive && !bShowMapInfo );
	m_pResetStatsButton->SetVisible( m_bInteractive && !bShowMapInfo );

	if ( m_pMapInfoPanel )
	{
		m_pMapInfoPanel->SetVisible( bShowMapInfo );
		vgui::Panel* pInfoBG = m_pMapInfoPanel->FindChildByName( "InfoBG" );
		if ( pInfoBG )
		{
			pInfoBG->SetVisible( bShowMapInfo && !bIsMVM && !bBackgroundOverride );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnMapLoad( const char *pMapName )
{
	if ( g_bIsReplayRewinding || engine->IsLoadingDemo() || engine->IsPlayingDemo() || engine->IsSkippingPlayback() )
		return;

	bool bWidescreenBackground = false;

	bool bIsMVM = ( pMapName && !Q_strncmp( pMapName, "mvm_", 4 ) );
	bool bIsMVMBackground = false;
	const char *pszBackgroundOverride = NULL;
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GTFGCClientSystem()->GetLiveMatchGroup() );
	if ( pMatchDesc )
	{
		int screenWide, screenTall;
		surface()->GetScreenSize( screenWide, screenTall );
		float aspectRatio = (float)screenWide/(float)screenTall;
		bool bWideScreen = aspectRatio >= 1.5999f;

		// Check if there's a widescreen override
		if( bWideScreen )
		{
			pszBackgroundOverride = pMatchDesc->GetMapLoadBackgroundOverride( true );
			if ( pszBackgroundOverride )
			{
				// Success!  We're done
				bWidescreenBackground = true;
			}
		}
		
		if ( !bWideScreen && !pszBackgroundOverride )
		{
			pszBackgroundOverride = pMatchDesc->GetMapLoadBackgroundOverride( false );
		}
		
	}
	
	if ( bIsMVM && !pszBackgroundOverride )
	{
		// this will preserve the current behavior for non-matchmaking servers
		pszBackgroundOverride = "mvm_background_map";
		bIsMVMBackground = true;
	}

	bool bIsCommunityMap = false;
	const char *pAuthors = NULL;
	
	const MapDef_t *pMapInfo = GetItemSchema()->GetMasterMapDefByName( pMapName );
	if ( pMapInfo )
	{
		bIsCommunityMap = pMapInfo->IsCommunityMap();
		pAuthors = pMapInfo->pszAuthorsLocKey;
	}
	
	ShowMapInfo( true, bIsMVM, ( pszBackgroundOverride != NULL ) );

	m_xStartLeaderboard = 0;
	m_yStartLeaderboard = 0;

	// If we're loading a background map, don't display anything
	// HACK: Client doesn't get gpGlobals->eLoadType, so just do string compare for now.
	if ( Q_stristr( pMapName, "background") )
	{
		ClearMapLabel();
	}
	else
	{
		// set the map name in the UI
		wchar_t wzMapName[255]=L"";
		g_pVGuiLocalize->ConvertANSIToUnicode( GetMapDisplayName( pMapName ), wzMapName, sizeof( wzMapName ) );

		SetDialogVariable( "maplabel", wzMapName );
		SetDialogVariable( "maptype", g_pVGuiLocalize->Find( GetMapType( pMapName ) ) );

		vgui::Label *pLabel = dynamic_cast<Label *>( FindChildByName( "OnYourWayLabel" ) );
		if ( pLabel && !pLabel->IsVisible() )
		{
			pLabel->SetVisible( true );
		}

		pLabel = dynamic_cast<Label *>( FindChildByName( "MapType" ) );
		if ( pLabel && !pLabel->IsVisible() )
		{
			pLabel->SetVisible( true );
		}

		ImagePanel *pMapImage = m_pMapInfoPanel ? dynamic_cast< ImagePanel *>( m_pMapInfoPanel->FindChildByName( "MapImage" ) ) : NULL;
		if ( pMapImage )
		{
			// load the map image (if it exists for the current map)
			char szMapImage[ MAX_PATH ];
			Q_snprintf( szMapImage, sizeof( szMapImage ), "VGUI/maps/menu_photos_%s", pMapName );
			Q_strlower( szMapImage );

			IMaterial *pMapMaterial = materials->FindMaterial( szMapImage, TEXTURE_GROUP_VGUI, false );
			if ( pMapMaterial && !IsErrorMaterial( pMapMaterial ) && ( !pszBackgroundOverride || bIsMVMBackground ) )
			{
				// take off the vgui/ at the beginning when we set the image
				Q_snprintf( szMapImage, sizeof( szMapImage ), "maps/menu_photos_%s", pMapName );
				Q_strlower( szMapImage );
				pMapImage->SetImage( szMapImage );
				pMapImage->SetVisible( true );
			}
			else
			{
				pMapImage->SetVisible( false );
			}
		}

		ImagePanel *pBackgroundImage = m_pMapInfoPanel ? dynamic_cast< ImagePanel *>( m_pMapInfoPanel->FindChildByName( "Background" ) ) : NULL;
		if ( pBackgroundImage )
		{
			const char* pszBackgroundImage = pszBackgroundOverride ? pszBackgroundOverride : "stamp_background_map";

			pBackgroundImage->SetImage( pszBackgroundImage );

			// Resize to accomodate the background image coming in
			if ( bWidescreenBackground )
			{
				pBackgroundImage->SetWide( GetWide() );
			}
			else 
			{
				pBackgroundImage->SetWide( GetTall() * ( 4.f / 3.f ) );
			}

		}

		if ( bIsMVM )
		{
			UpdateClassDetails( true );
			m_pMapInfoPanel->SetDialogVariable( "map_leaderboard_title", "" );
			m_pMapInfoPanel->SetDialogVariable( "title", "" );
			m_pMapInfoPanel->SetDialogVariable( "authors", "" );

			FOR_EACH_VEC( m_vecLeaderboardEntries, i )
			{
				EditablePanel *pContainer = dynamic_cast< EditablePanel* >( m_vecLeaderboardEntries[i] );
				if ( pContainer )
				{
					pContainer->SetVisible( false );
				}
			}
		}
		else
		{
			m_pLeaderboardTitle = NULL;
			// add authors
			if ( m_pMapInfoPanel )
			{
				if ( bIsCommunityMap )
				{
					m_pMapInfoPanel->SetDialogVariable( "title", g_pVGuiLocalize->Find( "#TF_MapAuthors_Community_Title" ) );
					m_pMapInfoPanel->SetDialogVariable( "map_leaderboard_title", "" );
					m_pMapInfoPanel->SetDialogVariable( "authors", g_pVGuiLocalize->Find( pAuthors ) ); 
					m_pLeaderboardTitle = m_pMapInfoPanel->FindChildByName( "MapLeaderboardTitle" );
				}
				else
				{
					m_pMapInfoPanel->SetDialogVariable( "title", g_pVGuiLocalize->Find( "#TF_DuelLeaderboard_Title" ) );
					m_pMapInfoPanel->SetDialogVariable( "map_leaderboard_title", "" );
					m_pMapInfoPanel->SetDialogVariable( "authors", "" );
					m_pLeaderboardTitle = m_pMapInfoPanel->FindChildByName( "Title" );
				}
			}
			if ( m_pLeaderboardTitle )
			{
				m_pLeaderboardTitle->GetPos( m_xStartLeaderboard, m_yStartLeaderboard );
				m_yStartLeaderboard += m_pLeaderboardTitle->GetTall();
			}

			//  request leaderboard data
			m_bShowingLeaderboard = true;
			if ( bIsCommunityMap )
			{
				MapInfo_RefreshLeaderboard( pMapName );
			}
			else
			{
				Leaderboards_Refresh();
			}
			m_bLoadingCommunityMap = bIsCommunityMap;

			if ( m_pContributedPanel && steamapicontext && steamapicontext->SteamUser() && steamapicontext->SteamFriends() )
			{
				int iDonationAmount = MapInfo_GetDonationAmount( steamapicontext->SteamUser()->GetSteamID().GetAccountID(), pMapName );
				m_pContributedPanel->SetVisible( iDonationAmount != 0 );
				if ( iDonationAmount != 0 )
				{
					m_pContributedPanel->SetDialogVariable( "playername", steamapicontext->SteamFriends()->GetPersonaName() );
				}
			}

			UpdateLeaderboard();
		}
	}
}

void CTFStatsSummaryPanel::UpdateLeaderboard()
{
	if ( m_pMapInfoPanel == NULL || steamapicontext == NULL || steamapicontext->SteamUserStats() == NULL || steamapicontext->SteamUser() == NULL )
		return;

	const int kMaxVisible_Supporters = 5;
	const int kIdeallyNumVisible_Supporters = 3;
	const int kMaxVisible_DuelWins = 10;
	const int kIdeallyNumVisible_DuelWins = 5;

	//  retrieve scores
	CUtlVector< LeaderboardEntry_t* > scores;
	bool bVisible = true;
	int iNumLeaderboardEntries = 0;
	if ( m_bLoadingCommunityMap )
	{
		bVisible = MapInfo_GetLeaderboardInfo( engine->GetLevelName(), scores, iNumLeaderboardEntries, kIdeallyNumVisible_Supporters );
		wchar_t wzNumEntriesString[256];
		_snwprintf( wzNumEntriesString, ARRAYSIZE( wzNumEntriesString ), L"%i", iNumLeaderboardEntries );
		wchar_t wzTitle[256];
		g_pVGuiLocalize->ConstructString_safe( wzTitle, g_pVGuiLocalize->Find( "#TF_MapDonators_Title" ), 1, wzNumEntriesString );
		m_pMapInfoPanel->SetDialogVariable( "map_leaderboard_title", wzTitle );
	}
	else
	{
		bVisible = Leaderboards_GetDuelWins( scores, false );
		if ( bVisible && scores.Count() < kIdeallyNumVisible_DuelWins )
		{
			bVisible = Leaderboards_GetDuelWins( scores, true ) && scores.Count() > 0;
		}
		// show old stats
		m_pPlayerData->SetVisible( bVisible == false );
		if ( m_pMapInfoPanel )
		{
			vgui::Panel* pInfoBG = m_pMapInfoPanel->FindChildByName( "InfoBG" );
			if ( pInfoBG )
			{
				pInfoBG->SetVisible( bVisible );
			}
		}
	}

	const int kMaxVisible = m_bLoadingCommunityMap ? kMaxVisible_Supporters : kMaxVisible_DuelWins;

	// try to show local player in relation to the people in the list
	if ( bVisible && scores.Count() > 0 && steamapicontext && steamapicontext->SteamUser() && steamapicontext->SteamUserStats() )
	{
		int iLocalPlayerIdx = -1;
		CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
		FOR_EACH_VEC( scores, i )
		{
			const LeaderboardEntry_t *leaderboardEntry = scores[i];
			if ( leaderboardEntry->m_steamIDUser == localSteamID )
			{
				iLocalPlayerIdx = i;
				break;
			}
		}
		// local player is in the list, but is outside the visible range
		// so we want to move them to the last spot
		// and move the closest person above them as well
		if ( iLocalPlayerIdx >= kMaxVisible )
		{
			LeaderboardEntry_t *entryLocalPlayer = scores[iLocalPlayerIdx];
			LeaderboardEntry_t *closestPlayer = scores[iLocalPlayerIdx - 1];
			scores[kMaxVisible - 1] = entryLocalPlayer;
			scores[kMaxVisible - 2] = closestPlayer;
		}
	}

	// set avatars and names
	int x = m_xStartLeaderboard;
	int y = m_yStartLeaderboard;
	FOR_EACH_VEC( m_vecLeaderboardEntries, i )
	{
		EditablePanel *pContainer = dynamic_cast< EditablePanel* >( m_vecLeaderboardEntries[i] );
		if ( pContainer )
		{
			bool bIsEntryVisible = bVisible && i < scores.Count() && i < kMaxVisible;
			pContainer->SetVisible( bIsEntryVisible );
			pContainer->SetPos( x, y );
			y += pContainer->GetTall();
			if ( bIsEntryVisible )
			{
				const LeaderboardEntry_t *leaderboardEntry = scores[i];
				const CSteamID &steamID = leaderboardEntry->m_steamIDUser;
				pContainer->SetDialogVariable( "username", CFmtStr( "%d. %s - %d", leaderboardEntry->m_nGlobalRank, InventoryManager()->PersonaName_Get( steamID.GetAccountID() ), leaderboardEntry->m_nScore ) );
				CAvatarImagePanel *pAvatar = dynamic_cast< CAvatarImagePanel* >( pContainer->FindChildByName( "AvatarImage" ) );
				if ( pAvatar )
				{
					pAvatar->SetShouldDrawFriendIcon( false );
					pAvatar->SetPlayer( steamID, k_EAvatarSize32x32 );
				}
			}								
		}			
	}

	if ( m_pLeaderboardTitle )
	{
		bool bShowTitle = bVisible && scores.Count() > 0;
		if ( m_pLeaderboardTitle->IsVisible() != bShowTitle )
		{
			m_pLeaderboardTitle->SetVisible( bShowTitle );
		}
	}

	m_bShowingLeaderboard = bVisible;
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::UpdateDialog()
{
	UpdateMainBackground();

	if ( g_bIsReplayRewinding || engine->IsLoadingDemo() || engine->IsPlayingDemo() || engine->IsSkippingPlayback() )
	{
		// hide all of the various panels for the other loadscreen modes
		if ( IsPC() )
		{
			ClearMapLabel();

			m_pPlayerData->SetVisible( false );
			m_pNextTipButton->SetVisible( false );
			m_pResetStatsButton->SetVisible( false );
			m_pInteractiveHeaders->SetVisible( false );
			m_pNonInteractiveHeaders->SetVisible( false );
			m_pTipText->SetVisible( false );
			m_pTipImage->SetVisible( false );

			if ( m_pMapInfoPanel )
			{
				m_pMapInfoPanel->SetVisible( false );

				vgui::Panel* pInfoBG = m_pMapInfoPanel->FindChildByName( "InfoBG" );
				if ( pInfoBG )
				{
					pInfoBG->SetVisible( false );
				}
			}
		}

		return;
	}

	RandomSeed( Plat_MSTime() );

	m_iTotalSpawns = 0;

	// if we don't have stats for any class, add empty stat entries for them 
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( iClass == TF_CLASS_CIVILIAN )
			continue; // Ignore the civilian.

		int j;
		for ( j = 0; j < m_aClassStats.Count(); j++ )
		{
			if ( m_aClassStats[j].iPlayerClass == iClass )
			{
				m_iTotalSpawns += m_aClassStats[j].iNumberOfRounds;
				break;
			}
		}
		if ( j == m_aClassStats.Count() )
		{
			ClassStats_t stats;
			stats.iPlayerClass = iClass;
			m_aClassStats.AddToTail( stats );
		}
	}

	ClearMapLabel();

#ifdef _X360
	if ( m_pFooter )
	{
		m_pFooter->ShowButtonLabel( "nexttip", m_bShowBackButton );
		m_pFooter->ShowButtonLabel( "back", m_bShowBackButton );
	}
#endif

	// fill out bar charts
	UpdateBarCharts();
	// fill out class details
	UpdateClassDetails();
	// update the tip
	UpdateTip();
	// show or hide controls depending on if we're interactive or not
	UpdateControls();		
}

//-----------------------------------------------------------------------------
// Purpose: Updates bar charts
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::UpdateBarCharts()
{
	// sort the class stats by the selected stat for right-hand bar chart
	m_aClassStats.Sort( &CTFStatsSummaryPanel::CompareClassStats );

	// loop for left & right hand charts
	for ( int iChart = 0; iChart < 2; iChart++ )
	{
		float flMax = 0;
		for ( int i = 0; i < m_aClassStats.Count(); i++ )
		{
			// get max value of stat being charted so we know how to scale the graph
			float flVal = GetDisplayValue( m_aClassStats[i], m_statBarGraph[iChart], m_displayBarGraph[iChart] );
			flMax = MAX( flVal, flMax );
		}

		// draw the bar chart value for each player class
		// TODO: Fix up after the civilian becomes playable.
		int iChartBar = 0;
		for ( int i = 0; i < m_aClassStats.Count(); i++ )
		{	
			int iClass = m_aClassStats[i].iPlayerClass;
			if ( iClass == TF_CLASS_CIVILIAN )
			{
				continue;
			}
			if ( 0 == iChart )
			{
				// if this is the first chart, set the class label for each class
				m_pPlayerData->SetDialogVariable( CFmtStr( "class%d", iChartBar+1 ), g_pVGuiLocalize->Find( g_aPlayerClassNames[iClass] ) );
			}
			// draw the bar for this class
			DisplayBarValue( iChart, iChartBar++, m_aClassStats[i], m_statBarGraph[iChart], m_displayBarGraph[iChart], flMax );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates class details
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::UpdateClassDetails( bool bIsMVM )
{
	vgui::Label *pTitle = assert_cast< vgui::Label* >( FindChildByName( "RecordsLabel1", true ) );
	if ( pTitle )
	{
		pTitle->SetText( bIsMVM ? "#StatSummary_Label_BestMVMMoments" : "#StatSummary_Label_BestMoments" );
	}

	const wchar_t *wzWithClassFmt = g_pVGuiLocalize->Find( "#StatSummary_ScoreAsClassFmt" );
	const wchar_t *wzWithoutClassFmt = L"%s1";

	ClassDetails_t *pStatDetails = ( bIsMVM ? g_PerClassMVMStatDetails : g_PerClassStatDetails );
	int nArraySize = ( bIsMVM ? ARRAYSIZE( g_PerClassMVMStatDetails ) : ARRAYSIZE( g_PerClassStatDetails ) );

	// display the record for each stat
	int iRow = 0;
	for ( int i = 0; i < nArraySize; i++ )
	{
		TFStatType_t statType = pStatDetails[i].statType;

		int iClass = TF_CLASS_UNDEFINED;
		int iMaxVal = 0;

		// if there is a selected class, and if this stat should not be shown for this class, skip this stat
		if ( m_iSelectedClass != TF_CLASS_UNDEFINED && ( 0 == ( pStatDetails[i].iFlagsClass & MAKESTATFLAG( m_iSelectedClass ) ) ) )
			continue;

		if ( m_iSelectedClass == TF_CLASS_UNDEFINED )
		{
			// if showing best from any class, look through all player classes to determine the max value of this stat
			for ( int j = 0; j  < m_aClassStats.Count(); j++ )
			{
				RoundStats_t *pRoundStats = &( bIsMVM ? m_aClassStats[j].maxMVM : m_aClassStats[j].max );
				if ( pRoundStats->m_iStat[statType] > iMaxVal )
				{
					// remember max value and class that has max value
					iMaxVal = pRoundStats->m_iStat[statType];
					iClass = m_aClassStats[j].iPlayerClass;
				}
			}
		}
		else
		{
			// show best from selected class
			iClass = m_iSelectedClass;
			for ( int j = 0; j  < m_aClassStats.Count(); j++ )
			{
				if ( m_aClassStats[j].iPlayerClass == iClass )
				{
					RoundStats_t *pRoundStats = &( bIsMVM ? m_aClassStats[j].maxMVM : m_aClassStats[j].max );
					iMaxVal = pRoundStats->m_iStat[statType];
					break;
				}
			}
		}

		wchar_t wzStatNum[32];
		wchar_t wzStatVal[128];
		if ( TFSTAT_PLAYTIME == statType )
		{
			// playtime gets displayed as a time string
			g_pVGuiLocalize->ConvertANSIToUnicode( FormatSeconds( iMaxVal ), wzStatNum, sizeof( wzStatNum ) );
		}
		else
		{
			// all other stats are just shown as a #
			swprintf_s( wzStatNum, ARRAYSIZE( wzStatNum ), L"%d", iMaxVal );
		}

		if ( TF_CLASS_UNDEFINED == m_iSelectedClass && iMaxVal > 0 )
		{
			// if we are doing a cross-class view (no single selected class) and the max value is non-zero, show "# (as <class>)"
			wchar_t *wzLocalizedClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[iClass] );
			g_pVGuiLocalize->ConstructString_safe( wzStatVal, wzWithClassFmt, 2, wzStatNum, wzLocalizedClassName );
		}
		else
		{
			// just show the value
			g_pVGuiLocalize->ConstructString_safe( wzStatVal, wzWithoutClassFmt, 1, wzStatNum );
		}				

		// set the label
		m_pPlayerData->SetDialogVariable( CFmtStr( "classrecord%dlabel", iRow+1 ), g_pVGuiLocalize->Find( pStatDetails[i].szResourceName ) );
		// set the value 
		m_pPlayerData->SetDialogVariable( CFmtStr( "classrecord%dvalue", iRow+1 ), wzStatVal );

		iRow++;
	}

	// if there are any leftover rows for the selected class, fill out the remaining rows with blank labels and values
	for ( ; iRow < 15; iRow ++ )
	{
		m_pPlayerData->SetDialogVariable( CFmtStr( "classrecord%dlabel", iRow+1 ), "" );
		m_pPlayerData->SetDialogVariable( CFmtStr( "classrecord%dvalue", iRow+1 ), "" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the tip
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::UpdateTip()
{
	int iTipClass = TF_CLASS_UNDEFINED;

	SetDialogVariable( "tiptext", g_TFTips.GetRandomTip( iTipClass ) );

	if ( m_pTipImage )
	{
		if ( iTipClass > TF_CLASS_UNDEFINED && iTipClass <= TF_CLASS_ENGINEER )
		{
			m_pTipImage->SetVisible( true );
			m_pTipImage->SetImage( g_pszTipsClassImages[iTipClass] );
		}
		else
		{
			m_pTipImage->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shows or hides controls
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::UpdateControls()
{
	// show or hide controls depending on what mode we're in
#ifndef _X360
	bool bShowPlayerData = ( m_bInteractive || m_iTotalSpawns > 0 );
#else
	bool bShowPlayerData = ( m_bInteractive || m_bShowBackButton || m_iTotalSpawns > 0 );
#endif
	m_pPlayerData->SetVisible( bShowPlayerData );
	m_pInteractiveHeaders->SetVisible( m_bInteractive );
	m_pNonInteractiveHeaders->SetVisible( !m_bInteractive );
	m_pTipText->SetVisible( bShowPlayerData );
	m_pTipImage->SetVisible( bShowPlayerData );

	if ( !IsX360() )
	{
		if ( !m_bInteractive )
		{
			char szTemp[128];

			// update our non-interactive headers to match the current combo box selections
			Label *pLabel = dynamic_cast<Label *>( m_pNonInteractiveHeaders->FindChildByName( "BarChartLabelA" ) );
			if ( pLabel && m_pBarChartComboBoxA )
			{
				m_pBarChartComboBoxA->GetItemText( m_pBarChartComboBoxA->GetActiveItem(), szTemp, sizeof( szTemp ) );
				pLabel->SetText( szTemp );
			}

			pLabel = dynamic_cast<Label *>( m_pNonInteractiveHeaders->FindChildByName( "BarChartLabelB" ) );
			if ( pLabel && m_pBarChartComboBoxB )
			{
				m_pBarChartComboBoxB->GetItemText( m_pBarChartComboBoxB->GetActiveItem(), szTemp, sizeof( szTemp ) );
				pLabel->SetText( szTemp );
			}

			pLabel = dynamic_cast<Label *>( m_pNonInteractiveHeaders->FindChildByName( "OverallRecordLabel" ) );
			if ( pLabel && m_pClassComboBox )
			{
				m_pClassComboBox->GetItemText( m_pClassComboBox->GetActiveItem(), szTemp, sizeof( szTemp ) );
				pLabel->SetText( szTemp );
			}
		}
	}

#ifndef _X360
	m_pNextTipButton->SetVisible( m_bInteractive );
	m_pResetStatsButton->SetVisible( m_bInteractive );
	m_pCloseButton->SetVisible( m_bInteractive && !m_bEmbedded );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Initializes a bar chart combo box
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::InitBarChartComboBox( ComboBox *pComboBox )
{
	struct BarChartComboInit_t
	{
		TFStatType_t statType;
		StatDisplay_t statDisplay;
		const char *szName;
	};

	BarChartComboInit_t initData[] =
	{
		{ TFSTAT_POINTSSCORED, SHOW_MAX, "#StatSummary_StatTitle_MostPoints" },
		{ TFSTAT_POINTSSCORED, SHOW_AVG, "#StatSummary_StatTitle_AvgPoints" },
		{ TFSTAT_KILLS, SHOW_MAX, "#StatSummary_StatTitle_MostKills" },
		{ TFSTAT_KILLS, SHOW_AVG, "#StatSummary_StatTitle_AvgKills" },
		{ TFSTAT_CAPTURES, SHOW_MAX, "#StatSummary_StatTitle_MostCaptures" },
		{ TFSTAT_CAPTURES, SHOW_AVG, "#StatSummary_StatTitle_AvgCaptures" },
		{ TFSTAT_KILLASSISTS, SHOW_MAX, "#StatSummary_StatTitle_MostAssists" },
		{ TFSTAT_KILLASSISTS, SHOW_AVG, "#StatSummary_StatTitle_AvgAssists" },
		{ TFSTAT_DAMAGE, SHOW_MAX, "#StatSummary_StatTitle_MostDamage" },
		{ TFSTAT_DAMAGE, SHOW_AVG, "#StatSummary_StatTitle_AvgDamage" },
		{ TFSTAT_PLAYTIME, SHOW_TOTAL, "#StatSummary_StatTitle_TotalPlaytime" },
		{ TFSTAT_PLAYTIME, SHOW_MAX, "#StatSummary_StatTitle_LongestLife" },
	};

	// set the font
	HFont hFont = scheme()->GetIScheme( GetScheme() )->GetFont( "ScoreboardVerySmall", true );
	pComboBox->SetFont( hFont );
	pComboBox->RemoveAll();
	// add all the options to the combo box
	for ( int i=0; i < ARRAYSIZE( initData ); i++ )
	{
		KeyValues *pKeyValues = new KeyValues( "data" );
		pKeyValues->SetInt( "stattype", initData[i].statType );
		pKeyValues->SetInt( "statdisplay", initData[i].statDisplay );
		pComboBox->AddItem(  g_pVGuiLocalize->Find( initData[i].szName ), pKeyValues );
	}
	pComboBox->SetNumberOfEditLines( ARRAYSIZE( initData ) );
}

//-----------------------------------------------------------------------------
// Purpose: Helper function that sets the specified dialog variable to
//		"<value> (as <localized class name>)"
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::SetValueAsClass( const char *pDialogVariable, int iValue, int iPlayerClass )
{
	if ( iValue > 0 )
	{
		wchar_t *wzScoreAsClassFmt = g_pVGuiLocalize->Find( "#StatSummary_ScoreAsClassFmt" );
		wchar_t *wzLocalizedClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[iPlayerClass] );
		wchar_t wzVal[16];
		wchar_t wzMsg[128];
		swprintf( wzVal, ARRAYSIZE( wzVal ), L"%d", iValue );
		g_pVGuiLocalize->ConstructString_safe( wzMsg, wzScoreAsClassFmt, 2, wzVal, wzLocalizedClassName );
		m_pPlayerData->SetDialogVariable( pDialogVariable, wzMsg );
	}
	else
	{
		m_pPlayerData->SetDialogVariable( pDialogVariable, "0" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the specified bar chart item to the specified value, in range 0->1
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::DisplayBarValue( int iChart, int iBar, ClassStats_t &stats, TFStatType_t statType, StatDisplay_t statDisplay, float flMaxValue )
{
	const char *szControlSuffix = ( 0 == iChart ? "A" : "B" );
	Panel *pBar = m_pPlayerData->FindChildByName( CFmtStr( "ClassBar%d%s", iBar+1, szControlSuffix  ) );
	Label *pLabel = dynamic_cast<Label*>( m_pPlayerData->FindChildByName( CFmtStr( "classbarlabel%d%s", iBar+1, szControlSuffix ) ) );
	if ( !pBar || !pLabel )
		return;

	// get the stat value
	float flValue = GetDisplayValue( stats, statType, statDisplay );
	// calculate the bar size to draw, in the range of 0.0->1.0
	float flBarRange = SafeCalcFraction( flValue, flMaxValue );
	// calculate the # of pixels of bar width to draw
	int iBarWidth = MAX( (int) ( flBarRange * (float) m_iBarMaxWidth ), 1 );

	// Get the text label to draw for this bar.  For values of 0, draw nothing, to minimize clutter
	const char *szLabel = ( flValue > 0 ? RenderValue( flValue, statType, statDisplay ) : "" );
	// draw the label outside the bar if there's room
	bool bLabelOutsideBar = true;
	const int iLabelSpacing = 4;
	HFont hFont = pLabel->GetFont();
	int iLabelWidth = UTIL_ComputeStringWidth( hFont, szLabel );
	if ( iBarWidth + iLabelWidth + iLabelSpacing > m_iBarMaxWidth )
	{
		// if there's not room outside the bar for the label, draw it inside the bar
		bLabelOutsideBar = false;
	}

	int xBar,yBar,xLabel,yLabel;
	pBar->GetPos( xBar,yBar );
	pLabel->GetPos( xLabel,yLabel );

	m_pPlayerData->SetDialogVariable( CFmtStr( "classbarlabel%d%s", iBar+1, szControlSuffix ), szLabel );
	if ( 1 == iChart )	
	{
		// drawing code for RH bar chart
		xBar = m_xStartRHBar;
		pBar->SetBounds( xBar, yBar, iBarWidth, m_iBarHeight );
		if ( bLabelOutsideBar )
		{	
			pLabel->SetPos( xBar + iBarWidth + iLabelSpacing, yLabel );
		}
		else
		{
			pLabel->SetPos( xBar + iBarWidth - ( iLabelWidth + iLabelSpacing ), yLabel );
		}
	}
	else
	{
		// drawing code for LH bar chart
		xBar = m_xStartLHBar + m_iBarMaxWidth - iBarWidth;
		pBar->SetBounds( xBar, yBar, iBarWidth, m_iBarHeight );
		if ( bLabelOutsideBar )
		{	
			pLabel->SetPos( xBar - ( iLabelWidth + iLabelSpacing ), yLabel );
		}
		else
		{
			pLabel->SetPos( xBar + iLabelSpacing, yLabel );
		}
	}		
}

//-----------------------------------------------------------------------------
// Purpose: Calculates a fraction and guards from divide by 0.  (Returns 0 if 
//			denominator is 0.)
//-----------------------------------------------------------------------------
float CTFStatsSummaryPanel::SafeCalcFraction( float flNumerator, float flDemoninator )
{
	if ( 0 == flDemoninator )
		return 0;
	return flNumerator / flDemoninator;
}

//-----------------------------------------------------------------------------
// Purpose: Formats # of seconds into a string
//-----------------------------------------------------------------------------
const char *FormatSeconds( int seconds )
{
	static char string[64];

	int hours = 0;
	int minutes = seconds / 60;

	if ( minutes > 0 )
	{
		seconds -= (minutes * 60);
		hours = minutes / 60;

		if ( hours > 0 )
		{
			minutes -= (hours * 60);
		}
	}

	if ( hours > 0 )
	{
		Q_snprintf( string, sizeof(string), "%2i:%02i:%02i", hours, minutes, seconds );
	}
	else
	{
		Q_snprintf( string, sizeof(string), "%02i:%02i", minutes, seconds );
	}

	return string;
}

//-----------------------------------------------------------------------------
// Purpose: Static sort function that sorts in descending order by play time
//-----------------------------------------------------------------------------
int __cdecl CTFStatsSummaryPanel::CompareClassStats( const ClassStats_t *pStats0, const ClassStats_t *pStats1 )
{
	// sort stats first by right-hand bar graph
	TFStatType_t statTypePrimary = GStatsSummaryPanel()->m_statBarGraph[1];
	StatDisplay_t statDisplayPrimary = GStatsSummaryPanel()->m_displayBarGraph[1];
	// then by left-hand bar graph
	TFStatType_t statTypeSecondary = GStatsSummaryPanel()->m_statBarGraph[0];
	StatDisplay_t statDisplaySecondary = GStatsSummaryPanel()->m_displayBarGraph[0];

	float flValPrimary0 = GetDisplayValue( (ClassStats_t &) *pStats0, statTypePrimary, statDisplayPrimary );
	float flValPrimary1 = GetDisplayValue( (ClassStats_t &) *pStats1, statTypePrimary, statDisplayPrimary );
	float flValSecondary0 = GetDisplayValue( (ClassStats_t &) *pStats0, statTypeSecondary, statDisplaySecondary );
	float flValSecondary1 = GetDisplayValue( (ClassStats_t &) *pStats1, statTypeSecondary, statDisplaySecondary );

	// sort in descending order by primary stat value
	if ( flValPrimary1 > flValPrimary0 )
		return 1;
	if ( flValPrimary1 < flValPrimary0 )
		return -1;

	// if primary stat values are equal, sort in descending order by secondary stat value
	if ( flValSecondary1 > flValSecondary0 )
		return 1;
	if ( flValSecondary1 < flValSecondary0 )
		return -1;

	// if primary & secondary stats are equal, sort by class for consistent sort order
	return ( pStats1->iPlayerClass - pStats0->iPlayerClass );
}

//-----------------------------------------------------------------------------
// Purpose: Called when text changes in combo box
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );
	vgui::ComboBox *pComboBox = dynamic_cast<vgui::ComboBox *>( pPanel );
	
	if ( m_pBarChartComboBoxA == pComboBox || m_pBarChartComboBoxB == pComboBox )
	{
		// a bar chart combo box changed, update the bar charts

		KeyValues *pUserDataA = m_pBarChartComboBoxA->GetActiveItemUserData();
		KeyValues *pUserDataB = m_pBarChartComboBoxB->GetActiveItemUserData();
		if ( !pUserDataA || !pUserDataB )
			return;
		m_statBarGraph[0] = (TFStatType_t) pUserDataA->GetInt( "stattype" );
		m_displayBarGraph[0] = (StatDisplay_t) pUserDataA->GetInt( "statdisplay" );
		m_statBarGraph[1] = (TFStatType_t) pUserDataB->GetInt( "stattype" );
		m_displayBarGraph[1] = (StatDisplay_t) pUserDataB->GetInt( "statdisplay" );
		UpdateBarCharts();
	}	
	else if ( m_pClassComboBox == pComboBox )
	{
		// the class selection combo box changed, update class details

		KeyValues *pUserData = m_pClassComboBox->GetActiveItemUserData();
		if ( !pUserData )
			return;

		m_iSelectedClass = pUserData->GetInt( "class", TF_CLASS_UNDEFINED );

		UpdateClassDetails();
	}

}

//-----------------------------------------------------------------------------
// Purpose: Command target handler called from reset stats confirmation query box
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::DoResetStats()
{
#ifndef _X360
	// reset the stats
	engine->ClientCmd( "resetplayerstats" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Returns the stat value for specified display type
//-----------------------------------------------------------------------------
float CTFStatsSummaryPanel::GetDisplayValue( ClassStats_t &stats, TFStatType_t statType, StatDisplay_t statDisplay )
{
	switch ( statDisplay )
	{
	case SHOW_MAX:
		return stats.max.m_iStat[statType];
		break;
	case SHOW_TOTAL:
		return stats.accumulated.m_iStat[statType];
		break;
	case SHOW_AVG:
		return SafeCalcFraction( stats.accumulated.m_iStat[statType], stats.iNumberOfRounds );
		break;
	default:
		AssertOnce( false );
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the text representation of this value
//-----------------------------------------------------------------------------
const char *CTFStatsSummaryPanel::RenderValue( float flValue, TFStatType_t statType, StatDisplay_t statDisplay )
{
	static char szValue[64];
	if ( TFSTAT_PLAYTIME == statType )
	{
		// the playtime stat is shown in seconds
		return FormatSeconds( (int) flValue );
	}
	else if ( SHOW_AVG == statDisplay )
	{	
		// if it's an average, render as a float w/2 decimal places
		Q_snprintf( szValue, ARRAYSIZE( szValue ), "%.2f", flValue );
	}
	else
	{
		// otherwise, render as an integer
		Q_snprintf( szValue, ARRAYSIZE( szValue ), "%d", (int) flValue );
	}

	return szValue;
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::FireGameEvent( IGameEvent *event )
{
	const char *pEventName = event->GetName();

	// when we are changing levels and 
	if ( 0 == Q_strcmp( pEventName, "server_spawn" ) )
	{
		if ( !m_bInteractive )
		{
			const char *pMapName = event->GetString( "mapname" );
			if ( pMapName )
			{
				OnMapLoad( pMapName );
			}	
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when we are activated during level load
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnActivate()
{
	ClearMapLabel();

	m_bShowingLeaderboard = false;
	m_bLoadingCommunityMap = false;
	ShowMapInfo( false );

#ifdef _X360
	m_bShowBackButton = false;
#endif

	UpdateDialog();
}

//-----------------------------------------------------------------------------
// Purpose: Called when we are deactivated at end of level load
//-----------------------------------------------------------------------------
void CTFStatsSummaryPanel::OnDeactivate()
{
	ClearMapLabel();
}

CON_COMMAND( showstatsdlg, "Shows the player stats dialog" )
{
#ifdef _DEBUG
	GStatsSummaryPanel()->InvalidateLayout( false, true );
#endif
	GStatsSummaryPanel()->ShowModal();
#ifdef _DEBUG
	GStatsSummaryPanel()->OnMapLoad( "cp_coldfront" );
#endif
}
