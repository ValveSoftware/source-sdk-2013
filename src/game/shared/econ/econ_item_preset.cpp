//========= Copyright Valve Corporation, All rights reserved. ============//
//
//===================================================================

#include "cbase.h"
#include "econ_item_preset.h"
#include "tier1/generichash.h"


using namespace GCSDK;


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// --------------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------------
CEconItemPerClassPresetData::CEconItemPerClassPresetData()
	: m_unAccountID( 0 )
	, m_unClassID( (equipped_class_t)-1 )
	, m_unActivePreset( INVALID_PRESET_INDEX )
{
}

CEconItemPerClassPresetData::CEconItemPerClassPresetData( uint32 unAccountID, equipped_class_t unClassID )
	: m_unAccountID( unAccountID )
	, m_unClassID( unClassID )
	, m_unActivePreset( 0 )
{
}

void CEconItemPerClassPresetData::SerializeToProtoBufItem( CSOClassPresetClientData& msgPresetData ) const
{
	msgPresetData.set_account_id( m_unAccountID );
	msgPresetData.set_class_id( m_unClassID );
	msgPresetData.set_active_preset_id( m_unActivePreset );
}

void CEconItemPerClassPresetData::DeserializeFromProtoBufItem( const CSOClassPresetClientData &msgPresetData )
{
	m_unAccountID	 = msgPresetData.account_id();
	m_unClassID		 = msgPresetData.class_id();
	m_unActivePreset = msgPresetData.active_preset_id();
}

bool CEconItemPerClassPresetData::BIsKeyLess( const CSharedObject& soRHS ) const
{
	const CEconItemPerClassPresetData *soPresetData = assert_cast< const CEconItemPerClassPresetData * >( &soRHS );

	Assert( m_unAccountID == soPresetData->m_unAccountID );

	return m_unClassID < soPresetData->m_unClassID;
}


bool CEconItemPerClassPresetData::BParseFromMessage( const CUtlBuffer & buffer )
{
	CSOClassPresetClientData msgClientPresetData;
	if( !msgClientPresetData.ParseFromArray( buffer.Base(), buffer.TellMaxPut() ) )
		return false;

	DeserializeFromProtoBufItem( msgClientPresetData );

	return true;
}

bool CEconItemPerClassPresetData::BParseFromMessage( const std::string &buffer )
{
	CSOClassPresetClientData msgClientPresetData;
	if( !msgClientPresetData.ParseFromString( buffer ) )
		return false;

	DeserializeFromProtoBufItem( msgClientPresetData );

	return true;
}

//----------------------------------------------------------------------------
// Purpose:
//----------------------------------------------------------------------------
bool CEconItemPerClassPresetData::BUpdateFromNetwork( const CSharedObject & objUpdate )
{
	Copy( objUpdate );
	return true;
}

void CEconItemPerClassPresetData::Copy( const CSharedObject & soRHS )
{
	const CEconItemPerClassPresetData& rhs = static_cast<const CEconItemPerClassPresetData&>( soRHS );

	m_unAccountID = rhs.m_unAccountID;
	m_unClassID		= rhs.m_unClassID;
	m_unActivePreset = rhs.m_unActivePreset;

	for ( int i = 0; i < ARRAYSIZE( m_PresetData ); i++ )
	{
		m_PresetData[i].CopyArray( rhs.m_PresetData[i].Base(), rhs.m_PresetData[i].Count() );
	}
}

void CEconItemPerClassPresetData::Dump() const
{
#if 0
	EmitInfo( SPEW_GC, SPEW_ALWAYS, LOG_ALWAYS, "preset id=%d  class id=%d   slot id=%d  item id=%llu\n",
		m_unPresetID, m_unClassID, m_unSlotID, m_ulItemID );
#endif
}

