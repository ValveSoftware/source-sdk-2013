//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "bm_props.h"

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

	if ( !tf_bm_render_props.GetBool() )
	{
		pEntity->AddEffects( EF_NODRAW );
		return;
	}

	const char *pszModel = BM_SelectModel( ppszModels, nModels );
	if ( pszModel )
	{
		pEntity->SetModel( pszModel );
		if ( flModelScale != 1.0f )
		{
			pEntity->SetModelScale( flModelScale, 0.0f );
		}
	}
	else
	{
		static bool s_bWarnedMissingModels = false;
		if ( !s_bWarnedMissingModels )
		{
			s_bWarnedMissingModels = true;
			Warning( "BM: no prop models found — hiding arena props. Mount TF2 (appid 440) or set tf_bm_render_props 0.\n" );
		}
		// Fallback: still draw a crate so the arena is not invisible wireframe only.
		const char *pszFallback = "models/props_junk/wood_crate001a.mdl";
		if ( modelinfo->GetModelIndex( pszFallback ) > 0 )
		{
			pEntity->SetModel( pszFallback );
			if ( flModelScale != 1.0f )
			{
				pEntity->SetModelScale( flModelScale, 0.0f );
			}
			pEntity->SetRenderColor( 200, 180, 140 );
		}
		else
		{
			pEntity->AddEffects( EF_NODRAW );
		}
	}
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
		static bool s_bWarnedDeck = false;
		if ( !s_bWarnedDeck )
		{
			s_bWarnedDeck = true;
			Warning( "BM: no deck model — black void likely. Mount TF2 (appid 440) or add props to game paths.\n" );
		}
		return;
	}

	const int nTilesX = 3;
	const int nTilesY = 3;
	const float flTileW = flArenaW / nTilesX;
	const float flTileD = flArenaD / nTilesY;
	const float flScale = Max( flTileW, flTileD ) / 36.0f;

	for ( int iTileX = 0; iTileX < nTilesX; ++iTileX )
	{
		for ( int iTileY = 0; iTileY < nTilesY; ++iTileY )
		{
			Vector vecOrigin(
				vecArenaCenter.x - flArenaW * 0.5f + ( iTileX + 0.5f ) * flTileW,
				vecArenaCenter.y - flArenaD * 0.5f + ( iTileY + 0.5f ) * flTileD,
				flPlayZ + 6.0f );

			CBaseAnimating *pProp = dynamic_cast<CBaseAnimating *>( CreateEntityByName( "prop_dynamic" ) );
			if ( !pProp )
			{
				continue;
			}

			pProp->SetModel( pszModel );
			pProp->SetModelScale( flScale, 0.0f );
			pProp->SetAbsOrigin( vecOrigin );
			pProp->SetAbsAngles( vec3_angle );
			pProp->SetRenderColor( 96, 140, 180 );
			pProp->RemoveEffects( EF_NODRAW );
			pProp->SetSolid( SOLID_NONE );
			pProp->SetMoveType( MOVETYPE_NONE );

			DispatchSpawn( pProp );
			pProp->Activate();
			BM_TrackArenaVisual( pProp );
		}
	}
}

#endif // SOURCESDK
