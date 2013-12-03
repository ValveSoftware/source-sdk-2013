//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VGUI_DEFAULTINPUTSIGNAL_H
#define VGUI_DEFAULTINPUTSIGNAL_H
#ifdef _WIN32
#pragma once
#endif


#include "vgui_inputsignal.h"


namespace vgui
{
	// This class derives from vgui::InputSignal and implements empty defaults for all of its functions.
	class CDefaultInputSignal : public vgui::InputSignal
	{
	public:
		virtual void cursorMoved(int x,int y,Panel* panel)				{}
		virtual void cursorEntered(Panel* panel)						{}
		virtual void cursorExited(Panel* panel)							{}
		virtual void mousePressed(MouseCode code,Panel* panel)			{}
		virtual void mouseDoublePressed(MouseCode code,Panel* panel)	{}
		virtual void mouseReleased(MouseCode code,Panel* panel)			{}
		virtual void mouseWheeled(int delta,Panel* panel)				{}
		virtual void keyPressed(KeyCode code,Panel* panel)				{}
		virtual void keyTyped(KeyCode code,Panel* panel)				{}
		virtual void keyReleased(KeyCode code,Panel* panel)				{}
		virtual void keyFocusTicked(Panel* panel)						{}
	};
}


#endif // VGUI_DEFAULTINPUTSIGNAL_H
