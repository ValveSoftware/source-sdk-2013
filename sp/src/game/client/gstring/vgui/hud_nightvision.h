#ifndef HUD_LENSFLAREFX_H
#define HUD_LENSFLAREFX_H

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "vgui_controls/panel.h"

class CHudNightvision : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudNightvision, vgui::Panel );

public:
	CHudNightvision( const char *pElementName );
	~CHudNightvision();

	void Init();
	void PostDLLInit();
	void LevelInit();
	void Reset();

	virtual bool ShouldDraw();

	virtual CHud::HUDRENDERSTAGE_t	GetRenderStage()
	{
		return CHud::HUDRENDERSTAGE_DEFAULT_HUD;
	};

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnThink();
	virtual void Paint(){};
	virtual void PaintBackground(){};

private:
	CPanelAnimationVarAliasType( float, m_flFade, "fade_black", "0", "float" );
	CPanelAnimationVarAliasType( float, m_flNightvisionAmount, "nightvision", "0", "float" );
	CPanelAnimationVarAliasType( float, m_flOverbright, "overbright", "0", "float" );

	bool m_bWasPainting;
};

#endif