#ifndef MAPCONTEXTMENU_H
#define MAPCONTEXTMENU_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Basic right-click context menu for servers
//-----------------------------------------------------------------------------
class CMapContextMenu : public vgui::Menu
{
public:
    CMapContextMenu(vgui::Panel *parent);
    ~CMapContextMenu();

    // call this to Activate the menu
    void ShowMenu(
        vgui::Panel *target,
        bool showMapStart,
        bool showViewGameInfo);
};


#endif // MAPCONTEXTMENU_H