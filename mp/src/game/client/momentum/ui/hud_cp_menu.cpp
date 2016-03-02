#include "cbase.h"
#include "hud_cp_menu.h"

#include "tier0/memdbgon.h"
/*

MOM_TODO:
make creating a checkpoint stop your timer
make checkpoints available for output to files
*/
using namespace vgui;

C_CP_Menu::C_CP_Menu(const char *pElementName) : CHudMenuStatic(pElementName)
{
    //SetHiddenBits(HIDEHUD_WEAPONSELECTION);
};

DECLARE_HUDELEMENT(C_CP_Menu);


CON_COMMAND(showCPmenu, "Opens the Checkpoint Menu.\n")
{
    C_CP_Menu *cpMenu = GET_HUDELEMENT(C_CP_Menu);
    if (!cpMenu) 
        return;
    else if (cpMenu->IsMenuDisplayed())
        cpMenu->HideMenu();
    else
        cpMenu->ShowMenu();
}

//Overridden for CP menu functionality
void C_CP_Menu::SelectMenuItem(int menu_item)
{
    engine->ServerCmd(VarArgs("cpmenu %i", menu_item));
    BaseClass::SelectMenuItem(menu_item);
}

void C_CP_Menu::ShowMenu()
{
    engine->ServerCmd("cpmenu");
    KeyValues* pKv = new KeyValues("CP Menu");
    pKv->AddSubKey(new KeyValues("#MOM_Menu_CreateCP"));
    pKv->AddSubKey(new KeyValues("#MOM_Menu_ToPreviousCP"));
    pKv->AddSubKey(new KeyValues("#MOM_Menu_ToNextCP"));
    pKv->AddSubKey(new KeyValues("#MOM_Menu_ToLastCP"));
    pKv->AddSubKey(new KeyValues("#MOM_Menu_RemoveCurrentCP"));
    pKv->AddSubKey(new KeyValues("#MOM_Menu_RemoveEveryCP"));
    BaseClass::ShowMenu_KeyValueItems(pKv);
    pKv->deleteThis();
}