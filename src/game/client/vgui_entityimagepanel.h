//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a panel which is rendered image on top of an entity
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef VGUI_ENTITYIMAGEPANEL_H
#define VGUI_ENTITYIMAGEPANEL_H

#include "vgui_EntityPanel.h"
#include "shareddefs.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class C_BaseEntity;
class KeyValues;
class BitmapImage;

//-----------------------------------------------------------------------------
// This is a base class for a panel which always is rendered on top of an entity
//-----------------------------------------------------------------------------

class CEntityImagePanel : public CEntityPanel
{
	DECLARE_CLASS( CEntityImagePanel, CEntityPanel );

public:
	// constructor
	CEntityImagePanel( vgui::Panel* pParent, const char *panelName );
	~CEntityImagePanel();

	// initialization
	virtual bool Init( KeyValues* pInitData, C_BaseEntity* pEntity );

	bool ShouldDraw();

	virtual void Paint( void );
	virtual void PaintBackground( void ) {}

private:
	// The bitmap to render
	BitmapImage *m_pImage;

protected:
	int m_r, m_g, m_b, m_a;
};


//-----------------------------------------------------------------------------
// Purpose: Same as above, but understands how to parse color/material out of
//  Team1/Team2 sections
//-----------------------------------------------------------------------------
class CEntityTeamImagePanel : public CEntityImagePanel
{
	DECLARE_CLASS( CEntityTeamImagePanel, CEntityImagePanel );

public:
	CEntityTeamImagePanel( vgui::Panel* pParent, const char *panelName );
	~CEntityTeamImagePanel( void );
	// initialization
	virtual bool Init( KeyValues* pInitData, C_BaseEntity* pEntity );

	virtual void Paint( void );

private:
	struct TEAMIMAGE
	{
		BitmapImage *m_pImage;
		int m_r, m_g, m_b, m_a;
	};

	TEAMIMAGE m_Images[ MAX_TEAMS ];
};

#endif //  VGUI_ENTITYIMAGEPANEL_H
