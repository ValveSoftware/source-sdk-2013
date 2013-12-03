//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VGUI_HELPERS_H
#define VGUI_HELPERS_H
#ifdef _WIN32
#pragma once
#endif


#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>


inline int PanelTop(vgui::Panel *pPanel)	{int x,y,w,h; pPanel->GetBounds(x,y,w,h); return y;}
inline int PanelLeft(vgui::Panel *pPanel)	{int x,y,w,h; pPanel->GetBounds(x,y,w,h); return x;}
inline int PanelRight(vgui::Panel *pPanel)	{int x,y,w,h; pPanel->GetBounds(x,y,w,h); return x+w;}
inline int PanelBottom(vgui::Panel *pPanel)	{int x,y,w,h; pPanel->GetBounds(x,y,w,h); return y+h;}
inline int PanelWidth(vgui::Panel *pPanel)	{int x,y,w,h; pPanel->GetBounds(x,y,w,h); return w;}
inline int PanelHeight(vgui::Panel *pPanel)	{int x,y,w,h; pPanel->GetBounds(x,y,w,h); return h;}

// Places child at the requested position inside pParent. iAlignment is from Label::Alignment.
void AlignPanel(vgui::Panel *pChild, vgui::Panel *pParent, int alignment);


#endif // VGUI_HELPERS_H

