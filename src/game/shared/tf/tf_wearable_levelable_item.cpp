//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_wearable_levelable_item.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

LINK_ENTITY_TO_CLASS( tf_wearable_levelable_item, CTFWearableLevelableItem );
IMPLEMENT_NETWORKCLASS_ALIASED( TFWearableLevelableItem, DT_TFWearableLevelableItem )

BEGIN_NETWORK_TABLE( CTFWearableLevelableItem, DT_TFWearableLevelableItem )
#if defined( CLIENT_DLL )
	RecvPropInt( RECVINFO( m_unLevel ) ),
#else
	SendPropInt( SENDINFO( m_unLevel ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFWearableLevelableItem )
END_DATADESC()


CTFWearableLevelableItem::CTFWearableLevelableItem()
{
	m_unLevel = 0;
}


void CTFWearableLevelableItem::IncrementLevel()
{
	int nBodyGroup = FindBodygroupByName( LEVELABLE_ITEM_BODYGROUP_NAME );
	if ( nBodyGroup == -1 )
	{
		AssertMsg( 0, "This model needs a bodygroup called \"level\"\n" );
		return;
	}

	if ( m_unLevel < (uint)GetBodygroupCount(nBodyGroup) - 1 )
	{
		m_unLevel++;
		SetBodygroup( nBodyGroup, m_unLevel );
	}
}


void CTFWearableLevelableItem::ResetLevel()
{
	int nBodyGroup = FindBodygroupByName( LEVELABLE_ITEM_BODYGROUP_NAME );
	if ( nBodyGroup == -1 )
	{
		AssertMsg( 0, "This model needs a bodygroup called \"level\"\n" );
		return;
	}

	m_unLevel = 0;
	SetBodygroup( nBodyGroup, m_unLevel );
}
