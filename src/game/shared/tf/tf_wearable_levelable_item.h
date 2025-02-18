//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef TF_WEARABLE_LEVELABLE_ITEM_H
#define TF_WEARABLE_LEVELABLE_ITEM_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_item_wearable.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"

#else
#include "tf_player.h"
#endif

#ifdef CLIENT_DLL
#define CTFWearableLevelableItem C_TFWearableLevelableItem
#endif

#define LEVELABLE_ITEM_BODYGROUP_NAME	"level"
							   
//=============================================================================
//
// 
//
class CTFWearableLevelableItem : public CTFWearable
{
	DECLARE_CLASS( CTFWearableLevelableItem, CTFWearable );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFWearableLevelableItem();

	void IncrementLevel();
	void ResetLevel();

	uint GetLevel( void ){ return m_unLevel; }
private:

	CNetworkVar( uint, m_unLevel );
};


#endif // TF_WEARABLE_LEVELABLE_ITEM_H
