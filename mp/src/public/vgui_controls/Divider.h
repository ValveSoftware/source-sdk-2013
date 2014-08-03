//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DIVIDER_H
#define DIVIDER_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Thin line used to divide sections in dialogs
//-----------------------------------------------------------------------------
class Divider : public Panel
{
	DECLARE_CLASS_SIMPLE( Divider, Panel );

public:
	Divider(Panel *parent, const char *name);
	~Divider();

	virtual void ApplySchemeSettings(IScheme *pScheme);
};


} // namespace vgui


#endif // DIVIDER_H
