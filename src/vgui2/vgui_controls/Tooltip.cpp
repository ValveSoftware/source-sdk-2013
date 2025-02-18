//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
// This class is a message box that has two buttons, ok and cancel instead of
// just the ok button of a message box. We use a message box class for the ok button
// and implement another button here.
//=============================================================================//

#include <math.h>
#define PROTECTED_THINGS_DISABLE

#include <vgui/IInput.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/IPanel.h>

#include <vgui_controls/Tooltip.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Controls.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;


//------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------
static vgui::DHANDLE< TextEntry > s_TooltipWindow;
static int s_iTooltipWindowCount = 0;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
BaseTooltip::BaseTooltip(Panel *parent, const char *text) 
{
	SetText(text);

	_displayOnOneLine = false;
	_makeVisible = false;
	_isDirty = false;
	_enabled = true;

	_tooltipDelay = 500; // default delay for opening tooltips
	_delay = 0;
}

BaseTooltip::~BaseTooltip()
{
}

//-----------------------------------------------------------------------------
// Purpose: Reset the tooltip delay timer
//-----------------------------------------------------------------------------
void BaseTooltip::ResetDelay()
{
	_isDirty = true;
	_delay = system()->GetTimeMillis() + _tooltipDelay;
}

//-----------------------------------------------------------------------------
// Purpose: Set the tooltip delay before a tooltip comes up.
//-----------------------------------------------------------------------------
void BaseTooltip::SetTooltipDelay( int tooltipDelay )
{
	_tooltipDelay = tooltipDelay;
}

//-----------------------------------------------------------------------------
// Purpose: Get the tooltip delay before a tooltip comes up.
//-----------------------------------------------------------------------------
int BaseTooltip::GetTooltipDelay()
{
	return _tooltipDelay;
}

//-----------------------------------------------------------------------------
// Purpose: Set the tool tip to display on one line only
//			Default is multiple lines.
//-----------------------------------------------------------------------------
void BaseTooltip::SetTooltipFormatToSingleLine()
{
	_displayOnOneLine = true;
	_isDirty = true;
}

//-----------------------------------------------------------------------------
// Purpose: Set the tool tip to display on multiple lines.
//-----------------------------------------------------------------------------
void BaseTooltip::SetTooltipFormatToMultiLine()
{
	_displayOnOneLine = false;
	_isDirty = true;
}

//-----------------------------------------------------------------------------
// Purpose: Display the tooltip
//-----------------------------------------------------------------------------
void BaseTooltip::ShowTooltip(Panel *currentPanel)
{
	_makeVisible = true;

	PerformLayout();
}

void BaseTooltip::SetEnabled( bool bState )
{
	_enabled = bState;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool BaseTooltip::ShouldLayout( void )
{
	if (!_makeVisible)
		return false;

	if (_delay > system()->GetTimeMillis())
		return false;	

	// We only need to layout when we first become visible
	if ( !_isDirty )
		return false;

	Panel* pMouseOverPanel = ipanel()->GetPanel( input()->GetMouseOver(), GetControlsModuleName() );
	if ( pMouseOverPanel && pMouseOverPanel->GetTooltip() != this )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Display the tooltip
//-----------------------------------------------------------------------------
void BaseTooltip::HideTooltip()
{
	_makeVisible = false;
	_isDirty = true;
}

//-----------------------------------------------------------------------------
// Purpose: Set the tooltip text
//-----------------------------------------------------------------------------
void BaseTooltip::SetText(const char *text)
{
	_isDirty = true;

	if (!text)
	{
		text = "";
	}

	if (m_Text.Size() > 0)
	{
		m_Text.RemoveAll();
	}

	for (unsigned int i = 0; i < strlen(text); i++)
	{
		m_Text.AddToTail(text[i]);
	}
	m_Text.AddToTail('\0');
	
	if (s_TooltipWindow.Get() && m_pParent == s_TooltipWindow.Get()->GetParent())
	{
		s_TooltipWindow->SetText(m_Text.Base());
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the tooltip text
//-----------------------------------------------------------------------------
const char *BaseTooltip::GetText()
{
	return m_Text.Base();
}

//-----------------------------------------------------------------------------
// Purpose: Position the tool tip
//-----------------------------------------------------------------------------
void BaseTooltip::PositionWindow( Panel *pTipPanel )
{
	int iTipW, iTipH;
	pTipPanel->GetSize( iTipW, iTipH );

	int cursorX, cursorY;
	input()->GetCursorPos(cursorX, cursorY);

	int wide, tall;
	surface()->GetScreenSize(wide, tall);

	int iParentX = 0, iParentY = 0;
	if ( !pTipPanel->IsPopup() )
	{
		pTipPanel->GetParent()->GetPos( iParentX, iParentY );
		pTipPanel->GetParent()->LocalToScreen( iParentX, iParentY );
	}

	cursorX -= iParentX;
	cursorY -= iParentY;

	if (wide - iTipW > cursorX)
	{
		cursorY += 20;
		// menu hanging right
		if (tall - iTipH > cursorY)
		{
			// menu hanging down
			pTipPanel->SetPos(cursorX, cursorY);
		}
		else
		{
			// menu hanging up
			pTipPanel->SetPos(cursorX, cursorY - iTipH - 20);
		}
	}
	else
	{
		// menu hanging left
		if (tall - iTipH > cursorY)
		{
			// menu hanging down
			pTipPanel->SetPos(cursorX - iTipW, cursorY);
		}
		else
		{
			// menu hanging up
			pTipPanel->SetPos(cursorX - iTipW, cursorY - iTipH - 20 );
		}
	}	
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
TextTooltip::TextTooltip(Panel *parent, const char *text) : BaseTooltip( parent, text )
{
	if (!s_TooltipWindow.Get())
	{
		s_TooltipWindow = new TextEntry(NULL, "tooltip");

 		s_TooltipWindow->InvalidateLayout(false, true);

		// this bit of hackery is necessary because tooltips don't get ApplySchemeSettings called from their parents
		IScheme *pScheme = scheme()->GetIScheme( s_TooltipWindow->GetScheme() );
		s_TooltipWindow->SetBgColor(s_TooltipWindow->GetSchemeColor("Tooltip.BgColor", s_TooltipWindow->GetBgColor(), pScheme));
		s_TooltipWindow->SetFgColor(s_TooltipWindow->GetSchemeColor("Tooltip.TextColor", s_TooltipWindow->GetFgColor(), pScheme));
		s_TooltipWindow->SetBorder(pScheme->GetBorder("ToolTipBorder"));
		s_TooltipWindow->SetFont( pScheme->GetFont("DefaultSmall", s_TooltipWindow->IsProportional()));
	}
	s_iTooltipWindowCount++;

	// this line prevents the main window from losing focus
	// when a tooltip pops up
	s_TooltipWindow->MakePopup(false, true);
	s_TooltipWindow->SetKeyBoardInputEnabled( false );
	s_TooltipWindow->SetMouseInputEnabled( false );
	
	SetText(text);
	s_TooltipWindow->SetText(m_Text.Base());
	s_TooltipWindow->SetEditable(false);
	s_TooltipWindow->SetMultiline(true);
	s_TooltipWindow->SetVisible(false);
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
TextTooltip::~TextTooltip()
{
	if (--s_iTooltipWindowCount < 1)
	{
		if ( s_TooltipWindow.Get() )
		{
			s_TooltipWindow->MarkForDeletion();
		}
		s_TooltipWindow = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the tooltip text
//-----------------------------------------------------------------------------
void TextTooltip::SetText(const char *text)
{
	BaseTooltip::SetText( text );
	
	if (s_TooltipWindow.Get())
	{
		s_TooltipWindow->SetText(m_Text.Base());
	}
}

//-----------------------------------------------------------------------------
// Purpose: gets the font from the scheme
//-----------------------------------------------------------------------------
void TextTooltip::ApplySchemeSettings(IScheme *pScheme)
{
	if ( s_TooltipWindow )
	{
		s_TooltipWindow->SetFont(pScheme->GetFont("DefaultSmall", s_TooltipWindow->IsProportional()));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Display the tooltip
//-----------------------------------------------------------------------------
void TextTooltip::ShowTooltip(Panel *currentPanel)
{
	if ( s_TooltipWindow.Get() )
	{
		int nLen = s_TooltipWindow->GetTextLength();

		if ( nLen <= 0 )
		{
			// Empty tool tip, no need to show it
			_makeVisible = false;
			return;
		}

		char *pBuf = (char*)_alloca( nLen+1 );
		s_TooltipWindow->GetText( pBuf, nLen+1 );
		Panel *pCurrentParent = s_TooltipWindow->GetParent();

		_isDirty = _isDirty || ( pCurrentParent != currentPanel );
		s_TooltipWindow->SetText( m_Text.Base() );
		s_TooltipWindow->SetParent(currentPanel);

		// Apply proportional scaling to the tooltip if the parent has scaling enabled.
		//	This requires us to re-apply the font to see the changes.
		if ( IScheme* pScheme = scheme()->GetIScheme( s_TooltipWindow->GetScheme() ) )
		{
			s_TooltipWindow->SetProportional( currentPanel && currentPanel->IsProportional() );
			s_TooltipWindow->SetFont( pScheme->GetFont( "DefaultSmall", s_TooltipWindow->IsProportional() ) );
		}
	}
	BaseTooltip::ShowTooltip( currentPanel );
}

//-----------------------------------------------------------------------------
// Purpose: Display the tooltip
//-----------------------------------------------------------------------------
void TextTooltip::PerformLayout()
{
	if ( !ShouldLayout() )
		return;
	// we're ready, just make us visible
	if ( !s_TooltipWindow.Get() )
		return;

	_isDirty = false;

	s_TooltipWindow->SetVisible(true);
	s_TooltipWindow->MakePopup( false, true );
	s_TooltipWindow->SetKeyBoardInputEnabled( false );
	s_TooltipWindow->SetMouseInputEnabled( false );

	// relayout the textwindow immediately so that we know it's size
	//m_pTextEntry->InvalidateLayout(true);

	SizeTextWindow();
	PositionWindow( s_TooltipWindow );
}

//-----------------------------------------------------------------------------
// Purpose: Size the text window so all the text fits inside it.
//-----------------------------------------------------------------------------
void TextTooltip::SizeTextWindow()
{
	if ( !s_TooltipWindow.Get() )
		return;

	if (_displayOnOneLine)
	{
		// We want the tool tip to be one line
		s_TooltipWindow->SetMultiline(false);
		s_TooltipWindow->SetToFullWidth();
	}
	else
	{
		// We want the tool tip to be one line
		s_TooltipWindow->SetMultiline(false);
		s_TooltipWindow->SetToFullWidth();
		int wide, tall;
		s_TooltipWindow->GetSize( wide, tall );
		double newWide = sqrt( (2.0/1) * wide * tall );
		double newTall = (1/2) * newWide;
		s_TooltipWindow->SetMultiline(true);
		s_TooltipWindow->SetSize((int)newWide, (int)newTall );
		s_TooltipWindow->SetToFullHeight();
		
		s_TooltipWindow->GetSize( wide, tall );
		
		if (( wide < 100 ) && ( s_TooltipWindow->GetNumLines() == 2) ) 
		{
			s_TooltipWindow->SetMultiline(false);
			s_TooltipWindow->SetToFullWidth();	
		}
		else
		{
			
			while ( (float)wide/(float)tall < 2 )
			{
				s_TooltipWindow->SetSize( wide + 1, tall );
				s_TooltipWindow->SetToFullHeight();
				s_TooltipWindow->GetSize( wide, tall );
			}
		}
		s_TooltipWindow->GetSize( wide, tall );
	//	ivgui()->DPrintf("End Ratio: %f\n", (float)wide/(float)tall);		
	}
}

//-----------------------------------------------------------------------------
// Purpose: Display the tooltip
//-----------------------------------------------------------------------------
void TextTooltip::HideTooltip()
{
	if ( s_TooltipWindow.Get() )
	{
		s_TooltipWindow->SetVisible(false);
	}

	BaseTooltip::HideTooltip();
}

