//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TOGGLETEXTUREPROXY
#define TOGGLETEXTUREPROXY

#include "materialsystem/imaterialproxy.h"

class IMaterial;
class IMaterialVar;

#pragma warning (disable : 4100)

class CBaseToggleTextureProxy : public IMaterialProxy
{
public:
	CBaseToggleTextureProxy();
	virtual ~CBaseToggleTextureProxy();

	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial();

private:
	void Cleanup();

	C_BaseEntity *BindArgToEntity( void *pArg );

	IMaterialVar *m_TextureVar;
	IMaterialVar *m_TextureFrameNumVar;
	bool m_WrapAnimation;
};

#endif // TOGGLETEXTUREPROXY