#include "cbase.h"
#include "view.h"
#include "menu.h"
#include "iclientmode.h"
#include "utlvector.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include "text_message.h"
#include "hud_macros.h"
#include "weapon_selection.h"

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/VGUI.h>
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>

class CHudMenuStatic : public CHudElement, public vgui::Panel
{

    DECLARE_CLASS_SIMPLE(CHudMenuStatic, vgui::Panel);

public:
    CHudMenuStatic(const char*);

    wchar_t g_szMenuString[512];
    char g_szPrelocalisedMenuString[512];

    void Init(void);
    void VidInit(void);
    void Reset(void);
    virtual bool ShouldDraw(void);
    virtual bool IsMenuDisplayed();
    void HideMenu(void);
    virtual void Paint();
    void OnThink();

    //Overrides
    //Called from a CON_COMMAND most likely.
    //kv is the menu items, SelecFunc is the override/custom code for SelectMenuItem
    virtual void ShowMenu(KeyValues* kv, void(*SelecFunc)(int))
    {
        SelectFunc = SelecFunc;
        ShowMenu_KeyValueItems(kv);
    }
    virtual void SelectMenuItem(int menu_item);
    
    void		ProcessText(void);
    void ShowMenu_KeyValueItems(KeyValues *pKV);
    virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
    void PaintString(const wchar_t *text, int textlen, vgui::HFont& font, int x, int y);

private:
    void(*SelectFunc)(int);

    struct ProcessedLine
    {
        int	menuitem; // -1 for just text
        int startchar;
        int length;
        int pixels;
        int height;
    };

    CUtlVector< ProcessedLine >	m_Processed;

    int				m_nMaxPixels;
    int				m_nHeight;

    bool			m_bMenuDisplayed;
    int				m_bitsValidSlots;
    float			m_flShutoffTime;
    int				m_fWaitingForMore;
    int				m_nSelectedItem;
    bool			m_bMenuTakesInput;
    float           m_flSelectionTime;

private:
    CPanelAnimationVar(Color, m_TextColor, "TextColor", "FgColor");
    CPanelAnimationVar(vgui::HFont, textFont, "TextFont", "Default");
    CPanelAnimationVar(float, m_flOpenCloseTime, "OpenCloseTime", "1");

    CPanelAnimationVar(float, m_flBlur, "Blur", "0");
    CPanelAnimationVar(float, m_flTextScan, "TextScan", "1.0");

    CPanelAnimationVar(float, m_flAlphaOverride, "Alpha", "255.0");
    CPanelAnimationVar(float, m_flSelectionAlphaOverride, "SelectionAlpha", "255.0");

    CPanelAnimationVar(vgui::HFont, m_hItemFont, "ItemFont", "Default");
    CPanelAnimationVar(vgui::HFont, m_hItemFontPulsing, "ItemFontPulsing", "Default");//"MenuItemFontPulsing");

    CPanelAnimationVar(Color, m_MenuColor, "MenuColor", "BgColor");
    CPanelAnimationVar(Color, m_ItemColor, "MenuItemColor", "FgColor");
    CPanelAnimationVar(Color, m_BoxColor, "MenuBoxColor", "BgColor");

};