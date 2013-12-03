//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FUNC_AREAPORTALWINDOW_H
#define FUNC_AREAPORTALWINDOW_H
#ifdef _WIN32
#pragma once
#endif


#include "baseentity.h"
#include "utllinkedlist.h"
#include "func_areaportalbase.h"


class CFuncAreaPortalWindow : public CFuncAreaPortalBase
{
public:
	DECLARE_CLASS( CFuncAreaPortalWindow, CFuncAreaPortalBase );	
	
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

					CFuncAreaPortalWindow();
					~CFuncAreaPortalWindow();


// Overrides.
public:

	virtual void	Spawn();
	virtual void	Activate();


// CFuncAreaPortalBase stuff.
public:

	virtual bool	UpdateVisibility( const Vector &vOrigin, float fovDistanceAdjustFactor, bool &bIsOpenOnClient );


public:
	// Returns false if the viewer is past the fadeout distance.
	bool IsWindowOpen( const Vector &vOrigin, float fovDistanceAdjustFactor );

public:
	
	CNetworkVar( float, m_flFadeStartDist );	// Distance at which it starts fading (when <= this, alpha=m_flTranslucencyLimit).
	CNetworkVar( float, m_flFadeDist );		// Distance at which it becomes solid.

	// 0-1 value - minimum translucency it's allowed to get to.
	CNetworkVar( float, m_flTranslucencyLimit );

	string_t 		m_iBackgroundBModelName;	// string name of background bmodel
	CNetworkVar( int, m_iBackgroundModelIndex );

	//Input handlers
	void InputSetFadeStartDistance( inputdata_t &inputdata );
	void InputSetFadeEndDistance( inputdata_t &inputdata );
};



#endif // FUNC_AREAPORTALWINDOW_H
