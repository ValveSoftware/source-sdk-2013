#include "cbase.h"
#include "hud_menu_static.h"

class CHudCPMenu : public CHudMenuStatic {

	DECLARE_CLASS_SIMPLE(CHudCPMenu, CHudMenuStatic);

public:
    //This constructor (name) is determined by the DECLARE_HUDELEMENT macro,
    //which means this element will be called CHudCPMenu in engine
    CHudCPMenu::CHudCPMenu(const char *pElementName) : CHudMenuStatic(pElementName) {};

	//Overrides
    virtual void ShowMenu();
	virtual void SelectMenuItem(int menu_item);
};