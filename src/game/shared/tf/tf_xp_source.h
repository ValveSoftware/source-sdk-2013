//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds XP source data
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_XP_SOURCE_H
#define TF_XP_SOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "tf_gcmessages.h"


//---------------------------------------------------------------------------------
// Purpose: Contains a delta for a player's XP
//---------------------------------------------------------------------------------
class CXPSource : public GCSDK::CProtoBufSharedObject< CMsgTFXPSource, k_EEconTypeXPSource >
{
public:
	CXPSource();
};

#endif // TF_XP_SOURCE_H
