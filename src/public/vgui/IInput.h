//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef VGUI_IINPUT_H
#define VGUI_IINPUT_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include "tier1/interface.h"
#include "vgui/MouseCode.h"
#include "vgui/KeyCode.h"

namespace vgui
{

class Cursor;
typedef uint32 HCursor;

#define VGUI_GCS_COMPREADSTR                 0x0001
#define VGUI_GCS_COMPREADATTR                0x0002
#define VGUI_GCS_COMPREADCLAUSE              0x0004
#define VGUI_GCS_COMPSTR                     0x0008
#define VGUI_GCS_COMPATTR                    0x0010
#define VGUI_GCS_COMPCLAUSE                  0x0020
#define VGUI_GCS_CURSORPOS                   0x0080
#define VGUI_GCS_DELTASTART                  0x0100
#define VGUI_GCS_RESULTREADSTR               0x0200
#define VGUI_GCS_RESULTREADCLAUSE            0x0400
#define VGUI_GCS_RESULTSTR                   0x0800
#define VGUI_GCS_RESULTCLAUSE                0x1000
// style bit flags for WM_IME_COMPOSITION
#define VGUI_CS_INSERTCHAR                   0x2000
#define VGUI_CS_NOMOVECARET                  0x4000

#define MESSAGE_CURSOR_POS -1
#define MESSAGE_CURRENT_KEYFOCUS -2


class IInput : public IBaseInterface
{
public:
	virtual void SetMouseFocus(VPANEL newMouseFocus) = 0;
	virtual void SetMouseCapture(VPANEL panel) = 0;

	// returns the string name of a scan code
	virtual void GetKeyCodeText(KeyCode code, OUT_Z_BYTECAP(buflen) char *buf, int buflen) = 0;

	// focus
	virtual VPANEL GetFocus() = 0;
	virtual VPANEL GetCalculatedFocus() = 0;// to handle cases where the focus changes inside a frame.
	virtual VPANEL GetMouseOver() = 0;		// returns the panel the mouse is currently over, ignoring mouse capture

	// mouse state
	virtual void SetCursorPos(int x, int y) = 0;
	virtual void GetCursorPos(int &x, int &y) = 0;
	virtual bool WasMousePressed(MouseCode code) = 0;
	virtual bool WasMouseDoublePressed(MouseCode code) = 0;
	virtual bool IsMouseDown(MouseCode code) = 0;

	// cursor override
	virtual void SetCursorOveride(HCursor cursor) = 0;
	virtual HCursor GetCursorOveride() = 0;

	// key state
	virtual bool WasMouseReleased(MouseCode code) = 0;
	virtual bool WasKeyPressed(KeyCode code) = 0;
	virtual bool IsKeyDown(KeyCode code) = 0;
	virtual bool WasKeyTyped(KeyCode code) = 0;
	virtual bool WasKeyReleased(KeyCode code) = 0;
	
	virtual VPANEL GetAppModalSurface() = 0;
	// set the modal dialog panel.
	// all events will go only to this panel and its children.
	virtual void SetAppModalSurface(VPANEL panel) = 0;
	// release the modal dialog panel
	// do this when your modal dialog finishes.
	virtual void ReleaseAppModalSurface() = 0;

	virtual void GetCursorPosition( int &x, int &y ) = 0;

	virtual void SetIMEWindow( void *hwnd ) = 0;
	virtual void *GetIMEWindow() = 0;

	virtual void OnChangeIME( bool forward ) = 0;
	virtual int  GetCurrentIMEHandle() = 0;
	virtual int  GetEnglishIMEHandle() = 0;

	// Returns the Language Bar label (Chinese, Korean, Japanese, Russion, Thai, etc.)
	virtual void GetIMELanguageName( OUT_Z_BYTECAP(unicodeBufferSizeInBytes) wchar_t *buf, int unicodeBufferSizeInBytes ) = 0;
	// Returns the short code for the language (EN, CH, KO, JP, RU, TH, etc. ).
	virtual void GetIMELanguageShortCode( OUT_Z_BYTECAP(unicodeBufferSizeInBytes) wchar_t *buf, int unicodeBufferSizeInBytes ) = 0;

	struct LanguageItem
	{
		wchar_t		shortname[ 4 ];
		wchar_t		menuname[ 128 ];
		int			handleValue;
		bool		active; // true if this is the active language
	};

	struct ConversionModeItem
	{
		wchar_t		menuname[ 128 ];
		int			handleValue;
		bool		active; // true if this is the active conversion mode
	};

	struct SentenceModeItem
	{
		wchar_t		menuname[ 128 ];
		int			handleValue;
		bool		active; // true if this is the active sentence mode
	};

	// Call with NULL dest to get item count
	virtual int	 GetIMELanguageList( LanguageItem *dest, int destcount ) = 0;
	virtual int	 GetIMEConversionModes( ConversionModeItem *dest, int destcount ) = 0;
	virtual int	 GetIMESentenceModes( SentenceModeItem *dest, int destcount ) = 0;

	virtual void OnChangeIMEByHandle( int handleValue ) = 0;
	virtual void OnChangeIMEConversionModeByHandle( int handleValue ) = 0;
	virtual void OnChangeIMESentenceModeByHandle( int handleValue ) = 0;

	virtual void OnInputLanguageChanged() = 0;
	virtual void OnIMEStartComposition() = 0;
	virtual void OnIMEComposition( int flags ) = 0;
	virtual void OnIMEEndComposition() = 0;

	virtual void OnIMEShowCandidates() = 0;
	virtual void OnIMEChangeCandidates() = 0;
	virtual void OnIMECloseCandidates() = 0;
	virtual void OnIMERecomputeModes() = 0;

	virtual int  GetCandidateListCount() = 0;
	virtual void GetCandidate( int num, OUT_Z_BYTECAP(destSizeBytes) wchar_t *dest, int destSizeBytes ) = 0;
	virtual int  GetCandidateListSelectedItem() = 0;
	virtual int  GetCandidateListPageSize() = 0;
	virtual int  GetCandidateListPageStart() = 0;
	
	//NOTE:  We render our own candidate lists most of the time...
	virtual void SetCandidateWindowPos( int x, int y ) = 0;

	virtual bool GetShouldInvertCompositionString() = 0;
	virtual bool CandidateListStartsAtOne() = 0;

	virtual void SetCandidateListPageStart( int start ) = 0;

	// Passes in a keycode which allows hitting other mouse buttons w/o cancelling capture mode
	virtual void SetMouseCaptureEx(VPANEL panel, MouseCode captureStartMouseCode ) = 0;

	// Because OnKeyCodeTyped uses CallParentFunction and is therefore message based, there's no way
	//  to know if handler actually swallowed the specified keycode.  To get around this, I set a global before calling the
	//  kb focus OnKeyCodeTyped function and if we ever get to a Panel::OnKeyCodeTypes we know that nobody handled the message
	//  and in that case we can post a message to any "unhandled keycode" listeners
	// This will generate an MESSAGE_FUNC_INT( "KeyCodeUnhandled" "code" code ) message to each such listener
	virtual void RegisterKeyCodeUnhandledListener( VPANEL panel ) = 0;
	virtual void UnregisterKeyCodeUnhandledListener( VPANEL panel ) = 0;

	// Posts unhandled message to all interested panels
	virtual void OnKeyCodeUnhandled( int keyCode ) = 0;

	// Assumes subTree is a child panel of the root panel for the vgui contect
	//  if restrictMessagesToSubTree is true, then mouse and kb messages are only routed to the subTree and it's children and mouse/kb focus
	//   can only be on one of the subTree children, if a mouse click occurs outside of the subtree, and "UnhandledMouseClick" message is sent to unhandledMouseClickListener panel
	//   if it's set
	//  if restrictMessagesToSubTree is false, then mouse and kb messages are routed as normal except that they are not routed down into the subtree
	//   however, if a mouse click occurs outside of the subtree, and "UnhandleMouseClick" message is sent to unhandledMouseClickListener panel
	//   if it's set
	virtual void	SetModalSubTree( VPANEL subTree, VPANEL unhandledMouseClickListener, bool restrictMessagesToSubTree = true ) = 0;
	virtual void	ReleaseModalSubTree() = 0;
	virtual VPANEL	GetModalSubTree() = 0;

	// These toggle whether the modal subtree is exclusively receiving messages or conversely whether it's being excluded from receiving messages
	// Sends a "ModalSubTree", state message
	virtual void	SetModalSubTreeReceiveMessages( bool state ) = 0;
	virtual bool	ShouldModalSubTreeReceiveMessages() const = 0;

	virtual VPANEL 	GetMouseCapture() = 0;
};

#define VGUI_INPUT_INTERFACE_VERSION "VGUI_Input005"

} // namespace vgui


#endif // VGUI_IINPUT_H
