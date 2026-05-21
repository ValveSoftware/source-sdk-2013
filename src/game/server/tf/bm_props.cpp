//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "bm_props.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_bm_render_props( "tf_bm_render_props", "1", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Bomberman: draw wall/crate/bomb models. Set 0 for invisible props (grid overlay only)." );

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
		pEntity->AddEffects( EF_NODRAW );
	}
}

#endif // SOURCESDK
