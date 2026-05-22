//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "bm_props.h"
#include "props.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_bm_render_props( "tf_bm_render_props", "1", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Bomberman: draw wall/crate models (0=invisible collision only)." );
ConVar tf_bm_deck_visible( "tf_bm_deck_visible", "1", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Bomberman: spawn visible floor deck props (required on itemtest void fallback)." );
ConVar tf_bm_deck_props( "tf_bm_deck_props", "0", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Bomberman: legacy giant deck tiles (0=use normal deck grid)." );

//-----------------------------------------------------------------------------
void BM_PrecacheModelCandidates( const char *const *ppszModels, int nModels )
{
	for ( int i = 0; i < nModels; ++i )
	{
		if ( ppszModels[i] && ppszModels[i][0] )
		{
			CBaseEntity::PrecacheModel( ppszModels[i], false );
		}
	}
}

//-----------------------------------------------------------------------------
const char *BM_SelectModel( const char *const *ppszModels, int nModels )
{
	for ( int i = 0; i < nModels; ++i )
	{
		const char *pszModel = ppszModels[i];
		if ( !pszModel || !pszModel[0] )
		{
			continue;
		}

		const int iIndex = modelinfo->GetModelIndex( pszModel );
		if ( iIndex > 0 )
		{
			return pszModel;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
void BM_ApplyPropModelOrHidden( CBaseAnimating *pEntity, const char *const *ppszModels, int nModels, float flModelScale )
{
	if ( !pEntity )
	{
		return;
	}

	// Server entities have no client class — drawing models here causes "Bind: NULL material" on clients.
	// Arena visuals are drawn client-side (tf_bm_draw_floor / tf_bm_show_grid).
	(void)ppszModels;
	(void)nModels;
	(void)flModelScale;
	pEntity->AddEffects( EF_NODRAW );
}

static CUtlVector<EHANDLE> s_hArenaVisuals;

//-----------------------------------------------------------------------------
void BM_ClearArenaVisuals( void )
{
	for ( int i = 0; i < s_hArenaVisuals.Count(); ++i )
	{
		CBaseEntity *pEnt = s_hArenaVisuals[i].Get();
		if ( pEnt )
		{
			UTIL_Remove( pEnt );
		}
	}
	s_hArenaVisuals.Purge();
}

//-----------------------------------------------------------------------------
// Old builds left huge scaled prop_dynamic decks — remove on fix/rebuild.
//-----------------------------------------------------------------------------
void BM_RemoveStrayArenaProps( void )
{
	BM_ClearArenaVisuals();

	for ( CBaseEntity *pEnt = gEntList.FirstEnt(); pEnt != NULL; pEnt = gEntList.NextEnt( pEnt ) )
	{
		CBaseAnimating *pAnim = dynamic_cast<CBaseAnimating *>( pEnt );
		if ( !pAnim )
		{
			continue;
		}

		const float flScale = pAnim->GetModelScale();
		if ( flScale < 1.25f )
		{
			continue;
		}

		const char *pszClass = pEnt->GetClassname();
		if ( !pszClass || ( Q_stricmp( pszClass, "prop_dynamic" ) != 0 && Q_stricmp( pszClass, "prop_dynamic_override" ) != 0 ) )
		{
			continue;
		}

		UTIL_Remove( pEnt );
	}
}

//-----------------------------------------------------------------------------
static void BM_TrackArenaVisual( CBaseEntity *pEnt )
{
	if ( pEnt )
	{
		s_hArenaVisuals.AddToTail( pEnt );
	}
}

static const char *const g_BMDeckModels[] = {
	"models/props_junk/wood_crate001a.mdl",
	"models/props_gameplay/orange_cone001.mdl",
	"models/error.mdl",
};

//-----------------------------------------------------------------------------
void BM_SpawnArenaVisuals( const Vector &vecArenaCenter, float flArenaW, float flArenaD, float flPlayZ )
{
	if ( !tf_bm_deck_visible.GetBool() )
	{
		return;
	}

	BM_PrecacheModelCandidates( g_BMDeckModels, ARRAYSIZE( g_BMDeckModels ) );
	const char *pszModel = BM_SelectModel( g_BMDeckModels, ARRAYSIZE( g_BMDeckModels ) );
	if ( !pszModel )
	{
		return;
	}

	// One large deck prop (networked, has a client class) — debug overlays are invisible in normal play.
	CDynamicProp *pDeck = dynamic_cast<CDynamicProp *>( CreateEntityByName( "prop_dynamic_override" ) );
	if ( pDeck )
	{
		Vector vecDeckOrigin( vecArenaCenter.x, vecArenaCenter.y, flPlayZ + 4.0f );
		const float flScale = Max( flArenaW, flArenaD ) / 50.0f;

		pDeck->SetModel( pszModel );
		pDeck->SetAbsOrigin( vecDeckOrigin );
		pDeck->SetAbsAngles( vec3_angle );
		pDeck->SetModelScale( flScale );
		pDeck->SetSolid( SOLID_NONE );
		pDeck->Spawn();
		pDeck->Activate();
		BM_TrackArenaVisual( pDeck );
	}

	const float flCorners[4][2] = {
		{ -0.5f, -0.5f },
		{  0.5f, -0.5f },
		{  0.5f,  0.5f },
		{ -0.5f,  0.5f },
	};

	for ( int i = 0; i < 4; ++i )
	{
		CDynamicProp *pProp = dynamic_cast<CDynamicProp *>( CreateEntityByName( "prop_dynamic_override" ) );
		if ( !pProp )
		{
			continue;
		}

		Vector vecOrigin(
			vecArenaCenter.x + flCorners[i][0] * flArenaW,
			vecArenaCenter.y + flCorners[i][1] * flArenaD,
			flPlayZ + 12.0f );

		pProp->SetModel( pszModel );
		pProp->SetAbsOrigin( vecOrigin );
		pProp->SetAbsAngles( vec3_angle );
		pProp->SetSolid( SOLID_NONE );
		pProp->Spawn();
		pProp->Activate();
		BM_TrackArenaVisual( pProp );
	}
}

#endif // SOURCESDK
