//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Main header file for the serializers DLL
//
//=============================================================================

#ifndef IDMSERIALIZERS_H
#define IDMSERIALIZERS_H

#ifdef _WIN32
#pragma once
#endif

#include "appframework/IAppSystem.h"


//-----------------------------------------------------------------------------
// Interface
//-----------------------------------------------------------------------------
class IDmSerializers : public IAppSystem
{
};


//-----------------------------------------------------------------------------
// Used only by applications to hook in DmSerializers
//-----------------------------------------------------------------------------
#define DMSERIALIZERS_INTERFACE_VERSION		"VDmSerializers001"


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
extern IDmSerializers *g_pDmSerializers;


#endif // DMSERIALIZERS_H


