#include "cbase.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include "menu.h"

using namespace vgui;

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "vgui_helpers.h"

#include "tier0/memdbgon.h"

using namespace vgui;


class CHudVersionWarn : public CHudElement, public Panel
{
    DECLARE_CLASS_SIMPLE(CHudVersionWarn, Panel);

public:
    CHudVersionWarn(const char *pElementName);
    virtual bool ShouldDraw()
    {
        return true;
    }
    virtual void Paint();

protected:
    CPanelAnimationVar(HFont, m_hTextFont, "TextFont", "Default");

};

DECLARE_HUDELEMENT(CHudVersionWarn);


CHudVersionWarn::CHudVersionWarn(const char *pElementName) : CHudElement(pElementName), Panel(g_pClientMode->GetViewport(), "CHudVersionWarn")
{
    SetProportional(true);
    SetKeyBoardInputEnabled(false);
    SetMouseInputEnabled(false);
}

void CHudVersionWarn::Paint()
{
    surface()->DrawSetTextPos(0, 0);
    surface()->DrawSetTextFont(m_hTextFont);
    surface()->DrawSetTextColor(225, 225, 225, 225);
    surface()->DrawPrintText(L"Pre-Alpha Build", wcslen(L"Pre-Alpha Build"));
    SetBgColor(Color::Color(0, 0, 0, 0));
}

