//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Generic in-game abuse reporting
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "abuse_report_ui.h"
#include "econ/econ_controls.h"
#include "ienginevgui.h"
#include "vgui/ISurface.h"
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/RadioButton.h>
#include "vgui_bitmappanel.h"
#include "vgui_avatarimage.h"
#include "gc_clientsystem.h"
#include "econ/tool_items/tool_items.h"
#include "econ/econ_gcmessages.h"
#include "econ/confirm_dialog.h"
#include "tool_items/custom_texture_cache.h"

vgui::DHANDLE<CAbuseReportDlg> g_AbuseReportDlg;

CAbuseReportDlg::CAbuseReportDlg( vgui::Panel *parent, AbuseIncidentData_t *pIncidentData )
: EditablePanel( parent, "AbuseReportSubmitDialog" )
, m_pSubmitButton( NULL )
, m_pScreenShot( NULL )
, m_pScreenShotAttachCheckButton( NULL )
, m_pOffensiveImage( NULL )
, m_pDescriptionTextEntry( NULL )
, m_pPlayerLabel( NULL )
, m_pPlayerRadio( NULL )
, m_pGameServerRadio( NULL )
, m_pPlayerCombo( NULL )
, m_pAbuseContentLabel( NULL )
, m_pAbuseContentCombo( NULL )
, m_pAbuseTypeLabel( NULL )
, m_pAbuseTypeCombo( NULL )
, m_pScreenShotBitmap( NULL )
, m_pAvatarImage( NULL )
, m_pNoAvatarLabel( NULL )
, m_pCustomTextureImagePanel( NULL )
, m_pNoCustomTexturesLabel( NULL )
, m_pCustomTextureNextButton( NULL )
, m_pCustomTexturePrevButton( NULL )
, m_iUserImageIndex( 0 )
, m_pIncidentData( pIncidentData )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme(scheme);
	SetProportional( true );
	//m_pContainer = new vgui::EditablePanel( this, "Container" );

	Assert( g_AbuseReportDlg.Get() == NULL );
	g_AbuseReportDlg.Set( this );

	engine->ExecuteClientCmd("gameui_preventescape");
}

CAbuseReportDlg::~CAbuseReportDlg()
{
	Assert( g_AbuseReportDlg.Get() == this );
	if ( g_AbuseReportDlg.Get() == this )
	{
		engine->ExecuteClientCmd("gameui_allowescape");
		g_AbuseReportDlg = NULL;
	}
}

void CAbuseReportDlg::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "cancel" ) )
	{
		Close();
		return;
	}
	if ( !Q_stricmp( command, "discard" ) )
	{
		Close();
		g_AbuseReportMgr->DestroyIncidentData();
		return;
	}
	if ( !Q_stricmp( command, "submit" ) )
	{
		OnSubmitReport();
		return;
	}
	if ( !Q_stricmp( command, "nextcustomtexture" ) )
	{
		++m_iUserImageIndex;
		UpdateCustomTextures();
		return;
	}

	if ( !Q_stricmp( command, "prevcustomtexture" ) )
	{
		--m_iUserImageIndex;
		UpdateCustomTextures();
		return;
	}

}

void CAbuseReportDlg::MakeModal()
{
	TFModalStack()->PushModal( this );
	MakePopup();
	MoveToFront();
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	// !KLUDGE! Initially set the dialog to be hidden, so we can take a screenshot!
	SetEnabled( m_pIncidentData != NULL );
	//SetVisible( m_pIncidentData != NULL );
}

void CAbuseReportDlg::Close()
{
	TFModalStack()->PopModal( this );
	SetVisible( false );
	MarkForDeletion();
}

const char *CAbuseReportDlg::GetResFilename()
{
	return "Resource/UI/AbuseReportSubmitDialog.res";
	//return "Resource/UI/QuickplayDialog.res";
}

void CAbuseReportDlg::PerformLayout()
{
	BaseClass::PerformLayout();

	// Center it, keeping requested size
	int x, y, ww, wt, wide, tall;
	vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
	GetSize(wide, tall);
	SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
	
	// @todo setup 
}

class CCustomTextureImagePanel : public vgui::Panel
{
public:
	CCustomTextureImagePanel( Panel *parent, const char *panelName ) : vgui::Panel( parent, panelName )
	{
		m_ugcHandle = 0;
	}

	uint64 m_ugcHandle;

	virtual void Paint()
	{
		if ( m_ugcHandle == 0 )
		{
			return;
		}
		int iTextureHandle = GetCustomTextureGuiHandle( m_ugcHandle );
		if ( iTextureHandle <= 0)
		{
			return;
		}

		vgui::surface()->DrawSetColor(COLOR_WHITE);
		vgui::surface()->DrawSetTexture( iTextureHandle );
		int iWide, iTall;
		GetSize( iWide, iTall );
		
		vgui::Vertex_t verts[4];
		verts[0].Init( Vector2D(     0,	    0 ), Vector2D( 0.0f, 0.0f ) );
		verts[1].Init( Vector2D( iWide,     0 ), Vector2D( 1.0f, 0.0f ) );
		verts[2].Init( Vector2D( iWide, iTall ), Vector2D( 1.0f, 1.0f ) );
		verts[3].Init( Vector2D(     0,	iTall ), Vector2D( 0.0f, 1.0f ) );

		vgui::surface()->DrawTexturedPolygon( 4, verts );	
		vgui::surface()->DrawSetColor(COLOR_WHITE);
	}

};

class CAbuseReportScreenShotPanel : public CBitmapPanel
{
public:
	CAbuseReportScreenShotPanel( CAbuseReportDlg *pDlg, const char *panelName )
	: CBitmapPanel( pDlg, panelName )
	, m_pDlg( pDlg )
	{}

	CAbuseReportDlg *m_pDlg;

	virtual void Paint()
	{
		CBitmapPanel::Paint();

		const AbuseIncidentData_t::PlayerData_t *p = m_pDlg->GetAccusedPlayerPtr();
		if ( p == NULL || !p->m_bRenderBoundsValid )
		{
			return;
		}
		int w, t;
		GetSize( w, t );

		int x0 = int( p->m_screenBoundsMin.x * (float)w );
		int y0 = int( p->m_screenBoundsMin.y * (float)t );
		int x1 = int( p->m_screenBoundsMax.x * (float)w );
		int y1 = int( p->m_screenBoundsMax.y * (float)t );

		vgui::surface()->DrawSetColor( Color(200, 10, 10, 200 ) );
		vgui::surface()->DrawOutlinedRect( x0, y0, x1, y1 );
		vgui::surface()->DrawSetColor( COLOR_WHITE );
	}
};

void CAbuseReportDlg::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	EditablePanel::ApplySchemeSettings( pScheme );

	m_pScreenShotBitmap = new CAbuseReportScreenShotPanel( this, "ScreenShotBitmap" );
	m_pCustomTextureImagePanel = new CCustomTextureImagePanel( this, "CustomTextureImage" );

	LoadControlSettings( GetResFilename() );

	m_pPlayerRadio = dynamic_cast<vgui::RadioButton *>(FindChildByName( "PlayerRadio", true ));
	Assert( m_pPlayerRadio );
	if ( m_pPlayerRadio )
	{
		m_pPlayerRadio->SetVisible( m_pIncidentData->m_bCanReportGameServer );
	}

	m_pGameServerRadio = dynamic_cast<vgui::RadioButton *>(FindChildByName( "GameServerRadio", true ));
	Assert( m_pGameServerRadio );
	if ( m_pGameServerRadio )
	{
		m_pGameServerRadio->SetVisible( m_pIncidentData->m_bCanReportGameServer );
	}

	m_pPlayerLabel = FindChildByName( "PlayerLabel", true );
	Assert( m_pPlayerLabel );

	m_pScreenShotAttachCheckButton = dynamic_cast<vgui::CheckButton *>(FindChildByName( "ScreenShotAttachCheckButton", true ));
	Assert( m_pScreenShotAttachCheckButton );
	if ( m_pScreenShotAttachCheckButton )
	{
		m_pScreenShotAttachCheckButton->SetSelected( true );
	}

	m_pSubmitButton = dynamic_cast<vgui::Button *>(FindChildByName( "SubmitButton", true ));
	Assert( m_pSubmitButton );

	m_pDescriptionTextEntry = dynamic_cast<vgui::TextEntry *>(FindChildByName( "DescriptionTextEntry", true ));
	Assert( m_pDescriptionTextEntry );
	if ( m_pDescriptionTextEntry )
	{
		m_pDescriptionTextEntry->SetMultiline( true );
	}

	m_pAvatarImage = dynamic_cast<CAvatarImagePanel *>(FindChildByName( "AvatarImage", true ));
	Assert( m_pAvatarImage );

	m_pNoAvatarLabel = FindChildByName( "NoAvatarLabel", true );
	Assert( m_pNoAvatarLabel );

	m_pNoCustomTexturesLabel = FindChildByName( "NoCustomTexturesLabel", true );
	Assert( m_pNoCustomTexturesLabel );

	m_pCustomTextureNextButton = dynamic_cast<vgui::Button *>(FindChildByName( "CustomTextureNextButton", true ));
	Assert( m_pCustomTextureNextButton );

	m_pCustomTexturePrevButton = dynamic_cast<vgui::Button *>(FindChildByName( "CustomTexturePrevButton", true ));
	Assert( m_pCustomTexturePrevButton );

	m_pPlayerCombo = dynamic_cast<vgui::ComboBox *>(FindChildByName( "PlayerComboBox", true ));
	Assert( m_pPlayerCombo );

	m_pAbuseContentLabel = FindChildByName( "AbuseContentLabel", true );
	Assert( m_pAbuseContentLabel );

	m_pAbuseContentCombo = dynamic_cast<vgui::ComboBox *>(FindChildByName( "AbuseContentComboBox", true ));
	Assert( m_pAbuseContentCombo );
	if ( m_pAbuseContentCombo )
	{
		m_pAbuseContentCombo->AddItem( "#AbuseReport_SelectOne", new KeyValues( "AbuseContent", "code", k_EAbuseReportContentNoSelection ) );
		m_pAbuseContentCombo->AddItem( "#AbuseReport_ContentAvatarImage", new KeyValues( "AbuseContent", "code", k_EAbuseReportContentAvatarImage ) );
		m_pAbuseContentCombo->AddItem( "#AbuseReport_ContentPlayerName", new KeyValues( "AbuseContent", "code", k_EAbuseReportContentPersonaName ) );
		m_pAbuseContentCombo->AddItem( "#AbuseReport_ContentItemDecal", new KeyValues( "AbuseContent", "code", k_EAbuseReportContentUGCImage ) );
		m_pAbuseContentCombo->AddItem( "#AbuseReport_ContentChatText", new KeyValues( "AbuseContent", "code", k_EAbuseReportContentComments ) );
		m_pAbuseContentCombo->AddItem( "#AbuseReport_ContentCheating", new KeyValues( "AbuseContent", "code", k_EAbuseReportContentCheating ) );
		m_pAbuseContentCombo->AddItem( "#AbuseReport_ContentOther", new KeyValues( "AbuseContent", "code", k_EAbuseReportContentUnspecified ) );
		m_pAbuseContentCombo->SilentActivateItemByRow( 0 );
		m_pAbuseContentCombo->SetNumberOfEditLines( m_pAbuseContentCombo->GetItemCount() );
	}

	m_pAbuseTypeLabel = FindChildByName( "AbuseTypeLabel", true );
	Assert( m_pAbuseTypeLabel );

	m_pAbuseTypeCombo = dynamic_cast<vgui::ComboBox *>(FindChildByName( "AbuseTypeComboBox", true ));
	Assert( m_pAbuseTypeCombo );

	Assert( m_pScreenShotBitmap );
	if ( m_pScreenShotBitmap && m_pIncidentData->m_bitmapScreenshot.IsValid() )
	{
		m_pScreenShotBitmap->SetBitmap( m_pIncidentData->m_bitmapScreenshot );
	}

	PopulatePlayerList();
	SetIsAccusingGameServer( false );

	SetEnabled( true );
	SetVisible( true );
}

bool CAbuseReportDlg::IsAccusingGameServer()
{
	return m_pIncidentData && m_pIncidentData->m_bCanReportGameServer && m_pGameServerRadio && m_pGameServerRadio->IsSelected();
}

EAbuseReportContentType CAbuseReportDlg::GetAbuseContentType()
{
	if ( m_pAbuseContentCombo == NULL || IsAccusingGameServer() )
	{
		Assert( m_pAbuseContentCombo );
		return k_EAbuseReportContentNoSelection;
	}
	KeyValues *pUserData = m_pAbuseContentCombo->GetActiveItemUserData();
	if ( pUserData == NULL )
	{
		return k_EAbuseReportContentNoSelection;
	}
	return (EAbuseReportContentType)pUserData->GetInt( "code", k_EAbuseReportContentNoSelection );
}

EAbuseReportType CAbuseReportDlg::GetAbuseType()
{
	if ( m_pAbuseTypeCombo == NULL || IsAccusingGameServer() )
	{
		Assert( m_pAbuseTypeCombo );
		return k_EAbuseReportTypeNoSelection;
	}
	KeyValues *pUserData = m_pAbuseTypeCombo->GetActiveItemUserData();
	if ( pUserData == NULL )
	{
		return k_EAbuseReportTypeNoSelection;
	}
	return (EAbuseReportType)pUserData->GetInt( "code", k_EAbuseReportTypeNoSelection );
}

CUtlString CAbuseReportDlg::GetAbuseDescription()
{
	char buf[ 1024 ] = "";
	if ( m_pDescriptionTextEntry )
	{
		m_pDescriptionTextEntry->GetText( buf, ARRAYSIZE(buf) );
	}

	return CUtlString( buf );
}

int CAbuseReportDlg::GetAccusedPlayerIndex()
{
	// If accusing a game server, then there's no player
	if ( IsAccusingGameServer() )
	{
		return -1;
	}

	if ( m_pPlayerCombo == NULL )
	{
		Assert( m_pPlayerCombo );
		return -1;
	}

	// Item 0 is the "<select one>" item
	return m_pPlayerCombo->GetActiveItem() - 1;
}

const AbuseIncidentData_t::PlayerData_t *CAbuseReportDlg::GetAccusedPlayerPtr()
{
	int iPlayerIndex = GetAccusedPlayerIndex();
	if ( iPlayerIndex < 0 )
		return NULL;
	return &m_pIncidentData->m_vecPlayers[ iPlayerIndex ];
}

bool CAbuseReportDlg::GetAttachScreenShot()
{
	if ( m_pScreenShotAttachCheckButton == NULL )
	{
		return false;
	}
	if ( !m_pScreenShotAttachCheckButton->IsVisible() )
	{
		// We hide the checkbutton when the option is not applicable
		return false;
	}
	return m_pScreenShotAttachCheckButton->IsSelected();
}

void CAbuseReportDlg::PopulatePlayerList()
{
	if ( m_pIncidentData == NULL || m_pPlayerCombo == NULL )
	{
		Assert( m_pIncidentData );
		Assert( m_pPlayerCombo );
		return;
	}
	m_pPlayerCombo->RemoveAll();
	m_pPlayerCombo->AddItem( "#AbuseReport_SelectOne", NULL );
	for ( int i = 0 ; i < m_pIncidentData->m_vecPlayers.Count() ; ++i )
	{
		AbuseIncidentData_t::PlayerData_t *p = &m_pIncidentData->m_vecPlayers[i];
		m_pPlayerCombo->AddItem( p->m_sPersona, NULL );
	}
	m_pPlayerCombo->SilentActivateItemByRow( 0 );

	m_pPlayerCombo->SetNumberOfEditLines( MIN( m_pPlayerCombo->GetItemCount()+1, 12 ) );
}

void CAbuseReportDlg::UpdateSubmitButton()
{
	if ( !m_pSubmitButton )
	{
		Assert( m_pSubmitButton );
		return;
	}

	bool bEnable = false;
	if ( IsAccusingGameServer() )
	{
		bEnable = true;
	}
	else
	{
		EAbuseReportContentType eContent = GetAbuseContentType();
		const AbuseIncidentData_t::PlayerData_t *pAccused = GetAccusedPlayerPtr();
		if (
			eContent >= 0
			&& GetAbuseType() >= 0
			&& pAccused != NULL )
		{
			bEnable = true;
			if ( eContent == k_EAbuseReportContentAvatarImage && pAccused->m_iSteamAvatarIndex <= 0 )
			{
				// Cannot accuse somebody of having a bad avatar image, if they
				// don't have one set
				bEnable = false;
			}
		}
	}
	if ( GetAbuseDescription().IsEmpty() )
	{
		bEnable = false;
	}

	m_pSubmitButton->SetEnabled( bEnable );
}

void CAbuseReportDlg::ContentTypeChanged()
{

	// Save current abuse type.  We want to keep it the same,
	// if possible
	EAbuseReportType abuseType = GetAbuseType();
	EAbuseReportContentType contentType = GetAbuseContentType();

	// Show/hide screen shot / image select
	bool bShowScreenshot = false;
	bool bShowAttach = false;
	switch ( contentType )
	{
		default:
			Assert( false );
		case k_EAbuseReportContentNoSelection:
		case k_EAbuseReportContentPersonaName:
			bShowScreenshot = true;
			bShowAttach = false;
			break;

		case k_EAbuseReportContentUnspecified:
		case k_EAbuseReportContentComments:
		case k_EAbuseReportContentCheating:
			bShowScreenshot = true;
			bShowAttach = true;
			break;

		case k_EAbuseReportContentAvatarImage:
		case k_EAbuseReportContentUGCImage:
			bShowScreenshot = false;
			bShowAttach = false;
			break;
	}

	bShowScreenshot = bShowScreenshot && m_pIncidentData->m_bitmapScreenshot.IsValid();

	// Make sure we have everything we need to upload a screenshot
	bShowAttach = bShowAttach
		&& bShowScreenshot
		&& ( GetAccusedPlayerIndex() >= 0 )
		&& m_pIncidentData->m_bufScreenshotFileData.TellPut() > 0
		&& steamapicontext
		&& ( steamapicontext->SteamUtils() != NULL )
		&& ( steamapicontext->SteamRemoteStorage() != NULL );

	if ( m_pScreenShotBitmap )
	{
		m_pScreenShotBitmap->SetVisible( bShowScreenshot );
	}
	if ( m_pScreenShotAttachCheckButton )
	{
		m_pScreenShotAttachCheckButton->SetVisible( bShowAttach );
	}

	UpdateAvatarImage();
	UpdateCustomTextures();

	// Populate abuse type
	if ( m_pAbuseTypeCombo )
	{

		// If the combo box was invisible, then they didn't really make a purposeful decision
		if ( !m_pAbuseTypeCombo->IsVisible() )
		{
			abuseType = k_EAbuseReportTypeNoSelection;
		}
		m_pAbuseTypeCombo->RemoveAll();
		switch ( contentType )
		{
			default:
				Assert( false );
			case k_EAbuseReportContentNoSelection:
				m_pAbuseTypeCombo->SetVisible( false );
				abuseType = k_EAbuseReportTypeNoSelection;
				m_pAbuseTypeCombo->AddItem( "#AbuseReport_SelectOne", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeNoSelection ) );
				break;

			case k_EAbuseReportContentCheating:
				m_pAbuseTypeCombo->SetVisible( false );
				abuseType = k_EAbuseReportTypeCheating;
				m_pAbuseTypeCombo->AddItem( "#AbuseReport_TypeCheating", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeCheating ) );
				break;

			case k_EAbuseReportContentUnspecified:
			case k_EAbuseReportContentComments:
			case k_EAbuseReportContentPersonaName:
			case k_EAbuseReportContentAvatarImage:
			case k_EAbuseReportContentUGCImage:
				m_pAbuseTypeCombo->SetVisible( true );
				m_pAbuseTypeCombo->AddItem( "#AbuseReport_SelectOne", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeNoSelection ) );
				m_pAbuseTypeCombo->AddItem( "#AbuseReport_TypeSpam", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeSpamming ) );
				m_pAbuseTypeCombo->AddItem( "#AbuseReport_TypeAdvertisement", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeAdvertisement ) );
				m_pAbuseTypeCombo->AddItem( "#AbuseReport_TypeLanguage", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeLanguage ) );
				m_pAbuseTypeCombo->AddItem( "#AbuseReport_TypeAdultContent", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeAdultContent ) );
				m_pAbuseTypeCombo->AddItem( "#AbuseReport_TypeHarassment", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeHarassment ) );
				m_pAbuseTypeCombo->AddItem( "#AbuseReport_TypeProhibited", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeProhibited ) );
				m_pAbuseTypeCombo->AddItem( "#AbuseReport_TypeSpoofing", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeSpoofing ) );
				//m_pAbuseTypeCombo->AddItem( "#AbuseReport_TypeCheating", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeCheating ) );
				m_pAbuseTypeCombo->AddItem( "#AbuseReport_TypeInappropriate", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeInappropriate ) );
				m_pAbuseTypeCombo->AddItem( "#AbuseReport_TypeOther", new KeyValues( "AbuseType", "code", k_EAbuseReportTypeUnspecified ) );
				break;
		}

		// Now select the proper row
		int sel = 0;
		for ( int i = 0 ; i < m_pAbuseTypeCombo->GetItemCount() ; ++i ) {
			if ( m_pAbuseTypeCombo->GetItemUserData(i)->GetInt("code") == abuseType )
			{
				sel = i;
				break;
			}
		}
		m_pAbuseTypeCombo->SilentActivateItemByRow( sel );
		m_pAbuseTypeCombo->SetNumberOfEditLines( m_pAbuseTypeCombo->GetItemCount() );
		if ( m_pAbuseTypeLabel )
		{
			m_pAbuseTypeLabel->SetVisible( m_pAbuseTypeCombo->IsVisible() );
		}
	}

	UpdateSubmitButton();
}

void CAbuseReportDlg::OnRadioButtonChecked( vgui::Panel *panel )
{
	if ( panel == m_pPlayerRadio )
	{
		SetIsAccusingGameServer( false );
	}
	else if ( panel == m_pGameServerRadio )
	{
		SetIsAccusingGameServer( true );
	}
	else
	{
		Assert( !"Clicked on unknown radio" );
	}
}

void  CAbuseReportDlg::SetIsAccusingGameServer( bool bAccuseGameServer )
{
	if ( m_pGameServerRadio && m_pGameServerRadio->IsSelected() != bAccuseGameServer )
	{
		m_pGameServerRadio->SetSelected( bAccuseGameServer );
	}
	if ( m_pPlayerRadio && m_pPlayerRadio->IsSelected() == bAccuseGameServer)
	{
		m_pPlayerRadio->SetSelected( !bAccuseGameServer );
	}
	if ( m_pPlayerLabel )
	{
		m_pPlayerLabel->SetVisible( !bAccuseGameServer );
	}
	if ( m_pPlayerCombo )
	{
		m_pPlayerCombo->SetVisible( !bAccuseGameServer );
	}

	PlayerChanged();
}

void  CAbuseReportDlg::PlayerChanged()
{
	m_iUserImageIndex = 0;

	bool bShow = ( GetAccusedPlayerIndex() >= 0 );
	if ( m_pAbuseContentCombo != NULL )
	{
		m_pAbuseContentCombo->SetVisible( bShow );
	}
	if ( m_pAbuseContentLabel != NULL )
	{
		m_pAbuseContentLabel->SetVisible( bShow );
	}

	ContentTypeChanged();
}

void  CAbuseReportDlg::UpdateAvatarImage()
{
	if ( m_pAvatarImage == NULL || m_pNoAvatarLabel == NULL )
	{
		Assert( m_pAvatarImage );
		Assert( m_pNoAvatarLabel );
		return;
	}

	const AbuseIncidentData_t::PlayerData_t *pAccused = GetAccusedPlayerPtr();
	if ( GetAbuseContentType() == k_EAbuseReportContentAvatarImage && pAccused != NULL )
	{
		if ( pAccused->m_iSteamAvatarIndex > 0 )
		{
			m_pAvatarImage->SetShouldDrawFriendIcon( false );
			m_pAvatarImage->SetPlayer( pAccused->m_steamID, k_EAvatarSize184x184 );

			m_pAvatarImage->SetVisible( true );
			m_pNoAvatarLabel->SetVisible( false );
		}
		else
		{
			m_pAvatarImage->SetVisible( false );
			m_pNoAvatarLabel->SetVisible( true );
		}
	}
	else
	{
		m_pAvatarImage->SetVisible( false );
		m_pNoAvatarLabel->SetVisible( false );
	}
}

void  CAbuseReportDlg::UpdateCustomTextures()
{
	if ( m_pCustomTextureImagePanel == NULL || m_pNoCustomTexturesLabel == NULL || m_pCustomTextureNextButton == NULL || m_pCustomTexturePrevButton == NULL )
	{
		Assert( m_pCustomTextureImagePanel );
		Assert( m_pNoCustomTexturesLabel );
		Assert( m_pCustomTextureNextButton );
		Assert( m_pCustomTexturePrevButton );
		return;
	}

	const AbuseIncidentData_t::PlayerData_t *pAccused = GetAccusedPlayerPtr();
	bool bShowScrollButtons = false;
	if ( GetAbuseContentType() == k_EAbuseReportContentUGCImage && pAccused != NULL )
	{
		int iSelectedCustomImage = GetSelectedCustomImage();

		if ( iSelectedCustomImage >= 0 )
		{

			// Currently the only thing we support...
			Assert( pAccused->m_vecImages[ iSelectedCustomImage].m_eType == AbuseIncidentData_t::k_PlayerImageType_UGC );

			m_pCustomTextureImagePanel->m_ugcHandle = pAccused->m_vecImages[ iSelectedCustomImage].m_hUGCHandle;

			m_pCustomTextureImagePanel->SetVisible( true );
			m_pNoCustomTexturesLabel->SetVisible( false );

			int n = pAccused->m_vecImages.Count();
			if ( n > 1 )
			{
				bShowScrollButtons = true;
				m_pCustomTextureNextButton->SetEnabled( iSelectedCustomImage < n-1 );
				m_pCustomTexturePrevButton->SetEnabled( iSelectedCustomImage > 0 );
			}
		}
		else
		{
			m_pCustomTextureImagePanel->SetVisible( false );
			m_pNoCustomTexturesLabel->SetVisible( true );
		}
	}
	else
	{
		m_pCustomTextureImagePanel->SetVisible( false );
		m_pNoCustomTexturesLabel->SetVisible( false );
	}
	m_pCustomTextureNextButton->SetVisible( bShowScrollButtons );
	m_pCustomTexturePrevButton->SetVisible( bShowScrollButtons );
}

int CAbuseReportDlg::GetSelectedCustomImage()
{
	if ( GetAbuseContentType() != k_EAbuseReportContentUGCImage )
	{
		m_iUserImageIndex = 0;
		return -1;
	}

	const AbuseIncidentData_t::PlayerData_t *pAccused = GetAccusedPlayerPtr();
	if ( pAccused == NULL )
	{
		m_iUserImageIndex = 0;
		return -1;
	}

	int n = pAccused->m_vecImages.Count();
	if ( n < 1 )
	{
		m_iUserImageIndex = 0;
		return -1;
	}

	// Wrap currently selected index
	m_iUserImageIndex = ( m_iUserImageIndex + n*10 ) % n;

	// Return it
	return m_iUserImageIndex;
}

void CAbuseReportDlg::OnTextChanged( vgui::Panel *panel )
{
	if ( panel == m_pPlayerCombo )
	{
		PlayerChanged();
	}
	else if ( panel == m_pAbuseContentCombo )
	{
		ContentTypeChanged();
	}
	else
	{
		UpdateSubmitButton();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Job to do the async work of submitting the report
//-----------------------------------------------------------------------------
class CSubmitAbuseReportJob : public GCSDK::CGCClientJob
{
public:
	bool m_bGameServer;


	CSubmitAbuseReportJob( )
	: GCSDK::CGCClientJob( GCClientSystem()->GetGCClient() )
	{
		m_bGameServer = false;
	}

	virtual bool BYieldingRunGCJob()
	{
		EResult result = RunJob();

		// Tear down our dialogs
		CloseWaitingDialog();

		CAbuseReportDlg *pDlg = g_AbuseReportDlg.Get();
		if ( pDlg )
		{
			pDlg->Close();
			pDlg = NULL;
		}

		// And destroy the queued report!
		g_AbuseReportMgr->DestroyIncidentData();

		// now show a dialog box explaining the outcome
		switch ( result )
		{
			case k_EResultOK:
				ShowMessageBox( "#AbuseReport_SucceededTitle", "#AbuseReport_SucceededMessage", "#GameUI_OK" );
				break;

			case k_EResultLimitExceeded:
				ShowMessageBox(
					"#AbuseReport_TooMuchFailedTitle",
					m_bGameServer ? "#AbuseReport_TooMuchFailedMessageGameServer" : "#AbuseReport_TooMuchFailedMessage",
					"#GameUI_OK"
				);
				break;

			default:
				ShowMessageBox( "#AbuseReport_GenericFailureTitle", "#AbuseReport_GenericFailureMessage", "#GameUI_OK" );
				break;
		}

		return true;
	}

	EResult RunJob()
	{
		CAbuseReportDlg *pDlg = g_AbuseReportDlg.Get();
		if ( pDlg == NULL )
		{
			return k_EResultFail;
		}
		m_bGameServer = pDlg->IsAccusingGameServer();
		EAbuseReportContentType eContentSelected = pDlg->GetAbuseContentType();
		EAbuseReportContentType eContentReported = eContentSelected;
		EAbuseReportType eAbuseType = pDlg->GetAbuseType();
		const AbuseIncidentData_t::PlayerData_t *pAccused = pDlg->GetAccusedPlayerPtr();
		const AbuseIncidentData_t *pIncidentData = g_AbuseReportMgr->GetIncidentData();
		CUtlString sAbuseDescription = pDlg->GetAbuseDescription();
		netadr_t adrGameServer = pIncidentData->m_adrGameServer;
		CSteamID steamIDGameServer = pIncidentData->m_steamIDGameServer;
		uint64 gid = 0;

		// Check if we should upload the screenshot
		if ( pDlg->GetAttachScreenShot() && steamapicontext && steamapicontext->SteamUtils() && steamapicontext->SteamRemoteStorage() )
		{

			// Write the local copy of the file
			if ( !steamapicontext->SteamRemoteStorage()->FileWrite( CAbuseReportManager::k_rchScreenShotFilename, pIncidentData->m_bufScreenshotFileData.Base(), pIncidentData->m_bufScreenshotFileData.TellPut() ) )
			{
				Warning( "Failed to save local cloud copy of %s\n", CAbuseReportManager::k_rchScreenShotFilename );
				return k_EResultFail;
			}

			// Share it.  This initiates the upload to cloud
			Msg( "Starting upload of %s to UFS....\n", CAbuseReportManager::k_rchScreenShotFilename );
			SteamAPICall_t hFileShareApiCall = steamapicontext->SteamRemoteStorage()->FileShare( CAbuseReportManager::k_rchScreenShotFilename );
			if ( hFileShareApiCall == k_uAPICallInvalid )
			{
				Warning( "Failed to share %s\n", CAbuseReportManager::k_rchScreenShotFilename );
				return k_EResultFail;
			}

			// Check if we're busy
			bool bFailed;
			RemoteStorageFileShareResult_t result;
			while ( !steamapicontext->SteamUtils()->GetAPICallResult(hFileShareApiCall,
				&result, sizeof(result), RemoteStorageFileShareResult_t::k_iCallback, &bFailed) )
			{
				BYield();
			}

			// Clear pointer, it could have been destroyed while we were yielding, make sure we don't reference it
			pDlg = NULL;

			if ( bFailed || result.m_eResult != k_EResultOK )
			{
				Warning( "Failed to share %s; result code %d\n", CAbuseReportManager::k_rchScreenShotFilename, result.m_eResult );
				return result.m_eResult;
			}

			Msg( "%s shared to UGC OK\n", CAbuseReportManager::k_rchScreenShotFilename );
			gid = result.m_hFile;

			// SWitch the content type being reported, so the support tool will know what to
			// do with the GID.
			eContentReported = k_EAbuseReportContentActorUGCImage;
		}
		else if ( eContentSelected == k_EAbuseReportContentUGCImage )
		{
			Assert( !m_bGameServer );
			int iImageindex = pDlg->GetSelectedCustomImage();
			Assert( iImageindex >= 0 );
			gid = pAccused->m_vecImages[iImageindex].m_hUGCHandle;
		}

		//
		// Fill out the report message
		//
		GCSDK::CProtoBufMsg<CMsgGCReportAbuse> msg( k_EMsgGC_ReportAbuse );
		if ( m_bGameServer )
		{
			msg.Body().set_target_steam_id( steamIDGameServer.ConvertToUint64() );
			msg.Body().set_target_game_server_ip( adrGameServer.GetIPHostByteOrder() );
			msg.Body().set_target_game_server_port( adrGameServer.GetPort() );
		}
		else
		{
			msg.Body().set_target_steam_id( pAccused->m_steamID.ConvertToUint64() );
			msg.Body().set_content_type( eContentReported );
			msg.Body().set_abuse_type( eAbuseType );
		}
		msg.Body().set_description( sAbuseDescription );
		if (gid != 0 )
		{
			msg.Body().set_gid( gid );
		}

		// Send the message to the GC, and await the reply
		GCSDK::CProtoBufMsg<CMsgGCReportAbuseResponse> msgReply;
		if ( !BYldSendMessageAndGetReply( msg, 10, &msgReply, k_EMsgGC_ReportAbuseResponse ) )
		{
			Warning( "Abuse report failed: Did not get reply from GC\n" );
			return k_EResultTimeout;
		}

		EResult result = (EResult)msgReply.Body().result();
		if ( result != k_EResultOK )
		{
			Warning( "Abuse report failed with failure code %d.  %s\n", result, msgReply.Body().error_message().c_str() );
		}

		// OK
		return result;
	}

};

void CAbuseReportDlg::OnSubmitReport()
{
	// throw up a waiting dialog
	SetEnabled( false );
	ShowWaitingDialog( new CGenericWaitingDialog( this ), "#AbuseReport_Busy", true, false, 0.0f );

	// We need to be in the global singleton handle, because that's how the job knows
	// to get to us (and how it knows if we've died)!
	Assert( g_AbuseReportDlg.Get() == this );

	// Start a job
	CSubmitAbuseReportJob *pJob = new CSubmitAbuseReportJob();
	pJob->StartJob( NULL );
}
