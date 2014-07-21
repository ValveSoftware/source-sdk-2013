//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef IINPUTINTERNAL_H
#define IINPUTINTERNAL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IInput.h>

namespace vgui
{

enum MouseCodeState_t
{
	BUTTON_RELEASED = 0,
	BUTTON_PRESSED,
	BUTTON_DOUBLECLICKED,
};

typedef int HInputContext;

#define DEFAULT_INPUT_CONTEXT ((vgui::HInputContext)~0)

class IInputInternal : public IInput
{
public:
	// processes input for a frame
	virtual void RunFrame() = 0;

	virtual void UpdateMouseFocus(int x, int y) = 0;

	// called when a panel becomes invalid
	virtual void PanelDeleted(VPANEL panel) = 0;

	// inputs into vgui input handling 
	virtual bool InternalCursorMoved(int x,int y) = 0; //expects input in surface space
	virtual bool InternalMousePressed(MouseCode code) = 0;
	virtual bool InternalMouseDoublePressed(MouseCode code) = 0;
	virtual bool InternalMouseReleased(MouseCode code) = 0;
	virtual bool InternalMouseWheeled(int delta) = 0;
	virtual bool InternalKeyCodePressed(KeyCode code) = 0;
	virtual void InternalKeyCodeTyped(KeyCode code) = 0;
	virtual void InternalKeyTyped(wchar_t unichar) = 0;
	virtual bool InternalKeyCodeReleased(KeyCode code) = 0;

	// Creates/ destroys "input" contexts, which contains information
	// about which controls have mouse + key focus, for example.
	virtual HInputContext CreateInputContext() = 0;
	virtual void DestroyInputContext( HInputContext context ) = 0; 

	// Associates a particular panel with an input context
	// Associating NULL is valid; it disconnects the panel from the context
	virtual void AssociatePanelWithInputContext( HInputContext context, VPANEL pRoot ) = 0;

	// Activates a particular input context, use DEFAULT_INPUT_CONTEXT
	// to get the one normally used by VGUI
	virtual void ActivateInputContext( HInputContext context ) = 0;

	// This method is called to post a cursor message to the current input context
	virtual void PostCursorMessage() = 0;

	// Cursor position; this is the current position read from the input queue.
	// We need to set it because client code may read this during Mouse Pressed
	// events, etc.
	virtual void UpdateCursorPosInternal( int x, int y ) = 0;

	// Called to handle explicit calls to CursorSetPos after input processing is complete
	virtual void HandleExplicitSetCursor( ) = 0;

	// Updates the internal key/mouse state associated with the current input context without sending messages
	virtual void SetKeyCodeState( KeyCode code, bool bPressed ) = 0;
	virtual void SetMouseCodeState( MouseCode code, MouseCodeState_t state ) = 0;
	virtual void UpdateButtonState( const InputEvent_t &event ) = 0;
};

} // namespace vgui

#define VGUI_INPUTINTERNAL_INTERFACE_VERSION "VGUI_InputInternal001"

#endif // IINPUTINTERNAL_H
