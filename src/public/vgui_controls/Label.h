//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef LABEL_H
#define LABEL_H

#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "vgui/VGUI.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/PHandle.h"

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Contains and displays a set of images
//			By default starts with one TextImage
//-----------------------------------------------------------------------------
class Label : public Panel
{
	DECLARE_CLASS_SIMPLE( Label, Panel );

public:
	// Constructors
	Label(Panel *parent, const char *panelName, const char *text);
	Label(Panel *parent, const char *panelName, const wchar_t *wszText);
	~Label();

public:
	// Take the string and looks it up in the localization file to convert it to unicode
	virtual void SetText(const char *tokenName);

	// Set unicode text directly
	virtual void SetText(const wchar_t *unicodeString, bool bClearUnlocalizedSymbol = false );

	// Get the current text
	virtual void GetText(OUT_Z_BYTECAP(bufferLen) char *textOut, int bufferLen);
	virtual void GetText(OUT_Z_BYTECAP(bufLenInBytes) wchar_t *textOut, int bufLenInBytes);

	// Content alignment
	// Get the size of the content within the label
	virtual void GetContentSize(int &wide, int &tall);

	// Set how the content aligns itself within the label
	// alignment code, used to determine how the images are layed out within the Label
	enum Alignment
	{
		a_northwest = 0,
		a_north,
		a_northeast,
		a_west,
		a_center,
		a_east,
		a_southwest,
		a_south,
		a_southeast,
	};

	virtual void SetContentAlignment(Alignment alignment);
	virtual void SetEnabled(bool state);
	// Additional offset at the Start of the text (from whichever sides it is aligned)
	virtual void SetTextInset(int xInset, int yInset);		
	virtual void GetTextInset(int *xInset, int *yInset );

	// Text colors
	virtual void SetFgColor(Color color);
	virtual Color GetFgColor();

	// colors to use when the label is disabled
	virtual void SetDisabledFgColor1(Color color);
	virtual void SetDisabledFgColor2(Color color);
	virtual Color GetDisabledFgColor1();
	virtual Color GetDisabledFgColor2();

	// Set whether the text is displayed bright or dull
	enum EColorState
	{
		CS_NORMAL,
		CS_DULL,
		CS_BRIGHT,
	};
	virtual void SetTextColorState(EColorState state);

	// Font
	virtual void SetFont(HFont font);
	virtual HFont GetFont();

	// Hotkey
	virtual Panel *HasHotkey(wchar_t key);
	virtual void SetHotkey(wchar_t key);
	virtual wchar_t GetHotKey();

	// Labels can be associated with controls, and alter behaviour based on the associates behaviour
	// If the associate is disabled, so are we
	// If the associate has focus, we may alter how we draw
	// If we get a hotkey press or focus message, we forward the focus to the associate
	virtual void SetAssociatedControl(Panel *control);

	// Multiple image handling
	// Images are drawn from left to right across the label, ordered by index
	// By default there is a TextImage in position 0 (see GetTextImage()/SetTextImageIndex())
	virtual int AddImage(IImage *image, int preOffset);  // Return the index the image was placed in
	virtual void SetImageAtIndex(int index, IImage *image, int preOffset);	
	virtual void SetImagePreOffset(int index, int preOffset);  // Set the offset in pixels before the image
	virtual IImage *GetImageAtIndex(int index);
	virtual int GetImageCount();
	virtual void ClearImages();
	virtual void ResetToSimpleTextImage();
	// fixes the layout bounds of the image within the label
	virtual void SetImageBounds(int index, int x, int width);

	// Teturns a pointer to the default text image
	virtual TextImage *GetTextImage();

	// Moves where the default text image is within the image array (it starts in position 0)
	// Setting it to -1 removes it from the image list
	// Returns the index the default text image was previously in
	virtual int SetTextImageIndex(int newIndex);

	// Message handling
	// outputData - keyName is the name of the attribute requested.
	// for Labels "text" is an option that returns the default text image text
	// returns true on success in finding the requested value. false on failure.
	virtual bool RequestInfo(KeyValues *outputData);
	/* INFO HANDLING
		"GetText"
			returns:
				"text" - text contained in the label
	*/

	/* CUSTOM MESSAGE HANDLING
		"SetText"
			input:	"text"	- label text is set to be this string
	*/

	virtual void SizeToContents();

	// the +8 is padding to the content size
	// the code which uses it should really set that itself; 
	// however a lot of existing code relies on this
	enum Padding
	{
		Content = 8,
	};

	void SetWrap( bool bWrap );
	void SetCenterWrap( bool bWrap );

	void SetAllCaps( bool bAllCaps );

protected:
	virtual void PerformLayout();
	virtual wchar_t CalculateHotkey(const char *text);
	virtual wchar_t CalculateHotkey(const wchar_t *text);
	virtual void ComputeAlignment(int &tx0, int &ty0, int &tx1, int &ty1);
	virtual void Paint();
	MESSAGE_FUNC_PARAMS( OnSetText, "SetText", params );
	virtual void DrawDashedLine(int x0, int y0, int x1, int y1, int dashLen, int gapLen);
	virtual void OnRequestFocus(VPANEL subFocus, VPANEL defaultPanel);
	MESSAGE_FUNC( OnHotkeyPressed, "Hotkey" );
	virtual void OnMousePressed(MouseCode code);
	virtual void OnSizeChanged(int wide, int tall);

	// makes sure that the maxIndex will be a valid index
	virtual void EnsureImageCapacity(int maxIndex);

	// editing
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void GetSettings( KeyValues *outResourceData );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual const char *GetDescription( void );

	MESSAGE_FUNC_PARAMS( OnDialogVariablesChanged, "DialogVariables", dialogVariables );

	void HandleAutoSizing( void );

private:
	void Init();

	Alignment  _contentAlignment;
	TextImage *_textImage; // this is the textImage, if the full text will not
							// fit we put as much as we can and add an elipsis (...)
	struct TImageInfo
	{
		IImage *image;
		int offset;
		int xpos;
		int width;
	};
	CUtlVector<TImageInfo> _imageDar;

	int		   _textInset[2];
	Color      _disabledFgColor1;
	Color      _disabledFgColor2;
	Color	   _associateColor;
	int		   _textImageIndex;	// index in the image array that the default _textimage resides
	EColorState _textColorState;

	PHandle		_associate;
	char	  *_associateName;

	char	  *_fontOverrideName;

	wchar_t	   _hotkey;		// the hotkey contained in the text

	bool	m_bWrap;
	bool	m_bCenterWrap;
	bool	m_bAllCaps;
	bool	m_bAutoWideToContents;
	bool	m_bAutoTallToContents;
	bool	m_bAutoWideDirty;
	bool	m_bAutoTallDirty;
	bool	m_bUseProportionalInsets;

};

} // namespace vgui

#endif // LABEL_H
