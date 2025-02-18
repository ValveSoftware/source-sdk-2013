//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef VMTPICKER_H
#define VMTPICKER_H
#ifdef _WIN32
#pragma once
#endif

#include "matsys_controls/baseassetpicker.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CVMTPreviewPanel;

namespace vgui
{
	class Splitter;
}


//-----------------------------------------------------------------------------
// Purpose: Base class for choosing raw assets
//-----------------------------------------------------------------------------
class CVMTPicker : public CBaseAssetPicker
{
	DECLARE_CLASS_SIMPLE( CVMTPicker, CBaseAssetPicker );

public:
	CVMTPicker( vgui::Panel *pParent, bool bAllowMultiselect = false );
	virtual ~CVMTPicker();

private:
	// Derived classes have this called when the previewed asset changes
	virtual void OnSelectedAssetPicked( const char *pAssetName );

	CVMTPreviewPanel *m_pVMTPreview2D;
	CVMTPreviewPanel *m_pVMTPreview3D;
	vgui::Splitter *m_p2D3DSplitter;
	vgui::Splitter *m_pPreviewSplitter;
};


//-----------------------------------------------------------------------------
// Purpose: Modal dialog for asset picker
//-----------------------------------------------------------------------------
class CVMTPickerFrame : public CBaseAssetPickerFrame
{
	DECLARE_CLASS_SIMPLE( CVMTPickerFrame, CBaseAssetPickerFrame );

public:
	CVMTPickerFrame( vgui::Panel *pParent, const char *pTitle, bool bAllowMultiselect = false );
};


#endif // VMTPICKER_H
