//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef ILOADINGDISC_H
#define ILOADINGDISC_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

namespace vgui
{
	class Panel;
}

abstract_class ILoadingDisc
{
public:
	virtual void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy( void ) = 0;

	// loading disc
	virtual void		SetLoadingVisible( bool bVisible ) = 0;

	// paused disc
	virtual void		SetPausedVisible( bool bVisible, float flExpireTime = -1.f ) = 0;
};

extern ILoadingDisc *loadingdisc;

#endif // ILOADINGDISC_H
