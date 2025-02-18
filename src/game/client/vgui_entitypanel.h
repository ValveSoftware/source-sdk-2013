//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a panel which is rendered on top of an entity
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef VGUI_ENTITYPANEL_H
#define VGUI_ENTITYPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include <vgui/MouseCode.h>
#include "vgui_basepanel.h"

// forward declarations
class KeyValues;


//-----------------------------------------------------------------------------
// This is a base class for a panel which always is rendered on top of an entity
//-----------------------------------------------------------------------------
class CEntityPanel : public CBasePanel
{
public:
	DECLARE_CLASS( CEntityPanel, CBasePanel );

	// constructor
	CEntityPanel( vgui::Panel *pParent, const char *panelName );

	virtual void	ComputeParent( void );
	virtual void	ComputeAndSetSize( void );

	// Initialize from key values
	bool Init( KeyValues* pKeyValues, C_BaseEntity* pEntity );

	// Determine where our entity is in screen space.
	void GetEntityPosition( int& sx, int& sy );

	// Base implementation of ShouldDraw
	bool ShouldDraw();

	// called when we're ticked (updates our position)...
	virtual void OnTick( void );

	virtual void OnCursorEntered();
	virtual void OnCursorExited();

	const char *GetMouseOverText( void );

	C_BaseEntity* GetEntity() { return (C_BaseEntity*)m_pBaseEntity; }

	// Attach to a new entity
	void	SetEntity( C_BaseEntity* pEntity );

public:
	enum
	{
		MAX_ENTITY_MOUSEOVER = 256
	};

	// Offset from entity that we should draw
	int m_OffsetX, m_OffsetY;

	char			m_szMouseOverText[ MAX_ENTITY_MOUSEOVER ];

	bool			m_bShowInNormal;

	int				m_iOrgWidth;
	int				m_iOrgHeight;
	int				m_iOrgOffsetX;
	int				m_iOrgOffsetY;
	float			m_flScale;

private:
	// This is the entity to which we're attached
	EHANDLE			m_pBaseEntity;
};

#endif //  VGUI_ENTITYPANEL_H
