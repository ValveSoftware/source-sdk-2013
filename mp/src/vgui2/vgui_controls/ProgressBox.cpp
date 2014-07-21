//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <vgui/IInput.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <KeyValues.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/ProgressBox.h>

#include <stdio.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ProgressBox::ProgressBox(const char *title, const char *text, const char *pszUnknownTimeString, Panel *parent) : Frame(parent, NULL, parent ? false : true)
{	
	// save off the non-localized title, since we may need to dynamically localize it (on progress updates)
	const wchar_t *ws = g_pVGuiLocalize->Find(title);
	if (ws)
	{
		wcsncpy(m_wszTitleString, ws, sizeof(m_wszTitleString) / sizeof(wchar_t));
	}
	else
	{
		g_pVGuiLocalize->ConvertANSIToUnicode(title, m_wszTitleString, sizeof(m_wszTitleString));
	}

	m_pMessageLabel = new Label(this, NULL, pszUnknownTimeString);

	ws = g_pVGuiLocalize->Find(text);
	if (ws)
	{
		wcsncpy(m_wcsInfoString, ws, sizeof(m_wcsInfoString) / sizeof(wchar_t));
	}
	else
	{
		m_wcsInfoString[0] = 0;
	}

	ws = g_pVGuiLocalize->Find(pszUnknownTimeString);
	if (ws)
	{
		wcsncpy(m_wszUnknownTimeString, ws, sizeof(m_wszUnknownTimeString) / sizeof(wchar_t));
	}
	else
	{
		m_wszUnknownTimeString[0] = 0;
	}
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ProgressBox::ProgressBox(const wchar_t *wszTitle, const wchar_t *wszText, const wchar_t *wszUnknownTimeString, Panel *parent) : Frame(parent, NULL, parent ? false : true)
{
	wcsncpy(m_wszTitleString, wszTitle, sizeof(m_wszTitleString) / sizeof(wchar_t));
	m_pMessageLabel = new Label(this, NULL, wszUnknownTimeString);
	wcsncpy(m_wcsInfoString, wszText, sizeof(m_wcsInfoString) / sizeof(wchar_t));
	wcsncpy(m_wszUnknownTimeString, wszUnknownTimeString, sizeof(m_wszUnknownTimeString) / sizeof(wchar_t));
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor Helper
//-----------------------------------------------------------------------------
void ProgressBox::Init()
{
	m_pProgressBar = new ProgressBar(this, NULL);
	m_pProgressBar->SetVisible(false);

	m_pCancelButton = new Button(this, NULL, "#VGui_Cancel");
	m_pCancelButton->SetSize(72, 24);
	m_pCancelButton->SetCommand("Cancel");

	SetMenuButtonResponsive(false);
	SetMinimizeButtonVisible(false);
	SetCancelButtonVisible(false);
	SetSizeable(false);
	SetSize(384, 128);
	m_flCurrentProgress = 0.0f;
	m_flFirstProgressUpdate = -0.1f;
	m_flLastProgressUpdate = 0.0f;

	// mark ourselves as needed ticked once a second, to force us to repaint
	ivgui()->AddTickSignal(GetVPanel(), 1000);

	UpdateTitle();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
ProgressBox::~ProgressBox()
{
}

//-----------------------------------------------------------------------------
// Purpose: resize the message label
//-----------------------------------------------------------------------------
void ProgressBox::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int wide, tall;
	m_pMessageLabel->GetContentSize(wide, tall);
	SetSize(384, tall + 92);
	m_pMessageLabel->SetSize(344, tall);
}

//-----------------------------------------------------------------------------
// Purpose: Put the message box into a modal state
//			Does not suspend execution - use addActionSignal to get return value
//-----------------------------------------------------------------------------
void ProgressBox::DoModal(Frame *pFrameOver)
{
    ShowWindow(pFrameOver);
	input()->SetAppModalSurface(GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: Activates the window
//-----------------------------------------------------------------------------
void ProgressBox::ShowWindow(Frame *pFrameOver)
{
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

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Put the text and OK buttons in correct place
//-----------------------------------------------------------------------------
void ProgressBox::PerformLayout()
{	
	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);
	wide += x;
	tall += y;

	int leftEdge = x + 16;
	m_pMessageLabel->SetPos(leftEdge, y + 12);
	m_pProgressBar->SetPos(leftEdge, y + 14 + m_pMessageLabel->GetTall() + 2);
	m_pProgressBar->SetSize(wide - 44, 24);

	if (m_pCancelButton->IsVisible())
	{
		// make room for cancel
		int px, py, pw, pt;
		int offs = m_pCancelButton->GetWide();
		m_pProgressBar->GetBounds(px, py, pw, pt);
		m_pCancelButton->SetPos(px + pw - offs, py);
		m_pProgressBar->SetSize(pw - offs - 10, pt);
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: updates progress bar, range [0, 1]
//-----------------------------------------------------------------------------
void ProgressBox::SetProgress(float progress)
{
	Assert(progress >= 0.0f && progress <= 1.0f);
	m_pProgressBar->SetProgress(progress);
	m_pProgressBar->SetVisible(true);

	// only update progress timings if the progress has actually changed
	if (progress != m_flCurrentProgress)
	{
		// store off timings for calculating time remaining
		if (m_flFirstProgressUpdate < 0.0f)
		{
			m_flFirstProgressUpdate = (float)system()->GetFrameTime();
		}
		m_flCurrentProgress = progress;
		m_flLastProgressUpdate = (float)system()->GetFrameTime();

		UpdateTitle();
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the info text
//-----------------------------------------------------------------------------
void ProgressBox::SetText(const char *text)
{
	m_pMessageLabel->SetText(text);
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog title text
//-----------------------------------------------------------------------------
void ProgressBox::UpdateTitle()
{
	// update progress text
	wchar_t unicode[256];
	wchar_t completion[64];
	if ((int)(m_flCurrentProgress * 100.0f) > 0)
	{
		_snwprintf(completion, sizeof(completion) / sizeof(wchar_t), L"- %d%% complete", (int)(m_flCurrentProgress * 100.0f));
	}
	else
	{
		completion[0] = 0;
	}
	g_pVGuiLocalize->ConstructString(unicode, sizeof(unicode), m_wszTitleString, 1, completion);
	SetTitle(unicode, true);
}

//-----------------------------------------------------------------------------
// Purpose: called every render
//-----------------------------------------------------------------------------
void ProgressBox::OnThink()
{
	// calculate the progress made
	if (m_flFirstProgressUpdate >= 0.0f && m_wcsInfoString[0])
	{
		wchar_t timeRemaining[128];
		if (ProgressBar::ConstructTimeRemainingString(timeRemaining, sizeof(timeRemaining), m_flFirstProgressUpdate, (float)system()->GetFrameTime(), m_flCurrentProgress, m_flLastProgressUpdate, true))
		{
			wchar_t unicode[256];
			g_pVGuiLocalize->ConstructString(unicode, sizeof(unicode), m_wcsInfoString, 1, timeRemaining);
			m_pMessageLabel->SetText(unicode);
		}
		else
		{
			m_pMessageLabel->SetText(m_wszUnknownTimeString);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Forces us to repaint once per second
//-----------------------------------------------------------------------------
void ProgressBox::OnTick()
{
	if (m_flFirstProgressUpdate >= 0.0f)
	{
		Repaint();
	}

	BaseClass::OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: Handles ESC closing dialog
//-----------------------------------------------------------------------------
void ProgressBox::OnCommand(const char *command)
{
	if (!stricmp(command, "Cancel"))
	{
		OnCancel();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: close button pressed
//-----------------------------------------------------------------------------
void ProgressBox::OnCloseFrameButtonPressed()
{
	OnCancel();
}

//-----------------------------------------------------------------------------
// Purpose: Deletes self when closed
//-----------------------------------------------------------------------------
void ProgressBox::OnClose()
{
	BaseClass::OnClose();
	// modal surface is released on deletion
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ProgressBox::OnShutdownRequest()
{
	// Shutdown the dialog
	PostMessage(this, new KeyValues("Command", "command", "Cancel"));
}

//-----------------------------------------------------------------------------
// Purpose: On update cancelled
//-----------------------------------------------------------------------------
void ProgressBox::OnCancel()
{
	// post a message that we've been cancelled
	PostActionSignal(new KeyValues("ProgressBoxCancelled"));

	// close this dialog
	Close();
}

//-----------------------------------------------------------------------------
// Purpose: Toggles visibility of the close box.
//-----------------------------------------------------------------------------
void ProgressBox::SetCancelButtonVisible(bool state)
{
	BaseClass::SetCloseButtonVisible(state);
	m_pCancelButton->SetVisible(state);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ProgressBox::SetCancelButtonEnabled(bool state)
{
	m_pCancelButton->SetEnabled(state);
	BaseClass::SetCloseButtonVisible(state);
	InvalidateLayout();
	Repaint();
}
