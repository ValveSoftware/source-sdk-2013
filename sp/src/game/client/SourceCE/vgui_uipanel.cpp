
#include "cbase.h"

#include "vgui/IVGui.h"
#include "vgui_controls/Frame.h"

#include "vgui_uipanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar cl_showuitest = ConVar("cl_showuitest", "0", FCVAR_CLIENTDLL);

class CUIPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CUIPanel, vgui::Frame);

	CUIPanel(vgui::VPANEL parent);
	~CUIPanel(){};

protected:
	virtual void OnTick();
	virtual void OnCommand(const char* pcCommand);

private:

	
};

CUIPanel::CUIPanel(vgui::VPANEL parent) : BaseClass(NULL, "CUIPanel")
{
	SetParent(parent);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(true);
	SetMoveable(true);
	SetVisible(true);

	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	//LoadControlSettings("resource/commentarydialog.res");

	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);

	DevMsg("CUIPanel has been constructed\n");

}

void CUIPanel::OnCommand(const char* cmd)
{
	BaseClass::OnCommand(cmd);
}


void CUIPanel::OnTick(void)
{
	BaseClass::OnTick();
	SetVisible(cl_showuitest.GetBool());
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