//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds the CTFMapContribution object
//
// $NoKeywords: $
//=============================================================================//

#ifndef TFMAPCONTRIBUTION_H
#define TFMAPCONTRIBUTION_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "tf_gcmessages.h"

namespace GCSDK
{
	class CSQLAccess;
};

//---------------------------------------------------------------------------------
// Purpose: All the account-level information that the GC tracks for TF
//---------------------------------------------------------------------------------
class CTFMapContribution : public GCSDK::CProtoBufSharedObject< CSOTFMapContribution, k_EEconTypeMapContribution >
{

public:
	CTFMapContribution() {}

};

#endif // TFMAPCONTRIBUTION_H

