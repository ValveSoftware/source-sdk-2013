//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <utlvector.h>

#include <vgui/Cursor.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include <KeyValues.h>
#include <vgui/MouseCode.h>

#include <vgui_controls/Menu.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/MenuItem.h>
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

enum
{
	// maximum size of text buffer
	BUFFER_SIZE=999999,
};

using namespace vgui;

#define DRAW_OFFSET_X() QuickPropScale( 3 )
#define DRAW_OFFSET_Y() QuickPropScale( 1 )

DECLARE_BUILD_FACTORY( TextEntry );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
TextEntry::TextEntry(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	SetTriplePressAllowed( true );

	_font = INVALID_FONT;
	_smallfont = INVALID_FONT;

	m_szComposition[ 0 ] = L'\0';

	m_bAllowNumericInputOnly = false;
	m_bAllowNonAsciiCharacters = false;
	_hideText = false;
	_editable = false;
	_verticalScrollbar = false;
	_cursorPos = 0;
	_currentStartIndex = 0;
	_horizScrollingAllowed = true;
	_cursorIsAtEnd = false;
	_putCursorAtEnd = false;
	_multiline = false;
	_cursorBlinkRate = 400;
	_mouseSelection = false;
	_mouseDragSelection = false;
	_vertScrollBar=NULL;
	_catchEnterKey = false;
	_maxCharCount = -1;
	_charCount = 0;
	_wrap = false; // don't wrap by default
	_sendNewLines = false; // don't pass on a newline msg by default
	_drawWidth = 0;
	m_bAutoProgressOnHittingCharLimit = false;
	m_pIMECandidates = NULL;
	m_hPreviousIME = input()->GetEnglishIMEHandle();
	m_bDrawLanguageIDAtLeft = false;
	m_nLangInset = 0;
	m_bUseFallbackFont = false;
	m_hFallbackFont = INVALID_FONT;

	//a -1 for _select[0] means that the selection is empty
	_select[0] = -1;
	_select[1] = -1;
	m_pEditMenu = NULL;
	
	//this really just inits it when in here	
	ResetCursorBlink();
	
	SetCursor(dc_ibeam);
	
	SetEditable(true);
	
	// initialize the line break array
	m_LineBreaks.AddToTail(BUFFER_SIZE);
	
	_recalculateBreaksIndex = 0;
	
	_selectAllOnFirstFocus = false;
	_selectAllOnFocusAlways = false;

	//position the cursor so it is at the end of the text
	GotoTextEnd();

	// If keyboard focus is in an edit control, don't chain keyboard mappings up to parents since it could mess with typing in text.
	SetAllowKeyBindingChainToParent( false );

	REGISTER_COLOR_AS_OVERRIDABLE( _disabledFgColor, "disabledFgColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _disabledBgColor, "disabledBgColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _selectionColor, "selectionColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _selectionTextColor, "selectionTextColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _defaultSelectionBG2Color, "defaultSelectionBG2Color_override" );
}


TextEntry::~TextEntry()
{
	delete m_pEditMenu;
	delete m_pIMECandidates;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextEntry::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	
	SetFgColor(GetSchemeColor("TextEntry.TextColor", pScheme));
	SetBgColor(GetSchemeColor("TextEntry.BgColor", pScheme));
	
	_cursorColor = GetSchemeColor("TextEntry.CursorColor", pScheme);
	_disabledFgColor = GetSchemeColor("TextEntry.DisabledTextColor", pScheme);
	_disabledBgColor = GetSchemeColor("TextEntry.DisabledBgColor", pScheme);
	
	_selectionTextColor = GetSchemeColor("TextEntry.SelectedTextColor", GetFgColor(), pScheme);
	_selectionColor = GetSchemeColor("TextEntry.SelectedBgColor", pScheme);
	_defaultSelectionBG2Color = GetSchemeColor("TextEntry.OutOfFocusSelectedBgColor", pScheme);
	_focusEdgeColor = GetSchemeColor("TextEntry.FocusEdgeColor", Color(0, 0, 0, 0), pScheme);

	SetBorder( pScheme->GetBorder("ButtonDepressedBorder"));

	if ( _font == INVALID_FONT ) _font = pScheme->GetFont("Default", IsProportional() );
	if ( _smallfont == INVALID_FONT ) _smallfont = pScheme->GetFont( "DefaultVerySmall", IsProportional() );

	SetFont( _font );
}

void TextEntry::SetSelectionTextColor( const Color& clr )
{
	_selectionTextColor = clr;
}

void TextEntry::SetSelectionBgColor( const Color& clr )
{
	_selectionColor = clr;
}

void TextEntry::SetSelectionUnfocusedBgColor( const Color& clr )
{
	_defaultSelectionBG2Color = clr;
}

//-----------------------------------------------------------------------------
// Purpose: sets the color of the background when the control is disabled
//-----------------------------------------------------------------------------
void TextEntry::SetDisabledBgColor(Color col)
{
	_disabledBgColor = col;
}


//-----------------------------------------------------------------------------
// Purpose: Sends a message if the data has changed
//          Turns off any selected text in the window if we are not using the edit menu
//-----------------------------------------------------------------------------
void TextEntry::OnKillFocus()
{
	m_szComposition[ 0 ] = L'\0';
	HideIMECandidates();

	if (_dataChanged)
	{
		FireActionSignal();
		_dataChanged = false;
	}
	
	// check if we clicked the right mouse button or if it is down
	bool mouseRightClicked = input()->WasMousePressed(MOUSE_RIGHT);
	bool mouseRightUp = input()->WasMouseReleased(MOUSE_RIGHT);
	bool mouseRightDown = input()->IsMouseDown(MOUSE_RIGHT);
	
	if (mouseRightClicked || mouseRightDown || mouseRightUp )
	{			
		int cursorX, cursorY;
		input()->GetCursorPos(cursorX, cursorY);

		// if we're right clicking within our window, we don't actually kill focus
		if (IsWithin(cursorX, cursorY))
			return;
	}
	
   	// clear any selection
    SelectNone();

	// move the cursor to the start
//	GotoTextStart();

	PostActionSignal( new KeyValues( "TextKillFocus" ) );

	// chain
	BaseClass::OnKillFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Wipe line breaks after the size of a panel has been changed
//-----------------------------------------------------------------------------
void TextEntry::OnSizeChanged(int newWide, int newTall)
{
	BaseClass::OnSizeChanged(newWide, newTall);

   	// blow away the line breaks list 
	_recalculateBreaksIndex = 0;
	m_LineBreaks.RemoveAll();
	m_LineBreaks.AddToTail(BUFFER_SIZE);

    // if we're bigger, see if we can scroll left to put more text in the window
    if (newWide > _drawWidth)
    {
        ScrollLeftForResize();
    }

	_drawWidth = newWide;
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: Set the text array - convert ANSI text to unicode and pass to unicode function
//-----------------------------------------------------------------------------
void TextEntry::SetText(const char *text)
{
	if (!text)
	{
		text = "";
	}

	if (text[0] == '#')
	{
		// check for localization
		wchar_t *wsz = g_pVGuiLocalize->Find(text);
		if (wsz)
		{
			SetText(wsz);
			return;
		}
	}

	int len = V_strlen( text );
	if ( len < 1023 )
	{
		wchar_t unicode[ 1024 ];
		g_pVGuiLocalize->ConvertANSIToUnicode( text, unicode, sizeof( unicode ) );
		SetText( unicode );
	}
	else
	{
		int lenUnicode = ( int )( len * sizeof( wchar_t ) + 4 );
		wchar_t *unicode = ( wchar_t * ) malloc( lenUnicode );
			g_pVGuiLocalize->ConvertANSIToUnicode( text, unicode, lenUnicode );
			SetText( unicode );
		free( unicode );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the text array
//          Using this function will cause all lineBreaks to be discarded.
//          This is because this fxn replaces the contents of the text buffer.
//          For modifying large buffers use insert functions.
//-----------------------------------------------------------------------------
void TextEntry::SetText(const wchar_t *wszText)
{
	if (!wszText)
	{
		wszText = L"";
	}
	int textLen = V_wcslen(wszText);
	m_TextStream.RemoveAll();
	m_TextStream.EnsureCapacity(textLen);

	int missed_count = 0;
	for (int i = 0; i < textLen; i++)
	{
		if(wszText[i]=='\r') // don't insert \r characters
		{
			missed_count++;
			continue;
		}
		m_TextStream.AddToTail(wszText[i]);
		SetCharAt(wszText[i], i-missed_count);
	}

	GotoTextStart();
	SelectNone();
	
	// reset the data changed flag
	_dataChanged = false;
	
	// blow away the line breaks list 
	_recalculateBreaksIndex = 0;
	m_LineBreaks.RemoveAll();
	m_LineBreaks.AddToTail(BUFFER_SIZE);
	
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the value of char at index position.
//-----------------------------------------------------------------------------
void TextEntry::SetCharAt(wchar_t ch, int index)
{
	if ((ch == '\n') || (ch == '\0')) 
	{
		// if its not at the end of the buffer it matters.
		// redo the linebreaks
		//if (index != m_TextStream.Count())
		{
			_recalculateBreaksIndex = 0;
			m_LineBreaks.RemoveAll();
			m_LineBreaks.AddToTail(BUFFER_SIZE);
		}
	}
	
	if (index < 0)
		return;

	if (index >= m_TextStream.Count())
	{
		m_TextStream.AddMultipleToTail(index - m_TextStream.Count() + 1);
	}
	m_TextStream[index] = ch;
	_dataChanged = true;
}

//-----------------------------------------------------------------------------
// Purpose: Restarts the time of the next cursor blink
//-----------------------------------------------------------------------------
void TextEntry::ResetCursorBlink()
{
	_cursorBlink=false;
	_cursorNextBlinkTime=system()->GetTimeMillis()+_cursorBlinkRate;
}

//-----------------------------------------------------------------------------
// Purpose: Hides the text buffer so it will not be drawn
//-----------------------------------------------------------------------------
void TextEntry::SetTextHidden(bool bHideText)
{
	_hideText = bHideText;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: return character width
//-----------------------------------------------------------------------------
int getCharWidth(HFont font, wchar_t ch)
{
	if (!iswcntrl(ch))
	{
		int a, b, c;
		surface()->GetCharABCwide(font, ch, a, b, c);
		return (a + b + c);
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Given cursor's position in the text buffer, convert it to
//   the local window's x and y pixel coordinates
// Input: cursorPos: cursor index
// Output: cx, cy, the corresponding coords in the local window
//-----------------------------------------------------------------------------
void TextEntry::CursorToPixelSpace(int cursorPos, int &cx, int &cy)
{
	int yStart = GetYStart();
	
	int x = DRAW_OFFSET_X(), y = yStart;
	_pixelsIndent = 0;
	int lineBreakIndexIndex = 0;
	
	for (int i = GetStartDrawIndex(lineBreakIndexIndex); i < m_TextStream.Count(); i++)
	{
		wchar_t ch = m_TextStream[i];
		if (_hideText)
		{
			ch = '*';
		}
		
		// if we've found the position, break
		if (cursorPos == i)
		{
			// even if this is a line break entry for the cursor, the next insert
			// will be at this position, which will push the line break forward one
			// so don't push the cursor down a line here...
			/*if (!_putCursorAtEnd)
			{		
				// if we've passed a line break go to that
				if (m_LineBreaks[lineBreakIndexIndex] == i)
				{
					// add another line
					AddAnotherLine(x,y);
					lineBreakIndexIndex++;
				}
			}*/
			break;
		}
		
		// if we've passed a line break go to that
		if (m_LineBreaks.Count() && 
			lineBreakIndexIndex < m_LineBreaks.Count() &&
			m_LineBreaks[lineBreakIndexIndex] == i)
		{
			// add another line
			AddAnotherLine(x,y);
			lineBreakIndexIndex++;
		}
		
		// add to the current position
		x += getCharWidth(_font, ch);
	}
	
	if ( m_bDrawLanguageIDAtLeft )
	{
		x += m_nLangInset;
	}

	cx = x;
	cy = y;
}

//-----------------------------------------------------------------------------
// Purpose: Converts local pixel coordinates to an index in the text buffer
//          This function appears to be used only in response to mouse clicking
// Input  : cx - 
//			cy - pixel location
//-----------------------------------------------------------------------------
int TextEntry::PixelToCursorSpace(int cx, int cy)
{
	
	int w, h;
	GetSize(w, h);
	cx = clamp(cx, 0, w+100);
	cy = clamp(cy, 0, h);

	_putCursorAtEnd = false; //	Start off assuming we clicked somewhere in the text
	
	int fontTall = surface()->GetFontTall(_font);
	
	// where to Start reading
	int yStart = GetYStart();
	int x = DRAW_OFFSET_X(), y = yStart;
	_pixelsIndent = 0;
	int lineBreakIndexIndex = 0;
	
	int startIndex = GetStartDrawIndex(lineBreakIndexIndex);
	bool onRightLine = false;
	int i;
	for (i = startIndex; i < m_TextStream.Count(); i++)
	{
		wchar_t ch = m_TextStream[i];
		if (_hideText)
		{
			ch = '*';
		}
		
		// if we are on the right line but off the end of if put the cursor at the end of the line
		if (m_LineBreaks[lineBreakIndexIndex] == i )
		{
			// add another line
			AddAnotherLine(x,y);
			lineBreakIndexIndex++;
			
			if (onRightLine)
			{	
				_putCursorAtEnd = true;
				return i;
			}
		}
		
		// check to see if we're on the right line
		if (cy < yStart)
		{
			// cursor is above panel
			onRightLine = true;
			_putCursorAtEnd = true;	// this will make the text scroll up if needed
		}
		else if (cy >= y && (cy < (y + fontTall + DRAW_OFFSET_Y())))
		{
			onRightLine = true;
		}
		
		int wide = getCharWidth(_font, ch);
		
		// if we've found the position, break
		if (onRightLine)
		{
			if (cx > GetWide())	  // off right side of window
			{
			}
			else if (cx < (DRAW_OFFSET_X() + _pixelsIndent) || cy < yStart)	 // off left side of window
			{
				return i; // move cursor one to left
			}
			
			if (cx >= x && cx < (x + wide))
			{
				// check which side of the letter they're on
				if (cx < (x + (wide * 0.5)))  // left side
				{
					return i;
				}
				else  // right side
				{						 
					return i + 1;
				}
			}
		}
		x += wide;
	}
	
	return i;
}

//-----------------------------------------------------------------------------
// Purpose: Draws a character in the panel
// Input:	ch - character to draw
//			font - font to use
//			x, y - pixel location to draw char at
// Output:	returns the width of the character drawn
//-----------------------------------------------------------------------------
int TextEntry::DrawChar(wchar_t ch, HFont font, int index, int x, int y)
{
	// add to the current position
	int charWide = getCharWidth(font, ch);
	int fontTall=surface()->GetFontTall(font);
	if (!iswcntrl(ch))
	{
		// draw selection, if any
		int selection0 = -1, selection1 = -1;
		GetSelectedRange(selection0, selection1);
		
		if (index >= selection0 && index < selection1)
		{
			// draw background selection color
            VPANEL focus = input()->GetFocus();
			Color bgColor;
			bool hasFocus = HasFocus();
			bool childOfFocus = focus && ipanel()->HasParent(focus, GetVPanel());

            // if one of the children of the SectionedListPanel has focus, then 'we have focus' if we're selected
            if ( hasFocus || childOfFocus )
			{
    			bgColor = _selectionColor;
			}
            else
			{
    			bgColor =_defaultSelectionBG2Color;
			}

			surface()->DrawSetColor(bgColor);

			surface()->DrawFilledRect(x, y, x + charWide, y + 1 + fontTall);
			
			// reset text color
			surface()->DrawSetTextColor(_selectionTextColor);
		}
		if (index == selection1)
		{
			// we've come out of selection, reset the color
			surface()->DrawSetTextColor(GetFgColor());
		}

		surface()->DrawSetTextPos(x, y);
		surface()->DrawUnicodeChar(ch);
		
		return charWide;
	}
	
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Draw the cursor, cursor is not drawn when it is blinked gone
// Input:  x,y where to draw cursor
// Output: returns true if cursor was drawn.
//-----------------------------------------------------------------------------
bool TextEntry::DrawCursor(int x, int y)
{
	if (!_cursorBlink)
	{
		int cx, cy;
		CursorToPixelSpace(_cursorPos, cx, cy);
		surface()->DrawSetColor(_cursorColor);
		int fontTall=surface()->GetFontTall(_font);
		surface()->DrawFilledRect(cx, cy, cx + 1, cy + fontTall);
		return true;
	}
	return false;
}

bool TextEntry::NeedsEllipses( HFont font, int *pIndex )
{
	Assert( pIndex );
	*pIndex = -1;
	int wide = DRAW_OFFSET_X(); // buffer on left and right end of text.
	for ( int i = 0; i < m_TextStream.Count(); ++i )
	{	
		wide += getCharWidth( font , m_TextStream[i] );
		if (wide > _drawWidth)
		{
			*pIndex = i;
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the text in the panel
//-----------------------------------------------------------------------------
void TextEntry::PaintBackground()
{
	BaseClass::PaintBackground();

	// draw background
	Color col;
	if (IsEnabled())
	{
		col = GetBgColor();
	}
	else
	{
		col = _disabledBgColor;
	}
	Color saveBgColor = col;

	int wide, tall;
	GetSize( wide, tall );

//	surface()->DrawSetColor(col);
//	surface()->DrawFilledRect(0, 0, wide, tall);

	// where to Start drawing
	int x = DRAW_OFFSET_X() + _pixelsIndent, y = GetYStart();

	m_nLangInset = 0;

	int langlen = 0;
	wchar_t shortcode[ 5 ];
	shortcode[ 0 ] = L'\0';

	if ( m_bAllowNonAsciiCharacters )
	{
		input()->GetIMELanguageShortCode( shortcode, sizeof( shortcode ) );

		if ( shortcode[ 0 ] != L'\0' &&
			 wcsicmp( shortcode, L"EN" ) )
		{
			m_nLangInset = 0;
			langlen = V_wcslen( shortcode );
			for ( int i = 0; i < langlen; ++i )
			{
				m_nLangInset += getCharWidth( _smallfont, shortcode[ i ] );
			}

			m_nLangInset += 4;

			if ( m_bDrawLanguageIDAtLeft )
			{
				x += m_nLangInset;
			}

			wide -= m_nLangInset;
		}
	}

	HFont useFont = _font;

	surface()->DrawSetTextFont(useFont);
	if (IsEnabled())
	{
		col = GetFgColor();
	}
	else
	{
		col = _disabledFgColor;
	}
	surface()->DrawSetTextColor(col);
	_pixelsIndent = 0;
	
	int lineBreakIndexIndex = 0;
	int startIndex = GetStartDrawIndex(lineBreakIndexIndex);
	int remembery = y;

	int oldEnd = m_TextStream.Count();
	int oldCursorPos = _cursorPos;
	int nCompStart = -1;
	int nCompEnd = -1;

	// FIXME: Should insert at cursor pos instead
	bool composing = m_bAllowNonAsciiCharacters && wcslen( m_szComposition ) > 0;
	bool invertcomposition = input()->GetShouldInvertCompositionString();

	if ( composing )
	{
		nCompStart = _cursorPos;

		wchar_t *s = m_szComposition;
		while ( *s != L'\0' )
		{
			m_TextStream.InsertBefore( _cursorPos, *s );
			++s;
			++_cursorPos;
		}

		nCompEnd = _cursorPos;
	}

	bool highlight_composition = ( nCompStart != -1 && nCompEnd != -1 ) ? true : false;

	// draw text with an elipsis
	if ( (!_multiline) && (!_horizScrollingAllowed) )
	{	
		int endIndex = m_TextStream.Count();
		// In editable windows only do the ellipsis if we don't have focus.
		// In non editable windows do it all the time.
		if ( (!HasFocus() && (IsEditable())) || (!IsEditable()) )
		{
			int i = -1;

			// loop through all the characters and sum their widths	
			bool addEllipses = NeedsEllipses( useFont, &i );
			if ( addEllipses && 
				!IsEditable() &&
				m_bUseFallbackFont && 
				INVALID_FONT != m_hFallbackFont )
			{
				// Switch to small font!!!
				useFont = m_hFallbackFont;
				surface()->DrawSetTextFont(useFont);
				addEllipses = NeedsEllipses( useFont, &i );
			}
			if (addEllipses)
			{
				int elipsisWidth = 3 * getCharWidth(useFont, '.');
				while (elipsisWidth > 0 && i >= 0)
				{
					elipsisWidth -= getCharWidth(useFont, m_TextStream[i]);
					i--;
				}
				endIndex = i + 1;	
			}

			// if we take off less than the last 3 chars we have to make sure
			// we take off the last 3 chars so selected text will look right.
			if (m_TextStream.Count() - endIndex < 3 && m_TextStream.Count() - endIndex > 0 )
			{
				endIndex = m_TextStream.Count() - 3;
			}
		}
		// draw the text
		int i;
		for (i = startIndex; i < endIndex; i++)
		{
			wchar_t ch = m_TextStream[i];
			if (_hideText)
			{
				ch = '*';
			}

			bool iscompositionchar = false;

			if ( highlight_composition )
			{
				iscompositionchar = ( i >= nCompStart && i < nCompEnd ) ? true : false;
				if ( iscompositionchar )
				{
					// Set the underline color to the text color
					surface()->DrawSetColor( col );

					int w = getCharWidth( useFont, ch );

					if ( invertcomposition )
					{
						// Invert color
						surface()->DrawSetTextColor( saveBgColor );
						surface()->DrawSetColor( col );
						
						surface()->DrawFilledRect(x, 0, x+w, tall);
						// Set the underline color to the text color
						surface()->DrawSetColor( saveBgColor );
					}

					surface()->DrawFilledRect( x, tall - 2, x + w, tall - 1 );
				}
			}


			// draw the character and update xposition  
			x += DrawChar(ch, useFont, i, x, y);

			// Restore color
			surface()->DrawSetTextColor(col);

		}
		if (endIndex < m_TextStream.Count()) // add an elipsis
		{
			x += DrawChar('.', useFont, i, x, y);
			i++;
			x += DrawChar('.', useFont, i, x, y);
			i++;
			x += DrawChar('.', useFont, i, x, y);
			i++;
		}
	}	
	else
	{
		// draw the text
		for ( int i = startIndex; i < m_TextStream.Count(); i++)
		{
			wchar_t ch = m_TextStream[i];
			if (_hideText)
			{
				ch = '*';
			}
			
			// if we've passed a line break go to that
			if ( _multiline && m_LineBreaks[lineBreakIndexIndex] == i)
			{
				// add another line
				AddAnotherLine(x, y);
				lineBreakIndexIndex++;
			}
			
			bool iscompositionchar = false;

			if ( highlight_composition )
			{
				iscompositionchar = ( i >= nCompStart && i < nCompEnd ) ? true : false;
				if ( iscompositionchar )
				{	
					// Set the underline color to the text color
					surface()->DrawSetColor( col );

					int w = getCharWidth( useFont, ch );

					if ( invertcomposition )
					{
						// Invert color
						surface()->DrawSetTextColor( saveBgColor );
						surface()->DrawFilledRect(x, 0, x+w, tall);
						// Set the underline color to the text color
						surface()->DrawSetColor( saveBgColor );
					}

					surface()->DrawFilledRect( x, tall - 2, x + w, tall - 1 );
				}
			}

			// draw the character and update xposition  
			x += DrawChar(ch, useFont, i, x, y);

			// Restore color
			surface()->DrawSetTextColor(col);
		}
	}

	// custom border
	//!! need to replace this with scheme stuff (TextEntryBorder/TextEntrySelectedBorder)
	surface()->DrawSetColor(50, 50, 50, 255);
	
	if (IsEnabled() && IsEditable() && HasFocus())
	{
		// set a more distinct border color
		surface()->DrawSetColor(0, 0, 0, 255);
		
		DrawCursor (x, y);

		if ( composing )
		{
			LocalToScreen( x, y );
			input()->SetCandidateWindowPos( x, y );
		}
	}

	int newEnd = m_TextStream.Count();
	int remove = newEnd - oldEnd;
	if ( remove > 0 )
	{
		m_TextStream.RemoveMultiple( oldCursorPos, remove );
	}
	_cursorPos = oldCursorPos;

	if ( HasFocus() && m_bAllowNonAsciiCharacters && langlen > 0 )
	{
		wide += m_nLangInset;

		if ( m_bDrawLanguageIDAtLeft )
		{
			x = 0;
		}
		else
		{
			// Draw language identififer
			x = wide - m_nLangInset;
		}

		surface()->DrawSetColor( col );

		surface()->DrawFilledRect( x, 2, x + m_nLangInset-2, tall - 2 );

		saveBgColor[ 3 ] = 255;
		surface()->DrawSetTextColor( saveBgColor );

		x += 1;

		surface()->DrawSetTextFont(_smallfont);
		for ( int i = 0; i < langlen; ++i )
		{
			x += DrawChar( shortcode[ i ], _smallfont, i, x, remembery );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when data changes or panel size changes
//-----------------------------------------------------------------------------
void TextEntry::PerformLayout()
{
	BaseClass::PerformLayout();

	RecalculateLineBreaks();
	
	// recalculate scrollbar position
	if (_verticalScrollbar)
	{
		LayoutVerticalScrollBarSlider();
	}
	
	// force a Repaint
	Repaint();
}

// moves x,y to the Start of the next line of text
void TextEntry::AddAnotherLine(int &cx, int &cy)
{
	cx = DRAW_OFFSET_X() + _pixelsIndent;
	cy += (surface()->GetFontTall(_font) + DRAW_OFFSET_Y());
}


//-----------------------------------------------------------------------------
// Purpose: Recalculates line breaks
//-----------------------------------------------------------------------------
void TextEntry::RecalculateLineBreaks()
{
	if (!_multiline || _hideText)
		return;

	if (m_TextStream.Count() < 1)
		return;
	
	HFont font = _font;
	
	// line break to our width -2 pixel to keep cursor blinking in window
	// (assumes borders are 1 pixel)
	int wide = GetWide()-2;
	
	// subtract the scrollbar width
	if (_vertScrollBar)
	{
		wide -= _vertScrollBar->GetWide();
	}
	
	int charWidth;
	int x = DRAW_OFFSET_X(), y = DRAW_OFFSET_Y();
		
	int wordStartIndex = 0;
	int wordLength = 0;
	bool hasWord = false;
	bool justStartedNewLine = true;
	bool wordStartedOnNewLine = true;
	
	int startChar;
	if (_recalculateBreaksIndex <= 0)
	{
		m_LineBreaks.RemoveAll();
		startChar=0;
	}
	else
	{
		// remove the rest of the linebreaks list since its out of date.
		for (int i=_recalculateBreaksIndex+1; i < m_LineBreaks.Count(); ++i)
		{
			m_LineBreaks.Remove((int)i);
			--i; // removing shrinks the list!
		}
		startChar = m_LineBreaks[_recalculateBreaksIndex];
	}
	
	// handle the case where this char is a new line, in that case
	// we have already taken its break index into account above so skip it.
	if (m_TextStream[startChar] == '\r' || m_TextStream[startChar] == '\n') 
	{
		startChar++;
	}
	
	// loop through all the characters	
	int i;
	for (i = startChar; i < m_TextStream.Count(); ++i)
	{
		wchar_t ch = m_TextStream[i];
		
		// line break only on whitespace characters
		if (!iswspace(ch))
		{
			if (hasWord)
			{
				// append to the current word
			}
			else
			{
				// Start a new word
				wordStartIndex = i;
				hasWord = true;
				wordStartedOnNewLine = justStartedNewLine;
				wordLength = 0;
			}
		}
		else
		{
			// whitespace/punctuation character
			// end the word
			hasWord = false;
		}
		
		// get the width
		charWidth = getCharWidth(font, ch);
		if (!iswcntrl(ch))
		{
			justStartedNewLine = false;
		}
				
		// check to see if the word is past the end of the line [wordStartIndex, i)
		if ((x + charWidth) >= wide || ch == '\r' || ch == '\n')
		{
			// add another line
			AddAnotherLine(x,y);
			
			justStartedNewLine = true;
			hasWord = false;
			
			if (ch == '\r' || ch == '\n')
			{
				// set the break at the current character
				m_LineBreaks.AddToTail(i);
			}
			else if (wordStartedOnNewLine)
			{
				// word is longer than a line, so set the break at the current cursor
				m_LineBreaks.AddToTail(i);
			}
			else
			{
				// set it at the last word Start
				m_LineBreaks.AddToTail(wordStartIndex);
				
				// just back to reparse the next line of text
				i = wordStartIndex;
			}
			
			// reset word length
			wordLength = 0;
		}
		
		// add to the size
		x += charWidth;
		wordLength += charWidth;
	}
	
	_charCount = i-1;
	
	// end the list
	m_LineBreaks.AddToTail(BUFFER_SIZE);
	
	// set up the scrollbar
	LayoutVerticalScrollBarSlider();
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate where the vertical scroll bar slider should be 
//			based on the current cursor line we are on.
//-----------------------------------------------------------------------------
void TextEntry::LayoutVerticalScrollBarSlider()
{
	// set up the scrollbar
	if (_vertScrollBar)
	{
		int wide, tall;
		GetSize (wide, tall);
		
		// make sure we factor in insets
		int ileft, iright, itop, ibottom;
		GetInset(ileft, iright, itop, ibottom);
		
		// with a scroll bar we take off the inset
		wide -= iright;
		
		_vertScrollBar->SetPos(wide - _vertScrollBar->GetWide(), 0);
		// scrollbar is inside the borders.
		_vertScrollBar->SetSize(_vertScrollBar->GetWide(), tall - ibottom - itop);
		
		// calculate how many lines we can fully display
		int displayLines = tall / (surface()->GetFontTall(_font) + DRAW_OFFSET_Y());
		int numLines = m_LineBreaks.Count();
		
		if (numLines <= displayLines)
		{
			// disable the scrollbar
			_vertScrollBar->SetEnabled(false);
			_vertScrollBar->SetRange(0, numLines);
			_vertScrollBar->SetRangeWindow(numLines);
			_vertScrollBar->SetValue(0);
		}
		else
		{
			// set the scrollbars range
			_vertScrollBar->SetRange(0, numLines);
			_vertScrollBar->SetRangeWindow(displayLines);
			
			_vertScrollBar->SetEnabled(true);
			
			// this should make it scroll one line at a time
			_vertScrollBar->SetButtonPressedScrollValue(1);
			
			// set the value to view the last entries
			int val = _vertScrollBar->GetValue();
			int maxval = _vertScrollBar->GetValue() + displayLines;
			if (GetCursorLine() < val )
			{
				while (GetCursorLine() < val)
				{
					val--;
				}
			}
			else if (GetCursorLine() >= maxval)
			{
				while (GetCursorLine() >= maxval)
				{
					maxval++;
				}
				maxval -= displayLines;	
				val = maxval;
			}
			else 
			{
				//val = GetCursorLine();
			}
			
			_vertScrollBar->SetValue(val);
			_vertScrollBar->InvalidateLayout();
			_vertScrollBar->Repaint();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Set boolean value of baseclass variables.
//-----------------------------------------------------------------------------
void TextEntry::SetEnabled(bool state)
{
	BaseClass::SetEnabled(state);
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether text wraps around multiple lines or not
// Input  : state - true or false
//-----------------------------------------------------------------------------
void TextEntry::SetMultiline(bool state)
{
	_multiline = state;
}

bool TextEntry::IsMultiline()
{
	return _multiline;
}

//-----------------------------------------------------------------------------
// Purpose: sets whether or not the edit catches and stores ENTER key presses
//-----------------------------------------------------------------------------
void TextEntry::SetCatchEnterKey(bool state)
{
	_catchEnterKey = state;
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether a vertical scrollbar is visible
// Input  : state - true or false
//-----------------------------------------------------------------------------
void TextEntry::SetVerticalScrollbar(bool state)
{
	_verticalScrollbar = state;
	
	if (_verticalScrollbar)
	{
		if (!_vertScrollBar)
		{
			_vertScrollBar = new ScrollBar(this, "ScrollBar", true);
			_vertScrollBar->AddActionSignalTarget(this);
		}
		
		_vertScrollBar->SetVisible(true);
	}
	else if (_vertScrollBar)
	{
		_vertScrollBar->SetVisible(false);
	}
	
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: sets _editable flag
// Input  : state - true or false
//-----------------------------------------------------------------------------
void TextEntry::SetEditable(bool state)
{
	if ( state )
	{
		SetDropEnabled( true, 1.0f );
	}
	else
	{
		SetDropEnabled( false );
	}
	_editable = state;
}

const wchar_t *UnlocalizeUnicode( wchar_t *unicode )
{
	if ( !unicode )
		return L"";

	if ( *unicode == L'#' )
	{
		char lookup[ 512 ];
		g_pVGuiLocalize->ConvertUnicodeToANSI( unicode + 1, lookup, sizeof( lookup ) );
		return g_pVGuiLocalize->Find( lookup );
	}
	return unicode;
}

Menu * TextEntry::GetEditMenu()
{
	return m_pEditMenu;
}

//-----------------------------------------------------------------------------
// Purpose: Create cut/copy/paste dropdown menu
//-----------------------------------------------------------------------------
void TextEntry::CreateEditMenu()
{	
	// create a drop down cut/copy/paste menu appropriate for this object's states
	if (m_pEditMenu)
		delete m_pEditMenu;
	m_pEditMenu = new Menu(this, "EditMenu");
	
	m_pEditMenu->SetFont( _font );
	
	// add cut/copy/paste drop down options if its editable, just copy if it is not
	if (_editable && !_hideText)
	{	
		m_pEditMenu->AddMenuItem("#TextEntry_Cut", new KeyValues("DoCutSelected"), this);
	}
	
	if ( !_hideText )
	{
		m_pEditMenu->AddMenuItem("#TextEntry_Copy", new KeyValues("DoCopySelected"), this);
	}
	
	if (_editable)
	{
		m_pEditMenu->AddMenuItem("#TextEntry_Paste", new KeyValues("DoPaste"), this);
	}
	

	if ( m_bAllowNonAsciiCharacters )
	{
		IInput::LanguageItem *langs = NULL;

		int count = input()->GetIMELanguageList( NULL, 0 );
		if ( count > 0 )
		{
			langs = new IInput::LanguageItem[ count ];
			input()->GetIMELanguageList( langs, count );

			// Create a submenu
			Menu *subMenu = new Menu( this, "LanguageMenu" );

			subMenu->SetFont( _font );

			for ( int i = 0; i < count; ++i )
			{
				int id = subMenu->AddCheckableMenuItem( "Language", UnlocalizeUnicode( langs[ i ].menuname ), new KeyValues( "DoLanguageChanged", "handle", langs[ i ].handleValue ), this );
				if ( langs[ i ].active )
				{
					subMenu->SetMenuItemChecked( id, true );
				}
			}

			m_pEditMenu->AddCascadingMenuItem( "Language", "#TextEntry_Language", "", this, subMenu );

			delete[] langs;
		}

		IInput::ConversionModeItem *modes = NULL;

		count = input()->GetIMEConversionModes( NULL, 0 );
		// if count == 0 then native mode is the only mode...
		if ( count > 0 )
		{
			modes = new IInput::ConversionModeItem[ count ];
			input()->GetIMEConversionModes( modes, count );

			// Create a submenu
			Menu *subMenu = new Menu( this, "ConversionModeMenu" );

			subMenu->SetFont( _font );

			for ( int i = 0; i < count; ++i )
			{
				int id = subMenu->AddCheckableMenuItem( "ConversionMode", UnlocalizeUnicode( modes[ i ].menuname ), new KeyValues( "DoConversionModeChanged", "handle", modes[ i ].handleValue ), this );
				if ( modes[ i ].active )
				{
					subMenu->SetMenuItemChecked( id, true );
				}
			}

			m_pEditMenu->AddCascadingMenuItem( "ConversionMode", "#TextEntry_ConversionMode", "", this, subMenu );

			delete[] modes;
		}

		IInput::SentenceModeItem *sentencemodes = NULL;

		count = input()->GetIMESentenceModes( NULL, 0 );
		// if count == 0 then native mode is the only mode...
		if ( count > 0 )
		{
			sentencemodes = new IInput::SentenceModeItem[ count ];
			input()->GetIMESentenceModes( sentencemodes, count );

			// Create a submenu
			Menu *subMenu = new Menu( this, "SentenceModeMenu" );

			subMenu->SetFont( _font );

			for ( int i = 0; i < count; ++i )
			{
				int id = subMenu->AddCheckableMenuItem( "SentenceMode", UnlocalizeUnicode( sentencemodes[ i ].menuname ), new KeyValues( "DoConversionModeChanged", "handle", modes[ i ].handleValue ), this );
				if ( modes[ i ].active )
				{
					subMenu->SetMenuItemChecked( id, true );
				}
			}

			m_pEditMenu->AddCascadingMenuItem( "SentenceMode", "#TextEntry_SentenceMode", "", this, subMenu );

			delete[] sentencemodes;
		}
	}
	

	m_pEditMenu->SetVisible(false);
	m_pEditMenu->SetParent(this);
	m_pEditMenu->AddActionSignalTarget(this);
}

//-----------------------------------------------------------------------------
// Purpsoe: Returns state of _editable flag
//-----------------------------------------------------------------------------
bool TextEntry::IsEditable()
{
	return _editable && IsEnabled();
}

//-----------------------------------------------------------------------------
// Purpose: We want single line windows to scroll horizontally and select text
//          in response to clicking and holding outside window
//-----------------------------------------------------------------------------
void TextEntry::OnMouseFocusTicked()
{
	// if a button is down move the scrollbar slider the appropriate direction
	if (_mouseDragSelection) // text is being selected via mouse clicking and dragging
	{
		OnCursorMoved(0,0);	// we want the text to scroll as if we were dragging
	}	
}

//-----------------------------------------------------------------------------
// Purpose: If a cursor enters the window, we are not elegible for 
//          MouseFocusTicked events
//-----------------------------------------------------------------------------
void TextEntry::OnCursorEntered()
{
	_mouseDragSelection = false; // outside of window dont recieve drag scrolling ticks
}

//-----------------------------------------------------------------------------
// Purpose: When the cursor is outside the window, if we are holding the mouse
//			button down, then we want the window to scroll the text one char at a time
//			using Ticks
//-----------------------------------------------------------------------------
void TextEntry::OnCursorExited() // outside of window recieve drag scrolling ticks
{
	if (_mouseSelection)
		_mouseDragSelection = true;
}

//-----------------------------------------------------------------------------
// Purpose: Handle selection of text by mouse
//-----------------------------------------------------------------------------
void TextEntry::OnCursorMoved(int ignX, int ignY)
{
	if (_mouseSelection)
	{
		// update the cursor position
		int x, y;
		input()->GetCursorPos(x, y);
		ScreenToLocal(x, y);
		_cursorPos = PixelToCursorSpace(x, y);
		
		// if we are at Start of buffer don't put cursor at end, this will keep
		// window from scrolling up to a blank line
		if (_cursorPos == 0)
			_putCursorAtEnd = false;
		
		// scroll if we went off left side
		if (_cursorPos == _currentStartIndex)
		{
			if (_cursorPos > 0)
				_cursorPos--;

			ScrollLeft();
			_cursorPos = _currentStartIndex;
		}
		if ( _cursorPos != _select[1])
		{
			_select[1] = _cursorPos;
			Repaint();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle Mouse button down events.
//-----------------------------------------------------------------------------
void TextEntry::OnMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		bool keepChecking = SelectCheck( true );
		if ( !keepChecking )
		{
			BaseClass::OnMousePressed( code );
			return;
		}
		
		// move the cursor to where the mouse was pressed
		int x, y;
		input()->GetCursorPos(x, y);
		ScreenToLocal(x, y);
		
		_cursorIsAtEnd = _putCursorAtEnd; // save this off before calling PixelToCursorSpace()
		_cursorPos = PixelToCursorSpace(x, y);
		// if we are at Start of buffer don't put cursor at end, this will keep
		// window from scrolling up to a blank line
		if (_cursorPos == 0)
			_putCursorAtEnd = false;
		
		// enter selection mode
		input()->SetMouseCapture(GetVPanel());
		_mouseSelection = true;
		
		if (_select[0] < 0)
		{
			// if no initial selection position, Start selection position at cursor
			_select[0] = _cursorPos;
		}
		_select[1] = _cursorPos;
		
		ResetCursorBlink();
		RequestFocus();
		Repaint();
	}
	else if (code == MOUSE_RIGHT) // check for context menu open
	{	
		CreateEditMenu();
		Assert(m_pEditMenu);
		
		OpenEditMenu();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle mouse button up events
//-----------------------------------------------------------------------------
void TextEntry::OnMouseReleased(MouseCode code)
{
	_mouseSelection = false;
	
	input()->SetMouseCapture(NULL);
	
	// make sure something has been selected
	int cx0, cx1;
	if (GetSelectedRange(cx0, cx1))
	{
		if (cx1 - cx0 == 0)
		{
			// nullify selection
			_select[0] = -1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
//-----------------------------------------------------------------------------
void TextEntry::OnMouseTriplePressed( MouseCode code )
{
	BaseClass::OnMouseTriplePressed( code );

	// left triple clicking on a word selects all
	if (code == MOUSE_LEFT)
	{
		GotoTextEnd();

		SelectAllText( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle mouse double clicks
//-----------------------------------------------------------------------------
void TextEntry::OnMouseDoublePressed(MouseCode code)
{
	// left double clicking on a word selects the word
	if (code == MOUSE_LEFT)
	{
		// move the cursor just as if you single clicked.
		OnMousePressed(code);
		// then find the start and end of the word we are in to highlight it.
		int selectSpot[2];
		GotoWordLeft();
		selectSpot[0] = _cursorPos;
		GotoWordRight();
		selectSpot[1] = _cursorPos;

		if (_cursorPos > 0)
		{
			if (iswspace(m_TextStream[_cursorPos - 1]))
			{
				selectSpot[1]--;
				_cursorPos--;
			}
			
			_select[0] = selectSpot[0];
			_select[1] = selectSpot[1];
			_mouseSelection = true;
		}
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: Turn off text selection code when mouse button is not down
//-----------------------------------------------------------------------------
void TextEntry::OnMouseCaptureLost()
{
	_mouseSelection = false;
}

//-----------------------------------------------------------------------------
// Purpose: Only pass some keys upwards 
// everything else we don't relay to the parent
//-----------------------------------------------------------------------------
void TextEntry::OnKeyCodePressed(KeyCode code)
{
	// Pass enter on only if _catchEnterKey isn't set
	if ( code == KEY_ENTER )
	{
		if ( !_catchEnterKey )
		{
			Panel::OnKeyCodePressed( code );
			return;
		}
	}
	
	// Forward on just a few key codes, everything else can be handled by TextEntry itself
	switch ( code )
	{
		case KEY_F1:
		case KEY_F2:
		case KEY_F3:
		case KEY_F4:
		case KEY_F5:
		case KEY_F6:
		case KEY_F7:
		case KEY_F8:
		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
		case KEY_ESCAPE:
		case KEY_APP:
			Panel::OnKeyCodePressed( code );
			return;
	}
	
	// Pass on the joystick and mouse codes
	if ( IsMouseCode(code) || IsNovintButtonCode(code) || IsJoystickCode(code) || IsJoystickButtonCode(code) ||
	     IsJoystickPOVCode(code) || IsJoystickAxisCode(code) )
	{
		Panel::OnKeyCodePressed( code );
		return;
	}
	    
}


//-----------------------------------------------------------------------------
// Purpose: Masks which keys get chained up
//			Maps keyboard input to text window functions.
//-----------------------------------------------------------------------------
void TextEntry::OnKeyCodeTyped(KeyCode code)
{
	_cursorIsAtEnd = _putCursorAtEnd;
	_putCursorAtEnd = false;
	
	bool shift = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
	bool ctrl = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));
	bool alt = (input()->IsKeyDown(KEY_LALT) || input()->IsKeyDown(KEY_RALT));
	bool winkey = (input()->IsKeyDown(KEY_LWIN) || input()->IsKeyDown(KEY_RWIN));
	bool fallThrough = false;
	
	if ( ( ctrl || ( winkey && IsOSX() ) ) && !alt)
	{
		switch(code)
		{
		case KEY_A:
			SelectAllText(false);
			// move the cursor to the end
			_cursorPos = _select[1];
			break;

		case KEY_INSERT:
		case KEY_C:
			{
				CopySelected();
				break;
			}
		case KEY_V:
			{
				DeleteSelected();
				Paste();
				break;
			}
		case KEY_X:
			{
				CopySelected();
				DeleteSelected();
				break;
			}
		case KEY_Z:
			{
				Undo();
				break;
			}
		case KEY_RIGHT:
			{
				GotoWordRight();
				break;
			}
		case KEY_LEFT:
			{
				GotoWordLeft();
				break;
			}
		case KEY_ENTER:
			{
				// insert a newline
				if (_multiline)
				{
					DeleteSelected();
					SaveUndoState();
					InsertChar('\n');
				}
				// fire newlines back to the main target if asked to
				if(_sendNewLines) 
				{
					PostActionSignal(new KeyValues("TextNewLine"));
				}
				break;
			}
		case KEY_HOME:
			{
				GotoTextStart();
				break;
			}
		case KEY_END:
			{
				GotoTextEnd();
				break;
			}
		case KEY_PAGEUP:
			{
				OnChangeIME( true );
			}
			break;
		case KEY_PAGEDOWN:
			{
				OnChangeIME( false );
			}
			break;
		case KEY_UP:
		case KEY_DOWN:
			if ( m_bAllowNonAsciiCharacters )
			{
				FlipToLastIME();
			}
			else
			{
				fallThrough = true;
			}
			break;
		default:
			{
				fallThrough = true;
				break;
			}
		}
	}
	else if (alt)
	{
		// do nothing with ALT-x keys
		if ( !m_bAllowNonAsciiCharacters || ( code != KEY_BACKQUOTE ) )
		{
			fallThrough = true;
		}
	}
	else
	{
		switch(code)
		{
		case KEY_TAB:
		case KEY_LSHIFT:
		case KEY_RSHIFT:
		case KEY_ESCAPE:
			{
				fallThrough = true;
				break;
			}
		case KEY_INSERT:
			{
				if (shift)
				{
					DeleteSelected();
					Paste();
				}
				else
				{
					fallThrough = true;
				}
				
				break;
			}
		case KEY_DELETE:
			{
				if (shift)
				{
					// shift-delete is cut
					CopySelected();
					DeleteSelected();
				}
				else
				{
					Delete();
				}
				break;
			}
		case KEY_LEFT:
			{
				GotoLeft();
				break;
			}
		case KEY_RIGHT:
			{
				GotoRight();
				break;
			}
		case KEY_UP:
			{
				if (_multiline)
				{
					GotoUp();
				}
				else
				{
					fallThrough = true;
				}
				break;
			}
		case KEY_DOWN:
			{
				if (_multiline)
				{
					GotoDown();
				}
				else
				{
					fallThrough = true;
				}
				break;
			}
		case KEY_HOME:
			{
				if (_multiline)
				{
					GotoFirstOfLine();
				}
				else
				{
					GotoTextStart();
				}
				break;
			}
		case KEY_END:
			{
				GotoEndOfLine();
				break;
			}
		case KEY_BACKSPACE:
			{
				int x0, x1;
				if (GetSelectedRange(x0, x1))
				{
					// act just like delete if there is a selection
					DeleteSelected();
				}
				else
				{
					Backspace();
				}
				break;
			}
		case KEY_ENTER:
			{
				// insert a newline
				if (_multiline && _catchEnterKey)
				{
					DeleteSelected();
					SaveUndoState();
					InsertChar('\n');
				}
				else
				{
					fallThrough = true;
				}
				// fire newlines back to the main target if asked to
				if(_sendNewLines) 
				{
					PostActionSignal(new KeyValues("TextNewLine"));
				}
				break;
			}
		case KEY_PAGEUP:
			{
				int val = 0;
				fallThrough = (!_multiline) && (!_vertScrollBar);
				if (_vertScrollBar)
				{
					val = _vertScrollBar->GetValue();
				}
				
				// if there is a scroll bar scroll down one rangewindow
				if (_multiline)
				{
					int displayLines = GetTall() / (surface()->GetFontTall(_font) + DRAW_OFFSET_Y());
					// move the cursor down
					for (int i=0; i < displayLines; i++)
					{
						GotoUp();
					}
				}
				
				// if there is a scroll bar scroll down one rangewindow
				if (_vertScrollBar)
				{
					int window = _vertScrollBar->GetRangeWindow();
					int newval = _vertScrollBar->GetValue();
					int linesToMove = window - (val - newval);
					_vertScrollBar->SetValue(val - linesToMove - 1);
				}
				break;
				
			}
		case KEY_PAGEDOWN:
			{
				int val = 0;
				fallThrough = (!_multiline) && (!_vertScrollBar);
				if (_vertScrollBar)
				{
					val = _vertScrollBar->GetValue();
				}
				
				if (_multiline)
				{
					int displayLines = GetTall() / (surface()->GetFontTall(_font) + DRAW_OFFSET_Y());
					// move the cursor down
					for (int i=0; i < displayLines; i++)
					{
						GotoDown();
					}
				}
				
				// if there is a scroll bar scroll down one rangewindow
				if (_vertScrollBar)
				{
					int window = _vertScrollBar->GetRangeWindow();
					int newval = _vertScrollBar->GetValue();
					int linesToMove = window - (newval - val);
					_vertScrollBar->SetValue(val + linesToMove + 1);
				}
				break;
			}

		case KEY_F1:
		case KEY_F2:
		case KEY_F3:
		case KEY_F4:
		case KEY_F5:
		case KEY_F6:
		case KEY_F7:
		case KEY_F8:
		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
			{
				fallThrough = true;
				break;
			}

		default:
			{
				// return if any other char is pressed.
				// as it will be a unicode char.
				// and we don't want select[1] changed unless a char was pressed that this fxn handles
				return;
			}
		}
	}
	
	// select[1] is the location in the line where the blinking cursor started
	_select[1] = _cursorPos;
	
	if (_dataChanged)
	{
		FireActionSignal();
	}
	
	// chain back on some keys
	if (fallThrough)
	{
		_putCursorAtEnd=_cursorIsAtEnd;	// keep state of cursor on fallthroughs
		BaseClass::OnKeyCodeTyped(code);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Masks which keys get chained up
//			Maps keyboard input to text window functions.
//-----------------------------------------------------------------------------
void TextEntry::OnKeyTyped(wchar_t unichar)
{
	_cursorIsAtEnd = _putCursorAtEnd;
	_putCursorAtEnd=false;
	
	bool fallThrough = false;
	
	// KeyCodes handle all non printable chars
	if (iswcntrl(unichar) || unichar == 9 ) // tab key (code 9) is printable but handled elsewhere
		return;
	
	// do readonly keys
	if (!IsEditable())
	{
		BaseClass::OnKeyTyped(unichar);
		return;
	}
	
	if (unichar != 0)
	{
		DeleteSelected();
		SaveUndoState();
		InsertChar(unichar);
	}
	
	// select[1] is the location in the line where the blinking cursor started
	_select[1] = _cursorPos;
	
	if (_dataChanged)
	{
		FireActionSignal();
	}
	
	// chain back on some keys
	if (fallThrough)
	{
		_putCursorAtEnd=_cursorIsAtEnd;	// keep state of cursor on fallthroughs
		BaseClass::OnKeyTyped(unichar);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Scrolls the list according to the mouse wheel movement
//-----------------------------------------------------------------------------
void TextEntry::OnMouseWheeled(int delta)
{
	if (_vertScrollBar)
	{
		MoveScrollBar(delta);
	}
	else
	{
		// if we don't use the input, chain back
		BaseClass::OnMouseWheeled(delta);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Scrolls the list 
// Input  : delta - amount to move scrollbar up
//-----------------------------------------------------------------------------
void TextEntry::MoveScrollBar(int delta)
{
	if (_vertScrollBar)
	{
		int val = _vertScrollBar->GetValue();
		val -= (delta * 3);
		_vertScrollBar->SetValue(val);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame the entry has keyboard focus; 
//          blinks the text cursor
//-----------------------------------------------------------------------------
void TextEntry::OnKeyFocusTicked()
{
	int time=system()->GetTimeMillis();
	if(time>_cursorNextBlinkTime)
	{
		_cursorBlink=!_cursorBlink;
		_cursorNextBlinkTime=time+_cursorBlinkRate;
		Repaint();
	}
}

Panel *TextEntry::GetDragPanel()
{
	if ( input()->IsMouseDown( MOUSE_LEFT ) )
	{
		int x, y;
		input()->GetCursorPos(x, y);
		ScreenToLocal(x, y);
		int cursor = PixelToCursorSpace(x, y);
	
		int cx0, cx1;
		bool check = GetSelectedRange( cx0, cx1 );

		if ( check && cursor >= cx0 && cursor < cx1 )
		{
			// Don't deselect in this case!!!
			return BaseClass::GetDragPanel();
		}
		return NULL;
	}

	return BaseClass::GetDragPanel();
}

void TextEntry::OnCreateDragData( KeyValues *msg )
{
	BaseClass::OnCreateDragData( msg );

	char txt[ 256 ];
	GetText( txt, sizeof( txt ) );

	int r0, r1;
	if ( GetSelectedRange( r0, r1 ) && r0 != r1 )
	{
		int len = r1 - r0;
		if ( len > 0 && r0 < 1024 )
		{
			char selection[ 512 ];
			Q_strncpy( selection, &txt[ r0 ], len + 1 );
			selection[ len ] = 0;
			msg->SetString( "text", selection );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check if we are selecting text (so we can highlight it)
//-----------------------------------------------------------------------------
bool TextEntry::SelectCheck( bool fromMouse /*=false*/ )
{
	bool bret = true;
	if (!HasFocus() || !(input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT)))
	{
		bool deselect = true;
		int cx0, cx1;
		if ( fromMouse && 
			GetDragPanel() != NULL )
		{
			// move the cursor to where the mouse was pressed
			int x, y;
			input()->GetCursorPos(x, y);
			ScreenToLocal(x, y);
			int cursor = PixelToCursorSpace(x, y);
		
			bool check = GetSelectedRange( cx0, cx1 );

			if ( check && cursor >= cx0 && cursor < cx1 )
			{
				// Don't deselect in this case!!!
				deselect = false;
				bret = false;
			}
		}

		if ( deselect )
		{
			_select[0] = -1;
		}
	}
	else if (_select[0] == -1)
	{
		_select[0] = _cursorPos;
	}
	return bret;
}

//-----------------------------------------------------------------------------
// Purpose: set the maximum number of chars in the text buffer
//-----------------------------------------------------------------------------
void TextEntry::SetMaximumCharCount(int maxChars)
{
	_maxCharCount = maxChars;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
int TextEntry::GetMaximumCharCount()
{
	return _maxCharCount;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void TextEntry::SetAutoProgressOnHittingCharLimit(bool state)
{
	m_bAutoProgressOnHittingCharLimit = state;
}

//-----------------------------------------------------------------------------
// Purpose: set whether to wrap the text buffer
//-----------------------------------------------------------------------------
void TextEntry::SetWrap(bool wrap)
{
	_wrap = wrap;
}

//-----------------------------------------------------------------------------
// Purpose: set whether to pass newline msgs to parent
//-----------------------------------------------------------------------------
void TextEntry::SendNewLine(bool send)
{
	_sendNewLines = send;
}

//-----------------------------------------------------------------------------
// Purpose: Tell if an index is a linebreakindex
//-----------------------------------------------------------------------------
bool TextEntry::IsLineBreak(int index)
{
	for (int i=0; i<m_LineBreaks.Count(); ++i)
	{
		if (index ==  m_LineBreaks[i])
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor one character to the left, scroll the text
//  horizontally if needed
//-----------------------------------------------------------------------------
void TextEntry::GotoLeft()
{
	SelectCheck();
	
	// if we are on a line break just move the cursor to the prev line
	if (IsLineBreak(_cursorPos))
	{
		// if we're already on the prev line at the end dont put it on the end
		if (!_cursorIsAtEnd)
			_putCursorAtEnd = true;
	}
	// if we are not at Start decrement cursor 
	if (!_putCursorAtEnd && _cursorPos > 0)
	{
		_cursorPos--;	
	}
	
    ScrollLeft();
	
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor one character to the right, scroll the text
//			horizontally if needed
//-----------------------------------------------------------------------------
void TextEntry::GotoRight()
{
	SelectCheck();
	
	// if we are on a line break just move the cursor to the next line
	if (IsLineBreak(_cursorPos))
	{
		if (_cursorIsAtEnd)
		{
			_putCursorAtEnd = false;
		}
		else
		{
			// if we are not at end increment cursor 
			if (_cursorPos < m_TextStream.Count())
			{
				_cursorPos++;
			}	
		}
	}
	else
	{
		// if we are not at end increment cursor 
		if (_cursorPos < m_TextStream.Count())
		{
			_cursorPos++;
		}
		
		// if we are on a line break move the cursor to end of line 
		if (IsLineBreak(_cursorPos))
		{
			if (!_cursorIsAtEnd)
				_putCursorAtEnd = true;
		}
	}
	// scroll right if we need to
	ScrollRight();
	
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Find out what line the cursor is on
//-----------------------------------------------------------------------------
int TextEntry::GetCursorLine()
{
	// find which line the cursor is on
	int cursorLine;
	for (cursorLine = 0; cursorLine < m_LineBreaks.Count(); cursorLine++)
	{
		if (_cursorPos < m_LineBreaks[cursorLine])
			break;
	}
	
	if (_putCursorAtEnd)  // correct for when cursor is at end of line rather than Start of next
	{
		// we are not at end of buffer, in which case there is no next line to be at the Start of
		if (_cursorPos != m_TextStream.Count() ) 
			cursorLine--;
	}
	
	return cursorLine;
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor one line up 
//-----------------------------------------------------------------------------
void TextEntry::GotoUp()
{
	SelectCheck();
	
	if (_cursorIsAtEnd)
	{
		if ( (GetCursorLine() - 1 ) == 0) // we are on first line
		{
			// stay at end of line
			_putCursorAtEnd = true;
			return;	 // dont move the cursor
		}
		else
			_cursorPos--;  
	}
	
	int cx, cy;
	CursorToPixelSpace(_cursorPos, cx, cy);
	
	// move the cursor to the previous line
	MoveCursor(GetCursorLine() - 1, cx);
}


//-----------------------------------------------------------------------------
// Purpose: Move the cursor one line down
//-----------------------------------------------------------------------------
void TextEntry::GotoDown()
{
	SelectCheck();
	
	if (_cursorIsAtEnd)
	{
		_cursorPos--;
		if (_cursorPos < 0)
			_cursorPos = 0;
	}
	
	int cx, cy;
	CursorToPixelSpace(_cursorPos, cx, cy);
	
	// move the cursor to the next line
	MoveCursor(GetCursorLine() + 1, cx);
	if (!_putCursorAtEnd && _cursorIsAtEnd )
	{
		_cursorPos++;
		if (_cursorPos > m_TextStream.Count())
		{
			_cursorPos = m_TextStream.Count();
		}
	}
	LayoutVerticalScrollBarSlider();	
}

//-----------------------------------------------------------------------------
// Purpose: Set the starting ypixel positon for a walk through the window
//-----------------------------------------------------------------------------
int TextEntry::GetYStart()
{
	if (_multiline)
	{
		// just Start from the top
		return DRAW_OFFSET_Y();
	}

	int fontTall = surface()->GetFontTall(_font);
	return (GetTall() / 2) - (fontTall / 2);
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor to a line, need to know how many pixels are in a line
//-----------------------------------------------------------------------------
void TextEntry::MoveCursor(int line, int pixelsAcross)
{
	// clamp to a valid line
	if (line < 0)
		line = 0;
	if (line >= m_LineBreaks.Count())
		line = m_LineBreaks.Count() -1;
	
	// walk the whole text set looking for our place
	// work out where to Start checking
	
	int yStart = GetYStart();
	
	int x = DRAW_OFFSET_X(), y = yStart;
	int lineBreakIndexIndex = 0;
	_pixelsIndent = 0;
	int i;
	for ( i = 0; i < m_TextStream.Count(); i++)
	{
		wchar_t ch = m_TextStream[i];
		
		if (_hideText)
		{
			ch = '*';
		}
		
		// if we've passed a line break go to that
		if (m_LineBreaks[lineBreakIndexIndex] == i)
		{
			if (lineBreakIndexIndex == line)
			{
				_putCursorAtEnd = true;
				_cursorPos = i;
				break;
			}
			
			// add another line
			AddAnotherLine(x,y);
			lineBreakIndexIndex++;
			
		}
		
		// add to the current position
		int charWidth = getCharWidth(_font, ch);		
		
		if (line == lineBreakIndexIndex)
		{
			// check to see if we're in range
			if ((x + (charWidth / 2)) > pixelsAcross)
			{
				// found position
				_cursorPos = i;
				break;
			}
		}
		
		x += charWidth;
	}
	
	// if we never find the cursor it must be past the end
	// of the text buffer, to let's just slap it on the end of the text buffer then.
	if (i ==  m_TextStream.Count())
	{
		GotoTextEnd();
	}
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Turn horizontal scrolling on or off.
//			Horizontal scrolling is disabled in multline windows. 
//		    Toggling this will disable it in single line windows as well.
//-----------------------------------------------------------------------------
void TextEntry::SetHorizontalScrolling(bool status) 
{
	_horizScrollingAllowed = status;
}

//-----------------------------------------------------------------------------
// Purpose: Horizontal scrolling function, not used in multiline windows
//			Function will scroll the buffer to the left if the cursor is not in the window
//			scroll left if we need to 
//-----------------------------------------------------------------------------
void TextEntry::ScrollLeft()
{
	if (_multiline)	  // early out
	{
		return;
	}
	
	if (!_horizScrollingAllowed)  //early out
	{
		return;
	}
	
	if(_cursorPos < _currentStartIndex)	 // scroll left if we need to
	{
		if (_cursorPos < 0)// dont scroll past the Start of buffer
		{
			_cursorPos=0;			
		}
		_currentStartIndex = _cursorPos;
	}
	
	LayoutVerticalScrollBarSlider();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextEntry::ScrollLeftForResize()
{
	if (_multiline)	  // early out
	{
		return;
	}
	
	if (!_horizScrollingAllowed)  //early out
	{
		return;
	}

    while (_currentStartIndex > 0)     // go until we hit leftmost
    {
        _currentStartIndex--;
		int nVal = _currentStartIndex;

        // check if the cursor is now off the screen
        if (IsCursorOffRightSideOfWindow(_cursorPos))
        {
            _currentStartIndex++;   // we've gone too far, return it
            break;
        }

        // IsCursorOffRightSideOfWindow actually fixes the _currentStartIndex, 
        // so if our value changed that menas we really are off the screen
		if (nVal != _currentStartIndex)
			break;
    }
	LayoutVerticalScrollBarSlider();
}

//-----------------------------------------------------------------------------
// Purpose: Horizontal scrolling function, not used in multiline windows
//			Scroll one char right until the cursor is visible in the window.
//			We do this one char at a time because char width isn't a constant.
//-----------------------------------------------------------------------------
void TextEntry::ScrollRight()
{
	if (!_horizScrollingAllowed)
		return;

	if (_multiline)	  
	{
	}
	// check if cursor is off the right side of window
	else if (IsCursorOffRightSideOfWindow(_cursorPos))
	{
		_currentStartIndex++; //scroll over
		ScrollRight(); // scroll again, check if cursor is in window yet 
	}
	
	LayoutVerticalScrollBarSlider();
}

//-----------------------------------------------------------------------------
// Purpose: Check and see if cursor position is off the right side of the window
//			just compare cursor's pixel coords with the window size coords.
// Input:	an integer cursor Position, if you pass _cursorPos fxn will tell you
//			if current cursor is outside window.
// Output:	true: cursor is outside right edge or window 
//			false: cursor is inside right edge
//-----------------------------------------------------------------------------
bool TextEntry::IsCursorOffRightSideOfWindow(int cursorPos)
{
	int cx, cy;
	CursorToPixelSpace(cursorPos, cx, cy);
	int wx=GetWide()-1;	//width of inside of window is GetWide()-1
	if ( wx <= 0 )
		return false;

	return (cx >= wx);
}

//-----------------------------------------------------------------------------
// Purpose: Check and see if cursor position is off the left side of the window
//			just compare cursor's pixel coords with the window size coords.
// Input:	an integer cursor Position, if you pass _cursorPos fxn will tell you
//			if current cursor is outside window.
// Output:	true - cursor is outside left edge or window 
//			false - cursor is inside left edge
//-----------------------------------------------------------------------------
bool TextEntry::IsCursorOffLeftSideOfWindow(int cursorPos)
{
	int cx, cy;
	CursorToPixelSpace(cursorPos, cx, cy);
	return (cx <= 0);
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor over to the Start of the next word to the right
//-----------------------------------------------------------------------------
void TextEntry::GotoWordRight()
{
	SelectCheck();
	
	// search right until we hit a whitespace character or a newline
	while (++_cursorPos < m_TextStream.Count())
	{
		if (iswspace(m_TextStream[_cursorPos]))
			break;
	}
	
	// search right until we hit an nonspace character
	while (++_cursorPos < m_TextStream.Count())
	{
		if (!iswspace(m_TextStream[_cursorPos]))
			break;
	}
	
	if (_cursorPos > m_TextStream.Count())
		_cursorPos = m_TextStream.Count();
	
	// now we are at the start of the next word
		
	// scroll right if we need to
	ScrollRight();
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor over to the Start of the next word to the left
//-----------------------------------------------------------------------------
void TextEntry::GotoWordLeft()
{
	SelectCheck();
	
	if (_cursorPos < 1)
		return;
	
	// search left until we hit an nonspace character
	while (--_cursorPos >= 0)
	{
		if (!iswspace(m_TextStream[_cursorPos]))
			break;
	}
	
	// search left until we hit a whitespace character
	while (--_cursorPos >= 0)
	{
		if (iswspace(m_TextStream[_cursorPos]))
		{
			break;
		}
	}
	
	// we end one character off
	_cursorPos++;
	// now we are at the Start of the previous word
	
	
	// scroll left if we need to
	ScrollLeft();
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move cursor to the Start of the text buffer
//-----------------------------------------------------------------------------
void TextEntry::GotoTextStart()
{
	SelectCheck();
	_cursorPos = 0;		   // set cursor to Start
	_putCursorAtEnd = false;
	_currentStartIndex=0;  // scroll over to Start
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move cursor to the end of the text buffer
//-----------------------------------------------------------------------------
void TextEntry::GotoTextEnd()
{
	SelectCheck();
	_cursorPos=m_TextStream.Count();	// set cursor to end of buffer
	_putCursorAtEnd = true; // move cursor Start of next line
	ScrollRight();				// scroll over until cursor is on screen
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move cursor to the Start of the current line
//-----------------------------------------------------------------------------
void TextEntry::GotoFirstOfLine()
{
	SelectCheck();
	// to get to the Start of the line you have to take into account line wrap
	// we have to figure out at which point the line wraps
	// given the current cursor position, select[1], find the index that is the
	// line Start to the left of the cursor
	//_cursorPos = 0; //TODO: this is wrong, should go to first non-whitespace first, then to zero
	_cursorPos = GetCurrentLineStart();
	_putCursorAtEnd = false;
	
	_currentStartIndex=_cursorPos;
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Get the index of the first char on the current line
//-----------------------------------------------------------------------------
int TextEntry::GetCurrentLineStart()
{
	if (!_multiline)			// quick out for non multline buffers
		return _currentStartIndex;
	
	int i;
	if (IsLineBreak(_cursorPos))
	{
		for (i = 0; i < m_LineBreaks.Count(); ++i )
		{
			if (_cursorPos == m_LineBreaks[i])
				break;
		}
		if (_cursorIsAtEnd)
		{
			if (i > 0)
			{
				return m_LineBreaks[i-1];
			}
			return m_LineBreaks[0];
		}
		else
			return _cursorPos; // we are already at Start
	}
	
	for ( i = 0; i < m_LineBreaks.Count(); ++i )
	{
		if (_cursorPos < m_LineBreaks[i])
		{
			if (i == 0)
				return 0;
			else
				return m_LineBreaks[i-1];
		}
	}
	// if there were no line breaks, the first char in the line is the Start of the buffer
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Move cursor to the end of the current line
//-----------------------------------------------------------------------------
void TextEntry::GotoEndOfLine()
{
	SelectCheck();
	// to get to the end of the line you have to take into account line wrap in the buffer
	// we have to figure out at which point the line wraps
	// given the current cursor position, select[1], find the index that is the
	// line end to the right of the cursor
	//_cursorPos=m_TextStream.Count(); //TODO: this is wrong, should go to last non-whitespace, then to true EOL
    _cursorPos = GetCurrentLineEnd();
	_putCursorAtEnd = true;
	
	ScrollRight();
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Get the index of the last char on the current line
//-----------------------------------------------------------------------------
int TextEntry::GetCurrentLineEnd()
{
	int i;
	if (IsLineBreak(_cursorPos)	)
	{
		for ( i = 0; i < m_LineBreaks.Count()-1; ++i )
		{
			if (_cursorPos == m_LineBreaks[i])
				break;
		}
		if (!_cursorIsAtEnd)
		{
			if (i == m_LineBreaks.Count()-2 )
				m_TextStream.Count();		
			else
				return m_LineBreaks[i+1];
		}
		else
			return _cursorPos; // we are already at end
	}
	
	for ( i = 0; i < m_LineBreaks.Count()-1; i++ )
	{
		if ( _cursorPos < m_LineBreaks[i])
		{
			return m_LineBreaks[i];
		}
	}
	return m_TextStream.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Insert a character into the text buffer
//-----------------------------------------------------------------------------
void TextEntry::InsertChar(wchar_t ch)
{
	// throw away redundant linefeed characters
	if (ch == '\r')
		return;
	
	// no newline characters in single-line dialogs
	if (!_multiline && ch == '\n')
		return;

	// no tab characters
	if (ch == '\t')
		return;

	if (m_bAllowNumericInputOnly)
	{
		if (!iswdigit(ch) && ((char)ch != '.'))
		{
			surface()->PlaySound("Resource\\warning.wav");
			return;
		}
	}
	
	// check against unicode characters
	if (!m_bAllowNonAsciiCharacters)
	{
		if (ch > 127)
			return;
	}

	// don't add characters if the max char count has been reached
	// ding at the user
	if (_maxCharCount > -1 && m_TextStream.Count() >= _maxCharCount)
	{
		if (_maxCharCount>0 && _multiline && _wrap)
		{
			// if we wrap lines rather than stopping
			while (m_TextStream.Count() > _maxCharCount)
			{
				if (_recalculateBreaksIndex==0) 
				{
					// we can get called before this has been run for the first time :)
					RecalculateLineBreaks();
				}
				if (m_LineBreaks[0]> m_TextStream.Count())
				{
					// if the line break is the past the end of the buffer recalc
					_recalculateBreaksIndex=-1;
					RecalculateLineBreaks();
				}
				
				if (m_LineBreaks[0]+1 < m_TextStream.Count())
				{
					// delete the line
					m_TextStream.RemoveMultiple(0, m_LineBreaks[0]);
					
					// in case we just deleted text from where the cursor is
					if (_cursorPos> m_TextStream.Count())
					{
						_cursorPos = m_TextStream.Count(); 
					}
					else
					{ // shift the cursor up. don't let it wander past zero
						_cursorPos-=m_LineBreaks[0]+1;
						if (_cursorPos<0)
						{
							_cursorPos=0;
						}
					}
					
					// move any selection area up
					if(_select[0]>-1)
					{
						_select[0] -=m_LineBreaks[0]+1;
						
						if(_select[0] <=0)
						{
							_select[0] =-1;
						}
						
						_select[1] -=m_LineBreaks[0]+1;
						if(_select[1] <=0)
						{
							_select[1] =-1;
						}
						
					}
					
					// now redraw the buffer
					for (int i = m_TextStream.Count() - 1; i >= 0; i--)
					{
						SetCharAt(m_TextStream[i], i+1);
					}
					
					// redo all the line breaks
					_recalculateBreaksIndex=-1;
					RecalculateLineBreaks();
					
				}
			}
			
		} 
		else
		{
			// make a sound
			// we've hit the max character limit
			surface()->PlaySound("Resource\\warning.wav");
			return;
		}
	}
	
	
	if (_wrap) 
	{
		// when wrapping you always insert the new char at the end of the buffer
		SetCharAt(ch, m_TextStream.Count());
		_cursorPos=m_TextStream.Count(); 
	}
	else 
	{
		// move chars right 1 starting from cursor, then replace cursorPos with char and increment cursor
		for (int i =  m_TextStream.Count()- 1; i >= _cursorPos; i--)
		{
			SetCharAt(m_TextStream[i], i+1);
		}
		
		SetCharAt(ch, _cursorPos);
		_cursorPos++;
	}
	
	// if its a newline char we can't do the slider until we recalc the line breaks
	if (ch == '\n')
	{
		RecalculateLineBreaks();
	}
	
	// see if we've hit the char limit
	if (m_bAutoProgressOnHittingCharLimit && m_TextStream.Count() == _maxCharCount)
	{
		// move the next panel (most likely another TextEntry)
		RequestFocusNext();
	}

	// scroll right if this pushed the cursor off screen
	ScrollRight();
	
	_dataChanged = true;
	
	CalcBreakIndex();
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Get the lineBreakIndex index of the line before the cursor
//			note _recalculateBreaksIndex < 0 flags RecalculateLineBreaks 
//			to figure it all out from scratch
//-----------------------------------------------------------------------------
void TextEntry::CalcBreakIndex()
{
	// an optimization to handle when the cursor is at the end of the buffer.
	// pays off if the buffer is large, and the search loop would be long.
	if (_cursorPos == m_TextStream.Count())
	{
		// we know m_LineBreaks array always has at least one element in it (99999 sentinel)
		// when there is just one line this will make recalc = -1 which is ok.
		_recalculateBreaksIndex = m_LineBreaks.Count()-2;
		return;
	}
	
	_recalculateBreaksIndex=0;
	// find the line break just before the cursor position
	while (_cursorPos > m_LineBreaks[_recalculateBreaksIndex])
		++_recalculateBreaksIndex;
	
	// -1  is ok.
	--_recalculateBreaksIndex;	
}

//-----------------------------------------------------------------------------
// Purpose: Insert a string into the text buffer, this is just a series
//			of char inserts because we have to check each char is ok to insert
//-----------------------------------------------------------------------------
void TextEntry::InsertString(const wchar_t *wszText)
{
	SaveUndoState();

	for (const wchar_t *ch = wszText; *ch != 0; ++ch)
	{
		InsertChar(*ch);
	}
	
	if (_dataChanged)
	{
		FireActionSignal();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Converts an ansi string to unicode and inserts it into the text stream
//-----------------------------------------------------------------------------
void TextEntry::InsertString(const char *text)
{
	// check for to see if the string is in the localization tables
	if (text[0] == '#')
	{
		wchar_t *wsz = g_pVGuiLocalize->Find(text);
		if (wsz)
		{
			InsertString(wsz);
			return;
		}
	}

	// straight convert the ansi to unicode and insert
	wchar_t unicode[1024];
	g_pVGuiLocalize->ConvertANSIToUnicode(text, unicode, sizeof(unicode));
	InsertString(unicode);
}

//-----------------------------------------------------------------------------
// Purpose: Handle the effect of user hitting backspace key
//			we delete the char before the cursor and reformat the text so it
//			behaves like in windows.
//-----------------------------------------------------------------------------
void TextEntry::Backspace()
{
	if (!IsEditable())
		return;

	//if you are at the first position don't do anything
	if(_cursorPos==0)
	{
		return;
	}
	
	//if the line is empty, don't do anything
	if(m_TextStream.Count()==0)
	{
		return;
	}
	
	SaveUndoState();
	
	//shift chars left one, starting at the cursor position, then make the line one smaller
	for(int i=_cursorPos;i<m_TextStream.Count(); ++i)
	{
		SetCharAt(m_TextStream[i],i-1);
	}
	m_TextStream.Remove(m_TextStream.Count() - 1);
	
	// As we hit the Start of the window, expose more chars so we can see what we are deleting
	if (_cursorPos==_currentStartIndex)
	{
		// windows tabs over 6 chars
		if (_currentStartIndex-6 >= 0) // dont scroll if there are not enough chars to scroll
		{
			_currentStartIndex-=6; 
		}
		else
			_currentStartIndex=0;
	}
	
	//move the cursor left one
	_cursorPos--;
	
	_dataChanged = true;
	
	// recalculate linebreaks (the fast incremental linebreak function doesn't work in this case)
	_recalculateBreaksIndex = 0;
	m_LineBreaks.RemoveAll();
	m_LineBreaks.AddToTail(BUFFER_SIZE);

	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Deletes the current selection, if any, moving the cursor to the Start
//			of the selection
//-----------------------------------------------------------------------------
void TextEntry::DeleteSelected()
{
	if (!IsEditable())
		return;

	// if the line is empty, don't do anything
	if (m_TextStream.Count() == 0)
		return;
	
	// get the range to delete
	int x0, x1;
	if (!GetSelectedRange(x0, x1))
	{
		// no selection, don't touch anything
		return;
	}
	
	SaveUndoState();
	
	// shift chars left one starting after cursor position, then make the line one smaller
	int dif = x1 - x0;
	for (int i = 0; i < dif; ++i)
	{
		m_TextStream.Remove(x0);
	}
	
	// clear any selection
	SelectNone();
	ResetCursorBlink();
	
	// move the cursor to just after the deleted section
	_cursorPos = x0;
	
	_dataChanged = true;
	
	_recalculateBreaksIndex = 0;
	m_LineBreaks.RemoveAll();
	m_LineBreaks.AddToTail(BUFFER_SIZE);

	CalcBreakIndex();
	
	LayoutVerticalScrollBarSlider();
}

//-----------------------------------------------------------------------------
// Purpose: Handle the effect of the user hitting the delete key
//			removes the char in front of the cursor
//-----------------------------------------------------------------------------
void TextEntry::Delete()
{
	if (!IsEditable())
		return;

	// if the line is empty, don't do anything
	if (m_TextStream.Count() == 0)
		return;
	
	// get the range to delete
	int x0, x1;
	if (!GetSelectedRange(x0, x1))
	{
		// no selection, so just delete the one character
		x0 = _cursorPos;
		x1 = x0 + 1;
		
		// if we're at the end of the line don't do anything
		if (_cursorPos >= m_TextStream.Count())
			return;
	}
	
	SaveUndoState();
	
	// shift chars left one starting after cursor position, then make the line one smaller
	int dif = x1 - x0;
	for (int i = 0; i < dif; i++)
	{
		m_TextStream.Remove((int)x0);
	}
	
	ResetCursorBlink();
	
	// clear any selection
	SelectNone();
	
	// move the cursor to just after the deleted section
	_cursorPos = x0;
	
	_dataChanged = true;
	
	_recalculateBreaksIndex = 0;
	m_LineBreaks.RemoveAll();
	m_LineBreaks.AddToTail(BUFFER_SIZE);

	CalcBreakIndex();
	
	LayoutVerticalScrollBarSlider();
}

//-----------------------------------------------------------------------------
// Purpose: Declare a selection empty
//-----------------------------------------------------------------------------
void TextEntry::SelectNone()
{
	// tag the selection as empty
	_select[0] = -1;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Load in the selection range so cx0 is the Start and cx1 is the end
//			from smallest to highest (right to left)
//-----------------------------------------------------------------------------
bool TextEntry::GetSelectedRange(int& cx0,int& cx1)
{
	// if there is nothing selected return false
	if (_select[0] == -1)
	{
		return false;
	}
	
	// sort the two position so cx0 is the smallest
	cx0=_select[0];
	cx1=_select[1];
	int temp;
	if(cx1<cx0){temp=cx0;cx0=cx1;cx1=temp;}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Opens the cut/copy/paste dropdown menu
//-----------------------------------------------------------------------------
void TextEntry::OpenEditMenu()
{
	// get cursor position, this is local to this text edit window
	int cursorX, cursorY;
	input()->GetCursorPos(cursorX, cursorY);
	
	/* !!	disabled since it recursively gets panel pointers, potentially across dll boundaries, 
			and doesn't need to be necessary (it's just for handling windowed mode)

	// find the frame that has no parent (the one on the desktop)
	Panel *panel = this;
	while ( panel->GetParent() != NULL)
	{
		panel = panel->GetParent();
	}
	panel->ScreenToLocal(cursorX, cursorY);
	int x, y;
	// get base panel's postition
	panel->GetPos(x, y);	  
	
	// adjust our cursor position accordingly
	cursorX += x;
	cursorY += y;
	*/
	
	int x0, x1;
	if (GetSelectedRange(x0, x1)) // there is something selected
	{
		m_pEditMenu->SetItemEnabled("&Cut", true);
		m_pEditMenu->SetItemEnabled("C&opy", true);
	}
	else	// there is nothing selected, disable cut/copy options
	{
		m_pEditMenu->SetItemEnabled("&Cut", false);
		m_pEditMenu->SetItemEnabled("C&opy", false);
	}
	m_pEditMenu->SetVisible(true);
	m_pEditMenu->RequestFocus();
	
	// relayout the menu immediately so that we know it's size
	m_pEditMenu->InvalidateLayout(true);
	int menuWide, menuTall;
	m_pEditMenu->GetSize(menuWide, menuTall);
	
	// work out where the cursor is and therefore the best place to put the menu
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	
	if (wide - menuWide > cursorX)
	{
		// menu hanging right
		if (tall - menuTall > cursorY)
		{
			// menu hanging down
			m_pEditMenu->SetPos(cursorX, cursorY);
		}
		else
		{
			// menu hanging up
			m_pEditMenu->SetPos(cursorX, cursorY - menuTall);
		}
	}
	else
	{
		// menu hanging left
		if (tall - menuTall > cursorY)
		{
			// menu hanging down
			m_pEditMenu->SetPos(cursorX - menuWide, cursorY);
		}
		else
		{
			// menu hanging up
			m_pEditMenu->SetPos(cursorX - menuWide, cursorY - menuTall);
		}
	}
	
	m_pEditMenu->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Cuts the selected chars from the buffer and 
//          copies them into the clipboard
//-----------------------------------------------------------------------------
void TextEntry::CutSelected()
{
	CopySelected();
	DeleteSelected();
	// have to request focus if we used the menu
	RequestFocus();	

	if ( _dataChanged )
	{
		FireActionSignal();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Copies the selected chars into the clipboard
//-----------------------------------------------------------------------------
void TextEntry::CopySelected()
{
	if (_hideText)
		return;
	
	int x0, x1;
	if (GetSelectedRange(x0, x1))
	{
		CUtlVector<wchar_t> buf;
		for (int i = x0; i < x1; i++)
		{
			if ( m_TextStream[i]=='\n') 
			{
				buf.AddToTail( '\r' );
			}
			buf.AddToTail(m_TextStream[i]);
		}
		buf.AddToTail('\0');
		system()->SetClipboardText(buf.Base(), buf.Count());
	}
	
	// have to request focus if we used the menu
	RequestFocus();	
	
	if ( _dataChanged )
	{
		FireActionSignal();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pastes the selected chars from the clipboard into the text buffer
//			truncates if text is longer than our _maxCharCount
//-----------------------------------------------------------------------------
void TextEntry::Paste()
{
	if (!IsEditable())
		return;

	CUtlVector<wchar_t> buf;
	int bufferSize = system()->GetClipboardTextCount();
	if (!m_bAutoProgressOnHittingCharLimit)
	{
		bufferSize = _maxCharCount > 0 ? _maxCharCount + 1 : system()->GetClipboardTextCount();  // +1 for terminator
	}

	buf.AddMultipleToTail(bufferSize);
	int len = system()->GetClipboardText(0, buf.Base(), bufferSize * sizeof(wchar_t));
	if (len < 1)
		return;
	
	SaveUndoState();
	bool bHaveMovedFocusAwayFromCurrentEntry = false;

	// insert all the characters
	for (int i = 0; i < len && buf[i] != 0; i++)
	{
		if (m_bAutoProgressOnHittingCharLimit)
		{
			// see if we're about to hit the char limit
			if (m_TextStream.Count() == _maxCharCount)
			{
				// move the next panel (most likely another TextEntry)
				RequestFocusNext();
				// copy the remainder into the clipboard
				wchar_t *remainingText = &buf[i];
				system()->SetClipboardText(remainingText, len - i - 1);
				// set the next entry to paste
				if (GetVParent() && ipanel()->GetCurrentKeyFocus(GetVParent()) != GetVPanel())
				{
					bHaveMovedFocusAwayFromCurrentEntry = true;
					ipanel()->SendMessage(ipanel()->GetCurrentKeyFocus(GetVParent()), new KeyValues("DoPaste"), GetVPanel());
				}
				break;
			}
		}

		// insert the character
		InsertChar(buf[i]);
	}

	// restore the original clipboard text if neccessary
	if (m_bAutoProgressOnHittingCharLimit)
	{
		system()->SetClipboardText(buf.Base(), bufferSize);
	}
	
	_dataChanged = true;
	FireActionSignal();

	if (!bHaveMovedFocusAwayFromCurrentEntry)
	{
		// have to request focus if we used the menu
		RequestFocus();	
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reverts back to last saved changes
//-----------------------------------------------------------------------------
void TextEntry::Undo()
{
	_cursorPos = _undoCursorPos;
	m_TextStream.CopyArray(m_UndoTextStream.Base(), m_UndoTextStream.Count());
	
	InvalidateLayout();
	Repaint();
	SelectNone();
}

//-----------------------------------------------------------------------------
// Purpose: Saves the current state to the undo stack
//-----------------------------------------------------------------------------
void TextEntry::SaveUndoState()
{
	_undoCursorPos = _cursorPos;
	m_UndoTextStream.CopyArray(m_TextStream.Base(), m_TextStream.Count());
}

//-----------------------------------------------------------------------------
// Purpose: Returns the index in the text buffer of the
//          character the drawing should Start at
//-----------------------------------------------------------------------------
int TextEntry::GetStartDrawIndex(int &lineBreakIndexIndex)
{
	int startIndex = 0;
	
	int numLines = m_LineBreaks.Count();
	int startLine = 0;
	
	// determine the Start point from the scroll bar
	// do this only if we are not selecting text in the window with the mouse
	if (_vertScrollBar && !_mouseDragSelection)
	{	
		// skip to line indicated by scrollbar
		startLine = _vertScrollBar->GetValue();
	}
	else
	{
		// check to see if the cursor is off the screen-multiline case
		HFont font = _font;
		int displayLines = GetTall() / (surface()->GetFontTall(font) + DRAW_OFFSET_Y());
		if (displayLines < 1)
		{
			displayLines = 1;
		}
		if (numLines > displayLines)
		{
			int cursorLine = GetCursorLine();
			
			startLine = _currentStartLine;
			
			// see if that is visible
			if (cursorLine < _currentStartLine)
			{
				// cursor is above visible area; scroll back
				startLine = cursorLine;
				if (_vertScrollBar)
				{
					MoveScrollBar( 1 ); // should be calibrated for speed 
					// adjust startline incase we hit a limit
					startLine = _vertScrollBar->GetValue(); 
				}
			}
			else if (cursorLine > (_currentStartLine + displayLines - 1))
			{
				// cursor is down below visible area; scroll forward
				startLine = cursorLine - displayLines + 1;
				if (_vertScrollBar)
				{
					MoveScrollBar( -1 );
					startLine = _vertScrollBar->GetValue();
				}
			}
		}
		else if (!_multiline)
		{
			// check to see if cursor is off the right side of screen-single line case
			// get cursor's x coordinate in pixel space
			bool done = false;
			while ( !done )
			{
				done = true;
				int x = DRAW_OFFSET_X();
				for (int i = _currentStartIndex; i < m_TextStream.Count(); i++)
				{
					done = false;
					wchar_t ch = m_TextStream[i];			
					if (_hideText)
					{
						ch = '*';
					}
					
					// if we've found the position, break
					if (_cursorPos == i)
					{
						break;
					}
					
					// add to the current position		
					x += getCharWidth(font, ch);				
				}
				
				if ( x >= GetWide() )
				{
					_currentStartIndex++;
					// Keep searching...
					continue;
				}
				
				if ( x <= 0 )
				{
					// dont go past the Start of buffer
					if (_currentStartIndex > 0)
						_currentStartIndex--;
				}

				break;
			}
		}
	}
	
	if (startLine > 0)
	{
		lineBreakIndexIndex = startLine;
		if (startLine && startLine < m_LineBreaks.Count())
		{
			startIndex = m_LineBreaks[startLine - 1];
		}
	}
	
	if (!_horizScrollingAllowed)
		return 0;

	_currentStartLine = startLine;
	if (_multiline)
		return startIndex;
	else 
		return _currentStartIndex;

	
}

// helper accessors for common gets
float TextEntry::GetValueAsFloat()
{
	int nTextLength = GetTextLength() + 1;
	char* txt = ( char* )_alloca( nTextLength * sizeof( char ) );
	GetText( txt, nTextLength );

	return V_atof( txt );
}

int TextEntry::GetValueAsInt()
{
	int nTextLength = GetTextLength() + 1;
	char* txt = ( char* )_alloca( nTextLength * sizeof( char ) );
	GetText( txt, nTextLength );

	return V_atoi( txt );
}

//-----------------------------------------------------------------------------
// Purpose: Get a string from text buffer
// Input:	offset - index to Start reading from 
//			bufLenInBytes - length of string
//-----------------------------------------------------------------------------
void TextEntry::GetText(OUT_Z_BYTECAP(bufLenInBytes) char *buf, int bufLenInBytes)
{
	Assert(bufLenInBytes >= sizeof(buf[0]));
	if (m_TextStream.Count())
	{
		// temporarily null terminate the text stream so we can use the conversion function
		int nullTerminatorIndex = m_TextStream.AddToTail((wchar_t)0);
		g_pVGuiLocalize->ConvertUnicodeToANSI(m_TextStream.Base(), buf, bufLenInBytes);
		m_TextStream.FastRemove(nullTerminatorIndex);
	}
	else
	{
		// no characters in the stream
		buf[0] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get a string from text buffer
// Input:	offset - index to Start reading from 
//			bufLen - length of string
//-----------------------------------------------------------------------------
void TextEntry::GetText(OUT_Z_BYTECAP(bufLenInBytes) wchar_t *wbuf, int bufLenInBytes)
{
	Assert(bufLenInBytes >= sizeof(wbuf[0]));
	int len = m_TextStream.Count();
	if (m_TextStream.Count())
	{
		int terminator = min(len, (bufLenInBytes / (int)sizeof(wchar_t)) - 1);
		wcsncpy(wbuf, m_TextStream.Base(), terminator);
		wbuf[terminator] = 0;
	}
	else
	{
		wbuf[0] = 0;
	}
}

void TextEntry::GetTextRange( wchar_t *buf, int from, int numchars )
{
	int len = m_TextStream.Count();
	int cpChars = max( 0, min( numchars, len - from ) );
	
	wcsncpy( buf, m_TextStream.Base() + max( 0, min( len, from ) ), cpChars );
	buf[ cpChars ] = 0;
}

void TextEntry::GetTextRange( char *buf, int from, int numchars )
{
	int len = m_TextStream.Count();
	int cpChars = max( 0, min( numchars, len - from ) );

	g_pVGuiLocalize->ConvertUnicodeToANSI( m_TextStream.Base() + max( 0, min( len, from ) ), buf, cpChars + 1 );
	buf[ cpChars ] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message that the text has changed
//-----------------------------------------------------------------------------
void TextEntry::FireActionSignal()
{
	PostActionSignal(new KeyValues("TextChanged"));
	_dataChanged = false;	// reset the data changed flag
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Set the font of the buffer text 
// Input:	font to change to
//-----------------------------------------------------------------------------
void TextEntry::SetFont(HFont font)
{
	_font = font;
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the scrollbar slider is moved
//-----------------------------------------------------------------------------
void TextEntry::OnSliderMoved()
{
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool TextEntry::RequestInfo(KeyValues *outputData)
{
	if (!stricmp(outputData->GetName(), "GetText"))
	{
		wchar_t wbuf[256];
		GetText(wbuf, 255);
		outputData->SetWString("text", wbuf);
		return true;
	}
	else if (!stricmp(outputData->GetName(), "GetState"))
	{
		char buf[64];
		GetText(buf, sizeof(buf));
		outputData->SetInt("state", atoi(buf));
		return true;
	}
	return BaseClass::RequestInfo(outputData);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextEntry::OnSetText(const wchar_t *text)
{
	SetText(text);
}

//-----------------------------------------------------------------------------
// Purpose: as above, but sets an integer
//-----------------------------------------------------------------------------
void TextEntry::OnSetState(int state)
{
	char buf[64];
	Q_snprintf(buf, sizeof(buf), "%d", state);
	SetText(buf);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextEntry::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	_font = scheme()->GetIScheme( GetScheme() )->GetFont( inResourceData->GetString( "font", "Default" ), IsProportional() );
	SetFont( _font );

	SetTextHidden((bool)inResourceData->GetInt("textHidden", 0));
	SetEditable((bool)inResourceData->GetInt("editable", 1));
	SetMaximumCharCount(inResourceData->GetInt("maxchars", -1));
	SetAllowNumericInputOnly(inResourceData->GetInt("NumericInputOnly", 0));
	SetAllowNonAsciiCharacters(inResourceData->GetInt("unicode", 0));
	SelectAllOnFirstFocus(inResourceData->GetInt("selectallonfirstfocus", 0));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextEntry::GetSettings( KeyValues *outResourceData )
{
	BaseClass::GetSettings( outResourceData );
	outResourceData->SetInt("textHidden", _hideText);
	outResourceData->SetInt("editable", IsEditable());
	outResourceData->SetInt("maxchars", GetMaximumCharCount());
	outResourceData->SetInt("NumericInputOnly", m_bAllowNumericInputOnly);
	outResourceData->SetInt("unicode", m_bAllowNonAsciiCharacters);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *TextEntry::GetDescription()
{
	static char buf[1024];
	Q_snprintf(buf, sizeof(buf), "%s, bool textHidden, bool editable, bool unicode, bool NumericInputOnly, int maxchars", BaseClass::GetDescription());
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of lines in the window
//-----------------------------------------------------------------------------
int TextEntry::GetNumLines()
{
	return m_LineBreaks.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the height of the text entry window so all text will fit inside
//-----------------------------------------------------------------------------
void TextEntry::SetToFullHeight()
{
	PerformLayout();
	int wide, tall;
	GetSize(wide, tall);
	
	tall = GetNumLines() * (surface()->GetFontTall(_font) + DRAW_OFFSET_Y()) + DRAW_OFFSET_Y() + 2;
	SetSize (wide, tall);
	PerformLayout();
	
}

//-----------------------------------------------------------------------------
// Purpose: Select all the text.
//-----------------------------------------------------------------------------
void TextEntry::SelectAllText( bool bResetCursorPos )
{
	// if there's no text at all, select none
	if ( m_TextStream.Count() == 0 )
	{
		_select[0] = -1;
	}
	else
	{
		_select[0] = 0;
	}

	_select[1] = m_TextStream.Count();

	if ( bResetCursorPos )
	{
		_cursorPos = _select[1];
	}
}

//-----------------------------------------------------------------------------
// Purpose: Select no text.
//-----------------------------------------------------------------------------
void TextEntry::SelectNoText()
{
	_select[0] = -1;
	_select[1] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the width of the text entry window so all text will fit inside
//-----------------------------------------------------------------------------
void TextEntry::SetToFullWidth()
{
	// probably be problems if you try using this on multi line buffers
	// or buffers with clickable text in them.
	if (_multiline)
		return;
	
	PerformLayout();
	int wide = 2*DRAW_OFFSET_X(); // buffer on left and right end of text.
	
	// loop through all the characters and sum their widths	
	for (int i = 0; i < m_TextStream.Count(); ++i)
	{
		wide += getCharWidth(_font, m_TextStream[i]);	
	}
	
	// height of one line of text
	int tall = (surface()->GetFontTall(_font) + DRAW_OFFSET_Y()) + DRAW_OFFSET_Y() + 2;
	
	SetSize (wide, tall);
	PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextEntry::SelectAllOnFirstFocus( bool status )
{
	_selectAllOnFirstFocus = status;
}

void TextEntry::SelectAllOnFocusAlways( bool status )
{
	_selectAllOnFirstFocus = status;
	_selectAllOnFocusAlways = status;
}

//-----------------------------------------------------------------------------
// Purpose: called when the text entry receives focus
//-----------------------------------------------------------------------------
void TextEntry::OnSetFocus()
{ 
	// see if we should highlight all on selection
    if (_selectAllOnFirstFocus)
	{
		_select[1] = m_TextStream.Count();
		_select[0] = _select[1] > 0 ? 0 : -1;
		_cursorPos = _select[1]; // cursor at end of line
		if ( !_selectAllOnFocusAlways )
		{
			_selectAllOnFirstFocus = false;
		}
    }
	else if (input()->IsKeyDown(KEY_TAB) || input()->WasKeyReleased(KEY_TAB))
	{
		// if we've tabbed to this field then move to the end of the text
		GotoTextEnd();
		// clear any selection
		SelectNone();
	}
	
	BaseClass::OnSetFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Set the width we have to draw text in.
//			Do not use in multiline windows.
//-----------------------------------------------------------------------------
void TextEntry::SetDrawWidth(int width)
{
	_drawWidth = width;
}

//-----------------------------------------------------------------------------
// Purpose: Get the width we have to draw text in.
//-----------------------------------------------------------------------------
int TextEntry::GetDrawWidth()
{
	return _drawWidth;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void TextEntry::SetAllowNonAsciiCharacters(bool state)
{
	m_bAllowNonAsciiCharacters = state;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void TextEntry::SetAllowNumericInputOnly(bool state)
{
	m_bAllowNumericInputOnly = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : forward - 
//-----------------------------------------------------------------------------
void TextEntry::OnChangeIME( bool forward )
{
	// Only change ime if Unicode aware
	if ( m_bAllowNonAsciiCharacters )
	{
		input()->OnChangeIME( forward );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : handleValue - 
//-----------------------------------------------------------------------------
void TextEntry::LanguageChanged( int handleValue )
{
	input()->OnChangeIMEByHandle( handleValue );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : handleValue - 
//-----------------------------------------------------------------------------
void TextEntry::ConversionModeChanged( int handleValue )
{
	input()->OnChangeIMEConversionModeByHandle( handleValue );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : handleValue - 
//-----------------------------------------------------------------------------
void TextEntry::SentenceModeChanged( int handleValue )
{
	input()->OnChangeIMESentenceModeByHandle( handleValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *compstr - 
//-----------------------------------------------------------------------------
void TextEntry::CompositionString( const wchar_t *compstr )
{
	wcsncpy( m_szComposition, compstr, sizeof( m_szComposition ) / sizeof( wchar_t ) - 1 );
	m_szComposition[  sizeof( m_szComposition ) / sizeof( wchar_t ) - 1 ] = L'\0';
}

void TextEntry::ShowIMECandidates()
{
	HideIMECandidates();

	int c = input()->GetCandidateListCount();
	if ( c == 0 )
	{
		return;
	}

	m_pIMECandidates = new Menu( this, "IMECandidatesMenu" );
	
	int pageStart = input()->GetCandidateListPageStart();
	int pageSize = input()->GetCandidateListPageSize();
	int selected = input()->GetCandidateListSelectedItem();

	int startAtOne = input()->CandidateListStartsAtOne() ? 1 : 0;

	if ( ( selected < pageStart ) || ( selected >= pageStart + pageSize ) )
	{
		pageStart = ( selected / pageSize ) * pageSize;
		input()->SetCandidateListPageStart( pageStart );
	}

	for ( int i = pageStart; i < pageStart + pageSize; ++i )
	{
		if ( i >= c )
			continue;

		bool isSelected = ( i == selected ) ? true : false;

		wchar_t unicode[ 32 ];
		input()->GetCandidate( i, unicode, sizeof( unicode ) );

		wchar_t label[ 64 ];
		_snwprintf( label, sizeof( label ) / sizeof( wchar_t ) - 1, L"%i %s", i - pageStart + startAtOne, unicode );
		label[ sizeof( label ) / sizeof( wchar_t ) - 1 ] = L'\0';

		int id = m_pIMECandidates->AddMenuItem( "Candidate", label, (KeyValues *)NULL, this );
		if ( isSelected )
		{
			m_pIMECandidates->SetCurrentlyHighlightedItem( id );
		}
	}
	
	m_pIMECandidates->SetVisible(true);
	m_pIMECandidates->SetParent(this);
	m_pIMECandidates->AddActionSignalTarget(this);
	m_pIMECandidates->SetKeyBoardInputEnabled( false );

	int cx, cy;
	CursorToPixelSpace(_cursorPos, cx, cy);
	cy = GetTall();

	LocalToScreen( cx, cy );

	//m_pIMECandidates->SetPos( cx, cy );

	// relayout the menu immediately so that we know it's size
	m_pIMECandidates->InvalidateLayout(true);
	int menuWide, menuTall;
	m_pIMECandidates->GetSize(menuWide, menuTall);
	
	// work out where the cursor is and therefore the best place to put the menu
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	
	if (wide - menuWide > cx)
	{
		// menu hanging right
		if (tall - menuTall > cy)
		{
			// menu hanging down
			m_pIMECandidates->SetPos(cx, cy);
		}
		else
		{
			// menu hanging up
			m_pIMECandidates->SetPos(cx, cy - menuTall - GetTall());
		}
	}
	else
	{
		// menu hanging left
		if (tall - menuTall > cy)
		{
			// menu hanging down
			m_pIMECandidates->SetPos(cx - menuWide, cy);
		}
		else
		{
			// menu hanging up
			m_pIMECandidates->SetPos(cx - menuWide, cy - menuTall-GetTall());
		}
	}
}

void TextEntry::HideIMECandidates()
{
	if ( m_pIMECandidates )
	{
		m_pIMECandidates->SetVisible( false );
	}
	delete m_pIMECandidates;
	m_pIMECandidates = NULL;
}

void TextEntry::UpdateIMECandidates()
{
	if ( !m_pIMECandidates )
		return;

	int c = input()->GetCandidateListCount();
	if ( c == 0 )
	{
		HideIMECandidates();
		return;
	}

	int oldCount = m_pIMECandidates->GetItemCount();
	int newCount = input()->GetCandidateListPageSize();

	if ( oldCount != newCount )
	{
		// Recreate the entire menu
		ShowIMECandidates();
		return;
	}

	int pageSize = input()->GetCandidateListPageSize();
	int selected = input()->GetCandidateListSelectedItem();
	int pageStart = input()->GetCandidateListPageStart();

	if ( ( selected < pageStart ) || selected >= pageStart + pageSize )
	{
		pageStart = ( selected / pageSize ) * pageSize;
		input()->SetCandidateListPageStart( pageStart );
	}

	int startAtOne = input()->CandidateListStartsAtOne() ? 1 : 0;

	for ( int i = pageStart; i < pageStart + pageSize; ++i )
	{
		int id = m_pIMECandidates->GetMenuID( i - pageStart );

		MenuItem *item = m_pIMECandidates->GetMenuItem( id );
		if ( !item )
			continue;

		if ( i >= c )
		{
			item->SetVisible( false );
			continue;
		}
		else
		{
			item->SetVisible( true );
		}

		bool isSelected = ( i == selected ) ? true : false;

		wchar_t unicode[ 32 ];
		input()->GetCandidate( i, unicode, sizeof( unicode ) );

		wchar_t label[ 64 ];
		_snwprintf( label, sizeof( label ) / sizeof( wchar_t ) - 1, L"%i %s", i - pageStart + startAtOne, unicode );
		label[ sizeof( label ) / sizeof( wchar_t ) - 1 ] = L'\0';
		item->SetText( label );
		if ( isSelected )
		{
			m_pIMECandidates->SetCurrentlyHighlightedItem( id );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextEntry::FlipToLastIME()
{
	int hCurrentIME = input()->GetCurrentIMEHandle();
	int hEnglishIME = input()->GetEnglishIMEHandle();

	bool isEnglish = ( hCurrentIME == hEnglishIME ) ? true : false;

	// If in english, flip back to previous
	if ( isEnglish )
	{
		input()->OnChangeIMEByHandle( m_hPreviousIME );
	}
	else
	{
		// If not, remember language and flip to english...
		m_hPreviousIME = hCurrentIME;
		input()->OnChangeIMEByHandle( hEnglishIME );
	}
}

void TextEntry::SetDrawLanguageIDAtLeft( bool state )
{
	m_bDrawLanguageIDAtLeft = state;
}

bool TextEntry::GetDropContextMenu( Menu *menu, CUtlVector< KeyValues * >& msglist )
{
	menu->AddMenuItem( "replace", "#TextEntry_ReplaceText", "replace", this );
	menu->AddMenuItem( "append", "#TextEntry_AppendText", "append", this );
	menu->AddMenuItem( "prepend", "#TextEntry_PrependText", "prepend", this );
	return true;
}

bool TextEntry::IsDroppable( CUtlVector< KeyValues * >& msglist )
{
	if ( msglist.Count() != 1 )
		return false;

	if ( !IsEnabled() )
		return false;

	KeyValues *msg = msglist[ 0 ];

	const wchar_t *txt = msg->GetWString( "text", L"" );
	if ( !txt || txt[ 0 ] == L'\0' )
		return false;

	return true;
}

void TextEntry::OnPanelDropped( CUtlVector< KeyValues * >& msglist )
{
	if ( msglist.Count() != 1 )
		return;

	KeyValues *data = msglist[ 0 ];

	const wchar_t *newText = data->GetWString( "text" );
	if ( !newText || newText[ 0 ] == L'\0' )
		return;

	char const *cmd = data->GetString( "command" );
	if ( !Q_stricmp( cmd, "replace" ) ||
		 !Q_stricmp( cmd, "default" ) )
	{
		SetText( newText );
		_dataChanged = true;
		FireActionSignal();
	}
	else if ( !Q_stricmp( cmd, "append" ) )
	{
		int newLen = V_wcslen( newText );
		int curLen = m_TextStream.Count();

		int outsize = (int)( sizeof( wchar_t ) * ( newLen + curLen + 1 ) );
		wchar_t *out = (wchar_t *)_alloca( outsize );
		Q_memset( out, 0, outsize );
		wcsncpy( out, m_TextStream.Base(), curLen );
		wcsncat( out, newText, V_wcslen( newText ) );
		out[ newLen + curLen ] = L'\0';
		SetText( out );
		_dataChanged = true;
		FireActionSignal();
	}
	else if ( !Q_stricmp( cmd, "prepend" ) )
	{
		int newLen = V_wcslen( newText );
		int curLen = m_TextStream.Count();

		int outsize = (int)( sizeof( wchar_t ) * ( newLen + curLen + 1 ) );
		wchar_t *out = (wchar_t *)_alloca( outsize );
		Q_memset( out, 0, outsize );
		wcsncpy( out, newText, V_wcslen( newText ) );
		wcsncat( out, m_TextStream.Base(), curLen );
		out[ newLen + curLen ] = L'\0';
		SetText( out );
		_dataChanged = true;
		FireActionSignal();
	}
}

int TextEntry::GetTextLength() const
{
	return m_TextStream.Count();
}

bool TextEntry::IsTextFullySelected() const 
{
	if ( _select[ 0 ] != 0 )
		return false;

	if ( _select[ 1 ] != GetTextLength() )
		return false;

	return true;
}

void TextEntry::SetUseFallbackFont( bool bState, HFont hFallback )
{
	m_bUseFallbackFont = bState;
	m_hFallbackFont = hFallback;
}
