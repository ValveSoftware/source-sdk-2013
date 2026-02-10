//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "filesystem.h"
#include "matsys_controls/vmtpicker.h"
#include "matsys_controls/vmtpreviewpanel.h"
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
CVMTPicker::CVMTPicker( vgui::Panel *pParent, bool bAllowMultiselect ) : 
	BaseClass( pParent, "VMT Files", "vmt", "materials", "vmtName" )
{
	// Horizontal splitter for preview
	m_pPreviewSplitter = new Splitter( this, "PreviewSplitter", SPLITTER_MODE_VERTICAL, 1 );
	vgui::Panel *pSplitterLeftSide = m_pPreviewSplitter->GetChild( 0 );
	vgui::Panel *pSplitterRightSide = m_pPreviewSplitter->GetChild( 1 );

	m_p2D3DSplitter = new Splitter( pSplitterRightSide, "2D3DSplitter", SPLITTER_MODE_HORIZONTAL, 1 );
 	vgui::Panel *pSplitterTopSide = m_p2D3DSplitter->GetChild( 0 );
	vgui::Panel *pSplitterBottomSide = m_p2D3DSplitter->GetChild( 1 );

	// VMT preview
	m_pVMTPreview2D = new CVMTPreviewPanel( pSplitterTopSide, "VMTPreview2D" );
	m_pVMTPreview3D = new CVMTPreviewPanel( pSplitterBottomSide, "VMTPreview3D" );
	m_pVMTPreview3D->DrawIn3DMode( true );

	// Standard browser controls
 	CreateStandardControls( pSplitterLeftSide, bAllowMultiselect );

	LoadControlSettingsAndUserConfig( "resource/vmtpicker.res" );
}

CVMTPicker::~CVMTPicker()
{
}


//-----------------------------------------------------------------------------
// Derived classes have this called when the previewed asset changes
//-----------------------------------------------------------------------------
void CVMTPicker::OnSelectedAssetPicked( const char *pAssetName )
{
	m_pVMTPreview2D->SetVMT( pAssetName );
	m_pVMTPreview3D->SetVMT( pAssetName );
}


//-----------------------------------------------------------------------------
//
// Purpose: Modal picker frame
//
//-----------------------------------------------------------------------------
CVMTPickerFrame::CVMTPickerFrame( vgui::Panel *pParent, const char *pTitle, bool bAllowMultiselect ) : 
	BaseClass( pParent )
{
	SetAssetPicker( new CVMTPicker( this, bAllowMultiselect ) );
	LoadControlSettingsAndUserConfig( "resource/vmtpickerframe.res" );
	SetTitle( pTitle, false );
}


	
