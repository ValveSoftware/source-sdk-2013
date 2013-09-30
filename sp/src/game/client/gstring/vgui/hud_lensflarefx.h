#ifndef HUD_LENSFLAREFX_H
#define HUD_LENSFLAREFX_H

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "vgui_controls/panel.h"

struct vLensflare_Collection;
class CGlowOverlay;

class CHudLensflareEffects : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudLensflareEffects, vgui::Panel );

public:

	CHudLensflareEffects( const char *pElementName );
	~CHudLensflareEffects();

	void Init( void );
	void PostDLLInit();
	void Reset( void );

	virtual void OnSizeChanged(int newWide, int newTall);

	virtual bool ShouldDraw( void );

	virtual CHud::HUDRENDERSTAGE_t	GetRenderStage()
	{
		return CHud::HUDRENDERSTAGE_PRE_HDR;
	};

	vLensflare_Collection *LoadLensflare( CGlowOverlay *pSrc, KeyValues *pData );

	void FreeLensflare( vLensflare_Collection **flare );

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnThink();

private:

	CUtlVector< vLensflare_Collection* > m_hLensflares;
	vgui::Panel *m_pLensflareRoot;

};

#endif