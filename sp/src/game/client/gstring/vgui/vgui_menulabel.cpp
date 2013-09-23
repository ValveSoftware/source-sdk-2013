
#include "cbase.h"
#include "Gstring/vgui/vgui_gstringMain.h"
#include "vgui_controls/Label.h"


CVGUIMenuLabel::CVGUIMenuLabel( vgui::Panel *parent, const char *pszText ) : BaseClass( parent, "", pszText )
{
}


void CVGUIMenuLabel::Paint()
{
	Paint3DAdvanceDepth();

	BaseClass::Paint();
}
