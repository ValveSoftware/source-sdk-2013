#include "cbase.h"
#include "hud_menu_static.h"

class C_CP_Menu : public CHudMenuStatic {

	DECLARE_CLASS_SIMPLE(C_CP_Menu, CHudMenuStatic);

public:
	C_CP_Menu(const char*);

	//Overrides
    virtual void ShowMenu();
	virtual void SelectMenuItem(int menu_item);
};