//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#if !defined( BEAM_FLAGS_H )
#define BEAM_FLAGS_H
#ifdef _WIN32
#pragma once
#endif

enum
{
	FBEAM_STARTENTITY		= 0x00000001,
	FBEAM_ENDENTITY			= 0x00000002,
	FBEAM_FADEIN			= 0x00000004,
	FBEAM_FADEOUT			= 0x00000008,
	FBEAM_SINENOISE			= 0x00000010,
	FBEAM_SOLID				= 0x00000020,
	FBEAM_SHADEIN			= 0x00000040,
	FBEAM_SHADEOUT			= 0x00000080,
	FBEAM_ONLYNOISEONCE		= 0x00000100,		// Only calculate our noise once
	FBEAM_NOTILE			= 0x00000200,
	FBEAM_USE_HITBOXES		= 0x00000400,		// Attachment indices represent hitbox indices instead when this is set.
	FBEAM_STARTVISIBLE		= 0x00000800,		// Has this client actually seen this beam's start entity yet?
	FBEAM_ENDVISIBLE		= 0x00001000,		// Has this client actually seen this beam's end entity yet?
	FBEAM_ISACTIVE			= 0x00002000,
	FBEAM_FOREVER			= 0x00004000,
	FBEAM_HALOBEAM			= 0x00008000,		// When drawing a beam with a halo, don't ignore the segments and endwidth
	FBEAM_REVERSED			= 0x00010000,
	NUM_BEAM_FLAGS = 17	// KEEP THIS UPDATED!
};

#endif // BEAM_FLAGS_H
