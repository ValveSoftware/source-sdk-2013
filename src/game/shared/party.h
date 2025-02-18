//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  Parties are a specific type of CPlayerGroup with a leader that can invite and kick members.
//			
//=============================================================================

#ifndef PARTY_H
#define PARTY_H
#ifdef _WIN32
#pragma once
#endif

#include "playergroup.h"

namespace GCSDK
{
class CSharedObject;

class IParty : public IPlayerGroup
{
public:
	virtual ~IParty() { }
};

}

#endif
