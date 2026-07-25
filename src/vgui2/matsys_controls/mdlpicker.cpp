//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "matsys_controls/mdlpicker.h"
#include "tier1/KeyValues.h"
#include "tier1/utldict.h"
#include "filesystem.h"
#include "studio.h"
#include "matsys_controls/matsyscontrols.h"
#include "matsys_controls/mdlpanel.h"
#include "vgui_controls/Splitter.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/MessageBox.h"
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/DirectorySelectDialog.h"
#include "vgui/IVGui.h"
#include "vgui/IInput.h"
#include "vgui/ISurface.h"
#include "vgui/Cursor.h"
#include "matsys_controls/assetpicker.h"
#include "matsys_controls/colorpickerpanel.h"
#include "dmxloader/dmxloader.h"
#include "tier1/utlbuffer.h"
#include "bitmap/tgawriter.h"
#include "tier3/tier3.h"
#include "istudiorender.h"
#include "../vgui2/src/VPanel.h"
#include "tier2/p4helpers.h"
#include "ivtex.h"
#include "bitmap/tgaloader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;

static bool SaveTgaAndAddToP4( unsigned char *pImage, ImageFormat imageFormat, int Width, int Height, const char *szDestFilename )
{

	// allocate a buffer to write the tga into
	int iMaxTGASize = 1024 + (Width * Height * 4);
	void *pTGA = malloc( iMaxTGASize );
	CUtlBuffer buffer( pTGA, iMaxTGASize );

	if( !TGAWriter::WriteToBuffer( pImage, buffer, Width, Height, imageFormat, IMAGE_FORMAT_BGRA8888 ) )
	{
		Error( "Couldn't write bitmap data snapshot.\n" );
		return false;
	}

	CP4AutoEditAddFile autop4( szDestFilename );

	// async write to disk (this will take ownership of the memory)
	char szDirName[ _MAX_PATH ];
	strcpy( szDirName, szDestFilename );
	V_StripFilename( szDirName );
	g_pFullFileSystem->CreateDirHierarchy( szDirName, "" );
	g_pFullFileSystem->AsyncWrite( szDestFilename, buffer.Base(), buffer.TellPut(), true );

	return true;
}

//-----------------------------------------------------------------------------
//
// MDL Picker
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Sort by MDL name
//-----------------------------------------------------------------------------
static int __cdecl MDLBrowserSortFunc( vgui::ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	const char *string1 = item1.kv->GetString("mdl");
	const char *string2 = item2.kv->GetString("mdl");
	return stricmp( string1, string2 );
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMDLPicker::CMDLPicker( vgui::Panel *pParent, int nFlags ) : 
	BaseClass( pParent, "MDL Files", "mdl", "models", "mdlName" )
{
	for( int i = 0; i < MAX_SELECTED_MODELS; i++ )
	{
		m_hSelectedMDL[ i ] = MDLHANDLE_INVALID;
	}

	m_nFlags = nFlags;	// remember what we show and what not

	m_pRenderPage = NULL;
	m_pSequencesPage = NULL;
	m_pActivitiesPage = NULL;
	m_pSkinsPage = NULL;
	m_pInfoPage = NULL;
	m_pScreenCapsPage = NULL;

	m_pSequencesList = NULL;
	m_pActivitiesList = NULL;

	m_hDirectorySelectDialog = NULL;

	// Horizontal splitter for mdls
	m_pFileBrowserSplitter = new Splitter( this, "FileBrowserSplitter", SPLITTER_MODE_VERTICAL, 1 );

	float flFractions[] = { 0.33f, 0.67f };

	m_pFileBrowserSplitter->RespaceSplitters( flFractions );

	vgui::Panel *pSplitterLeftSide = m_pFileBrowserSplitter->GetChild( 0 );
	vgui::Panel *pSplitterRightSide = m_pFileBrowserSplitter->GetChild( 1 );

	// Standard browser controls
	pSplitterLeftSide->RequestFocus();
	CreateStandardControls( pSplitterLeftSide, false );

	// property sheet - revisions, changes, etc.
	m_pPreviewSplitter = new Splitter( pSplitterRightSide, "PreviewSplitter", SPLITTER_MODE_HORIZONTAL, 1 );

	vgui::Panel *pSplitterTopSide = m_pPreviewSplitter->GetChild( 0 );
	vgui::Panel *pSplitterBottomSide = m_pPreviewSplitter->GetChild( 1 );

	// MDL preview
	m_pMDLPreview = new CMDLPanel( pSplitterTopSide, "MDLPreview" );
	SetSkipChildDuringPainting( m_pMDLPreview );

	m_pViewsSheet = new vgui::PropertySheet( pSplitterBottomSide, "ViewsSheet" );
	m_pViewsSheet->AddActionSignalTarget( this );

	// now add wanted features
	if ( nFlags & PAGE_RENDER )
	{
		m_pRenderPage = new vgui::PropertyPage( m_pViewsSheet, "RenderPage" );

		m_pRenderPage->AddActionSignalTarget( this );

        m_pRenderPage->LoadControlSettingsAndUserConfig( "resource/mdlpickerrender.res" );

		RefreshRenderSettings();

		// ground
		Button *pSelectProbe = (Button*)m_pRenderPage->FindChildByName( "ChooseLightProbe" );
		pSelectProbe->AddActionSignalTarget( this );
	}

	if ( nFlags & PAGE_SEQUENCES )
	{
		m_pSequencesPage = new vgui::PropertyPage( m_pViewsSheet, "SequencesPage" );

		m_pSequencesList = new vgui::ListPanel( m_pSequencesPage, "SequencesList" );
		m_pSequencesList->AddColumnHeader( 0, "sequence", "sequence", 52, 0 );
		m_pSequencesList->AddActionSignalTarget( this );
		m_pSequencesList->SetSelectIndividualCells( true );
		m_pSequencesList->SetEmptyListText("No .MDL file currently selected.");
		m_pSequencesList->SetDragEnabled( true );
		m_pSequencesList->SetAutoResize( Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 6, 6, -6, -6 );
	}

	if ( nFlags & PAGE_ACTIVITIES )
	{
		m_pActivitiesPage = new vgui::PropertyPage( m_pViewsSheet, "ActivitiesPage" );

		m_pActivitiesList = new vgui::ListPanel( m_pActivitiesPage, "ActivitiesList" );
		m_pActivitiesList->AddColumnHeader( 0, "activity", "activity", 52, 0 );
		m_pActivitiesList->AddActionSignalTarget( this );
		m_pActivitiesList->SetSelectIndividualCells( true );
		m_pActivitiesList->SetEmptyListText( "No .MDL file currently selected." );
		m_pActivitiesList->SetDragEnabled( true );
		m_pActivitiesList->SetAutoResize( Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 6, 6, -6, -6 );
	}

	if ( nFlags & PAGE_SKINS )
	{
		m_pSkinsPage = new vgui::PropertyPage( m_pViewsSheet, "SkinsPage" );

		m_pSkinsList = new vgui::ListPanel( m_pSkinsPage, "SkinsList" );
		m_pSkinsList->AddColumnHeader( 0, "skin", "skin", 52, 0 );
		m_pSkinsList->AddActionSignalTarget( this );
		m_pSkinsList->SetSelectIndividualCells( true );
		m_pSkinsList->SetEmptyListText( "No .MDL file currently selected." );
		m_pSkinsList->SetDragEnabled( true );
		m_pSkinsList->SetAutoResize( Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 6, 6, -6, -6 );		
	}

	if ( nFlags & PAGE_INFO )
	{
		m_pInfoPage = new vgui::PropertyPage( m_pViewsSheet, "InfoPage" );

		m_pInfoPage->AddActionSignalTarget( this );

		m_pInfoPage->LoadControlSettingsAndUserConfig( "resource/mdlpickerinfo.res" );

		CheckButton * pTempCheck = (CheckButton *)m_pInfoPage->FindChildByName( "PhysicsObject" );
		pTempCheck->SetDisabledFgColor1( pTempCheck->GetFgColor());
		pTempCheck->SetDisabledFgColor2( pTempCheck->GetFgColor());
		pTempCheck = (CheckButton *)m_pInfoPage->FindChildByName( "StaticObject" );
		pTempCheck->SetDisabledFgColor1( pTempCheck->GetFgColor());
		pTempCheck->SetDisabledFgColor2( pTempCheck->GetFgColor());
		pTempCheck = (CheckButton *)m_pInfoPage->FindChildByName( "DynamicObject" );
		pTempCheck->SetDisabledFgColor1( pTempCheck->GetFgColor());
		pTempCheck->SetDisabledFgColor2( pTempCheck->GetFgColor());

		m_pPropDataList = new vgui::ListPanel( m_pInfoPage, "PropData" );
		m_pPropDataList->AddColumnHeader( 0, "key", "key", 250, ListPanel::COLUMN_FIXEDSIZE );		
		m_pPropDataList->AddColumnHeader( 1, "value", "value", 52, 0 );
		m_pPropDataList->AddActionSignalTarget( this );
		m_pPropDataList->SetSelectIndividualCells( false );
		m_pPropDataList->SetEmptyListText( "No prop_data available." );
		m_pPropDataList->SetDragEnabled( true );
		m_pPropDataList->SetAutoResize( Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 6, 72, -6, -6 );

		RefreshRenderSettings();
	}

	if ( nFlags & PAGE_SCREEN_CAPS )
	{
		m_pScreenCapsPage = new vgui::PropertyPage( m_pViewsSheet, "ScreenCapsPage" );

//		not sure why we have to do this for the color picker
		CColorPickerButton *m_pBackgroundColor;
		m_pBackgroundColor = new CColorPickerButton( m_pScreenCapsPage, "BackgroundColor", this );

		m_pScreenCapsPage->LoadControlSettingsAndUserConfig( "resource/mdlpickerscreencaps.res" );

		TextEntry	*pTempValue;
		pTempValue = ( TextEntry * )m_pScreenCapsPage->FindChildByName( "WidthText" );
		pTempValue->SetText( "256" );
		pTempValue = ( TextEntry * )m_pScreenCapsPage->FindChildByName( "HeightText" );
		pTempValue->SetText( "256" );

//		not sure why this doesn't work
//		m_pBackgroundColor = ( CColorPickerButton * )m_pScreenCapsPage->FindChildByName( "BackgroundColor" );
		m_pBackgroundColor->SetColor( 255, 0, 0, 255 );

		Label *m_pOutputDirectory;
		m_pOutputDirectory = ( Label * )m_pScreenCapsPage->FindChildByName( "OutputDirectory" );
		m_pOutputDirectory->SetText( "c:\\" );

		Button *pSelectProbe = ( Button * )m_pScreenCapsPage->FindChildByName( "Capture" );
		pSelectProbe->AddActionSignalTarget( this );
		pSelectProbe = ( Button * )m_pScreenCapsPage->FindChildByName( "OutputDirectorySelect" );
		pSelectProbe->AddActionSignalTarget( this );
		pSelectProbe = ( Button * )m_pScreenCapsPage->FindChildByName( "SaveCaps" );
		pSelectProbe->AddActionSignalTarget( this );
		pSelectProbe = ( Button * )m_pScreenCapsPage->FindChildByName( "RestoreCaps" );
		pSelectProbe->AddActionSignalTarget( this );
		pSelectProbe = ( Button * )m_pScreenCapsPage->FindChildByName( "GenerateBackpackIcons" );
		pSelectProbe->AddActionSignalTarget( this );
	}

	// Load layout settings; has to happen before pinning occurs in code
	LoadControlSettingsAndUserConfig( "resource/mdlpicker.res" );

	// Pages must be added after control settings are set up
	if ( m_pRenderPage )
	{
		m_pViewsSheet->AddPage( m_pRenderPage, "Render" );
	}
	if ( m_pSequencesPage )
	{
		m_pViewsSheet->AddPage( m_pSequencesPage, "Sequences" );
	}
	if ( m_pActivitiesPage )
	{
		m_pViewsSheet->AddPage( m_pActivitiesPage, "Activities" );
	}
	if ( m_pSkinsPage )
	{
		m_pViewsSheet->AddPage( m_pSkinsPage, "Skins" );
	}
	if ( m_pInfoPage )
	{
		m_pViewsSheet->AddPage( m_pInfoPage, "Info" );
	}
	if ( m_pScreenCapsPage )
	{
		m_pViewsSheet->AddPage( m_pScreenCapsPage, "Screen Caps" );
	}
}

void CMDLPicker::RefreshRenderSettings()
{
	vgui::CheckButton *pToggle;

	if ( !m_pRenderPage )
		return;

	// ground
	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName("NoGround");
	pToggle->AddActionSignalTarget( this );
	m_pMDLPreview->SetGroundGrid( !pToggle->IsSelected() );
	
	// collision
	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName("Collision");
	pToggle->AddActionSignalTarget( this );
	m_pMDLPreview->SetCollsionModel( pToggle->IsSelected() );

	// wireframe
	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName("Wireframe");
	pToggle->AddActionSignalTarget( this );
	m_pMDLPreview->SetWireFrame( pToggle->IsSelected() );

	// lockview
	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName("LockView");
	pToggle->AddActionSignalTarget( this );
	m_pMDLPreview->SetLockView( pToggle->IsSelected() );

	// look at camera
	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName("LookAtCamera");
	pToggle->AddActionSignalTarget( this );
	m_pMDLPreview->SetLookAtCamera( pToggle->IsSelected() );

	// thumbnail safe zone
	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName("ThumbnailSafeZone");
	pToggle->AddActionSignalTarget( this );
	m_pMDLPreview->SetThumbnailSafeZone( pToggle->IsSelected() );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMDLPicker::~CMDLPicker()
{
}


//-----------------------------------------------------------------------------
// Performs layout
//-----------------------------------------------------------------------------
void CMDLPicker::PerformLayout()
{
	// NOTE: This call should cause auto-resize to occur
	// which should fix up the width of the panels
	BaseClass::PerformLayout();

	int w, h;
	GetSize( w, h );

	// Layout the mdl splitter
	m_pFileBrowserSplitter->SetBounds( 0, 0, w, h );
}


//-----------------------------------------------------------------------------
// Buttons on various pages
//-----------------------------------------------------------------------------
void CMDLPicker::OnAssetSelected( KeyValues *pParams )
{
	const char *pAsset = pParams->GetString( "asset" );

	char pProbeBuf[MAX_PATH];
	Q_snprintf( pProbeBuf, sizeof(pProbeBuf), "materials/lightprobes/%s", pAsset );

	BeginDMXContext();
	CDmxElement *pLightProbe = NULL; 
	bool bOk = UnserializeDMX( pProbeBuf, "GAME", true, &pLightProbe );
	if ( !pLightProbe || !bOk )
	{
		char pBuf[1024];
		Q_snprintf( pBuf, sizeof(pBuf), "Error loading lightprobe file '%s'!\n", pProbeBuf ); 
		vgui::MessageBox *pMessageBox = new vgui::MessageBox( "Error Loading File!\n", pBuf, GetParent() );
		pMessageBox->DoModal( );

		EndDMXContext( true );
		return;
	}

	m_pMDLPreview->SetLightProbe( pLightProbe );
	EndDMXContext( true );
}


//-----------------------------------------------------------------------------
// Buttons on various pages
//-----------------------------------------------------------------------------
void CMDLPicker::OnCommand( const char *pCommand )
{
	if ( !Q_stricmp( pCommand, "ChooseLightProbe" ) )
	{
		CAssetPickerFrame *pPicker = new CAssetPickerFrame( this, "Select Light Probe (.prb) File",
			"Light Probe", "prb", "materials/lightprobes", "lightprobe" );
		pPicker->DoModal();
		return;
	}
	else if ( !Q_stricmp( pCommand, "OutputDirectorySelect" ) )
	{
		if ( !m_hDirectorySelectDialog.Get() )
		{
			m_hDirectorySelectDialog = new DirectorySelectDialog( this, "Choose Screen Caps output folder" );
		}

		Label	*m_pOutputDirectory;
		char	temp[ MAX_PATH ];
		m_pOutputDirectory = ( Label * )m_pScreenCapsPage->FindChildByName( "OutputDirectory" );
		m_pOutputDirectory->GetText( temp, sizeof( temp ) );

		m_hDirectorySelectDialog->MakeReadyForUse();
		m_hDirectorySelectDialog->SetStartDirectory( temp );
		m_hDirectorySelectDialog->DoModal();
		return;
	}
	else if ( !Q_stricmp( pCommand, "Capture" ) )
	{
		CaptureScreenCaps();
		return;
	}
	else if ( !Q_stricmp( pCommand, "GenerateBackpackIcons" ) )
	{
		// shut off the ground grid
		vgui::CheckButton *pGroundToggle = (vgui::CheckButton *)m_pRenderPage->FindChildByName( "NoGround" );
		bool bOriginalGridState = pGroundToggle->IsSelected();

		vgui::CheckButton *pSafeZoneToggle = (vgui::CheckButton *)m_pRenderPage->FindChildByName( "ThumbnailSafeZone" );
		bool bOriginalSafeZoneState = pSafeZoneToggle->IsSelected();

		m_pMDLPreview->SetGroundGrid( false );
		m_pMDLPreview->SetThumbnailSafeZone( false );

		// make the icons
		GenerateBackpackIcons();

		// return the ground grid to its original state
		m_pMDLPreview->SetGroundGrid( !bOriginalGridState );
		m_pMDLPreview->SetThumbnailSafeZone( bOriginalSafeZoneState );

		return;
	}
	else if ( !Q_stricmp( pCommand, "SaveCaps" ) )
	{
		SaveCaps( NULL );
		return;
	}
	else if ( !Q_stricmp( pCommand, "RestoreCaps" ) )
	{
		if ( input()->IsKeyDown( KEY_RCONTROL ) || input()->IsKeyDown( KEY_LCONTROL ) )
		{
			int nCount = m_AssetList.Count();
			for ( int i = 0; i < nCount; ++i )
			{
				if ( m_pAssetBrowser->IsItemVisible( m_AssetList[ i ].m_nItemId ) &&
					 m_pAssetBrowser->IsItemSelected( m_AssetList[ i ].m_nItemId ) )
				{
					KeyValues *pItemKeyValues = m_pAssetBrowser->GetItem( m_AssetList[ i ].m_nItemId );
					const char *pSelectedAsset = pItemKeyValues->GetString( "asset" );

					char		szBathPath[ _MAX_PATH ];
					Label		*m_pOutputDirectory;

					m_pOutputDirectory = ( Label * )m_pScreenCapsPage->FindChildByName( "OutputDirectory" );
					m_pOutputDirectory->GetText( szBathPath, sizeof( szBathPath ) );

					char szPathedFileName[ _MAX_PATH ];
					V_sprintf_safe( szPathedFileName, "%sbackpack\\%s", szBathPath, pSelectedAsset );

					Label		*m_pResults = ( Label * )m_pScreenCapsPage->FindChildByName( "CaptureResults" );
					if ( RestoreCaps( szPathedFileName ) )
					{
						m_pResults->SetText( "Prefs Restored" );
					}
					else
					{
						m_pResults->SetText( "Prefs NOT FOUND" );
					}
					break;
				}
			}
		}
		else
		{
			RestoreCaps( NULL );
		}
		return;
	}

	BaseClass::OnCommand( pCommand );
}


//-----------------------------------------------------------------------------
// Handles the directory selection for screen caps
//-----------------------------------------------------------------------------
void CMDLPicker::OnDirectorySelected( char const *dir )
{
	if ( m_hDirectorySelectDialog != 0 )
	{
		m_hDirectorySelectDialog->MarkForDeletion();
	}

	Label *m_pOutputDirectory;
	m_pOutputDirectory = ( Label * )m_pScreenCapsPage->FindChildByName( "OutputDirectory" );
	m_pOutputDirectory->SetText( dir );
}


//-----------------------------------------------------------------------------
// Screen captures the specific model and writes out a .tga.  Assumes the MDLPreview 
// panel has been properly adjusted to 0,0 in screen space and that width / height
// have been set.
//-----------------------------------------------------------------------------
const char *CMDLPicker::CaptureModel( int nModIndex, const char *AssetName, const char *OutputPath, int Width, int Height, Color BackgroundColor, bool bSelectedOnly )
{
	char pBuf[ MAX_PATH ];
	Q_snprintf( pBuf, sizeof( pBuf ), "%s\\%s\\%s", GetModPath( nModIndex ), m_pAssetSubDir, AssetName );
	Q_FixSlashes( pBuf );

	if ( !bSelectedOnly )
	{
		SelectMDL( pBuf, false, -1 );
	}

	CMatRenderContextPtr pRenderContext( materials );

	g_pMaterialSystem->BeginFrame( 0 );
	g_pStudioRender->BeginFrame();

//	pRenderContext->ClearColor4ub( 0, 0, 0, 0 ); 
//	pRenderContext->ClearBuffers( true, true );

	Color NewPanelColor;
	
	NewPanelColor.SetColor( 0, 0, 0, 0 );
	m_pMDLPreview->SetBackgroundColor( NewPanelColor );

	g_pVGuiSurface->PaintTraverseEx( m_pMDLPreview->GetVPanel(), false );

	g_pStudioRender->EndFrame();
	g_pMaterialSystem->EndFrame( );

	// get the data from the backbuffer and save to disk
	// bitmap bits
	unsigned char *pImageBlack = ( unsigned char * )malloc( Width * 4 * Height );

	// Get Bits from the material system
	pRenderContext->ReadPixels( 0, 0, Width, Height, pImageBlack, IMAGE_FORMAT_BGRA8888 );


	g_pMaterialSystem->BeginFrame( 0 );
	g_pStudioRender->BeginFrame();

//	pRenderContext->ClearColor4ub( 255, 255, 255, 0 ); 
//	pRenderContext->ClearBuffers( true, true );

	NewPanelColor.SetColor( 255, 255, 255, 0 );
	m_pMDLPreview->SetBackgroundColor( NewPanelColor );

	g_pVGuiSurface->PaintTraverseEx( m_pMDLPreview->GetVPanel(), false );

	g_pStudioRender->EndFrame();
	g_pMaterialSystem->EndFrame( );

	// get the data from the backbuffer and save to disk
	// bitmap bits
	unsigned char *pImageWhite = ( unsigned char * )malloc( Width * 4 * Height );

	// Get Bits from the material system
	pRenderContext->ReadPixels( 0, 0, Width, Height, pImageWhite, IMAGE_FORMAT_BGRA8888 );

	unsigned char *pBlackPos = pImageBlack;
	unsigned char *pWhitePos = pImageWhite;
	for( int y = 0; y < Height; y++ )
	{
		for( int x = 0; x < Width; x++, pBlackPos += 4, pWhitePos += 4 )
		{
			if ( ( *( pBlackPos + 0 ) ) != ( *( pWhitePos + 0 ) ) ||		// blue
				 ( *( pBlackPos + 1 ) ) != ( *( pWhitePos + 1 ) ) ||		// green
				 ( *( pBlackPos + 2 ) ) != ( *( pWhitePos + 2 ) ) )			// red
			{
				unsigned char	nBlueDiff = ( *( pBlackPos + 0 ) );
				unsigned char	nGreenDiff = ( *( pBlackPos + 1 ) );
				unsigned char	nRedDiff = ( *( pBlackPos + 2 ) );

				unsigned char	nMax = nBlueDiff;
				if ( nGreenDiff > nMax )
				{
					nMax = nGreenDiff;
				}
				if ( nRedDiff > nMax )
				{
					nMax = nRedDiff;
				}

				*( pBlackPos + 3 ) = nMax;
			}
			else
			{
				*( pBlackPos + 3 ) = 0xff;
			}
		}
	}

	static char szPathedFileName[ _MAX_PATH ];
	V_sprintf_safe( szPathedFileName, "%s%s", OutputPath, AssetName );
	V_SetExtension( szPathedFileName, ".tga", sizeof( szPathedFileName ) );

	bool bResult = SaveTgaAndAddToP4( pImageBlack, IMAGE_FORMAT_BGRA8888, Width, Height, szPathedFileName );

	free( pImageBlack );
	free( pImageWhite );

	if ( !bResult) return NULL;

	if ( bSelectedOnly )
	{
		SaveCaps( szPathedFileName );
	}

	return szPathedFileName;
}


//-----------------------------------------------------------------------------
// Will go through the asset browser and capture each visible item.
//-----------------------------------------------------------------------------
void CMDLPicker::CaptureScreenCaps( void )
{
	char		temp[ 256 ];
	TextEntry	*pTempValue;
	int			width;
	int			height;
	char		szBathPath[ _MAX_PATH ];
	Label		*m_pOutputDirectory;

	m_pOutputDirectory = ( Label * )m_pScreenCapsPage->FindChildByName( "OutputDirectory" );
	m_pOutputDirectory->GetText( szBathPath, sizeof( szBathPath ) );

	pTempValue = ( TextEntry * )m_pScreenCapsPage->FindChildByName( "WidthText" );
	pTempValue->GetText( temp, sizeof( temp ) );
	width = atoi( temp );
	pTempValue = ( TextEntry * )m_pScreenCapsPage->FindChildByName( "HeightText" );
	pTempValue->GetText( temp, sizeof( temp ) );
	height = atoi( temp );

	int		PanelX, PanelY, PanelWidth, PanelHeight;
	Color	PanelColor;

	Panel *pParent = m_pMDLPreview->GetParent();
	m_pMDLPreview->GetPos( PanelX, PanelY );
	m_pMDLPreview->GetSize( PanelWidth, PanelHeight );
	PanelColor = m_pMDLPreview->GetBackgroundColor();

	m_pMDLPreview->SetParent( ( vgui::Panel * )NULL );
	m_pMDLPreview->SetPos( 0, 0 );
	m_pMDLPreview->SetSize( width, height );

	CColorPickerButton *m_pBackgroundColor;
	m_pBackgroundColor = ( CColorPickerButton * )m_pScreenCapsPage->FindChildByName( "BackgroundColor" );
	
	Color NewPanelColor = m_pBackgroundColor->GetColor();
	NewPanelColor[3] = 0;
	m_pMDLPreview->SetBackgroundColor( NewPanelColor );
	((VPanel *)m_pMDLPreview->GetVPanel())->Solve();

	bool	bSelectedOnly = false;
	if ( input()->IsKeyDown( KEY_RCONTROL ) || input()->IsKeyDown( KEY_LCONTROL ) )
	{
		bSelectedOnly = true;
	}

	int nCount = m_AssetList.Count();
	int nNumItems = 0;
	for ( int i = 0; i < nCount; ++i )
	{
		if ( m_pAssetBrowser->IsItemVisible( m_AssetList[ i ].m_nItemId ) &&
			( !bSelectedOnly || m_pAssetBrowser->IsItemSelected( m_AssetList[ i ].m_nItemId ) ) )
		{
			KeyValues *pItemKeyValues = m_pAssetBrowser->GetItem( m_AssetList[ i ].m_nItemId );
			const char *pSelectedAsset = pItemKeyValues->GetString( "asset" );
			int			nModIndex = pItemKeyValues->GetInt( "modIndex" );

			CaptureModel( nModIndex, pSelectedAsset, szBathPath, width, height, NewPanelColor, bSelectedOnly );
			nNumItems++;
		}
	}

	m_pMDLPreview->SetParent( pParent );
	m_pMDLPreview->SetPos( PanelX, PanelY );
	m_pMDLPreview->SetSize( PanelWidth, PanelHeight );
	m_pMDLPreview->SetBackgroundColor( PanelColor );
	((VPanel *)m_pMDLPreview->GetVPanel())->Solve();

	Label		*m_pResults;

	V_sprintf_safe( temp, "Captured %d items", nNumItems );
	m_pResults = ( Label * )m_pScreenCapsPage->FindChildByName( "CaptureResults" );
	m_pResults->SetText( temp );
}


//-----------------------------------------------------------------------------
// Stub for XBox360 compiles
//-----------------------------------------------------------------------------
#if defined( _X360 )
const char *getenv( const char *varname )
{
	return NULL;
}
#endif


//-----------------------------------------------------------------------------
// Writes two very simple .vmt file, one for the passed in asset, 
// and the other for <asset>_large.
//-----------------------------------------------------------------------------
void CMDLPicker::WriteBackbackVMTFiles( const char *pAssetName )
{
	const char *pVProject = getenv( "VPROJECT" );
	if ( !pVProject )
		return;

	char pStrippedAssetName[ MAX_PATH ];
	V_StripExtension( pAssetName, pStrippedAssetName, sizeof( pStrippedAssetName ) );
	V_strcat_safe( pStrippedAssetName, GetOutputFileSuffix().Get() );

	char pVMTFilename[ MAX_PATH ];
	Q_snprintf( pVMTFilename, sizeof( pVMTFilename ), "%s\\materials\\backpack\\%s.vmt", pVProject, pStrippedAssetName );
	Q_FixSlashes( pVMTFilename );

	char pBaseTextureName[ MAX_PATH ];
	Q_snprintf( pBaseTextureName, sizeof( pBaseTextureName ), "backpack\\%s", pStrippedAssetName );
	Q_FixSlashes( pBaseTextureName );
	
	{
		CP4AutoEditAddFile autop4( pVMTFilename );
		FileHandle_t fileHandle = g_pFullFileSystem->Open( pVMTFilename, "w" );
		if ( fileHandle )
		{

			g_pFullFileSystem->FPrintf( fileHandle, "\"UnlitGeneric\"\n" );
			g_pFullFileSystem->FPrintf( fileHandle, "{\n" );
			g_pFullFileSystem->FPrintf( fileHandle, "	\"$baseTexture\" \"%s\"\n", pBaseTextureName );
			g_pFullFileSystem->FPrintf( fileHandle, "	$translucent 1\n" );
			g_pFullFileSystem->FPrintf( fileHandle, "	$vertexcolor 1\n" );
			g_pFullFileSystem->FPrintf( fileHandle, "}\n" );

			g_pFullFileSystem->Close( fileHandle );
		}
	}

	// now write the _large version
	Q_snprintf( pVMTFilename, sizeof( pVMTFilename ), "%s\\materials\\backpack\\%s_large", pVProject, pStrippedAssetName );
	V_SetExtension( pVMTFilename, ".vmt", sizeof( pVMTFilename ) );
	Q_FixSlashes( pVMTFilename );

	Q_snprintf( pBaseTextureName, sizeof( pBaseTextureName ), "backpack\\%s_large", pStrippedAssetName );
	Q_FixSlashes( pBaseTextureName );

	{
		CP4AutoEditAddFile autop4( pVMTFilename );
		FileHandle_t fileHandle = g_pFullFileSystem->Open( pVMTFilename, "w" );
		if ( fileHandle )
		{
			g_pFullFileSystem->FPrintf( fileHandle, "\"UnlitGeneric\"\n" );
			g_pFullFileSystem->FPrintf( fileHandle, "{\n" );
			g_pFullFileSystem->FPrintf( fileHandle, "	\"$baseTexture\" \"%s\"\n", pBaseTextureName );
			g_pFullFileSystem->FPrintf( fileHandle, "	$translucent 1\n" );
			g_pFullFileSystem->FPrintf( fileHandle, "}\n" );

			g_pFullFileSystem->Close( fileHandle );
		}
	}
}


//-----------------------------------------------------------------------------
void *VTexFilesystemFactory( const char *pName, int *pReturnCode )
{
	return g_pFullFileSystem;
}


void* MdlPickerFSFactory( const char *pName, int *pReturnCode )
{
	if ( IsX360() )
		return NULL;

	if ( Q_stricmp( pName, FILESYSTEM_INTERFACE_VERSION ) == 0 )
		return g_pFullFileSystem;

	return NULL;
}


//-----------------------------------------------------------------------------
// Creates the two required icons for the TF2 store/backpack system
//-----------------------------------------------------------------------------
void CMDLPicker::GenerateBackpackIcons( void )
{
	if ( !g_pVTex )
		return;

	int width;
	int	height;

	// find the index of the item we are currently viewing
	int selectedItemIndex;
	for ( selectedItemIndex = 0; selectedItemIndex < m_AssetList.Count(); ++selectedItemIndex )
	{
		if ( m_pAssetBrowser->IsItemVisible( m_AssetList[ selectedItemIndex ].m_nItemId ) &&
			m_pAssetBrowser->IsItemSelected( m_AssetList[ selectedItemIndex ].m_nItemId ) )
		{
			break;
		}
	}

	if ( selectedItemIndex >= m_AssetList.Count() )
		return;

	//
	// Fetch and check environment variables
	//
	const char *pVContent = getenv( "VCONTENT" );
	if ( !pVContent )
	{
		Error( "VCONTENT environment variable not set" );
		return;
	}

	const char *pVMod = getenv( "VMOD" );
	if ( !pVMod )
	{
		Error( "VMOD environment variable not set" );
		return;
	}

	const char *pVProject = getenv( "VPROJECT" );
	if ( !pVProject )
	{
		Error( "VPROJECT environment variable not set" );
		return;
	}

	// extract the filename of the model
	KeyValues  *pItemKeyValues = m_pAssetBrowser->GetItem( m_AssetList[ selectedItemIndex ].m_nItemId );
	const char *pSelectedAsset = pItemKeyValues->GetString( "asset" );

	// set the P4 changelist label to refer to this set of icons
	char pChangelistLabel[ MAX_PATH ];
	V_strcpy_safe( pChangelistLabel, pSelectedAsset );
	V_FileBase( pChangelistLabel, pChangelistLabel, sizeof( pChangelistLabel ) );
	V_strcat_safe( pChangelistLabel, " Auto Checkout", sizeof( pChangelistLabel ) );
	g_p4factory->SetOpenFileChangeList( pChangelistLabel );

	// generate .VMT files for normal and _large icons
	WriteBackbackVMTFiles( pSelectedAsset );

	// store original state of model preview panel
	int		PanelX, PanelY, PanelWidth, PanelHeight;
	Color	PanelColor;

	Panel *pParent = m_pMDLPreview->GetParent();
	m_pMDLPreview->GetPos( PanelX, PanelY );
	m_pMDLPreview->GetSize( PanelWidth, PanelHeight );
	PanelColor = m_pMDLPreview->GetBackgroundColor();

	// slam preview panel to desired TGA size for large icon, and write it out
	width = 512;
	height = 512;
	m_pMDLPreview->SetParent( ( vgui::Panel * )NULL );
	m_pMDLPreview->SetPos( 0, 0 );
	m_pMDLPreview->SetSize( width, height );

	CColorPickerButton *m_pBackgroundColor;
	m_pBackgroundColor = ( CColorPickerButton * )m_pScreenCapsPage->FindChildByName( "BackgroundColor" );

	Color NewPanelColor = m_pBackgroundColor->GetColor();
	NewPanelColor[3] = 0;
	m_pMDLPreview->SetBackgroundColor( NewPanelColor );
	((VPanel *)m_pMDLPreview->GetVPanel())->Solve();

	char pLargeAssetName[ MAX_PATH ];
	V_strcpy_safe( pLargeAssetName, pSelectedAsset );
	V_StripExtension( pLargeAssetName, pLargeAssetName, ARRAYSIZE( pLargeAssetName ) );

	CUtlString strExtention = GetOutputFileSuffix();
	strExtention += "_large.mdl";

	V_strcat_safe( pLargeAssetName, strExtention.String() );

	char pOutputPath[ MAX_PATH ];
	Q_snprintf( pOutputPath, sizeof( pOutputPath ), "%s\\%s\\materialsrc\\backpack\\", pVContent, pVMod );
	Q_FixSlashes( pOutputPath );

	int nModIndex = pItemKeyValues->GetInt( "modIndex" );
	const char *pLargeTGAName = CaptureModel( nModIndex, pLargeAssetName, pOutputPath, width, height, NewPanelColor, true );
	if ( !pLargeTGAName )
		return;

	// write corresponding .txt file with vtex options
	char pVTexOptionsFileName[ MAX_PATH ];
	V_strcpy_safe( pVTexOptionsFileName, pLargeTGAName );
	V_SetExtension( pVTexOptionsFileName, ".txt", sizeof( pVTexOptionsFileName ) );

	{
		CP4AutoEditAddFile autop4( pVTexOptionsFileName );
		FileHandle_t hVTexOptionsFile = g_pFullFileSystem->Open( pVTexOptionsFileName, "w" );
		if ( hVTexOptionsFile )
		{
			g_pFullFileSystem->FPrintf( hVTexOptionsFile, "nomip 1\n" );
			g_pFullFileSystem->FPrintf( hVTexOptionsFile, "nolod 1\n" );
			g_pFullFileSystem->Close( hVTexOptionsFile );
		}
	}

	// !KLUDGE! Everybody I've talked to says that vtex is *supposed* to
	// use VPROJECT.  But it doesn't.  I don't think I can safely change vtex
	// without breaking tons of stuff.  So just force the output directory.
	// Determine the proper output directory based on VPROJECT
	char pOutputPathGame[ MAX_PATH ];
	Q_strncpy( pOutputPathGame, pVProject, sizeof( pOutputPathGame ) );
	Q_StripTrailingSlash( pOutputPathGame );
	Q_strncat( pOutputPathGame, "/materials/", sizeof( pOutputPathGame ) );
	const char *pBackpack = Q_stristr( pLargeTGAName, "backpack" );
	if ( pBackpack )
	{
		Q_strncat( pOutputPathGame, pBackpack, sizeof( pOutputPathGame ) );
		Q_StripFilename( pOutputPathGame );
	}
	Q_FixSlashes( pOutputPathGame );

	// run vtex on the TGA and .txt file to create .VTF and add it to our Perforce changelist
	char *vTexArgv[64];
	int vTexArgc = 0;
	vTexArgv[ vTexArgc++ ] = "";
	vTexArgv[ vTexArgc++ ] = "-quiet";
	vTexArgv[ vTexArgc++ ] = "-UseStandardError";
	vTexArgv[ vTexArgc++ ] = "-WarningsAsErrors";
	vTexArgv[ vTexArgc++ ] = "-p4skip";
	vTexArgv[ vTexArgc++ ] = "-outdir";
	vTexArgv[ vTexArgc++ ] = pOutputPathGame;
	vTexArgv[ vTexArgc++ ] = (char *)pLargeTGAName;

	g_pVTex->VTex( MdlPickerFSFactory, pOutputPathGame, vTexArgc, vTexArgv );

	// Generale small TGA name, by removing the "large" part
	char pSmallTGAName[ MAX_PATH ];
	strcpy( pSmallTGAName, pLargeTGAName );
	char *_large = Q_stristr( pSmallTGAName, "_large");
	Assert(_large);
	strcpy(_large, _large+6);

	// Load up the large icon
	int nCheckWidth, nCheckHeight;
	ImageFormat largeFmt;
	float gamma;
	CUtlBuffer largeTGAFileData;
	CUtlMemory<unsigned char> largeTGAImageData;

	if ( !g_pFullFileSystem->ReadFile( pLargeTGAName, NULL, largeTGAFileData )
		|| !TGALoader::GetInfo( largeTGAFileData, &nCheckWidth, &nCheckHeight, &largeFmt, &gamma )
		|| nCheckWidth != width || nCheckHeight != height )
	{
		Error( "Failed to reload image header %s", pLargeTGAName );
		Assert( false );
		return;
	}

	largeTGAFileData.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
	if ( !TGALoader::LoadRGBA8888( largeTGAFileData, largeTGAImageData, nCheckWidth, nCheckHeight )
		|| nCheckWidth != width || nCheckHeight != height )
	{
		Error( "Failed to reload image data %s", pLargeTGAName );
		Assert( false );
		return;
	}

	//
	// Perform a downsample.  This is better than just re-rendering at the smaller size,
	// which essentially just point-samples the image.
	//
	CUtlMemory<unsigned char> smallTGAImageData;
	const int kSmallSize = 128;
	smallTGAImageData.EnsureCapacity(kSmallSize*kSmallSize*4);
	ImageLoader::ResampleInfo_t resampleInfo;
	resampleInfo.m_nSrcWidth = width;
	resampleInfo.m_nSrcHeight = height;
	resampleInfo.m_flSrcGamma = gamma;
	resampleInfo.m_pSrc = (unsigned char *)largeTGAImageData.Base();

	resampleInfo.m_nDestWidth = kSmallSize;
	resampleInfo.m_nDestHeight = kSmallSize;
	resampleInfo.m_flDestGamma = gamma;
	resampleInfo.m_pDest = (unsigned char *)smallTGAImageData.Base();

	resampleInfo.m_nFlags = ImageLoader::RESAMPLE_CLAMPS | ImageLoader::RESAMPLE_CLAMPT;
	//resampleInfo.m_nFlags |= ImageLoader::RESAMPLE_NICE_FILTER; // Turn this off.  It has some sort of edge enhancement or something.  Causes edges to ring.

	if ( !ImageLoader::ResampleRGBA8888( resampleInfo ) )
	{
		Error( "Failed to resample %s", pLargeTGAName );
		Assert( false );
		return;
	}

	// Save it
	if ( !SaveTgaAndAddToP4( resampleInfo.m_pDest, IMAGE_FORMAT_RGBA8888, resampleInfo.m_nDestWidth, resampleInfo.m_nDestHeight, pSmallTGAName ) )
	{
		return;
	}

	// Save the .cfg file.
	SaveCaps( pSmallTGAName );

	// write corresponding .txt file with vtex options
	V_strcpy_safe( pVTexOptionsFileName, pSmallTGAName );
	V_SetExtension( pVTexOptionsFileName, ".txt", sizeof( pVTexOptionsFileName ) );

	{
		CP4AutoEditAddFile autop4( pVTexOptionsFileName );
		FileHandle_t hVTexOptionsFile = g_pFullFileSystem->Open( pVTexOptionsFileName, "w" );
		if ( hVTexOptionsFile )
		{
			g_pFullFileSystem->FPrintf( hVTexOptionsFile, "nomip 1\n" );
			g_pFullFileSystem->FPrintf( hVTexOptionsFile, "nolod 1\n" );
			g_pFullFileSystem->Close( hVTexOptionsFile );
		}
	}

	// run vtex on the TGA and .txt file to create .VTF and add it to our Perforce changelist
	vTexArgc = 0;
	vTexArgv[ vTexArgc++ ] = "";
	vTexArgv[ vTexArgc++ ] = "-quiet";
	vTexArgv[ vTexArgc++ ] = "-UseStandardError";
	vTexArgv[ vTexArgc++ ] = "-WarningsAsErrors";
	vTexArgv[ vTexArgc++ ] = "-p4skip";
	vTexArgv[ vTexArgc++ ] = "-outdir";
	vTexArgv[ vTexArgc++ ] = pOutputPathGame;
	vTexArgv[ vTexArgc++ ] = (char *)pSmallTGAName;
	g_pVTex->VTex( MdlPickerFSFactory, pOutputPathGame, vTexArgc, vTexArgv );


	// restore the preview panel to its original state
	m_pMDLPreview->SetParent( pParent );
	m_pMDLPreview->SetPos( PanelX, PanelY );
	m_pMDLPreview->SetSize( PanelWidth, PanelHeight );
	m_pMDLPreview->SetBackgroundColor( PanelColor );
	((VPanel *)m_pMDLPreview->GetVPanel())->Solve();
}


CUtlString CMDLPicker::GetOutputFileSuffix()
{
	char temp[256];
	TextEntry *pTempValue = ( TextEntry * )m_pScreenCapsPage->FindChildByName( "SuffixText" );
	if ( pTempValue )
	{
		pTempValue->GetText( temp, sizeof( temp ) );
	}
	return temp;
}


//-----------------------------------------------------------------------------
// Saves the screen cap information and camera position
//-----------------------------------------------------------------------------
void CMDLPicker::SaveCaps( const char *szFileName )
{
	char	temp[ _MAX_PATH ];

	KeyValues *CaptureData = new KeyValues( "ScreenCaps" );

	Vector	vecPos;
	QAngle	angDir;
	m_pMDLPreview->GetCameraPositionAndAngles( vecPos, angDir );
	V_sprintf_safe( temp, "%g %g %g", vecPos.x, vecPos.y, vecPos.z );
	CaptureData->SetString( "CameraPosition", temp );
	V_sprintf_safe( temp, "%g %g %g", angDir.x, angDir.y, angDir.z );
	CaptureData->SetString( "CameraAngles", temp );

	Vector	vecOffset;
	m_pMDLPreview->GetCameraOffset( vecOffset );
	V_sprintf_safe( temp, "%g %g %g", vecOffset.x, vecOffset.y, vecOffset.z );
	CaptureData->SetString( "CameraOffset", temp );

	CColorPickerButton *m_pBackgroundColor;
	m_pBackgroundColor = ( CColorPickerButton * )m_pScreenCapsPage->FindChildByName( "BackgroundColor" );
	Color	color = m_pBackgroundColor->GetColor();

	V_sprintf_safe( temp, "%d %d %d %d", color.r(), color.g(), color.b(), color.a() );
	CaptureData->SetString( "BackgroundColor", temp );

	TextEntry	*pTempValue;
	pTempValue = ( TextEntry * )m_pScreenCapsPage->FindChildByName( "WidthText" );
	pTempValue->GetText( temp, sizeof( temp ) );
	CaptureData->SetString( "Width", temp );

	pTempValue = ( TextEntry * )m_pScreenCapsPage->FindChildByName( "HeightText" );
	pTempValue->GetText( temp, sizeof( temp ) );
	CaptureData->SetString( "Height", temp );

	vgui::CheckButton *pToggle;
	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName( "NoGround" );
	CaptureData->SetInt( "NoGround", pToggle->IsSelected() ? 1 : 0 );

	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName( "Collision" );
	CaptureData->SetInt( "Collision", pToggle->IsSelected() ? 1 : 0 );

	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName( "Wireframe" );
	CaptureData->SetInt( "Wifeframe", pToggle->IsSelected() ? 1 : 0 );

	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName( "LockView" );
	CaptureData->SetInt( "LockView", pToggle->IsSelected() ? 1 : 0 );

	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName( "LookAtCamera" );
	CaptureData->SetInt( "LookAtCamera", pToggle->IsSelected() ? 1 : 0 );

	for( int i = 1; i < MAX_SELECTED_MODELS; i++ )
	{
		if ( m_hSelectedMDL[ i ] != MDLHANDLE_INVALID )
		{
			const char *MergedModelName = vgui::MDLCache()->GetModelName( m_hSelectedMDL[ i ] );
			V_sprintf_safe( temp, "Merged_%d", i );
			CaptureData->SetString( temp, MergedModelName );
		}
	}

	if ( szFileName != NULL )
	{
		strcpy( temp, szFileName );
		V_SetExtension( temp, ".cfg", sizeof( temp ) );
	}
	else
	{
		Label	*m_pOutputDirectory = ( Label * )m_pScreenCapsPage->FindChildByName( "OutputDirectory" );
		m_pOutputDirectory->GetText( temp, sizeof( temp ) );

		strcat( temp, "ScreenCaps.cfg" );
	}

	CaptureData->SaveToFile( g_pFullFileSystem, temp );
	CP4AutoAddFile autop4( temp );
}


//-----------------------------------------------------------------------------
// Restores the screen cap information and camera position
//-----------------------------------------------------------------------------
bool CMDLPicker::RestoreCaps( const char *szFileName )
{
	char	temp[ _MAX_PATH ];

	if ( szFileName != NULL )
	{
		strcpy( temp, szFileName );
		V_SetExtension( temp, ".cfg", sizeof( temp ) );
	}
	else
	{
		Label	*m_pOutputDirectory = ( Label * )m_pScreenCapsPage->FindChildByName( "OutputDirectory" );
		m_pOutputDirectory->GetText( temp, sizeof( temp ) );
		strcat( temp, "ScreenCaps.cfg" );
	}

	KeyValues *CaptureData = new KeyValues( "ScreenCaps" );

	if ( !CaptureData->LoadFromFile( g_pFullFileSystem, temp ) )
	{
		return false;
	}

	Vector	vecPos;
	QAngle	angDir;
	Vector	vecOffset;
	sscanf( CaptureData->GetString( "CameraPosition" ), "%g %g %g", &vecPos.x, &vecPos.y, &vecPos.z );
	sscanf( CaptureData->GetString( "CameraAngles" ), "%g %g %g", &angDir.x, &angDir.y, &angDir.z );
	sscanf( CaptureData->GetString( "CameraOffset" ), "%g %g %g", &vecOffset.x, &vecOffset.y, &vecOffset.z );

	m_pMDLPreview->SetCameraOffset( vecOffset );
	m_pMDLPreview->SetCameraPositionAndAngles( vecPos, angDir );

	CColorPickerButton *m_pBackgroundColor;
	int		r, g, b, a;
	m_pBackgroundColor = ( CColorPickerButton * )m_pScreenCapsPage->FindChildByName( "BackgroundColor" );
	sscanf( CaptureData->GetString( "BackgroundColor" ), "%d %d %d %d", &r, &g, &b, &a );
	m_pBackgroundColor->SetColor( r, g, b, a );

	TextEntry	*pTempValue;
	pTempValue = ( TextEntry * )m_pScreenCapsPage->FindChildByName( "WidthText" );
	pTempValue->SetText( CaptureData->GetString( "Width" ) );

	pTempValue = ( TextEntry * )m_pScreenCapsPage->FindChildByName( "HeightText" );
	pTempValue->SetText( CaptureData->GetString( "Height" ) );


	vgui::CheckButton *pToggle;
	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName( "NoGround" );
	pToggle->SetSelected( ( CaptureData->GetInt( "NoGround" ) == 1 ) );

	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName( "Collision" );
	pToggle->SetSelected( ( CaptureData->GetInt( "Collision" ) == 1 ) );

	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName( "Wireframe" );
	pToggle->SetSelected( ( CaptureData->GetInt( "Wireframe" ) == 1 ) );

	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName( "LockView" );
	pToggle->SetSelected( ( CaptureData->GetInt( "LockView" ) == 1 ) );

	pToggle = (vgui::CheckButton*)m_pRenderPage->FindChildByName( "LookAtCamera" );
	pToggle->SetSelected( ( CaptureData->GetInt( "LookAtCamera" ) == 1 ) );

	for( int i = 1; i < MAX_SELECTED_MODELS; i++ )
	{
		V_sprintf_safe( temp, "Merged_%d", i );
		const char *MergedModelName = CaptureData->GetString( temp, NULL );
		if ( MergedModelName )
		{
			SelectMDL( MergedModelName, false, i );
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: rebuilds the list of activities
//-----------------------------------------------------------------------------
void CMDLPicker::RefreshActivitiesAndSequencesList()
{
	m_pActivitiesList->RemoveAll();
	m_pSequencesList->RemoveAll();
	m_pMDLPreview->SetSequence( 0 );

	if ( m_hSelectedMDL[ 0 ] == MDLHANDLE_INVALID )
	{
		m_pActivitiesList->SetEmptyListText("No .MDL file currently selected");
		m_pSequencesList->SetEmptyListText("No .MDL file currently selected");
		return;
	}

	m_pActivitiesList->SetEmptyListText(".MDL file contains no activities");
	m_pSequencesList->SetEmptyListText(".MDL file contains no sequences");

	studiohdr_t *hdr = vgui::MDLCache()->GetStudioHdr( m_hSelectedMDL[ 0 ] );
	
	CUtlDict<int, unsigned short> activityNames( true, 0, hdr->GetNumSeq() );

	for (int j = 0; j < hdr->GetNumSeq(); j++)
	{
		if ( /*g_viewerSettings.showHidden ||*/ !(hdr->pSeqdesc(j).flags & STUDIO_HIDDEN))
		{
			const char *pActivityName = hdr->pSeqdesc(j).pszActivityName();
			if ( pActivityName && pActivityName[0] )
			{
				// Multiple sequences can have the same activity name; only add unique activity names
				if ( activityNames.Find( pActivityName ) == activityNames.InvalidIndex() )
				{
					KeyValues *pkv = new KeyValues("node", "activity", pActivityName );
					int nItemID = m_pActivitiesList->AddItem( pkv, 0, false, false );

					KeyValues *pDrag = new KeyValues( "drag", "text", pActivityName );
					pDrag->SetString( "texttype", "activityName" );
					pDrag->SetString( "mdl", vgui::MDLCache()->GetModelName( m_hSelectedMDL[ 0 ] ) );
					m_pActivitiesList->SetItemDragData( nItemID, pDrag );

					activityNames.Insert( pActivityName, j );
				}
			}

			const char *pSequenceName = hdr->pSeqdesc(j).pszLabel();
			if ( pSequenceName && pSequenceName[0] )
			{
				KeyValues *pkv = new KeyValues("node", "sequence", pSequenceName);
				int nItemID = m_pSequencesList->AddItem( pkv, 0, false, false );

				KeyValues *pDrag = new KeyValues( "drag", "text", pSequenceName );
				pDrag->SetString( "texttype", "sequenceName" );
				pDrag->SetString( "mdl", vgui::MDLCache()->GetModelName( m_hSelectedMDL[ 0 ] ) );
				m_pSequencesList->SetItemDragData( nItemID, pDrag );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// A MDL was selected
//-----------------------------------------------------------------------------
void CMDLPicker::OnSelectedAssetPicked( const char *pMDLName )
{
	char pRelativePath[MAX_PATH];

	int nSelectSecondary = -1;
	if ( input()->IsKeyDown( KEY_RCONTROL ) || input()->IsKeyDown( KEY_LCONTROL ) )
	{
		nSelectSecondary = 0;
	}
	else if ( input()->IsMouseDown(MOUSE_RIGHT) )
	{
		nSelectSecondary = 1;
	}

	if ( pMDLName )
	{
		Q_snprintf( pRelativePath, sizeof(pRelativePath), "models\\%s", pMDLName );
		SelectMDL( pRelativePath, true, nSelectSecondary );
	}
	else
	{
		SelectMDL( NULL, true, nSelectSecondary );
	}
}


//-----------------------------------------------------------------------------
// Allows external apps to select a MDL
//-----------------------------------------------------------------------------
void CMDLPicker::SelectMDL( const char *pRelativePath, bool bDoLookAt, int nSelectSecondary )
{
	MDLHandle_t hSelectedMDL = pRelativePath ? vgui::MDLCache()->FindMDL( pRelativePath ) : MDLHANDLE_INVALID;
	int			index = ( nSelectSecondary > 0 ? nSelectSecondary : 0 );

	// We didn't change models after all...
	if ( hSelectedMDL == m_hSelectedMDL[ index ] )
	{
		// vgui::MDLCache()->FindMDL adds a reference by default we don't use, release it again
		if ( hSelectedMDL != MDLHANDLE_INVALID )
		{
			vgui::MDLCache()->Release( hSelectedMDL );
		}
		return;
	}

	m_hSelectedMDL[ index ] = hSelectedMDL;

	if ( vgui::MDLCache()->IsErrorModel( m_hSelectedMDL[ index ] ) )
	{
		m_hSelectedMDL[ index ] = MDLHANDLE_INVALID;
	}
	if ( nSelectSecondary != -1 )
	{
		m_pMDLPreview->ClearMergeMDLs();
		for( int i = 1; i < MAX_SELECTED_MODELS; i++ )
		{
			if ( i != index )
			{
				m_hSelectedMDL[ i ] = MDLHANDLE_INVALID;
			}
		}
	}

	if ( index > 0 )
	{
		m_pMDLPreview->SetMergeMDL( m_hSelectedMDL[ index ] );
	}
	else
	{
		m_pMDLPreview->SetMDL( m_hSelectedMDL[ index ] );

		if ( bDoLookAt )
		{
			m_pMDLPreview->LookAtMDL();
		}

		if ( m_nFlags & ( PAGE_SKINS ) )
		{
			UpdateSkinsList();
		}

		if ( m_nFlags & ( PAGE_INFO ) )
		{
			UpdateInfoTab();
		}

		if ( m_nFlags & (PAGE_ACTIVITIES|PAGE_SEQUENCES) )
		{
			RefreshActivitiesAndSequencesList();
		}
	}

	// vgui::MDLCache()->FindMDL adds a reference by default we don't use, release it again
	if ( hSelectedMDL != MDLHANDLE_INVALID )
	{
		vgui::MDLCache()->Release( hSelectedMDL );
	}

	PostActionSignal( new KeyValues( "MDLPreviewChanged", "mdl", pRelativePath ? pRelativePath : "" ) );
}


//-----------------------------------------------------------------------------
// Purpose: updates revision view on a file being selected
//-----------------------------------------------------------------------------
void CMDLPicker::OnCheckButtonChecked(KeyValues *kv)
{
//    RefreshMDLList();
	BaseClass::OnCheckButtonChecked( kv );
	RefreshRenderSettings();
}


void CMDLPicker::GetSelectedMDLName( char *pBuffer, int nMaxLen )
{
	Assert( nMaxLen > 0 );
	if ( GetSelectedAssetCount() > 0 )
	{
		Q_snprintf( pBuffer, nMaxLen, "models\\%s", GetSelectedAsset( ) );
	}
	else
	{
		pBuffer[0] = 0;
	}
}
	
//-----------------------------------------------------------------------------
// Gets the selected activity/sequence
//-----------------------------------------------------------------------------
int CMDLPicker::GetSelectedPage( )
{
	if ( m_pSequencesPage && ( m_pViewsSheet->GetActivePage() == m_pSequencesPage ) )
		return PAGE_SEQUENCES;

	if ( m_pActivitiesPage && ( m_pViewsSheet->GetActivePage() == m_pActivitiesPage ) )
		return PAGE_ACTIVITIES;

	return PAGE_NONE;
}

const char *CMDLPicker::GetSelectedSequenceName()
{
	if ( !m_pSequencesPage  )
		return NULL;

	int nIndex = m_pSequencesList->GetSelectedItem( 0 );
	if ( nIndex >= 0 )
	{
		KeyValues *pkv = m_pSequencesList->GetItem( nIndex );
		return pkv->GetString( "sequence", NULL );
	}

	return NULL;
}

const char *CMDLPicker::GetSelectedActivityName()
{
	if ( !m_pActivitiesPage  )
		return NULL;

	int nIndex = m_pActivitiesList->GetSelectedItem( 0 );
	if ( nIndex >= 0 )
	{
		KeyValues *pkv = m_pActivitiesList->GetItem( nIndex );
		return pkv->GetString( "activity", NULL );
	}
	return NULL;
}

int	CMDLPicker::GetSelectedSkin()
{
	if ( !m_pSkinsPage )
		return 0;

	int nIndex = m_pSkinsList->GetSelectedItem( 0 );
	if ( nIndex >= 0 )
	{
		return nIndex;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Plays the selected activity
//-----------------------------------------------------------------------------
void CMDLPicker::SelectActivity( const char *pActivityName )
{
	studiohdr_t *pstudiohdr = vgui::MDLCache()->GetStudioHdr( m_hSelectedMDL[ 0 ] );
	for ( int i = 0; i < pstudiohdr->GetNumSeq(); i++ )
	{
		mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( i );
		if ( stricmp( seqdesc.pszActivityName(), pActivityName ) == 0 )
		{
			// FIXME: Add weighted sequence selection logic?
			m_pMDLPreview->SetSequence( i );
			break;
		}
	}

	PostActionSignal( new KeyValues( "SequenceSelectionChanged", "activity", pActivityName ) );
}


//-----------------------------------------------------------------------------
// Plays the selected sequence
//-----------------------------------------------------------------------------
void CMDLPicker::SelectSequence( const char *pSequenceName )
{
	studiohdr_t *pstudiohdr = vgui::MDLCache()->GetStudioHdr( m_hSelectedMDL[ 0 ] );
	for (int i = 0; i < pstudiohdr->GetNumSeq(); i++)
	{
		mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( i );
		if ( !Q_stricmp( seqdesc.pszLabel(), pSequenceName ) )
		{
			m_pMDLPreview->SetSequence( i );
			break;
		}
	}

	PostActionSignal( new KeyValues( "SequenceSelectionChanged", "sequence", pSequenceName ) );
}

void CMDLPicker::SelectSkin( int nSkin )
{
	m_pMDLPreview->SetSkin( nSkin );
	PostActionSignal( new KeyValues( "SkinSelectionChanged", "skin", nSkin));
}

	
//-----------------------------------------------------------------------------
// Purpose: Updates preview when an item is selected
//-----------------------------------------------------------------------------
void CMDLPicker::OnItemSelected( KeyValues *kv )
{
	Panel *pPanel = (Panel *)kv->GetPtr("panel", NULL);
	if ( m_pSequencesList && (pPanel == m_pSequencesList ) )
	{
		const char *pSequenceName = GetSelectedSequenceName();
		if ( pSequenceName )
		{
			SelectSequence( pSequenceName );
		}
		return;
	}

	if ( m_pActivitiesList && ( pPanel == m_pActivitiesList ) )
	{
		const char *pActivityName = GetSelectedActivityName();
		if ( pActivityName )
		{
			SelectActivity( pActivityName );
		}
		return;
	}

	if ( m_pSkinsList && ( pPanel == m_pSkinsList ) )
	{
		int nSelectedSkin = GetSelectedSkin();
		SelectSkin( nSelectedSkin );
	
		return;
	}

	BaseClass::OnItemSelected( kv );
}


//-----------------------------------------------------------------------------
// Purpose: Called when a page is shown
//-----------------------------------------------------------------------------
void CMDLPicker::OnPageChanged( )
{
	if ( m_pSequencesPage && ( m_pViewsSheet->GetActivePage() == m_pSequencesPage ) )
	{
		m_pSequencesList->RequestFocus();

		const char *pSequenceName = GetSelectedSequenceName();

		if ( pSequenceName )
		{
			SelectSequence( pSequenceName );
		}
		return;
	}
	
	if ( m_pActivitiesPage && ( m_pViewsSheet->GetActivePage() == m_pActivitiesPage ) )
	{
		m_pActivitiesList->RequestFocus();

		const char *pActivityName = GetSelectedActivityName();

		if ( pActivityName )
		{
			SelectActivity( pActivityName );
		}
		return;
	}
}


//-----------------------------------------------------------------------------
//
// Purpose: Modal picker frame
//
//-----------------------------------------------------------------------------
CMDLPickerFrame::CMDLPickerFrame( vgui::Panel *pParent, const char *pTitle, int nFlags ) : 
	BaseClass( pParent )
{
	SetAssetPicker( new CMDLPicker( this, nFlags ) );
	LoadControlSettingsAndUserConfig( "resource/mdlpickerframe.res" );
	SetTitle( pTitle, false );
}

CMDLPickerFrame::~CMDLPickerFrame()
{
}


//-----------------------------------------------------------------------------
// Allows external apps to select a MDL
//-----------------------------------------------------------------------------
void CMDLPickerFrame::SelectMDL( const char *pRelativePath )
{
	static_cast<CMDLPicker*>( GetAssetPicker() )->SelectMDL( pRelativePath );
}

int CMDLPicker::UpdateSkinsList()
{
	int nNumSkins = 0;

	if ( m_pSkinsList )
	{
		m_pSkinsList->RemoveAll();

		studiohdr_t *hdr = vgui::MDLCache()->GetStudioHdr( m_hSelectedMDL[ 0 ] );
		if ( hdr )
		{
			nNumSkins = hdr->numskinfamilies;
			for ( int i = 0; i < nNumSkins; i++ )
			{
				char skinText[25] = "";
				V_sprintf_safe( skinText, "skin%i", i );
				KeyValues *pkv = new KeyValues("node", "skin", skinText );
				m_pSkinsList->AddItem( pkv, 0, false, false );
			}
		}
	}
		
	return nNumSkins;
}

void CMDLPicker::UpdateInfoTab()
{
	studiohdr_t *hdr = vgui::MDLCache()->GetStudioHdr( m_hSelectedMDL[ 0 ] );
	if ( !hdr )
		return;
	
	int nMass = hdr->mass;
	Panel *pTempPanel = m_pInfoPage->FindChildByName("MassValue");
	char massBuff[10];
	Q_snprintf( massBuff, 10, "%d", nMass );
	((vgui::Label *)pTempPanel)->SetText( massBuff );
	bool bIsStatic = hdr->flags & STUDIOHDR_FLAGS_STATIC_PROP;
	bool bIsPhysics = false;
	const char* buf = hdr->KeyValueText();
	Label * pTempLabel = (Label *)m_pInfoPage->FindChildByName("StaticText");
	pTempLabel->SetVisible( false );
	if( buf )
	{
		buf = Q_strstr( buf, "prop_data" );
		if ( buf )
		{
			int iPropDataCount = UpdatePropDataList( buf, bIsStatic );
			if( iPropDataCount )
			{
				bIsPhysics = true;
			}
		}
		else
		{
			m_pPropDataList->RemoveAll();
		}
	}
	else
	{
		m_pPropDataList->RemoveAll();
	}
	
	CheckButton * pTempCheck = (CheckButton *)m_pInfoPage->FindChildByName("StaticObject");
	pTempCheck->SetCheckButtonCheckable( true );
	pTempCheck->SetSelected( bIsStatic );
	pTempCheck->SetCheckButtonCheckable( false );
	pTempCheck = (CheckButton *)m_pInfoPage->FindChildByName("PhysicsObject");
	pTempCheck->SetCheckButtonCheckable( true );
	pTempCheck->SetSelected( bIsPhysics );
	pTempCheck->SetCheckButtonCheckable( false );
	pTempCheck = (CheckButton *)m_pInfoPage->FindChildByName("DynamicObject");
	pTempCheck->SetCheckButtonCheckable( true );
	pTempCheck->SetSelected( !bIsPhysics );
	pTempCheck->SetCheckButtonCheckable( false );


}

int CMDLPicker::UpdatePropDataList( const char* pszPropData, bool &bIsStatic )
{
	int iCount = 0;  

	if ( m_pPropDataList )
	{
		m_pPropDataList->RemoveAll();

		const char * endPropData = strchr( pszPropData, '}' );
		char keyText[255] = "";
		char valueText[255] = "";
		const char *beginChunk = strchr( pszPropData, '\"' );
		if ( !beginChunk )
		{
			return 0;
		}
		beginChunk++;
		const char *endChunk = strchr( beginChunk, '\"' );
		while( endChunk )
		{
			Q_memcpy( keyText, beginChunk, endChunk - beginChunk );
			beginChunk = endChunk + 1;
			beginChunk = strchr( beginChunk, '\"' ) + 1;
			endChunk = strchr( beginChunk, '\"' );
			Q_memcpy( valueText, beginChunk, endChunk - beginChunk );		
			if( !Q_strcmp( keyText, "allowstatic" ) && !Q_strcmp( valueText , "1" ) )
			{
				if ( !bIsStatic )
				{					
					Label * pTempLabel = (Label *)m_pInfoPage->FindChildByName("StaticText");
					pTempLabel->SetVisible( true );
				}
				bIsStatic &= true;
			}
			KeyValues *pkv = new KeyValues("node", "key", keyText, "value", valueText );
			m_pPropDataList->AddItem( pkv, 0, false, false );
			Q_memset( keyText, 0, 255 );
			Q_memset( valueText, 0, 255 );
			iCount++;
			beginChunk = endChunk + 1;
			beginChunk = strchr( beginChunk, '\"' );
			if ( !beginChunk || beginChunk > endPropData )
			{
				return iCount;
			}
			beginChunk++;
			endChunk = strchr( beginChunk, '\"' );		
		}
	}
	return iCount;
}
