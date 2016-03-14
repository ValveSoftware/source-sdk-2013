//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This class was needed because the base CHudMenu has
// a timer on it for when no selection is made in 5 seconds, as well as
// closing upon a selection.
//
//=============================================================================//
#include "cbase.h"
#include "hud_menu_static.h"

using namespace vgui;

CHudMenuStatic::CHudMenuStatic(const char *pElementName) : CHudElement(pElementName), Panel(g_pClientMode->GetViewport(), "CHudMenuStatic")
{
    SetHiddenBits(HIDEHUD_WEAPONSELECTION);
};

DECLARE_HUDELEMENT(CHudMenuStatic);

bool CHudMenuStatic::ShouldDraw()
{
    return CHudElement::ShouldDraw() && m_bMenuDisplayed;
}

bool CHudMenuStatic::IsMenuDisplayed()
{
    return m_bMenuDisplayed && m_bMenuTakesInput;
}

void CHudMenuStatic::Init(void)
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
void CHudMenuStatic::Reset(void)
{
    g_szPrelocalisedMenuString[0] = 0;
    m_fWaitingForMore = false;
}

void CHudMenuStatic::VidInit(void)
{
}

void CHudMenuStatic::OnThink()
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

void CHudMenuStatic::HideMenu(void)
{
    m_bMenuTakesInput = false;
    m_flShutoffTime = gpGlobals->realtime + m_flOpenCloseTime;
    g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuClose");
}

//Overridden because we want the menu to stay up after selection
void CHudMenuStatic::SelectMenuItem(int menu_item)
{
    if (SelectFunc)
        SelectFunc(menu_item);

    m_flSelectionTime = gpGlobals->realtime;
    m_nSelectedItem = menu_item;
    g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuPulse");
    C_BasePlayer *cPlayer = C_BasePlayer::GetLocalPlayer();
    if (cPlayer != NULL)
    {
        cPlayer->EmitSound("Momentum.UIMenuSelection");
    }
    if (menu_item == 0)
        HideMenu();
}

void CHudMenuStatic::Paint()
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

void CHudMenuStatic::PaintString(const wchar_t *text, int textlen, vgui::HFont& font, int x, int y)
{
    vgui::surface()->DrawSetTextFont(font);
    vgui::surface()->DrawSetTextPos(x, y);
    for (int ch = 0; ch < textlen; ch++)
    {
        vgui::surface()->DrawUnicodeChar(text[ch]);
    }
}

void CHudMenuStatic::ProcessText(void)
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

void CHudMenuStatic::ApplySchemeSettings(vgui::IScheme *pScheme)
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

void CHudMenuStatic::ShowMenu_KeyValueItems(KeyValues *pKV)
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