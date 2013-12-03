//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <vgui_controls/InputDialog.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/TextEntry.h>
#include "tier1/KeyValues.h"
#include "vgui/IInput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
BaseInputDialog::BaseInputDialog( vgui::Panel *parent, const char *title ) :
	BaseClass( parent, NULL )
{
	m_pContextKeyValues = NULL;

	SetDeleteSelfOnClose( true );
	SetTitle(title, true);
	SetSize(320, 180);
	SetSizeable( false );

	m_pCancelButton = new Button(this, "CancelButton", "#VGui_Cancel");
	m_pOKButton = new Button(this, "OKButton", "#VGui_OK");
	m_pCancelButton->SetCommand("Cancel");
	m_pOKButton->SetCommand("OK");
	m_pOKButton->SetAsDefaultButton( true );

	if ( parent )
	{
		AddActionSignalTarget( parent );
	}
}

BaseInputDialog::~BaseInputDialog()
{
	CleanUpContextKeyValues();
}

//-----------------------------------------------------------------------------
// Purpose: Cleans up the keyvalues
//-----------------------------------------------------------------------------
void BaseInputDialog::CleanUpContextKeyValues()
{
	if ( m_pContextKeyValues )
	{
		m_pContextKeyValues->deleteThis();
		m_pContextKeyValues = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BaseInputDialog::DoModal( KeyValues *pContextKeyValues )
{
	CleanUpContextKeyValues();
	m_pContextKeyValues = pContextKeyValues;
	BaseClass::DoModal();
}

//-----------------------------------------------------------------------------
// Purpose: lays out controls
//-----------------------------------------------------------------------------
void BaseInputDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h;
	GetSize( w, h );

	// lay out all the controls
	int topy = IsSmallCaption() ? 15 : 30;
	int halfw = w / 2;

	PerformLayout( 12, topy, w - 24, h - 100 );

	m_pOKButton->SetBounds( halfw - 84, h - 30, 72, 24 );
	m_pCancelButton->SetBounds( halfw + 12, h - 30, 72, 24 );
}


//-----------------------------------------------------------------------------
// Purpose: handles button commands
//-----------------------------------------------------------------------------
void BaseInputDialog::OnCommand(const char *command)
{
	KeyValues *kv = NULL;
	if ( !stricmp( command, "OK" ) )
	{
		kv = new KeyValues( "InputCompleted" );
		kv->SetPtr( "dialog", this );
	}
	else if ( !stricmp( command, "Cancel" ) )
	{
		kv = new KeyValues( "InputCanceled" );
	}
	else
	{
		BaseClass::OnCommand( command );
		return;
	}

	if ( m_pContextKeyValues )
	{
		kv->AddSubKey( m_pContextKeyValues );
		m_pContextKeyValues = NULL;
	}
	PostActionSignal( kv );
	CloseModal();
}


//-----------------------------------------------------------------------------
// Purpose: Utility dialog, used to ask yes/no questions of the user
//-----------------------------------------------------------------------------
InputMessageBox::InputMessageBox( vgui::Panel *parent, const char *title, char const *prompt )
: BaseClass( parent, title )
{
	SetSize( 320, 120 );

	m_pPrompt = new Label( this, "Prompt", prompt );
}

InputMessageBox::~InputMessageBox()
{
}

void InputMessageBox::PerformLayout( int x, int y, int w, int h )
{
	m_pPrompt->SetBounds( x, y, w, 24 );
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
InputDialog::InputDialog(vgui::Panel *parent, const char *title, char const *prompt, char const *defaultValue /*=""*/ ) : 
	BaseClass(parent, title)
{
	SetSize( 320, 120 );

	m_pPrompt = new Label( this, "Prompt", prompt );
	
	m_pInput = new TextEntry( this, "Text" );
	m_pInput->SetText( defaultValue );
	m_pInput->SelectAllText( true );
	m_pInput->RequestFocus();
}


InputDialog::~InputDialog()
{
}


//-----------------------------------------------------------------------------
// Sets the dialog to be multiline
//-----------------------------------------------------------------------------
void InputDialog::SetMultiline( bool state )
{
	m_pInput->SetMultiline( state );
	m_pInput->SetCatchEnterKey( state );
}

	
//-----------------------------------------------------------------------------
// Allow numeric input only
//-----------------------------------------------------------------------------
void InputDialog::AllowNumericInputOnly( bool bOnlyNumeric )
{
	if ( m_pInput )
	{
		m_pInput->SetAllowNumericInputOnly( bOnlyNumeric );
	}
}


//-----------------------------------------------------------------------------
// Purpose: lays out controls
//-----------------------------------------------------------------------------
void InputDialog::PerformLayout( int x, int y, int w, int h )
{
	m_pPrompt->SetBounds( x, y, w, 24 );
	m_pInput ->SetBounds( x, y + 30, w, m_pInput->IsMultiline() ? h - 30 : 24 );
}


//-----------------------------------------------------------------------------
// Purpose: handles button commands
//-----------------------------------------------------------------------------
void InputDialog::OnCommand(const char *command)
{
	// overriding OnCommand for backwards compatability
	// it'd be nice at some point to find all uses of InputDialog and just use BaseInputDialog's OnCommand

	if (!stricmp(command, "OK"))
	{
		int nTextLength = m_pInput->GetTextLength() + 1;
		char* txt = (char*)_alloca( nTextLength * sizeof(char) );
		m_pInput->GetText( txt, nTextLength );
		KeyValues *kv = new KeyValues( "InputCompleted", "text", txt );
		if ( m_pContextKeyValues )
		{
			kv->AddSubKey( m_pContextKeyValues );
			m_pContextKeyValues = NULL;
		}
		PostActionSignal( kv );
		CloseModal();
	}
	else if (!stricmp(command, "Cancel"))
	{
		KeyValues *kv = new KeyValues( "InputCanceled" );
		if ( m_pContextKeyValues )
		{
			kv->AddSubKey( m_pContextKeyValues );
			m_pContextKeyValues = NULL;
		}
		PostActionSignal( kv );
		CloseModal();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}
