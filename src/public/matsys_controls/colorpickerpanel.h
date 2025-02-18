//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef COLORPICKERPANEL_H
#define COLORPICKERPANEL_H

#ifdef _WIN32
#pragma once
#endif


#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/Button.h"
#include "bitmap/imageformat.h"
#include "mathlib/vector.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CColorXYPreview;
class CColorZPreview;

namespace vgui
{
	class RadioButton;
	class TextEntry;
	class IScheme;
}


//-----------------------------------------------------------------------------
//
// Color picker panel
//
//-----------------------------------------------------------------------------
class CColorPickerPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CColorPickerPanel, vgui::EditablePanel );

public:
	// constructor
	CColorPickerPanel( vgui::Panel *pParent, const char *pName );
	void SetInitialColor( Color initialColor );
	void GetCurrentColor( Color *pColor );
	void GetInitialColor( Color *pColor );

	// Inherited from Panel
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnMousePressed( vgui::MouseCode code );

private:
	MESSAGE_FUNC_PARAMS( OnRadioButtonChecked, "RadioButtonChecked", kv );
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );
	MESSAGE_FUNC_PARAMS( OnHSVSelected, "HSVSelected", data );
	MESSAGE_FUNC_PARAMS( OnColorSelected, "ColorSelected", data );

	// Called when the color changes
	void OnColorChanged( vgui::TextEntry *pChanged = NULL );

	// Updates the preview colors
	void UpdatePreviewColors();

	CColorXYPreview *m_pColorXYPreview;
	CColorZPreview *m_pColorZPreview;
	vgui::RadioButton* m_pHueRadio;
	vgui::RadioButton* m_pSaturationRadio;
	vgui::RadioButton* m_pValueRadio;
	vgui::RadioButton* m_pRedRadio;
	vgui::RadioButton* m_pGreenRadio;
	vgui::RadioButton* m_pBlueRadio;
	vgui::TextEntry* m_pHueText;
	vgui::TextEntry* m_pSaturationText;
	vgui::TextEntry* m_pValueText;
	vgui::TextEntry* m_pRedText;
	vgui::TextEntry* m_pGreenText;
	vgui::TextEntry* m_pBlueText;
	vgui::Panel* m_pInitialColor;
	vgui::Panel* m_pCurrentColor;
	vgui::TextEntry* m_pAlphaText;

	RGB888_t m_InitialColor;
	RGB888_t m_CurrentColor;
	unsigned char m_InitialAlpha;
	unsigned char m_CurrentAlpha;
	Vector m_CurrentHSVColor;
};


//-----------------------------------------------------------------------------
// Purpose: Modal dialog for picker
//-----------------------------------------------------------------------------
class CColorPickerFrame : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CColorPickerFrame, vgui::Frame );

public:
	CColorPickerFrame( vgui::Panel *pParent, const char *pTitle );
	~CColorPickerFrame();

	// Inherited from Frame
	virtual void OnCommand( const char *pCommand );

	// Purpose: Activate the dialog
	// If a color is picked, the message 'ColorPickerPicked' is sent with the "color" field set to the color
	// If cancel is hit, the message 'ColorPickerCancel' is sent
	// If the color is changed in the preview, the message 'ColorPickerPreview' is sent with the "color" field set to the color
	void DoModal( Color initialColor, KeyValues *pContextKeys = NULL );

	// Gets the initial color
	void GetInitialColor( Color *pColor );

private:
	void CleanUpMessage();

	CColorPickerPanel *m_pPicker;
	vgui::Button *m_pOpenButton;
	vgui::Button *m_pCancelButton;
	KeyValues *m_pContextKeys;
};


//-----------------------------------------------------------------------------
// Purpose: A button which brings up the color picker
//-----------------------------------------------------------------------------
class CColorPickerButton : public vgui::Button
{
	DECLARE_CLASS_SIMPLE( CColorPickerButton, vgui::Button );

	/*
	NOTE: Sends ColorPickerPicked message when a color is picked
			color - picked color
		  Sends ColorPickerPreview message when a color is previewed
				color - current preview color
		  Sends ColorPickerCancelled message when the cancel button was hit
				startingColor - color before the picking occurred
	*/

public:
	CColorPickerButton( vgui::Panel *pParent, const char *pName, vgui::Panel *pActionSignalTarget = NULL );
	~CColorPickerButton();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void DoClick();

	void	SetColor( const Color& clr );
	void	SetColor( int r, int g, int b, int a );
	Color	GetColor( void ) { return m_CurrentColor; }

private:
	MESSAGE_FUNC_PARAMS( OnPicked, "ColorPickerPicked", data );
	MESSAGE_FUNC_PARAMS( OnPreview, "ColorPickerPreview", data );
	MESSAGE_FUNC( OnCancelled, "ColorPickerCancel" );

	void UpdateButtonColor();
	Color m_CurrentColor;
};

#endif // COLORPICKERPANEL_H
