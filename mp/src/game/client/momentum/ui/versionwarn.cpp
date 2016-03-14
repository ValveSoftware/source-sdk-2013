#include "cbase.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include "menu.h"
#include "time.h"
#define BUFSIZELOCL (73)
using namespace vgui;

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "vgui_helpers.h"

#include "tier0/memdbgon.h"
#include "mom_shareddefs.h"
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

    virtual void Init();

protected:
    CPanelAnimationVar(HFont, m_hTextFont, "TextFont", "Default");

private:
    wchar_t uVersionText[BUFSIZELOCL];
};

DECLARE_HUDELEMENT(CHudVersionWarn);


CHudVersionWarn::CHudVersionWarn(const char *pElementName) : CHudElement(pElementName), Panel(g_pClientMode->GetViewport(), "CHudVersionWarn")
{
    SetProportional(true);
    SetKeyBoardInputEnabled(false);
    SetMouseInputEnabled(false);
}

void CHudVersionWarn::Init()
{
    char m_pszStringVersion[BUFSIZELOCL];
    char strVersion[BUFSIZELOCL];
    wchar_t *uVersionUnicode = g_pVGuiLocalize->Find("#MOM_BuildVersion");
    g_pVGuiLocalize->ConvertUnicodeToANSI(uVersionUnicode ? uVersionUnicode : L"#MOM_BuildVersion", strVersion, BUFSIZELOCL);
   
    Q_snprintf(m_pszStringVersion, sizeof(m_pszStringVersion), "%s %s",
        strVersion, // BuildVerison localization
        MOM_CURRENT_VERSION
        );
    g_pVGuiLocalize->ConvertANSIToUnicode(m_pszStringVersion, uVersionText, sizeof(m_pszStringVersion));
}

void CHudVersionWarn::Paint()
{
    SetBgColor(Color::Color(0, 0, 0, 0));
    surface()->DrawSetTextPos(0, 0);
    surface()->DrawSetTextFont(m_hTextFont);
    surface()->DrawSetTextColor(225, 225, 225, 225);
    surface()->DrawPrintText(uVersionText, wcslen(uVersionText));
}