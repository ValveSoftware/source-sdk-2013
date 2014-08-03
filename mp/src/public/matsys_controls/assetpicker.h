//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ASSETPICKER_H
#define ASSETPICKER_H
#ifdef _WIN32
#pragma once
#endif

#include "matsys_controls/baseassetpicker.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace vgui
{
	class Panel;
}


//-----------------------------------------------------------------------------
// Purpose: Base class for choosing raw assets
//-----------------------------------------------------------------------------
class CAssetPicker : public CBaseAssetPicker
{
	DECLARE_CLASS_SIMPLE( CAssetPicker, CBaseAssetPicker );

public:
	CAssetPicker( vgui::Panel *pParent, const char *pAssetType, 
		const char *pExt, const char *pSubDir, const char *pTextType );
};


//-----------------------------------------------------------------------------
// Purpose: Modal dialog for asset picker
//-----------------------------------------------------------------------------
class CAssetPickerFrame : public CBaseAssetPickerFrame
{
	DECLARE_CLASS_SIMPLE( CAssetPickerFrame, CBaseAssetPickerFrame );

public:
	CAssetPickerFrame( vgui::Panel *pParent, const char *pTitle, 
		const char *pAssetType, const char *pExt, const char *pSubDir, const char *pTextType );
};


#endif // ASSETPICKER_H
