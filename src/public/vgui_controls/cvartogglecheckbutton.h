//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CVARTOGGLECHECKBUTTON_H
#define CVARTOGGLECHECKBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui/VGUI.h"
#include "vgui/IVGui.h"
#include "vgui_controls/CheckButton.h"
#include "tier1/utlstring.h"
#include "tier1/KeyValues.h"

namespace vgui
{

template< class T >
class CvarToggleCheckButton : public CheckButton
{
	DECLARE_CLASS_SIMPLE( CvarToggleCheckButton, CheckButton );

public:
	CvarToggleCheckButton( Panel *parent, const char *panelName, const char *text = "", 
		char const *cvarname = NULL, bool ignoreMissingCvar = false );
	~CvarToggleCheckButton();

	virtual void	SetSelected( bool state );

	virtual void	Paint();

	void			Reset();
	void			ApplyChanges();
	bool			HasBeenModified();
	virtual void	ApplySettings( KeyValues *inResourceData );

private:
	// Called when the OK / Apply button is pressed.  Changed data should be written into cvar.
	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" );
	MESSAGE_FUNC( OnButtonChecked, "CheckButtonChecked" );

	T			m_cvar;
	bool			m_bStartValue;
	bool			m_bIgnoreMissingCvar;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
template< class T >
CvarToggleCheckButton<T>::CvarToggleCheckButton( Panel *parent, const char *panelName, const char *text, char const *cvarname, bool ignoreMissingCvar )
	: CheckButton( parent, panelName, text ), m_cvar( g_pVGui->GetVGUIEngine(), (cvarname)?cvarname:"", (cvarname)?ignoreMissingCvar:true )
{
	m_bIgnoreMissingCvar = ignoreMissingCvar;

	if (m_cvar.IsValid())
	{
		Reset();
	}
	AddActionSignalTarget( this );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
template< class T >
CvarToggleCheckButton<T>::~CvarToggleCheckButton()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template< class T >
void CvarToggleCheckButton<T>::Paint()
{
	if ( !m_cvar.IsValid() ) 
	{
		BaseClass::Paint();
		return;
	}

	bool value = m_cvar.GetBool();

	if ( value != m_bStartValue )
	{
		SetSelected( value );
		m_bStartValue = value;
	}
	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the OK / Apply button is pressed.  Changed data should be written into cvar.
//-----------------------------------------------------------------------------
template< class T >
void CvarToggleCheckButton<T>::OnApplyChanges()
{
	ApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template< class T >
void CvarToggleCheckButton<T>::ApplyChanges()
{
	if ( !m_cvar.IsValid() ) 
		return;

	m_bStartValue = IsSelected();
	m_cvar.SetValue( m_bStartValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template< class T >
void CvarToggleCheckButton<T>::Reset()
{
	if ( !m_cvar.IsValid() ) 
		return;

	m_bStartValue = m_cvar.GetBool();
	SetSelected(m_bStartValue);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template< class T >
bool CvarToggleCheckButton<T>::HasBeenModified()
{
	return IsSelected() != m_bStartValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *panel - 
//-----------------------------------------------------------------------------
template< class T >
void CvarToggleCheckButton<T>::SetSelected( bool state )
{
	BaseClass::SetSelected( state );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template< class T >
void CvarToggleCheckButton<T>::OnButtonChecked()
{
	if (HasBeenModified())
	{
		PostActionSignal(new KeyValues("ControlModified"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template< class T >
void CvarToggleCheckButton<T>::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	const char *cvarName = inResourceData->GetString("cvar_name", "");
	const char *cvarValue = inResourceData->GetString("cvar_value", "");

	if( Q_stricmp( cvarName, "") == 0 )
		return;// Doesn't have cvar set up in res file, must have been constructed with it.

	if( Q_stricmp( cvarValue, "1") == 0 )
		m_bStartValue = true;
	else
		m_bStartValue = false;

	m_cvar.Init( g_pVGui->GetVGUIEngine(), cvarName, m_bIgnoreMissingCvar );
	if ( m_cvar.IsValid() )
	{
		SetSelected( m_cvar.GetBool() );
	}
}

} // namespace vgui

#endif // CVARTOGGLECHECKBUTTON_H
