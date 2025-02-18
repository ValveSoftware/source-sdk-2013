//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Clients CBaseObject
//
// $NoKeywords: $
//=============================================================================//

#ifndef OBJECTCONTROLPANEL_H
#define OBJECTCONTROLPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "c_vguiscreen.h"

namespace vgui
{
	class Panel;
	class Label;
	class Button;
}

class C_BaseObject;
class CRotationSlider;
class C_TFPlayer;

//-----------------------------------------------------------------------------
// Base class for all vgui screens on objects: 
//-----------------------------------------------------------------------------
class CObjectControlPanel : public CVGuiScreenPanel
{
	DECLARE_CLASS( CObjectControlPanel, CVGuiScreenPanel );

public:
	CObjectControlPanel( vgui::Panel *parent, const char *panelName );

	virtual bool Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData );
	virtual void OnCommand( const char *command );
	virtual void OnTick();

protected:
	// Method to add controls to particular panels
	vgui::Panel *GetActivePanel() { return m_pActivePanel; }

	// Override these to deal with various controls in various modes
	virtual void OnTickActive( C_BaseObject *pObj, C_TFPlayer *pLocalPlayer );

	C_BaseObject *GetOwningObject() const;

	// This should update the current panel and return that panel.
	virtual vgui::Panel* TickCurrentPanel();

	// Send a message to the owner.
	void SendToServerObject( const char *pMsg );

private:

	vgui::EditablePanel	*m_pActivePanel;

	vgui::Panel *m_pCurrentPanel;
};


// This is used for child panels. It forwards the messages to the parent panel.
class CCommandChainingPanel : public vgui::EditablePanel
{
	typedef vgui::EditablePanel BaseClass;

public:
	CCommandChainingPanel( vgui::Panel *parent, const char *panelName ) :
		BaseClass( parent, panelName ) 
	{
		SetPaintBackgroundEnabled( false );
	}

	void OnCommand( const char *command )
	{
		BaseClass::OnCommand( command );
		if (GetParent())
		{
			GetParent()->OnCommand(command);
		}
	}
};


//-----------------------------------------------------------------------------
// This is a panel for an object that has rotational controls 
//-----------------------------------------------------------------------------
class CRotatingObjectControlPanel : public CObjectControlPanel
{
	DECLARE_CLASS( CRotatingObjectControlPanel, CObjectControlPanel );

public:
	CRotatingObjectControlPanel( vgui::Panel *parent, const char *panelName );

	virtual bool Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData );

protected:
	virtual void OnTickActive( C_BaseObject *pObj, C_TFPlayer *pLocalPlayer );

private:
	CRotationSlider *m_pRotationSlider;
	vgui::Label *m_pRotationLabel;
};

#endif // OBJECTCONTROLPANEL_H
