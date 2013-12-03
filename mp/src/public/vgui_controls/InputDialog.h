//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Controls.h>
#include <vgui_controls/Frame.h>

namespace vgui
{

class Label;
class Button;
class TextEntry;


//-----------------------------------------------------------------------------
// Purpose: Utility dialog base class - just has context kv and ok/cancel buttons
//-----------------------------------------------------------------------------
class BaseInputDialog : public Frame
{
	DECLARE_CLASS_SIMPLE( BaseInputDialog, Frame );

public:
	BaseInputDialog( vgui::Panel *parent, const char *title );
	~BaseInputDialog();

	void DoModal( KeyValues *pContextKeyValues = NULL );

protected:
	virtual void PerformLayout();
	virtual void PerformLayout( int x, int y, int w, int h ) {}

	// command buttons
	virtual void OnCommand( const char *command );

	void CleanUpContextKeyValues();
	KeyValues		*m_pContextKeyValues;

private:
	vgui::Button	*m_pCancelButton;
	vgui::Button	*m_pOKButton;
};

//-----------------------------------------------------------------------------
// Purpose: Utility dialog, used to ask yes/no questions of the user
//-----------------------------------------------------------------------------
class InputMessageBox : public BaseInputDialog
{
	DECLARE_CLASS_SIMPLE( InputMessageBox, BaseInputDialog );

public:
	InputMessageBox( vgui::Panel *parent, const char *title, char const *prompt );
	~InputMessageBox();

protected:
	virtual void PerformLayout( int x, int y, int w, int h );

private:
	vgui::Label			*m_pPrompt;
};

//-----------------------------------------------------------------------------
// Purpose: Utility dialog, used to let user type in some text
//-----------------------------------------------------------------------------
class InputDialog : public BaseInputDialog
{
	DECLARE_CLASS_SIMPLE( InputDialog, BaseInputDialog );

public:
	InputDialog( vgui::Panel *parent, const char *title, char const *prompt, char const *defaultValue = "" );
	~InputDialog();

	void SetMultiline( bool state );

	/* action signals

		"InputCompleted"
			"text"	- the text entered

		"InputCanceled"
	*/
	void AllowNumericInputOnly( bool bOnlyNumeric );

protected:
	virtual void PerformLayout( int x, int y, int w, int h );

	// command buttons
	virtual void OnCommand(const char *command);

private:
	vgui::Label			*m_pPrompt;
	vgui::TextEntry		*m_pInput;
};

} // namespace vgui


#endif // INPUTDIALOG_H
