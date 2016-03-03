#include "cbase.h"
#include "hud_menu_static.h"

#include "tier0/memdbgon.h"
/*

MOM_TODO:
make creating a checkpoint stop your timer
make checkpoints available for output to files
*/
using namespace vgui;

static void SelectMenuItem(int menu_item)
{
    engine->ServerCmd(VarArgs("cpmenu %i", menu_item));
}

CON_COMMAND(showCPmenu, "Opens the Checkpoint Menu.\n")
{
    CHudMenuStatic *cpMenu = GET_HUDELEMENT(CHudMenuStatic);
    if (!cpMenu) 
        return;
    else if (cpMenu->IsMenuDisplayed())
        cpMenu->HideMenu();//MOM_TODO: if another menu is open this will close it!
    else
    {
        engine->ServerCmd("cpmenu");
        KeyValues* pKv = new KeyValues("CP Menu");
        pKv->AddSubKey(new KeyValues("#MOM_Menu_CreateCP"));
        pKv->AddSubKey(new KeyValues("#MOM_Menu_ToPreviousCP"));
        pKv->AddSubKey(new KeyValues("#MOM_Menu_ToNextCP"));
        pKv->AddSubKey(new KeyValues("#MOM_Menu_ToLastCP"));
        pKv->AddSubKey(new KeyValues("#MOM_Menu_RemoveCurrentCP"));
        pKv->AddSubKey(new KeyValues("#MOM_Menu_RemoveEveryCP"));
        cpMenu->ShowMenu(pKv, SelectMenuItem);
        pKv->deleteThis();
    }
}