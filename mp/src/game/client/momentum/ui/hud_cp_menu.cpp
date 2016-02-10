#include "cbase.h"
#include "hud_cp_menu.h"

#include "tier0/memdbgon.h"
/*This class was needed because the base CHudMenu has
a timer on it for when no option is made in 5 seconds.
We override the class here, and to be honest, this can
probably be created into a sub-menu class if we need
other similar menus in the future.

MOM_TODO:
make creating a checkpoint stop your timer
make checkpoints available for output to files
*/
using namespace vgui;

C_CP_Menu::C_CP_Menu(const char *pElementName) : CHudElement(pElementName), Panel(g_pClientMode->GetViewport(), "CPMenu")
{
    SetHiddenBits(HIDEHUD_WEAPONSELECTION);
};

DECLARE_HUDELEMENT(C_CP_Menu);

//Override
bool C_CP_Menu::ShouldDraw()
{
    return CHudElement::ShouldDraw() && m_bMenuDisplayed;
}

//OVERRIDE
void C_CP_Menu::Init(void)
{
    m_nSelectedItem = -1;
    m_bMenuTakesInput = false;
    m_bMenuDisplayed = false;
    m_bitsValidSlots = 0;
    m_Processed.RemoveAll();
    m_nMaxPixels = 0;
    m_nHeight = 0;
    Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_CP_Menu::Reset(void)
{
    g_szPrelocalisedMenuString[0] = 0;
    m_fWaitingForMore = false;
}

void C_CP_Menu::VidInit(void)
{
}


CON_COMMAND(showCPmenu, "Opens the Checkpoint Menu.\n")
{
    C_CP_Menu *cpMenu = (C_CP_Menu *) gHUD.FindElement("C_CP_Menu");
    if (!cpMenu || cpMenu->ShouldDraw()) return;
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
        cpMenu->ShowMenu_KeyValueItems(pKv);
        pKv->deleteThis();
    }
}

void C_CP_Menu::OnThink()
{
    if (m_bMenuDisplayed)
    {
        if (m_nSelectedItem > 0)
        {
            if (gpGlobals->realtime - m_flSelectionTime >= 1.0f)
                m_nSelectedItem = -1;//reset the selection so colors are fine
        }

        if (m_flShutoffTime > 0 && m_flShutoffTime <= gpGlobals->realtime)
        {
            m_bMenuDisplayed = false;
        }
    }
}

void C_CP_Menu::HideMenu(void)
{
    m_bMenuTakesInput = false;
    m_flShutoffTime = gpGlobals->realtime + m_flOpenCloseTime;
    g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuClose");
}

//Overridden because we want the menu to stay up after selection
void C_CP_Menu::SelectMenuItem(int menu_item)
{
    m_nSelectedItem = menu_item;
    g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuPulse");
    C_BasePlayer *cPlayer = C_BasePlayer::GetLocalPlayer();
    if (cPlayer != NULL)
    {
        cPlayer->EmitSound("Momentum.UIMenuSelection");
    }
    if (menu_item == 0)
        HideMenu();

    engine->ServerCmd(VarArgs("cpmenu %i", menu_item));
}

int	C_CP_Menu::KeyInput(int down, ButtonCode_t keynum, const char *pszCurrentBinding)
{
    if (!m_bMenuDisplayed) return 1;
    if (down >= 1 && keynum >= KEY_0 && keynum <= KEY_9)
    {
        SelectMenuItem(keynum - KEY_0);
        m_flSelectionTime = gpGlobals->realtime;
        return 0;
    }
    else
    {
        return 1;
    }
}

void C_CP_Menu::Paint()
{
    if (!m_bMenuDisplayed)
    {
        return;
    }

    // center it
    int x = 20;

    Color	menuColor = m_MenuColor;
    Color itemColor = m_ItemColor;

    int c = m_Processed.Count();

    int border = 20;

    int wide = m_nMaxPixels + border;
    int tall = m_nHeight + border;

    int y = (ScreenHeight() - tall) * 0.5f;

    DrawBox(x - border / 2, y - border / 2, wide, tall, GetBgColor(), 1);

    menuColor[3] = menuColor[3] * (m_flSelectionAlphaOverride / 255.0f);
    itemColor[3] = itemColor[3] * (m_flSelectionAlphaOverride / 255.0f);

    for (int i = 0; i < c; i++)
    {
        ProcessedLine *line = &m_Processed[i];
        if (!line) continue;

        Color clr = line->menuitem != 0 ? itemColor : menuColor;

        bool canblur = false;
        if (line->menuitem != 0 &&
            m_nSelectedItem >= 0 &&
            m_nSelectedItem != m_Processed.Count() &&//Saves the zero from flashing
            (line->menuitem == m_nSelectedItem))
        {
            canblur = true;
        }

        vgui::surface()->DrawSetTextColor(GetFgColor());

        int drawLen = line->length;
        if (line->menuitem != 0)
        {
            drawLen *= m_flTextScan;
        }

        vgui::surface()->DrawSetTextFont(textFont);

        PaintString(&g_szMenuString[line->startchar], drawLen,
            line->menuitem != 0 ? m_hItemFont : textFont, x, y);

        if (canblur)
        {
            // draw the overbright blur
            for (float fl = m_flBlur; fl > 0.0f; fl -= 1.0f)
            {
                if (fl >= 1.0f)
                {
                    PaintString(&g_szMenuString[line->startchar], drawLen, m_hItemFontPulsing, x, y);
                }
                else
                {
                    // draw a percentage of the last one
                    Color col = clr;
                    col[3] *= fl;
                    vgui::surface()->DrawSetTextColor(col);
                    PaintString(&g_szMenuString[line->startchar], drawLen, m_hItemFontPulsing, x, y);
                }
            }
        }

        y += line->height;
    }
}

void C_CP_Menu::PaintString(const wchar_t *text, int textlen, vgui::HFont& font, int x, int y)
{
    vgui::surface()->DrawSetTextFont(font);
    vgui::surface()->DrawSetTextPos(x, y);
    for (int ch = 0; ch < textlen; ch++)
    {
        vgui::surface()->DrawUnicodeChar(text[ch]);
    }
}

void C_CP_Menu::ProcessText(void)
{
    m_Processed.RemoveAll();
    m_nMaxPixels = 0;
    m_nHeight = 0;

    int i = 0;
    int startpos = i;
    //int menuitem = 0;
    int menuitem = 1;
    while (i < 512)
    {
        wchar_t ch = g_szMenuString[i];
        if (ch == 0)
            break;

        // Skip to end of line
        while (i < 512 && g_szMenuString[i] != 0 && g_szMenuString[i] != L'\n')
        {
            i++;
        }

        // Store off line
        if ((i - startpos) >= 1)
        {
            ProcessedLine line;
            line.menuitem = menuitem;
            line.startchar = startpos;
            line.length = i - startpos;
            line.pixels = 0;
            line.height = 0;

            m_Processed.AddToTail(line);
        }

        //menuitem = 0;
        menuitem++;
        // Skip delimiter
        if (g_szMenuString[i] == '\n')
        {
            i++;
        }
        startpos = i;
    }
    menuitem = 0;
    // Add final block
    if (i - startpos >= 1)
    {
        ProcessedLine line;
        line.menuitem = menuitem;
        line.startchar = startpos;
        line.length = i - startpos;
        line.pixels = 0;
        line.height = 0;

        m_Processed.AddToTail(line);
    }

    // Now compute pixels needed
    int c = m_Processed.Count();
    for (i = 0; i < c; i++)
    {
        ProcessedLine *l = &m_Processed[i];
        Assert(l);

        int pixels = 0;
        vgui::HFont font = textFont;//l->menuitem != 0 ? m_hItemFont : m_hTextFont;
        for (int ch = 0; ch < l->length; ch++)
        {
            pixels += vgui::surface()->GetCharacterWidth(font, g_szMenuString[ch + l->startchar]);
        }

        l->pixels = pixels;
        l->height = vgui::surface()->GetFontTall(font);
        if (pixels > m_nMaxPixels)
        {
            m_nMaxPixels = pixels;
        }
        m_nHeight += l->height;
    }
}

void C_CP_Menu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    SetPaintBackgroundEnabled(false);
    textFont = pScheme->GetFont("Default", true);
    // set our size
    int screenWide, screenTall;
    int x, y;
    GetPos(x, y);
    GetHudSize(screenWide, screenTall);
    SetBounds(0, y, screenWide, screenTall - y);

    ProcessText();
}

void C_CP_Menu::ShowMenu_KeyValueItems(KeyValues *pKV)
{
    m_flShutoffTime = -1;
    m_fWaitingForMore = 0;
    m_bitsValidSlots = 0;

    g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuOpen");
    m_nSelectedItem = -1;

    g_szMenuString[0] = '\0';
    wchar_t *pWritePosition = g_szMenuString;
    int		nRemaining = sizeof(g_szMenuString) / sizeof(wchar_t);
    int		nCount;

    int i = 0;
    for (KeyValues *item = pKV->GetFirstSubKey(); item != NULL; item = item->GetNextKey())
    {
        // Set this slot valid
        m_bitsValidSlots |= (1 << i);
        const char *pszItem = item->GetName();

        wchar_t wLocalizedItem[512];
        wchar_t *wLocalizedItemPtr = g_pVGuiLocalize->Find(pszItem);
        if (!wLocalizedItemPtr)
        {
            // Try to find the localized string of the token. If null, we display pszItem instead.
            g_pVGuiLocalize->ConvertANSIToUnicode(pszItem, wLocalizedItem, 512);
            DevWarning("Missing localization for %s\n", pszItem);
        }
        else Q_wcsncpy(wLocalizedItem, wLocalizedItemPtr, 512);

        nCount = _snwprintf(pWritePosition, nRemaining, L"%d. %ls\n", i + 1, wLocalizedItem);
        nRemaining -= nCount;
        pWritePosition += nCount;

        i++;
    }
    // put a cancel on the end
    m_bitsValidSlots |= (1 << 9);

    nCount = _snwprintf(pWritePosition, nRemaining, L"0. %ls\n", g_pVGuiLocalize->Find("#MOM_Menu_Cancel"));
    nRemaining -= nCount;
    pWritePosition += nCount;

    ProcessText();

    m_bMenuDisplayed = true;
    m_bMenuTakesInput = true;
}