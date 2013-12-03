//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MOUSEOVERPANELBUTTON_H
#define MOUSEOVERPANELBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui_controls/Button.h>
#include <vgui/KeyCode.h>
#include <filesystem.h>

extern vgui::Panel *g_lastPanel;
extern vgui::Button *g_lastButton;

//-----------------------------------------------------------------------------
// Purpose: Triggers a new panel when the mouse goes over the button
//    
// the new panel has the same dimensions as the passed templatePanel and is of
// the same class.
//
// must at least inherit from vgui::EditablePanel to support LoadControlSettings
//-----------------------------------------------------------------------------
template <class T>
class MouseOverButton : public vgui::Button
{
private:
	DECLARE_CLASS_SIMPLE( MouseOverButton, vgui::Button );
	
public:
	MouseOverButton(vgui::Panel *parent, const char *panelName, T *templatePanel ) :
					Button( parent, panelName, "MouseOverButton")
	{
		m_pPanel = new T( parent, NULL );
		m_pPanel ->SetVisible( false );

		// copy size&pos from template panel
		int x,y,wide,tall;
		templatePanel->GetBounds( x, y, wide, tall );
		m_pPanel->SetBounds( x, y, wide, tall );
		int px, py;
		templatePanel->GetPinOffset( px, py );
		int rx, ry;
		templatePanel->GetResizeOffset( rx, ry );
		// Apply pin settings from template, too
		m_pPanel->SetAutoResize( templatePanel->GetPinCorner(), templatePanel->GetAutoResize(), px, py, rx, ry );

		m_bPreserveArmedButtons = false;
		m_bUpdateDefaultButtons = false;
	}

	virtual void SetPreserveArmedButtons( bool bPreserve ){ m_bPreserveArmedButtons = bPreserve; }
	virtual void SetUpdateDefaultButtons( bool bUpdate ){ m_bUpdateDefaultButtons = bUpdate; }

	virtual void ShowPage()
	{
		if( m_pPanel )
		{
			m_pPanel->SetVisible( true );
			m_pPanel->MoveToFront();
			g_lastPanel = m_pPanel;
		}
	}
	
	virtual void HidePage()
	{
		if ( m_pPanel )
		{
			m_pPanel->SetVisible( false );
		}
	}

	const char *GetClassPage( const char *className )
	{
		static char classPanel[ _MAX_PATH ];
		Q_snprintf( classPanel, sizeof( classPanel ), "classes/%s.res", className);

		if ( g_pFullFileSystem->FileExists( classPanel, IsX360() ? "MOD" : "GAME" ) )
		{
		}
		else if (g_pFullFileSystem->FileExists( "classes/default.res", IsX360() ? "MOD" : "GAME" ) )
		{
			Q_snprintf ( classPanel, sizeof( classPanel ), "classes/default.res" );
		}
		else
		{
			return NULL;
		}

		return classPanel;
	}

#ifdef REFRESH_CLASSMENU_TOOL

	void RefreshClassPage( void )
	{
		m_pPanel->LoadControlSettings( GetClassPage( GetName() ) );
	}

#endif

	virtual void ApplySettings( KeyValues *resourceData ) 
	{
		BaseClass::ApplySettings( resourceData );

		// name, position etc of button is set, now load matching
		// resource file for associated info panel:
		m_pPanel->LoadControlSettings( GetClassPage( GetName() ) );
	}		

	T *GetClassPanel( void ) { return m_pPanel; }

	virtual void OnCursorExited()
	{
		if ( !m_bPreserveArmedButtons )
		{
			BaseClass::OnCursorExited();
		}
	}

	virtual void OnCursorEntered() 
	{
		BaseClass::OnCursorEntered();

		if ( !IsEnabled() )
			return;

		// are we updating the default buttons?
		if ( m_bUpdateDefaultButtons )
		{
			SetAsDefaultButton( 1 );
		}

		// are we preserving the armed state (and need to turn off the old button)?
		if ( m_bPreserveArmedButtons )
		{
			if ( g_lastButton && g_lastButton != this )
			{
				g_lastButton->SetArmed( false );
			}

			g_lastButton = this;
		}

		// turn on our panel (if it isn't already)
		if ( m_pPanel && ( !m_pPanel->IsVisible() ) )
		{
			// turn off the previous panel
			if ( g_lastPanel && g_lastPanel->IsVisible() )
			{
				g_lastPanel->SetVisible( false );
			}

			ShowPage();
		}
	}

	virtual void OnKeyCodeReleased( vgui::KeyCode code )
	{
		BaseClass::OnKeyCodeReleased( code );

		if ( m_bPreserveArmedButtons )
		{
			if ( g_lastButton )
			{
				g_lastButton->SetArmed( true );
			}
		}
	}

private:

	T *m_pPanel;
	bool m_bPreserveArmedButtons;
	bool m_bUpdateDefaultButtons;
};

#define MouseOverPanelButton MouseOverButton<vgui::EditablePanel>

#endif // MOUSEOVERPANELBUTTON_H
