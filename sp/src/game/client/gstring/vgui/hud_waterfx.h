#ifndef HUD_WATERFX_H
#define HUD_WATERFX_H

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "vgui_controls/panel.h"
#include "Gstring/vgui/vParticleContainer.h"

class CHudWaterEffects : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudWaterEffects, vgui::Panel );

public:

	CHudWaterEffects( const char *pElementName );

	void Init( void );
	void PostDLLInit();
	void Reset( void );

	virtual bool ShouldDraw( void );

	virtual CHud::HUDRENDERSTAGE_t	GetRenderStage()
	{
		return CHud::HUDRENDERSTAGE_PRE_HDR; //CHud::HUDRENDERSTAGE_PRE_BARS;
	};

	void FlushAllParticles();

	void SetDropMultiplier( float m );
	void OnRainHit();

protected:
	virtual void OnThink();

	virtual void Paint();

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:

	int m_iTexture_WaterOverlay;
	int m_iTexture_RainOverlay;

	bool m_bSubmerged_Last;
	float m_flRainRegisterTimer;
	float m_flDropSpawnTimer;
	float m_flDropAmountMultiplier;

	float m_flRainOverlay_Alpha;

	void PlayerEffect_Emerge();
	void PlayerEffect_WaterDrop();

	CVGUIParticleContainer *m_pParticleParent_Emerge;
	CVGUIParticleContainer *m_pParticleParent_Drops;
};

#endif