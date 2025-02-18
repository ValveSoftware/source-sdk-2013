//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#include <vgui/IInput.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <KeyValues.h>

#include <vgui_controls/Label.h>
#include <vgui_controls/Image.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/Controls.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( Label, Label );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
Label::Label(Panel *parent, const char *panelName, const char *text) : BaseClass(parent, panelName)
{
	Init();

	_textImage = new TextImage(text);
	_textImage->SetColor(Color(0, 0, 0, 0));
	SetText(text);
	_textImageIndex = AddImage(_textImage, 0);

	REGISTER_COLOR_AS_OVERRIDABLE( _disabledFgColor2, "disabledfgcolor2_override" );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
Label::Label(Panel *parent, const char *panelName, const wchar_t *wszText) : BaseClass(parent, panelName)
{
	Init();

	_textImage = new TextImage(wszText);
	_textImage->SetColor(Color(0, 0, 0, 0));
	SetText(wszText);
	_textImageIndex = AddImage(_textImage, 0);

	REGISTER_COLOR_AS_OVERRIDABLE( _disabledFgColor2, "disabledfgcolor2_override" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
Label::~Label()
{
	delete _textImage;
	delete [] _associateName;
	delete [] _fontOverrideName;
}

//-----------------------------------------------------------------------------
// Purpose: Construct the label
//-----------------------------------------------------------------------------
void Label::Init()
{
	_contentAlignment = a_west;
	_textColorState = CS_NORMAL;
	_textInset[0] = 0;
	_textInset[1] = 0;
	_hotkey = 0;
	_associate = NULL;
	_associateName = NULL;
	_fontOverrideName = NULL;
	m_bWrap = false;
	m_bCenterWrap = false;
	m_bAutoWideToContents = false;
	m_bAutoTallToContents = false;
	m_bUseProportionalInsets = false;
	m_bAutoWideDirty = false;
	m_bAutoTallDirty = false;

//	SetPaintBackgroundEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: Set whether the text is displayed bright or dull
//-----------------------------------------------------------------------------
void Label::SetTextColorState(EColorState state)
{
	if (_textColorState != state)
	{
		_textColorState = state;
		InvalidateLayout();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return the full size of the contained content
//-----------------------------------------------------------------------------
void Label::GetContentSize(int &wide, int &tall)
{
	if( GetFont() == INVALID_FONT ) // we haven't loaded our font yet, so load it now
	{
		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		if ( pScheme )
		{
			SetFont( pScheme->GetFont( "Default", IsProportional() ) );
		}
	}


	int tx0, ty0, tx1, ty1;
	ComputeAlignment(tx0, ty0, tx1, ty1);

	// the +8 is padding to the content size
	// the code which uses it should really set that itself; 
	// however a lot of existing code relies on this
	wide = (tx1 - tx0) + _textInset[0]; 

	// get the size of the text image and remove it
	int iWide, iTall;
	_textImage->GetSize(iWide, iTall);
	wide -=  iWide;
	// get the full, untruncated (no elipsis) size of the text image.
	_textImage->GetContentSize(iWide, iTall);
	wide += iWide;

	// addin the image offsets as well
	for (int i=0; i < _imageDar.Size(); i++)
		wide += _imageDar[i].offset;

	tall = max((ty1 - ty0) + _textInset[1], iTall);
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the keyboard key that is a hotkey 
//-----------------------------------------------------------------------------
wchar_t Label::CalculateHotkey(const char *text)
{
	for (const char *ch = text; *ch != 0; ch++)
	{
		if (*ch == '&')
			{
			// get the next character
			ch++;

			if (*ch == '&')
			{
				// just an &
				continue;
			}
			else if (*ch == 0)
			{
				break;
			}
			else if (V_isalnum(*ch))
			{
				// found the hotkey
				return (wchar_t)tolower(*ch);
			}
		}
	}

	return '\0';
}

wchar_t Label::CalculateHotkey(const wchar_t *text)
{
	if( text )
	{
		for (const wchar_t *ch = text; *ch != 0; ch++)
		{
			if (*ch == '&')
			{
				// get the next character
				ch++;

				if (*ch == '&')
				{
					// just an &
					continue;
				}
				else if (*ch == 0)
				{
					break;
				}
				else if (iswalnum(*ch))
				{
					// found the hotkey
					return (wchar_t)towlower(*ch);
				}
			}
		}
	}

	return '\0';
}

//-----------------------------------------------------------------------------
// Purpose: Check if this label has a hotkey that is the key passed in.
//-----------------------------------------------------------------------------
Panel *Label::HasHotkey(wchar_t key)
{
#ifdef VGUI_HOTKEYS_ENABLED
	if ( iswalnum( key ) )
		key = towlower( key );

	if (_hotkey == key)
		return this;
#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Set the hotkey for this label
//-----------------------------------------------------------------------------
void Label::SetHotkey(wchar_t ch)
{
	_hotkey = ch;
}

wchar_t Label::GetHotKey()
{
	return _hotkey;
}

//-----------------------------------------------------------------------------
// Purpose: Handle a hotkey by passing on focus to associate
//-----------------------------------------------------------------------------
void Label::OnHotkeyPressed()
{
	// we can't accept focus, but if we are associated to a control give it to that
	if (_associate.Get() && !IsBuildModeActive())
	{
		_associate->RequestFocus();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Redirect mouse pressed to giving focus to associate
//-----------------------------------------------------------------------------
void Label::OnMousePressed(MouseCode code)
{
	if (_associate.Get() && !IsBuildModeActive())
	{
		_associate->RequestFocus();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return the text in the label
//-----------------------------------------------------------------------------
void Label::GetText(char *textOut, int bufferLen)
{
	_textImage->GetText(textOut, bufferLen);
}

//-----------------------------------------------------------------------------
// Purpose: Return the text in the label
//-----------------------------------------------------------------------------
void Label::GetText(wchar_t *textOut, int bufLenInBytes)
{
	_textImage->GetText(textOut, bufLenInBytes);
}

//-----------------------------------------------------------------------------
// Purpose: Take the string and looks it up in the localization file 
//          to convert it to unicode
//			Setting the text will not set the size of the label.
//			Set the size explicitly or use setToContent()
//-----------------------------------------------------------------------------
void Label::SetText(const char *text)
{
	// if set to null, just make blank
	if (!text)
	{
		text = "";
	}

	// let the text image do the translation itself
	_textImage->SetText(text);

	if( text[0] == '#' )
	{
		SetHotkey(CalculateHotkey(g_pVGuiLocalize->Find(text)));		
	}
	else
	{	
		SetHotkey(CalculateHotkey(text));
	}

	m_bAutoWideDirty = m_bAutoWideToContents;
	m_bAutoTallDirty = m_bAutoTallToContents;

	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Set unicode text directly
//-----------------------------------------------------------------------------
void Label::SetText(const wchar_t *unicodeString, bool bClearUnlocalizedSymbol)
{
	m_bAutoWideDirty = m_bAutoWideToContents;
	m_bAutoTallDirty = m_bAutoTallToContents;

	if ( unicodeString && _textImage->GetUText() && !Q_wcscmp(unicodeString,_textImage->GetUText()) )
		return;

	_textImage->SetText(unicodeString, bClearUnlocalizedSymbol);

//!! need to calculate hotkey from translated string
	SetHotkey(CalculateHotkey(unicodeString));

    InvalidateLayout();     // possible that the textimage needs to expand
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: updates localized text
//-----------------------------------------------------------------------------
void Label::OnDialogVariablesChanged(KeyValues *dialogVariables )
{
	StringIndex_t index = _textImage->GetUnlocalizedTextSymbol();
	if (index != INVALID_LOCALIZE_STRING_INDEX)
	{
		// reconstruct the string from the variables
		wchar_t buf[4096];
		g_pVGuiLocalize->ConstructString(buf, sizeof(buf), index, dialogVariables);
		SetText(buf);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Additional offset at the Start of the text (from whichever side it is aligned)
//-----------------------------------------------------------------------------
void Label::SetTextInset(int xInset, int yInset)
{
	_textInset[0] = xInset;
	_textInset[1] = yInset;

	int wide, tall;
	GetSize( wide, tall);
	_textImage->SetDrawWidth(wide - _textInset[0]);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Label::GetTextInset(int *xInset, int *yInset )
{
	if ( xInset )
	{
		*xInset = _textInset[0];
	}
	if ( yInset )
	{
		*yInset = _textInset[1];
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the enabled state
//-----------------------------------------------------------------------------
void Label::SetEnabled(bool state)
{
	Panel::SetEnabled(state);
}

//-----------------------------------------------------------------------------
// Purpose: Calculates where in the panel the content resides
// Input  : &tx0 - [out] position of the content
//			&ty0 - 
//			&tx1 - 
//			&ty1 - 
// Note:	horizontal alignment is west if the image dar has
//			more than one image in it, this is because we use image sizes
//			to determine layout in classes for example, Menu.
//-----------------------------------------------------------------------------
void Label::ComputeAlignment(int &tx0, int &ty0, int &tx1, int &ty1)
{
	int wide, tall;
	GetPaintSize(wide, tall);
	int tWide,tTall;

	// text bounding box
	tx0 = 0;
	ty0 = 0;

	// loop through all the images and calculate the complete bounds
	int maxX = 0, maxY = 0;

	int actualXAlignment = _contentAlignment;
	for (int i = 0; i < _imageDar.Count(); i++)
	{
		TImageInfo &imageInfo = _imageDar[i];
		IImage *image = imageInfo.image;
		if (!image)
			continue; // skip over null images

		// add up the bounds
		int iWide, iTall;
		image->GetSize(iWide, iTall);
		if (iWide > wide) // if the image is larger than the label just do a west alignment
			actualXAlignment = Label::a_west;
		
		// get the max height
		maxY = max(maxY, iTall);
		maxX += iWide;

		// add the offset to x
		maxX += imageInfo.offset;
	}

	tWide = maxX;
	tTall = maxY;
	
	// x align text
	switch (actualXAlignment)
	{
		// left
		case Label::a_northwest:
		case Label::a_west:
		case Label::a_southwest:
		{
			tx0 = 0;
			break;
		}
		// center
		case Label::a_north:
		case Label::a_center:
		case Label::a_south:
		{
			tx0 = (wide - tWide) / 2;
			break;
		}
		// right
		case Label::a_northeast:
		case Label::a_east:
		case Label::a_southeast:
		{
			tx0 = wide - tWide;
			break;
		}
	}

	// y align text
	switch (_contentAlignment)
	{
		//top
		case Label::a_northwest:
		case Label::a_north:
		case Label::a_northeast:
		{
			ty0 = 0;
			break;
		}
		// center
		case Label::a_west:
		case Label::a_center:
		case Label::a_east:
		{
			ty0 = (tall - tTall) / 2;
			break;
		}
		// south
		case Label::a_southwest:
		case Label::a_south:
		case Label::a_southeast:
		{
			ty0 = tall - tTall;
			break;
		}
	}

	tx1 = tx0 + tWide;
	ty1 = ty0 + tTall;
}

//-----------------------------------------------------------------------------
// Purpose: overridden main drawing function for the panel
//-----------------------------------------------------------------------------
void Label::Paint()
{
	int tx0, ty0, tx1, ty1;
	ComputeAlignment(tx0, ty0, tx1, ty1);

	// calculate who our associate is if we haven't already
	if (_associateName)
	{
		SetAssociatedControl(FindSiblingByName(_associateName));
		delete [] _associateName;
		_associateName = NULL;
	}

	int labelWide, labelTall;
	GetSize(labelWide, labelTall);
	int x = tx0, y = _textInset[1] + ty0;
	int imageYPos = 0; // a place to save the y offset for when we draw the disable version of the image

	// draw the set of images
	for (int i = 0; i < _imageDar.Count(); i++)
	{
		TImageInfo &imageInfo = _imageDar[i];
		IImage *image = imageInfo.image;
		if (!image)
			continue; // skip over null images

		// add the offset to x
		x += imageInfo.offset;

		// if this is the text image then add its inset to the left or from the right
		if (i == _textImageIndex)
		{
			switch ( _contentAlignment )
			{
				// left
				case Label::a_northwest:
				case Label::a_west:
				case Label::a_southwest:
				{
					x += _textInset[0];
					break;
				}
				// right
				case Label::a_northeast:
				case Label::a_east:
				case Label::a_southeast:
				{
					x -= _textInset[0];
					break;
				}
			}
		}

		// see if the image is in a fixed position
		if (imageInfo.xpos >= 0)
		{
			x = imageInfo.xpos;
		}

		// draw
		imageYPos = y;
		image->SetPos(x, y);

		// fix up y for center-aligned text
		if (_contentAlignment == Label::a_west || _contentAlignment == Label::a_center || _contentAlignment == Label::a_east)
		{
			int iw, it;
			image->GetSize(iw, it);
			if (it < (ty1 - ty0))
			{
				imageYPos = ((ty1 - ty0) - it) / 2 + y;
				image->SetPos(x, ((ty1 - ty0) - it) / 2 + y);
			}
		}

		// don't resize the image unless its too big
		if (imageInfo.width >= 0)
		{
			int w, t;
			image->GetSize(w, t);
			if (w > imageInfo.width)
			{
				image->SetSize(imageInfo.width, t);
			}
		}

		// if it's the basic text image then draw specially
		if (image == _textImage)
		{
			if (IsEnabled())
			{
				if (_associate.Get() && ipanel()->HasParent(input()->GetFocus(), _associate->GetVPanel()))
				{
					_textImage->SetColor(_associateColor);
				}
				else
				{
					_textImage->SetColor(GetFgColor());
				}

				_textImage->Paint();
			}
			else
			{
				// draw disabled version, with embossed look
				// offset image
				_textImage->SetPos(x + 1, imageYPos + 1);
				_textImage->SetColor(_disabledFgColor1);
				_textImage->Paint();

				surface()->DrawFlushText();

				// overlayed image
				_textImage->SetPos(x, imageYPos);
				_textImage->SetColor(_disabledFgColor2);
				_textImage->Paint();
			}
		}
		else
		{
			image->Paint();
		}

		// add the image size to x
		int wide, tall;
		image->GetSize(wide, tall);
		x += wide;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Helper function, draws a simple line with dashing parameters
//-----------------------------------------------------------------------------
void Label::DrawDashedLine(int x0, int y0, int x1, int y1, int dashLen, int gapLen)
{
	// work out which way the line goes
	if ((x1 - x0) > (y1 - y0))
	{
		// x direction line
		while (1)
		{
			if (x0 + dashLen > x1)
			{
				// draw partial
				surface()->DrawFilledRect(x0, y0, x1, y1);
			}
			else
			{
				surface()->DrawFilledRect(x0, y0, x0 + dashLen, y1);
			}

			x0 += dashLen;

			if (x0 + gapLen > x1)
				break;

			x0 += gapLen;
		}
	}
	else
	{
		// y direction
		while (1)
		{
			if (y0 + dashLen > y1)
			{
				// draw partial
				surface()->DrawFilledRect(x0, y0, x1, y1);
			}
			else
			{
				surface()->DrawFilledRect(x0, y0, x1, y0 + dashLen);
			}

			y0 += dashLen;

			if (y0 + gapLen > y1)
				break;

			y0 += gapLen;
		}
	}
}

void Label::SetContentAlignment(Alignment alignment)
{
	_contentAlignment=alignment;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Size the width of the label to its contents - only works from in ApplySchemeSettings or PerformLayout()
//-----------------------------------------------------------------------------
void Label::SizeToContents()
{
	int wide, tall;
	GetContentSize(wide, tall);

	SetSize(wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: Set the font the text is drawn in
//-----------------------------------------------------------------------------
void Label::SetFont(HFont font)
{
	_textImage->SetFont(font);
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Resond to resizing of the panel
//-----------------------------------------------------------------------------
void Label::OnSizeChanged(int wide, int tall)
{
	InvalidateLayout();
	Panel::OnSizeChanged(wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: Get the font the textImage is drawn in.
//-----------------------------------------------------------------------------
HFont Label::GetFont()
{
	return _textImage->GetFont();
}

//-----------------------------------------------------------------------------
// Purpose: Set the foreground color of the Label
//-----------------------------------------------------------------------------
void Label::SetFgColor(Color color)
{
	if (!(GetFgColor() == color))
	{
		BaseClass::SetFgColor(color);
		_textImage->SetColor(color);
		Repaint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the foreground color of the Label
//-----------------------------------------------------------------------------
Color Label::GetFgColor()
{
	Color clr = Panel::GetFgColor();
	return clr;
}

//-----------------------------------------------------------------------------
// Purpose: Set the foreground color 1 color of the Label
//-----------------------------------------------------------------------------
void Label::SetDisabledFgColor1(Color color)
{
	_disabledFgColor1 = color;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Label::SetDisabledFgColor2(Color color)
{
	_disabledFgColor2 = color;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color Label::GetDisabledFgColor1()
{
	return _disabledFgColor1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color Label::GetDisabledFgColor2()
{
	return _disabledFgColor2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
TextImage *Label::GetTextImage()
{
	return _textImage;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool Label::RequestInfo(KeyValues *outputData)
{
	if (!stricmp(outputData->GetName(), "GetText"))
	{
		wchar_t wbuf[256];
		_textImage->GetText(wbuf, 255);
		outputData->SetWString("text", wbuf);
		return true;
	}

	return Panel::RequestInfo(outputData);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text from the message
//-----------------------------------------------------------------------------
void Label::OnSetText(KeyValues *params)
{
	KeyValues *pkvText = params->FindKey("text", false);
	if (!pkvText)
		return;

	if (pkvText->GetDataType() == KeyValues::TYPE_STRING)
	{
		SetText(pkvText->GetString());
	}
	else if (pkvText->GetDataType() == KeyValues::TYPE_WSTRING)
	{
		SetText(pkvText->GetWString());
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add an image to the list
//			returns the index the image was placed in
//-----------------------------------------------------------------------------
int Label::AddImage(IImage *image, int offset)
{
	int newImage = _imageDar.AddToTail();
	_imageDar[newImage].image = image;
	_imageDar[newImage].offset = (int)offset;
	_imageDar[newImage].xpos = -1;
	_imageDar[newImage].width = -1;
	InvalidateLayout();
	return newImage;
}

//-----------------------------------------------------------------------------
// Purpose: removes all images from the list
//			user is responsible for the memory
//-----------------------------------------------------------------------------
void Label::ClearImages()
{
	_imageDar.RemoveAll();
	_textImageIndex = -1;
}

void Label::ResetToSimpleTextImage()
{
	ClearImages();
	_textImageIndex = AddImage(_textImage, 0);
}


//-----------------------------------------------------------------------------
// Purpose: Multiple image handling
//			Images are drawn from left to right across the label, ordered by index
//			By default there is a TextImage in position 0
//			Set the contents of an IImage in the IImage array.
//-----------------------------------------------------------------------------
void Label::SetImageAtIndex(int index, IImage *image, int offset)
{
	EnsureImageCapacity(index);
//	Assert( image );

	if ( _imageDar[index].image != image || _imageDar[index].offset != offset)
	{
		_imageDar[index].image = image;
		_imageDar[index].offset = (int)offset;
		InvalidateLayout();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get an IImage in the IImage array.
//-----------------------------------------------------------------------------
IImage *Label::GetImageAtIndex(int index)
{
	if ( _imageDar.IsValidIndex( index ) )
		return _imageDar[index].image;
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of images in the array.
//-----------------------------------------------------------------------------
int Label::GetImageCount()
{
	return _imageDar.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Move where the default text image is within the image array 
//			(it starts in position 0)
// Input  : newIndex - 
// Output : int - the index the default text image was previously in
//-----------------------------------------------------------------------------
int Label::SetTextImageIndex(int newIndex)
{
	if (newIndex == _textImageIndex)
		return _textImageIndex;

	EnsureImageCapacity(newIndex);

	int oldIndex = _textImageIndex;
	if ( _textImageIndex >= 0 )
	{
		_imageDar[_textImageIndex].image = NULL;
	}
	if (newIndex > -1)
	{
		_imageDar[newIndex].image = _textImage;
	}
	_textImageIndex = newIndex;
	return oldIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Ensure that the maxIndex will be a valid index
//-----------------------------------------------------------------------------
void Label::EnsureImageCapacity(int maxIndex)
{
	while (_imageDar.Size() <= maxIndex)
	{
		AddImage(NULL, 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the offset in pixels before the image
//-----------------------------------------------------------------------------
void Label::SetImagePreOffset(int index, int preOffset)
{
	if (_imageDar.IsValidIndex(index) && _imageDar[index].offset != preOffset)
	{
		_imageDar[index].offset = (int)preOffset;
		InvalidateLayout();
	}
}

//-----------------------------------------------------------------------------
// Purpose: fixes the layout bounds of the image within the label
//-----------------------------------------------------------------------------
void Label::SetImageBounds(int index, int x, int width)
{
	_imageDar[index].xpos = (int)x;
	_imageDar[index].width = (int)width;
}

//-----------------------------------------------------------------------------
// Purpose: Labels can be associated with controls, and alter behaviour based on the associates behaviour
//			If the associate is disabled, so are we
//			If the associate has focus, we may alter how we draw
//			If we get a hotkey press or focus message, we forward the focus to the associate
//-----------------------------------------------------------------------------
void Label::SetAssociatedControl(Panel *control)
{
	if (control != this)
	{
		_associate = control;
	}
	else
	{
		// don't let the associate ever be set to be ourself
		_associate = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called after a panel requests focus to fix up the whole chain
//-----------------------------------------------------------------------------
void Label::OnRequestFocus(VPANEL subFocus, VPANEL defaultPanel)
{
	if (_associate.Get() && subFocus == GetVPanel() && !IsBuildModeActive())
	{
		// we've received focus; pass the focus onto the associate instead
		_associate->RequestFocus();
	}
	else
	{
		BaseClass::OnRequestFocus(subFocus, defaultPanel);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets custom settings from the scheme file
//-----------------------------------------------------------------------------
void Label::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	if (_fontOverrideName)
	{
		// use the custom specified font since we have one set
		SetFont(pScheme->GetFont(_fontOverrideName, IsProportional()));
	}
	if ( GetFont() == INVALID_FONT )
	{
		SetFont( pScheme->GetFont( "Default", IsProportional() ) );
	}	

	if ( m_bWrap || m_bCenterWrap )
	{
		//tell it how big it is
		int wide, tall;
		Panel::GetSize(wide, tall);
		wide -= _textInset[0];		// take inset into account
		_textImage->SetSize(wide, tall);

		_textImage->RecalculateNewLinePositions();
	}
	else
	{
		// if you don't set the size of the image, many, many buttons will break - we might want to look into fixing this all over the place later
		int wide, tall;
		_textImage->GetContentSize(wide, tall);
		_textImage->SetSize(wide, tall);
	}

	if ( m_bAutoWideToContents )
	{
		m_bAutoWideDirty = true;
	}

	if ( m_bAutoTallToContents )
	{
		m_bAutoTallDirty = true;
	}

	if ( m_bAutoWideToContents || m_bAutoTallToContents )
	{
		HandleAutoSizing();
	}

	// clear out any the images, since they will have been invalidated
	for (int i = 0; i < _imageDar.Count(); i++)
	{
		IImage *image = _imageDar[i].image;
		if (!image)
			continue; // skip over null images

		if (i == _textImageIndex)
			continue;

		_imageDar[i].image = NULL;
	}

	SetDisabledFgColor1(GetSchemeColor("Label.DisabledFgColor1", pScheme));
	SetDisabledFgColor2(GetSchemeColor("Label.DisabledFgColor2", pScheme));
	SetBgColor(GetSchemeColor("Label.BgColor", pScheme));

	switch (_textColorState)
	{
	case CS_DULL:
		SetFgColor(GetSchemeColor("Label.TextDullColor", pScheme));
		break;
	case CS_BRIGHT:
		SetFgColor(GetSchemeColor("Label.TextBrightColor", pScheme));
		break;
	case CS_NORMAL:
	default:
		SetFgColor(GetSchemeColor("Label.TextColor", pScheme));
		break;
	}

	_associateColor = GetSchemeColor("Label.SelectedTextColor", pScheme);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Label::GetSettings( KeyValues *outResourceData )
{
	// panel settings
	Panel::GetSettings( outResourceData );

	// label settings
	char buf[256];
	_textImage->GetUnlocalizedText( buf, 255 );
	if (!strnicmp(buf, "#var_", 5))
	{
		// strip off the variable prepender on save
		outResourceData->SetString( "labelText", buf + 5 );
	}
	else
	{
		outResourceData->SetString( "labelText", buf );
	}

	const char *alignmentString = "";
	switch ( _contentAlignment )
	{
	case a_northwest:	alignmentString = "north-west";	break;
	case a_north:		alignmentString = "north";		break;
	case a_northeast:	alignmentString = "north-east";	break;
	case a_center:		alignmentString = "center";		break;
	case a_east:		alignmentString = "east";		break;
	case a_southwest:	alignmentString = "south-west";	break;
	case a_south:		alignmentString = "south";		break;
	case a_southeast:	alignmentString = "south-east";	break;
	case a_west:	
	default:			alignmentString = "west";	break;
	}

	outResourceData->SetString( "textAlignment", alignmentString );

	if (_associate)
	{
		outResourceData->SetString("associate", _associate->GetName());
	}

	outResourceData->SetInt("dulltext", (int)(_textColorState == CS_DULL));
	outResourceData->SetInt("brighttext", (int)(_textColorState == CS_BRIGHT));

	if (_fontOverrideName)
	{
		outResourceData->SetString("font", _fontOverrideName);
	}
	
	outResourceData->SetInt("wrap", ( m_bWrap ? 1 : 0 ));
	outResourceData->SetInt("centerwrap", ( m_bCenterWrap ? 1 : 0 ));

	if ( m_bUseProportionalInsets )
	{
		outResourceData->SetInt("textinsetx", scheme()->GetProportionalNormalizedValueEx( GetScheme(), _textInset[0] ) );
		outResourceData->SetInt("textinsety", _textInset[1]);
	}
	else
	{
		outResourceData->SetInt("textinsetx", _textInset[0]);
		outResourceData->SetInt("textinsety", _textInset[1]);
	}
	outResourceData->SetInt("auto_wide_tocontents", ( m_bAutoWideToContents ? 1 : 0 ));
	outResourceData->SetInt("auto_tall_tocontents", ( m_bAutoTallToContents ? 1 : 0 ));
	outResourceData->SetInt("use_proportional_insets", ( m_bUseProportionalInsets ? 1 : 0 ));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Label::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	// label settings
	const char *labelText =	inResourceData->GetString( "labelText", NULL );
	if ( labelText )
	{
		if (labelText[0] == '%' && labelText[strlen(labelText) - 1] == '%')
		{
			// it's a variable, set it to be a special variable localized string
			wchar_t unicodeVar[256];
			g_pVGuiLocalize->ConvertANSIToUnicode(labelText, unicodeVar, sizeof(unicodeVar));

			char var[256];
			_snprintf(var, sizeof(var), "#var_%s", labelText);
			g_pVGuiLocalize->AddString(var + 1, unicodeVar, "");
			SetText(var);
		}
		else
		{
			SetText(labelText);
		}
	}

	// text alignment
	const char *alignmentString = inResourceData->GetString( "textAlignment", "" );
	int align = -1;

	if ( !stricmp(alignmentString, "north-west") )
	{
		align = a_northwest;
	}
	else if ( !stricmp(alignmentString, "north") )
	{
		align = a_north;
	}
	else if ( !stricmp(alignmentString, "north-east") )
	{
		align = a_northeast;
	}
	else if ( !stricmp(alignmentString, "west") )
	{
		align = a_west;
	}
	else if ( !stricmp(alignmentString, "center") )
	{
		align = a_center;
	}
	else if ( !stricmp(alignmentString, "east") )
	{
		align = a_east;
	}
	else if ( !stricmp(alignmentString, "south-west") )
	{
		align = a_southwest;
	}
	else if ( !stricmp(alignmentString, "south") )
	{
		align = a_south;
	}
	else if ( !stricmp(alignmentString, "south-east") )
	{
		align = a_southeast;
	}

	if ( align != -1 )
	{
		SetContentAlignment( (Alignment)align );
	}

	// the control we are to be associated with may not have been created yet,
	// so keep a pointer to it's name and calculate it when we can
	const char *associateName = inResourceData->GetString("associate", "");
	if (associateName[0] != 0)
	{
		int len = Q_strlen(associateName) + 1;
		_associateName = new char[ len ];
		Q_strncpy( _associateName, associateName, len );
	}

	if (inResourceData->GetInt("dulltext", 0) == 1)
	{
		SetTextColorState(CS_DULL);
	}
	else if (inResourceData->GetInt("brighttext", 0) == 1)
	{
		SetTextColorState(CS_BRIGHT);
	}
	else
	{
		SetTextColorState(CS_NORMAL);
	}

	// font settings
	const char *overrideFont = inResourceData->GetString("font", "");
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

	if (*overrideFont)
	{
		delete [] _fontOverrideName;
		int len = Q_strlen(overrideFont) + 1;
		_fontOverrideName = new char[ len ];
		Q_strncpy(_fontOverrideName, overrideFont, len );
		SetFont(pScheme->GetFont(_fontOverrideName, IsProportional()));
	}
	else if (_fontOverrideName)
	{
		delete [] _fontOverrideName;
		_fontOverrideName = NULL;
		SetFont(pScheme->GetFont("Default", IsProportional()));
	}

	bool bWrapText = inResourceData->GetInt("centerwrap", 0) > 0;
	SetCenterWrap( bWrapText );

	m_bAutoWideToContents = inResourceData->GetInt("auto_wide_tocontents", 0) > 0;
	m_bAutoTallToContents = inResourceData->GetInt("auto_tall_tocontents", 0) > 0;

	bWrapText = inResourceData->GetInt("wrap", 0) > 0;
	SetWrap( bWrapText );

	int inset_x = inResourceData->GetInt("textinsetx", _textInset[0]);
	int inset_y = inResourceData->GetInt("textinsety", _textInset[1]);
	float flInsetY = inResourceData->GetFloat("textinsety", (float)_textInset[1]);
	// Had to play it safe and add a new key for backwards compatibility
	m_bUseProportionalInsets = inResourceData->GetInt("use_proportional_insets", 0) > 0;
	
	// We need to check if "textinsetx" is set before we scale inset_x because code might
	// have changed inset_x to some value > 0, which would cause GetProportionalScaledValueEx()
	// to make inset_x to scale up every time this code is called
	if ( m_bUseProportionalInsets && !inResourceData->IsEmpty( "textinsetx" ) )
	{
		inset_x = scheme()->GetProportionalScaledValueEx( GetScheme(), inset_x );
	}

	if ( m_bUseProportionalInsets && !inResourceData->IsEmpty( "textinsety" ) )
	{
		int nScale = scheme()->GetProportionalScaledValueEx( GetScheme(), 1000 );
		inset_y = int( ceilf( ( flInsetY * nScale ) / 1000.0f ) );
	}

	SetTextInset( inset_x, inset_y );

	bool bAllCaps = inResourceData->GetInt("allcaps", 0) > 0;
	SetAllCaps( bAllCaps );

	InvalidateLayout(true);
}

//-----------------------------------------------------------------------------
// Purpose: Returns a description of the label string
//-----------------------------------------------------------------------------
const char *Label::GetDescription( void )
{
	static char buf[1024];
	Q_snprintf(buf, sizeof(buf), "%s, string labelText, string associate, alignment textAlignment, int wrap, int dulltext, int brighttext, string font", BaseClass::GetDescription());
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: If a label has images in _imageDar, the size
//			must take those into account as well as the textImage part
//			Textimage part will shrink ONLY if there is not enough room.
//-----------------------------------------------------------------------------
void Label::PerformLayout()
{
	int wide, tall;
	Panel::GetSize(wide, tall);
	wide -= _textInset[0]; // take inset into account

	// if we just have a textImage, this is trivial.
	if (_imageDar.Count() == 1 && _textImageIndex == 0)
	{	
		if ( m_bWrap || m_bCenterWrap )
		{
			int twide, ttall;
			_textImage->GetContentSize(twide, ttall);
			_textImage->SetSize(wide, ttall);
		}
		else
		{
			int twide, ttall;
			_textImage->GetContentSize(twide, ttall);
			
			// tell the textImage how much space we have to draw in
			if ( wide < twide)
				_textImage->SetSize(wide, ttall);
			else
				_textImage->SetSize(twide, ttall);
		}

		HandleAutoSizing();

		HandleAutoSizing();

		return;
	}

	// assume the images in the dar cannot be resized, and if
	// the images + the textimage are too wide we shring the textimage part
	if (_textImageIndex < 0)
		return;
	
	// get the size of the images
	int	widthOfImages = 0;
	for (int i = 0; i < _imageDar.Count(); i++)
	{
		TImageInfo &imageInfo = _imageDar[i];
		IImage *image = imageInfo.image;
		if (!image)
			continue; // skip over null images

		if (i == _textImageIndex)
			continue;

		// add up the bounds
		int iWide, iTall;
		image->GetSize(iWide, iTall);		
		widthOfImages += iWide;
	}

	// so this is how much room we have to draw the textimage part
	int spaceAvail = wide - widthOfImages;

	// if we have no space at all just leave everything as is.
	if (spaceAvail < 0)
		return;

	int twide, ttall;
	_textImage->GetSize (twide, ttall);
	// tell the textImage how much space we have to draw in
	_textImage->SetSize(spaceAvail, ttall);	

	HandleAutoSizing();
}

void Label::SetWrap( bool bWrap )
{
	m_bWrap = bWrap;
	_textImage->SetWrap( m_bWrap );

	InvalidateLayout();
}

void Label::SetCenterWrap( bool bWrap )
{
	m_bCenterWrap = bWrap;
	_textImage->SetCenterWrap( m_bCenterWrap );

	InvalidateLayout();
}

void Label::SetAllCaps( bool bAllCaps )
{
	m_bAllCaps = bAllCaps;
	_textImage->SetAllCaps( m_bAllCaps );

	InvalidateLayout();
}

void Label::HandleAutoSizing( void )
{
	if ( m_bAutoWideDirty || m_bAutoTallDirty )
	{
		// Change our width and or height to match our content
		int wide, tall;
		GetContentSize(wide, tall);
		wide = m_bAutoWideDirty ? wide : GetWide();
		tall = m_bAutoTallDirty ? tall : GetTall();

		m_bAutoTallDirty = false;
		m_bAutoWideDirty = false;

		SetSize(wide, tall);
	}
}



