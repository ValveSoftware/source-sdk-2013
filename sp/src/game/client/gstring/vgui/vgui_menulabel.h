#ifndef VGUI_MENU_LABEL_H
#define VGUI_MENU_LABEL_H

#include "cbase.h"
#include <vgui_controls/controls.h>
#include <vgui_controls/Label.h>


class CVGUIMenuLabel : public vgui::Label
{
	DECLARE_CLASS_SIMPLE( CVGUIMenuLabel, vgui::Label );
public:

	CVGUIMenuLabel( vgui::Panel *parent, const char *pszText );

protected:

	virtual void Paint();

};

#endif