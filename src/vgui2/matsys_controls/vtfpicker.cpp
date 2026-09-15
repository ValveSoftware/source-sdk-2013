//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "filesystem.h"
#include "matsys_controls/vtfpicker.h"
#include "matsys_controls/vtfpreviewpanel.h"
#include "vgui_controls/Splitter.h"


using namespace vgui;


//-----------------------------------------------------------------------------
//
// Asset Picker with no preview
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CVTFPicker::CVTFPicker( vgui::Panel *pParent ) : 
	BaseClass( pParent, "VTF Files", "vtf", "materials", "vtfName" )
{
	// Horizontal splitter for preview
	m_pPreviewSplitter = new Splitter( this, "PreviewSplitter", SPLITTER_MODE_VERTICAL, 1 );
	vgui::Panel *pSplitterLeftSide = m_pPreviewSplitter->GetChild( 0 );
	vgui::Panel *pSplitterRightSide = m_pPreviewSplitter->GetChild( 1 );

	// VTF preview
	m_pVTFPreview = new CVTFPreviewPanel( pSplitterRightSide, "VTFPreview" );

	// Standard browser controls
 	CreateStandardControls( pSplitterLeftSide );

	LoadControlSettingsAndUserConfig( "resource/vtfpicker.res" );
}

CVTFPicker::~CVTFPicker()
{
}


//-----------------------------------------------------------------------------
// Derived classes have this called when the previewed asset changes
//-----------------------------------------------------------------------------
void CVTFPicker::OnSelectedAssetPicked( const char *pAssetName )
{
	m_pVTFPreview->SetVTF( pAssetName );
}


//-----------------------------------------------------------------------------
//
// Purpose: Modal picker frame
//
//-----------------------------------------------------------------------------
CVTFPickerFrame::CVTFPickerFrame( vgui::Panel *pParent, const char *pTitle ) : 
	BaseClass( pParent )
{
	SetAssetPicker( new CVTFPicker( this ) );
	LoadControlSettingsAndUserConfig( "resource/vtfpickerframe.res" );
	SetTitle( pTitle, false );
}


	
