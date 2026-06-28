//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "matsys_controls/assetpicker.h"


using namespace vgui;


//-----------------------------------------------------------------------------
//
// Asset Picker with no preview
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CAssetPicker::CAssetPicker( vgui::Panel *pParent, const char *pAssetType, 
	const char *pExt, const char *pSubDir, const char *pTextType ) : 
	BaseClass( pParent, pAssetType, pExt, pSubDir, pTextType )
{
	CreateStandardControls( this );
	LoadControlSettingsAndUserConfig( "resource/assetpicker.res" );
}

	
//-----------------------------------------------------------------------------
//
// Purpose: Modal picker frame
//
//-----------------------------------------------------------------------------
CAssetPickerFrame::CAssetPickerFrame( vgui::Panel *pParent, const char *pTitle, 
	const char *pAssetType, const char *pExt, const char *pSubDir, const char *pTextType ) : 
	BaseClass( pParent )
{
	SetAssetPicker( new CAssetPicker( this, pAssetType, pExt, pSubDir, pTextType ) );
	LoadControlSettingsAndUserConfig( "resource/assetpickerframe.res" );
	SetTitle( pTitle, false );
}


	
