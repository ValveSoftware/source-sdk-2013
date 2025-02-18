//====== Copyright (c), Valve Corporation, All rights reserved. =======
//
// Purpose: Minimal types and forward declarations useful to put 
//          in the precompiled header
//
//=============================================================================

#pragma once

typedef uint64 GID_t;
constexpr GID_t k_GIDNil = 0xffffffffffffffffull;

typedef uint64 JobID_t;			// Each Job has a unique ID
constexpr JobID_t k_JobIDNil = 0xffffffffffffffffull;

typedef uint32 PartnerId_t;
constexpr PartnerId_t k_uPartnerIdInvalid = 0;

namespace GCSDK
{
} // namespace GCSDK
