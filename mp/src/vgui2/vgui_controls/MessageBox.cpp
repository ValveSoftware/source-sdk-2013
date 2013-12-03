//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui/IInput.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/MessageBox.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

vgui::Panel *MessageBox_Factory()
{
	return new MessageBox("MessageBox", "MessageBoxText");
}

DECLARE_BUILD_FACTORY_CUSTOM( MessageBox, MessageBox_Factory );
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
MessageBox::MessageBox(const char *title, const char *text, Panel *parent) : Frame(parent, NULL, false)
{
	SetTitle(title, true);
	m_pMessageLabel = new Label(this, NULL, text);

	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
MessageBox::MessageBox(const wchar_t *wszTitle, const wchar_t *wszText, Panel *parent) : Frame(parent, NULL, false)
{	
	SetTitle(wszTitle, true);
	m_pMessageLabel = new Label(this, NULL, wszText);

	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor Helper
//-----------------------------------------------------------------------------
void MessageBox::Init()
{
	SetDeleteSelfOnClose(true);
	m_pFrameOver = NULL;
	m_bShowMessageBoxOverCursor = false;

	SetMenuButtonResponsive(false);
	SetMinimizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	
	m_pOkButton = new Button(this, NULL, "#MessageBox_OK");
	m_pOkButton->SetCommand( "OnOk" );
	m_pOkButton->AddActionSignalTarget(this);

	m_pCancelButton = new Button(this, NULL, "#MessageBox_Cancel");
	m_pCancelButton->SetCommand( "OnCancel" );
	m_pCancelButton->AddActionSignalTarget(this);
	m_pCancelButton->SetVisible( false );

	m_OkCommand = m_CancelCommand = NULL;
	m_bNoAutoClose = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
MessageBox::~MessageBox()
{
	if ( m_OkCommand )
	{
		m_OkCommand->deleteThis();
	}
	if ( m_CancelCommand )
	{
		m_CancelCommand->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Shows the message box over the cursor
//-----------------------------------------------------------------------------
void MessageBox::ShowMessageBoxOverCursor( bool bEnable )
{
	m_bShowMessageBoxOverCursor = bEnable;
}


//-----------------------------------------------------------------------------
// Purpose: size the message label properly
//-----------------------------------------------------------------------------
void MessageBox::OnCommand( const char *pCommand )
{
	if ( vgui::input()->GetAppModalSurface() == GetVPanel() )
	{
		vgui::input()->ReleaseAppModalSurface();
	}

	if ( !Q_stricmp( pCommand, "OnOk" ) )
	{
		if ( m_OkCommand )
		{
			PostActionSignal(m_OkCommand->MakeCopy());
		}
	}
	else if ( !Q_stricmp( pCommand, "OnCancel" ) )
	{
		if ( m_CancelCommand )
		{
			PostActionSignal(m_CancelCommand->MakeCopy());
		}
	}

	if ( !m_bNoAutoClose )
	{
		OnShutdownRequest();
	}
}

	
//-----------------------------------------------------------------------------
// Purpose: size the message label properly
//-----------------------------------------------------------------------------
void MessageBox::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int wide, tall;
	m_pMessageLabel->GetContentSize(wide, tall);
	m_pMessageLabel->SetSize(wide, tall);

	wide += 100;
	tall += 100;
	SetSize(wide, tall);

	if ( m_bShowMessageBoxOverCursor )
	{
		PlaceUnderCursor();
		return;
	}

	// move to the middle of the screen
	if ( m_pFrameOver )
	{
		int frameX, frameY;
		int frameWide, frameTall;
		m_pFrameOver->GetPos(frameX, frameY);
		m_pFrameOver->GetSize(frameWide, frameTall);

		SetPos((frameWide - wide) / 2 + frameX, (frameTall - tall) / 2 + frameY);
	}
	else
	{
		int swide, stall;
		surface()->GetScreenSize(swide, stall);
		// put the dialog in the middle of the screen
		SetPos((swide - wide) / 2, (stall - tall) / 2);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Put the message box into a modal state
//			Does not suspend execution - use addActionSignal to get return value
//-----------------------------------------------------------------------------
void MessageBox::DoModal(Frame* pFrameOver)
{
    ShowWindow(pFrameOver);
/*
	// move to the middle of the screen
	// get the screen size
	int wide, tall;
	// get our dialog size
	GetSize(wide, tall);

	if (pFrameOver)
	{
		int frameX, frameY;
		int frameWide, frameTall;
		pFrameOver->GetPos(frameX, frameY);
		pFrameOver->GetSize(frameWide, frameTall);

		SetPos((frameWide - wide) / 2 + frameX, (frameTall - tall) / 2 + frameY);
	}
	else
	{
		int swide, stall;
		surface()->GetScreenSize(swide, stall);
		// put the dialog in the middle of the screen
		SetPos((swide - wide) / 2, (stall - tall) / 2);
	}

	SetVisible( true );
	SetEnabled( true );
	MoveToFront();

	if (m_pOkButton->IsVisible())
		m_pOkButton->RequestFocus();
	else	 // handle message boxes with no button
		RequestFocus();
*/
	input()->SetAppModalSurface(GetVPanel());
}

void MessageBox::ShowWindow(Frame *pFrameOver)
{
	m_pFrameOver = pFrameOver;

	SetVisible( true );
	SetEnabled( true );
	MoveToFront();

	if ( m_pOkButton->IsVisible() )
	{
		m_pOkButton->RequestFocus();
	}
	else	 // handle message boxes with no button
	{
		RequestFocus();
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Put the text and OK buttons in correct place
//-----------------------------------------------------------------------------
void MessageBox::PerformLayout()
{	
	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);
	wide += x;
	tall += y;

	int boxWidth, boxTall;
	GetSize(boxWidth, boxTall);

	int oldWide, oldTall;
	m_pOkButton->GetSize(oldWide, oldTall);
	
	int btnWide, btnTall;
	m_pOkButton->GetContentSize(btnWide, btnTall);
	btnWide = max(oldWide, btnWide + 10);
	btnTall = max(oldTall, btnTall + 10);
	m_pOkButton->SetSize(btnWide, btnTall);

	int btnWide2 = 0, btnTall2 = 0;
	if ( m_pCancelButton->IsVisible() )
	{
		m_pCancelButton->GetSize(oldWide, oldTall);
		
		m_pCancelButton->GetContentSize(btnWide2, btnTall2);
		btnWide2 = max(oldWide, btnWide2 + 10);
		btnTall2 = max(oldTall, btnTall2 + 10);
		m_pCancelButton->SetSize(btnWide2, btnTall2);
	}

	boxWidth = max(boxWidth, m_pMessageLabel->GetWide() + 100);
	boxWidth = max(boxWidth, (btnWide + btnWide2) * 2 + 30);
	SetSize(boxWidth, boxTall);

	GetSize(boxWidth, boxTall);

	m_pMessageLabel->SetPos((wide/2)-(m_pMessageLabel->GetWide()/2) + x, y + 5 );
	if ( !m_pCancelButton->IsVisible() )
	{
		m_pOkButton->SetPos((wide/2)-(m_pOkButton->GetWide()/2) + x, tall - m_pOkButton->GetTall() - 15);
	}
	else
	{
		m_pOkButton->SetPos((wide/4)-(m_pOkButton->GetWide()/2) + x, tall - m_pOkButton->GetTall() - 15);
		m_pCancelButton->SetPos((3*wide/4)-(m_pOkButton->GetWide()/2) + x, tall - m_pOkButton->GetTall() - 15);
	}

	BaseClass::PerformLayout();
	GetSize(boxWidth, boxTall);
}


//-----------------------------------------------------------------------------
// Purpose: Set a string command to be sent when the OK button is pressed.
//-----------------------------------------------------------------------------
void MessageBox::SetCommand(const char *command)
{
	if (m_OkCommand)
	{
		m_OkCommand->deleteThis();
	}
	m_OkCommand = new KeyValues("Command", "command", command);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the command
//-----------------------------------------------------------------------------
void MessageBox::SetCommand(KeyValues *command)
{
	if (m_OkCommand)
	{
		m_OkCommand->deleteThis();
	}
	m_OkCommand = command;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void MessageBox::OnShutdownRequest()
{
	// Shutdown the dialog
	PostMessage(this, new KeyValues("Close"));
}

//-----------------------------------------------------------------------------
// Purpose: Set the visibility of the OK button.
//-----------------------------------------------------------------------------
void MessageBox::SetOKButtonVisible(bool state)
{
	m_pOkButton->SetVisible(state);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the Text on the OK button
//-----------------------------------------------------------------------------
void MessageBox::SetOKButtonText(const char *buttonText)
{
	m_pOkButton->SetText(buttonText);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the Text on the OK button
//-----------------------------------------------------------------------------
void MessageBox::SetOKButtonText(const wchar_t *wszButtonText)
{
	m_pOkButton->SetText(wszButtonText);
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Cancel button (off by default)
//-----------------------------------------------------------------------------
void MessageBox::SetCancelButtonVisible(bool state)
{
	m_pCancelButton->SetVisible(state);
	InvalidateLayout();
}

void MessageBox::SetCancelButtonText(const char *buttonText)
{
	m_pCancelButton->SetText(buttonText);
	InvalidateLayout();
}

void MessageBox::SetCancelButtonText(const wchar_t *wszButtonText)
{
	m_pCancelButton->SetText(wszButtonText);
	InvalidateLayout();
}

void MessageBox::SetCancelCommand( KeyValues *command )
{
	if (m_CancelCommand)
	{
		m_CancelCommand->deleteThis();
	}
	m_CancelCommand = command;
}

	
//-----------------------------------------------------------------------------
// Purpose: Toggles visibility of the close box.
//-----------------------------------------------------------------------------
void MessageBox::DisableCloseButton(bool state)
{
	BaseClass::SetCloseButtonVisible(state);
	m_bNoAutoClose = true;
}
