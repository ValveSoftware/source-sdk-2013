
#include "cbase.h"

#include "vgui/IVGui.h"
#include "vgui_controls/Frame.h"

#include "vgui_uipanel.h"

#include "vgui/ISurface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar cl_drawuilayer = ConVar("cl_drawuilayer", "1", FCVAR_CLIENTDLL);

class CUIPanel : public Panel
{
	DECLARE_CLASS_SIMPLE(CUIPanel, Panel);

	CUIPanel(VPANEL parent);
	~CUIPanel(){};

public:
	
	//virtual void Init(int x, int y, int wide, int tall);

	virtual void OnTick();
	virtual void OnCommand(const char* pcCommand);

	virtual void Paint();

	

private:

	
};

CUIPanel::CUIPanel(const VPANEL parent) : BaseClass(NULL, "UILayer")
{
	SetParent(parent);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetPos(0, 0);
	SetSize(ScreenWidth(), ScreenHeight());

	SetProportional(false);
	SetVisible(true);
	

	//SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	//LoadControlSettings("resource/commentarydialog.res");

	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);

	DevMsg("UI layer has been constructed\n");

}

void CUIPanel::OnCommand(const char* cmd)
{
	BaseClass::OnCommand(cmd);
}


void CUIPanel::OnTick(void)
{
	BaseClass::OnTick();
	SetVisible(cl_drawuilayer.GetBool());
}

void CUIPanel::Paint(void)
{
	BaseClass::Paint();
	//surface()->DrawSetColor(255, 0, 0, 255);
	//surface()->DrawFilledRect(0, 0, ScreenWidth(), ScreenHeight());
}


class CUIPanelInterface : public IUIPanelInterface
{
private:
	CUIPanel* panel;
public:
	CUIPanelInterface()
	{
		panel = NULL;
	}

	virtual void Create(vgui::VPANEL parent)
	{
		panel = new CUIPanel(parent);
	}

	virtual void Destroy()
	{
		if (panel) {
			delete panel;
		}
	}
};

static CUIPanelInterface g_UIPanel;
IUIPanelInterface* uipanel = &g_UIPanel;