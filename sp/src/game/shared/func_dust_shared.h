//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FUNC_DUST_SHARED_H
#define FUNC_DUST_SHARED_H
#ifdef _WIN32
#pragma once
#endif


// Flags for m_DustFlags.
#define DUSTFLAGS_ON			(1<<0)	// emit particles..
#define DUSTFLAGS_SCALEMOTES	(1<<1)	// scale to keep the same size on screen
#define DUSTFLAGS_FROZEN		(1<<2)	// just emit m_SpawnRate # of particles and freeze
#define DUST_NUMFLAGS			3


#endif // FUNC_DUST_SHARED_H
