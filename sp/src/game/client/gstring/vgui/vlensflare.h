#ifndef V_LENSFLARE_H
#define V_LENSFLARE_H

#include "cbase.h"
#include "glow_overlay.h"
#include "Gstring/vgui/vParticleContainer.h"

class CGlowOverlay;
class KeyValues;

struct vLensflare_Collection
{
	static vLensflare_Collection* InitFromScript( vgui::Panel *parent,
		CGlowOverlay *pSource,
		KeyValues *pData );

	void Destroy();
	void ResizePanel();

	CGlowOverlay *GetSource();

private:
	vLensflare_Collection(){};
	~vLensflare_Collection(){};

	vParticleOperatorBase *ParseOperator_Size( KeyValues *pData );
	vParticleOperatorBase *ParseOperator_Alpha( KeyValues *pData );
	vParticleOperatorBase *ParseOperator_Transforms( KeyValues *pData );

	CVGUIParticleContainer *m_pContainer;
	CGlowOverlay *m_pSource;
};


#endif