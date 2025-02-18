//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds the CEconClaimCode object
//
// $NoKeywords: $
//=============================================================================//

#ifndef ECON_CLAIMCODE_H
#define ECON_CLAIMCODE_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"

//---------------------------------------------------------------------------------
// Purpose: All the account-level information that the GC tracks for TF
//---------------------------------------------------------------------------------
class CEconClaimCode : public GCSDK::CProtoBufSharedObject< CSOEconClaimCode, k_EEconTypeClaimCode >
{

public:

};


#endif // ECON_CLAIMCODE_H

