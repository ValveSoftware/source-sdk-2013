//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "tf_rating_data.h"
#include "gcsdk/enumutils.h"

#ifdef CLIENT_DLL
#include "tf_matchmaking_shared.h"
#endif

using namespace GCSDK;

#define ASSERT_LAST_FIELD( foo ) // No Sch on client. As long as it fails somewhere.

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Get player's rating data by steamID.  Returns NULL if it doesn't exist
//-----------------------------------------------------------------------------
CTFRatingData *CTFRatingData::YieldingGetPlayerRatingDataBySteamID( const CSteamID &steamID, EMMRating eRatingType )
{
	GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamID );
	if ( pSOCache )
	{
		auto *pTypeCache = pSOCache->FindTypeCache( CTFRatingData::k_nTypeID );
		if ( pTypeCache )
		{
			for ( uint32 i = 0; i < pTypeCache->GetCount(); ++i )
			{
				CTFRatingData *pRatingData = (CTFRatingData*)pTypeCache->GetObject( i );
				if ( eRatingType == (EMMRating)pRatingData->Obj().rating_type() )
				{
					return pRatingData;
				}
			}
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CTFRatingData::CTFRatingData()
{}

//-----------------------------------------------------------------------------
CTFRatingData::CTFRatingData( uint32 unAccountID, EMMRating eRatingType, const MMRatingData_t &ratingData )
{
	MutObj().set_account_id( unAccountID );
	MutObj().set_rating_type( eRatingType );

	MutObj().set_rating_primary( ratingData.unRatingPrimary );
	MutObj().set_rating_secondary( ratingData.unRatingSecondary );
	MutObj().set_rating_tertiary( ratingData.unRatingTertiary );
	// MMRatingData_t fields
	ASSERT_LAST_FIELD( unRatingTertiary );
}



//-----------------------------------------------------------------------------
// Purpose: Reads out the rating fields for rating systems to use
//-----------------------------------------------------------------------------
MMRatingData_t CTFRatingData::GetRatingData() const
{
	MMRatingData_t rvoData;
	rvoData.unRatingPrimary = Obj().rating_primary();
	rvoData.unRatingSecondary = Obj().rating_secondary();
	rvoData.unRatingTertiary = Obj().rating_tertiary();

	// If you expanded MMRatingData_t
	ASSERT_LAST_FIELD( unRatingTertiary );

	return rvoData;
}
