//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef VTFPICKER_H
#define VTFPICKER_H
#ifdef _WIN32
#pragma once
#endif

#include "matsys_controls/baseassetpicker.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CVTFPreviewPanel;

namespace vgui
{
	class Splitter;
}


//-----------------------------------------------------------------------------
// Purpose: Base class for choosing raw assets
//-----------------------------------------------------------------------------
class CVTFPicker : public CBaseAssetPicker
{
	DECLARE_CLASS_SIMPLE( CVTFPicker, CBaseAssetPicker );

public:
	CVTFPicker( vgui::Panel *pParent );
	virtual ~CVTFPicker();

private:
	// Derived classes have this called when the previewed asset changes
	virtual void OnSelectedAssetPicked( const char *pAssetName );

	CVTFPreviewPanel *m_pVTFPreview;
	vgui::Splitter *m_pPreviewSplitter;
};


//-----------------------------------------------------------------------------
// Purpose: Modal dialog for asset picker
//-----------------------------------------------------------------------------
class CVTFPickerFrame : public CBaseAssetPickerFrame
{
	DECLARE_CLASS_SIMPLE( CVTFPickerFrame, CBaseAssetPickerFrame );

public:
	CVTFPickerFrame( vgui::Panel *pParent, const char *pTitle );
};


#endif // VTFPICKER_H
