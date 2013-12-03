//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
// This class is a message box that has two buttons, ok and cancel instead of
// just the ok button of a message box. We use a message box class for the ok button
// and implement another button here.
//
// $NoKeywords: $
//=============================================================================//

#include <vgui/KeyCode.h>

#include <vgui_controls/QueryBox.h>
#include <vgui_controls/TextImage.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
QueryBox::QueryBox(const char *title, const char *queryText, vgui::Panel *parent) : MessageBox(title, queryText,parent)
{
	SetDeleteSelfOnClose(true);
	m_pCancelButton = new Button(this, "CancelButton", "#QueryBox_Cancel");
	m_pCancelButton->SetCommand("Cancel"); 
	m_pOkButton->SetCommand("OK");
	m_pCancelCommand = NULL;
	m_pOkCommand = NULL;

	m_pOkButton->SetTabPosition(1);
	m_pCancelButton->SetTabPosition(2);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
QueryBox::QueryBox(const wchar_t *wszTitle, const wchar_t *wszQueryText,vgui::Panel *parent) : MessageBox(wszTitle, wszQueryText,parent)
{
	SetDeleteSelfOnClose(true);
	m_pCancelButton = new Button(this, "CancelButton", "#QueryBox_Cancel");
	m_pCancelButton->SetCommand("Cancel"); 
	m_pOkButton->SetCommand("OK");
	m_pCancelCommand = NULL;
	m_pOkCommand = NULL;

	m_pOkButton->SetTabPosition(1);
	m_pCancelButton->SetTabPosition(2);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
QueryBox::~QueryBox()
{
	delete m_pCancelButton;

	if ( m_pOkCommand )
	{
		m_pOkCommand->deleteThis();
	}
	if ( m_pCancelCommand )
	{
		m_pCancelCommand->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Layout the window for drawing 
//-----------------------------------------------------------------------------
void QueryBox::PerformLayout()
{
	BaseClass::PerformLayout();

	int boxWidth, boxTall;
	GetSize(boxWidth, boxTall);

	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);
	wide += x;
	tall += y;

	int oldWide, oldTall;
	m_pCancelButton->GetSize(oldWide, oldTall);
	
	int btnWide, btnTall;
	m_pCancelButton->GetContentSize(btnWide, btnTall);
	btnWide = max(oldWide, btnWide + 10);
	btnTall = max(oldTall, btnTall + 10);
	m_pCancelButton->SetSize(btnWide, btnTall);

//nt boxWidth, boxTall;
	GetSize(boxWidth, boxTall);
//	wide = max(wide, btnWide * 2 + 100);
//	SetSize(wide, tall);

	m_pOkButton->SetPos((wide/2)-(m_pOkButton->GetWide())-1 + x, tall - m_pOkButton->GetTall() - 15);
	m_pCancelButton->SetPos((wide/2) + x+16, tall - m_pCancelButton->GetTall() - 15);

}

//-----------------------------------------------------------------------------
// Purpose: Handles command text from the buttons
//			Deletes self when closed
//-----------------------------------------------------------------------------
void QueryBox::OnCommand(const char *command)
{
	if (!stricmp(command, "OK"))
	{
		OnCommand("Close");

		if ( m_pOkCommand )
		{
			PostActionSignal(m_pOkCommand->MakeCopy());
		}
	}
	else if (!stricmp(command, "Cancel"))
	{
		OnCommand("Close");	

		if (m_pCancelCommand)
		{
			PostActionSignal(m_pCancelCommand->MakeCopy());
		}
	}
	
	BaseClass::OnCommand(command);
	
}

//-----------------------------------------------------------------------------
// Purpose: Set the keyvalues to send when ok button is hit
//-----------------------------------------------------------------------------
void QueryBox::SetOKCommand(KeyValues *keyValues)
{
	if ( m_pOkCommand )
	{
		m_pOkCommand->deleteThis();
	}

	m_pOkCommand = keyValues;
}

//-----------------------------------------------------------------------------
// Purpose: Set a value of the ok command
//-----------------------------------------------------------------------------
void QueryBox::SetOKCommandValue(const char *keyName, int value)
{
	if ( !m_pOkCommand )
	{
		m_pOkCommand = new KeyValues("Command");
	}

	m_pOkCommand->SetInt(keyName, value);
}

//-----------------------------------------------------------------------------
// Purpose: Set the keyvalues to send when the cancel button is hit
//-----------------------------------------------------------------------------
void QueryBox::SetCancelCommand(KeyValues *keyValues)
{
	if ( m_pCancelCommand )
	{
		m_pCancelCommand->deleteThis();
	}

	m_pCancelCommand = keyValues;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the cancel button text
//-----------------------------------------------------------------------------
void QueryBox::SetCancelButtonText(const char* buttonText)
{
	m_pCancelButton->SetText(buttonText);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the cancel button text
//-----------------------------------------------------------------------------
void QueryBox::SetCancelButtonText(const wchar_t* wszButtonText)
{
	m_pCancelButton->SetText(wszButtonText);
	InvalidateLayout();
}

void QueryBox::OnKeyCodeTyped( KeyCode code )
{
	if ( code == KEY_ESCAPE )
	{
		OnCommand("Cancel");
	}
	else
	{
		Frame::OnKeyCodeTyped(code);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void QueryBox::OnKeyCodePressed( KeyCode code )
{
	if ( code == KEY_XBUTTON_B )
	{
		OnCommand("Cancel");
	}
	else
	{
		Frame::OnKeyCodePressed(code);
	}
}



