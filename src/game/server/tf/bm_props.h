//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef BM_PROPS_H
#define BM_PROPS_H

#ifdef SOURCESDK

class CBaseAnimating;

extern ConVar tf_bm_render_props;

void BM_PrecacheModelCandidates( const char *const *ppszModels, int nModels );
const char *BM_SelectModel( const char *const *ppszModels, int nModels );
void BM_ApplyPropModelOrHidden( CBaseAnimating *pEntity, const char *const *ppszModels, int nModels, float flModelScale = 1.0f );

#endif // SOURCESDK

#endif // BM_PROPS_H
