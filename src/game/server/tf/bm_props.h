//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef BM_PROPS_H
#define BM_PROPS_H

#ifdef SOURCESDK

class CBaseAnimating;

extern ConVar tf_bm_render_props;
extern ConVar tf_bm_deck_visible;

void BM_PrecacheModelCandidates( const char *const *ppszModels, int nModels );
const char *BM_SelectModel( const char *const *ppszModels, int nModels );
void BM_ApplyPropModelOrHidden( CBaseAnimating *pEntity, const char *const *ppszModels, int nModels, float flModelScale = 1.0f );
void BM_ClearArenaVisuals( void );
void BM_RemoveStrayArenaProps( void );
void BM_SpawnArenaVisuals( const Vector &vecArenaCenter, float flArenaW, float flArenaD, float flPlayZ );

#endif // SOURCESDK

#endif // BM_PROPS_H
