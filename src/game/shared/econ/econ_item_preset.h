//========= Copyright Valve Corporation, All rights reserved. ============//
//
//===================================================================

#ifndef ECONITEMPRESET_H
#define ECONITEMPRESET_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "gcsdk/gcclientsdk.h"
#include "base_gcmessages.pb.h"

#include "econ/econ_item_constants.h"

namespace GCSDK
{
	class CSQLAccess;
};

class CSOClassPresetClientData;

typedef uint8	equipped_preset_t;

struct PresetSlotItem_t
{

	equipped_slot_t		m_unSlotID;
	itemid_t			m_ulItemOriginalID;		// Original ID of the item in this slot. We store this instead of the current ID to avoid breaking presets when items get renamed, etc.
};

// --------------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------------
class CEconItemPerClassPresetData : public GCSDK::CSharedObject
{

public:
	typedef GCSDK::CSharedObject BaseClass;

	const static int k_nTypeID = k_EEconTypeItemPresetInstance;
	virtual int GetTypeID() const OVERRIDE { return k_nTypeID; }

	CEconItemPerClassPresetData();
	CEconItemPerClassPresetData( uint32 unAccountID, equipped_class_t unClassID );

	virtual bool BIsKeyLess( const CSharedObject& soRHS ) const;


	virtual bool BParseFromMessage( const CUtlBuffer & buffer ) OVERRIDE;
	virtual bool BParseFromMessage( const std::string &buffer ) OVERRIDE;
	virtual bool BUpdateFromNetwork( const CSharedObject & objUpdate ) OVERRIDE;
	virtual void Copy( const CSharedObject & soRHS );
	virtual void Dump() const;

	void SerializeToProtoBufItem( CSOClassPresetClientData &msgPresetInstance ) const;
	void DeserializeFromProtoBufItem( const CSOClassPresetClientData &msgPresetIntance );

	enum
	{
		kPerClassPresetDataDirtyField_ActivePreset,
		kPerClassPresetDataDirtyField_PresetData_Base,
	};

	equipped_preset_t GetActivePreset() const { return m_unActivePreset; }

private:
	CEconItemPerClassPresetData( const CEconItemPerClassPresetData& ) = delete;
	void operator=( const CEconItemPerClassPresetData& ) = delete;

private:
	uint32							m_unAccountID;
	equipped_class_t				m_unClassID;
	equipped_preset_t				m_unActivePreset;
	CUtlVector<PresetSlotItem_t>	m_PresetData[ CEconItemSchema::kMaxItemPresetCount ];
};

#endif // ECONITEMPRESET_H
