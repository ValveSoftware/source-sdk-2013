//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef QLIMITS_H
#define QLIMITS_H

#if defined( _WIN32 )
#pragma once
#endif

// DATA STRUCTURE INFO

#define MAX_NUM_ARGVS	50

// SYSTEM INFO
#define	MAX_QPATH		96			// max length of a game pathname
#define	MAX_OSPATH		260			// max length of a filesystem pathname

#define	ON_EPSILON		0.1			// point on plane side epsilon


// Resource counts;
#define MAX_MODEL_INDEX_BITS	12   // sent as a short
#define	MAX_MODELS				(1<<MAX_MODEL_INDEX_BITS)

#define MAX_GENERIC_INDEX_BITS	9
#define MAX_GENERIC				(1<<MAX_GENERIC_INDEX_BITS)
#define MAX_DECAL_INDEX_BITS	9
#define MAX_BASE_DECALS			(1<<MAX_DECAL_INDEX_BITS)

#endif // QLIMITS_H
