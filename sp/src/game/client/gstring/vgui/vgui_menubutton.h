#ifndef VGUI_MENU_BUTTON_H
#define VGUI_MENU_BUTTON_H

#include "cbase.h"
#include <vgui_controls/controls.h>
#include <vgui_controls/Button.h>


class CVGUIMenuButton : public vgui::Button
{
	DECLARE_CLASS_SIMPLE( CVGUIMenuButton, vgui::Button );
public:

	CVGUIMenuButton( vgui::Panel *parent, const char *pszText, const char *pszCmd, int order );
	~CVGUIMenuButton();

	int GetOrderIndex();

protected:

	virtual void Paint();

	virtual void OnThink();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void PerformLayout();

private:

	int m_iBgTexture;
	int m_iOrderIndex;

	float m_flBlurStrengthAmount;
	float m_flBackgroundAlpha;

	Label *m_pTextBlur;
};

#endif