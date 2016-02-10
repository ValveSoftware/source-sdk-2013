#include "pch_mapselection.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMapContextMenu::CMapContextMenu(Panel *parent) : Menu(parent, "MapContextMenu")
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMapContextMenu::~CMapContextMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: Activates the menu
//-----------------------------------------------------------------------------
void CMapContextMenu::ShowMenu(
    Panel *target,
    bool showStart,
    bool showViewGameInfo)
{
    if (showStart)
    {
        AddMenuItem("StartMap", "#MOM_MapSelector_StartMap", new KeyValues("StartMap"), target);
    }

    if (showViewGameInfo)
    {
        AddMenuItem("ViewMapInfo", "#MOM_MapSelector_ShowMapInfo", new KeyValues("ViewMapInfo"), target);
    }

    //if (showRefresh)
    //{
    //    AddMenuItem("RefreshServer", "#ServerBrowser_RefreshServer", new KeyValues("RefreshServer", "serverID", serverID), target);
    //}

    int x, y, gx, gy;
    input()->GetCursorPos(x, y);
    ipanel()->GetPos(surface()->GetEmbeddedPanel(), gx, gy);
    SetPos(x - gx, y - gy);
    SetVisible(true);
}