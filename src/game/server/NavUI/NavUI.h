//--------------------------------------------------------------------------------------------------------
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef NAVUI_H
#define NAVUI_H

#ifdef SERVER_USES_VGUI

#include "KeyValues.h"
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/CheckButton.h>
#include <vgui/ILocalize.h>
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "fmtstr.h"
#include <vgui_controls/CheckButton.h>


//--------------------------------------------------------------------------------------------------------
void GetNavUIEditVectors( Vector *pos, Vector *forward );


//--------------------------------------------------------------------------------------------------------
class CNavUIButton : public vgui::Button
{
	DECLARE_CLASS_SIMPLE( CNavUIButton, vgui::Button );

public:
	CNavUIButton( Panel *parent, const char *name ) : vgui::Button( parent, name, "" )
	{
		m_hideKey = BUTTON_CODE_INVALID;
	}

	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnKeyCodeReleased( vgui::KeyCode code );

private:
	void LookupKey( void );

	ButtonCode_t m_hideKey;
	IntervalTimer m_hidePressedTimer;
};


//--------------------------------------------------------------------------------------------------------
class CNavUITextEntry : public vgui::TextEntry
{
	DECLARE_CLASS_SIMPLE( CNavUIButton, vgui::TextEntry );

public:
	CNavUITextEntry( Panel *parent, const char *name ) : vgui::TextEntry( parent, name )
	{
		m_hideKey = BUTTON_CODE_INVALID;
	}

	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnKeyCodeReleased( vgui::KeyCode code );

private:
	void LookupKey( void );

	ButtonCode_t m_hideKey;
	IntervalTimer m_hidePressedTimer;
};


//--------------------------------------------------------------------------------------------------------
class CNavUIComboBox : public vgui::ComboBox
{
	DECLARE_CLASS_SIMPLE( CNavUIComboBox, vgui::ComboBox );

public:
	CNavUIComboBox( Panel *parent, const char *name, int numLines, bool editable ) : vgui::ComboBox( parent, name, numLines, editable )
	{
		m_hideKey = BUTTON_CODE_INVALID;
	}

	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnKeyCodeReleased( vgui::KeyCode code );

private:
	void LookupKey( void );

	ButtonCode_t m_hideKey;
	IntervalTimer m_hidePressedTimer;
};


//--------------------------------------------------------------------------------------------------------
class CNavUICheckButton : public vgui::CheckButton
{
	DECLARE_CLASS_SIMPLE( CNavUICheckButton, vgui::CheckButton );

public:
	CNavUICheckButton( Panel *parent, const char *name, const char *text ) : vgui::CheckButton( parent, name, text )
	{
		m_hideKey = BUTTON_CODE_INVALID;
	}

	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnKeyCodeReleased( vgui::KeyCode code );

private:
	void LookupKey( void );

	ButtonCode_t m_hideKey;
	IntervalTimer m_hidePressedTimer;
};


//--------------------------------------------------------------------------------------------------------
class CNavUIToolPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CNavUIToolPanel, vgui::EditablePanel );

public:
	CNavUIToolPanel( vgui::Panel *parent, const char *toolName ) : vgui::EditablePanel( parent, toolName )
	{
	}

	virtual void Init( void )
	{
	}

	virtual void Shutdown( void )
	{
	}

	virtual vgui::Panel *CreateControlByName( const char *controlName );

	virtual void StartLeftClickAction( const char *actionName )
	{
	}

	virtual void FinishLeftClickAction( const char *actionName )
	{
	}

	virtual void StartRightClickAction( const char *actionName )
	{
	}

protected:
	bool IsCheckButtonChecked( const char *name );
};


//--------------------------------------------------------------------------------------------------------
class CNavUIBasePanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CNavUIBasePanel, vgui::Frame );

public:
	CNavUIBasePanel();
	virtual ~CNavUIBasePanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	virtual void PerformLayout( void );
	virtual void PaintBackground( void );

	// GameUI panels are always visible by default, so here we hide ourselves if the GameUI is up.
	virtual void OnTick( void );

	void ToggleVisibility( void );
	virtual Panel *CreateControlByName( const char *controlName );

	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnKeyCodeReleased( vgui::KeyCode code );
	virtual void OnMousePressed( vgui::MouseCode code );

	virtual void OnCursorMoved( int x, int y );
	virtual void OnCursorEntered( void );
	virtual void OnCursorExited( void );
	virtual void OnMouseReleased( vgui::MouseCode code );

	void SetLeftClickAction( const char *action, const char *text );
	const char *GetLeftClickAction( void ) const
	{
		return m_leftClickAction;
	}

	void PlaySound( const char *sound );

protected:
	const char *ActiveToolName( void ) const;
	void ActivateTool( const char *toolName );
	virtual CNavUIToolPanel *CreateTool( const char *toolName, vgui::Panel *toolParent );

private:
	bool m_hidden;

	bool m_dragSelecting;
	bool m_dragUnselecting;

	CNavUIToolPanel *m_toolPanel;
	CNavUIToolPanel *m_selectionPanel;

	void LookupKey( void );

	ButtonCode_t m_hideKey;
	IntervalTimer m_hidePressedTimer;

	CountdownTimer m_audioTimer;

	char m_leftClickAction[ 64 ];
	bool m_performingLeftClickAction;
};


extern CNavUIBasePanel *CreateNavUI( void );
extern CNavUIBasePanel *TheNavUI( void );

#endif // SERVER_USES_VGUI

#endif // NAVUI_H
