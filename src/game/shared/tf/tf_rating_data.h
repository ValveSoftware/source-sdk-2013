//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef TF_RATING_DATA_H
#define TF_RATING_DATA_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "tf_gcmessages.h"
#if defined (CLIENT_DLL) || defined (GAME_DLL)
#include "gc_clientsystem.h"
#endif

#include "tf_matchmaking_shared.h"

//---------------------------------------------------------------------------------
// Purpose: The shared object that contains a specific MM rating
//---------------------------------------------------------------------------------
class CTFRatingData : public GCSDK::CProtoBufSharedObject< CSOTFRatingData, k_EProtoObjectTFRatingData, /* bPublicMutable */ false >
{
public:
	CTFRatingData();
	CTFRatingData( uint32 unAccountID, EMMRating eRatingType, const MMRatingData_t &ratingData );


	// Helpers
	static CTFRatingData *YieldingGetPlayerRatingDataBySteamID( const CSteamID &steamID, EMMRating eRatingType );


	// Private on the GC to encourage callers to go through the proper lookup in CTFSharedObjectCache (GC) or
	// GetLocalPlayerRatingData (client)
	MMRatingData_t GetRatingData() const;
};

#endif // TF_RATING_DATA_H
