#include "cbase.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include "menu.h"
#include "time.h"
#define BUFSIZELOCL (73)
#define RELEASE "Release"
#define BETA "Beta"
#define ALPHA "Alpha"
#define PREALPHA "Pre-Alpha"
#define CURRENTVERSION PREALPHA
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

    virtual void Init();

protected:
    CPanelAnimationVar(HFont, m_hTextFont, "TextFont", "Default");

private:
    int build_version();
    int days_from_civil(int y, unsigned m, unsigned d);
    int version = build_version();
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
    version = build_version();
    char m_pszStringVersion[BUFSIZELOCL];
    char strVersion[BUFSIZELOCL];
    wchar_t *uVersionUnicode = g_pVGuiLocalize->Find("#MOM_BuildVersion");
    g_pVGuiLocalize->ConvertUnicodeToANSI(uVersionUnicode ? uVersionUnicode : L"#MOM_BuildVersion", strVersion, BUFSIZELOCL);
   
    Q_snprintf(m_pszStringVersion, sizeof(m_pszStringVersion), "%s %s %i",
        CURRENTVERSION,
        strVersion, // BuildVerison localization
        version // Version Number
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

// http://howardhinnant.github.io/date_algorithms.html#days_from_civil
int CHudVersionWarn::days_from_civil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);      // [0, 399]
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;  // [0, 365]
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;         // [0, 146096]
    return era * 146097 + static_cast<int>(doe)-719468;
}

int CHudVersionWarn::build_version()
{
    int y, m, d;
    GetCurrentDate(&d, &m, &y);
    return days_from_civil(y, m, d) - days_from_civil(2015, 01, 01);
}