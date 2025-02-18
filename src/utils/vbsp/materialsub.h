//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This file loads a KeyValues file containing material name mappings.
//			When the bsp is compiled, all materials listed in the file will
//			be replaced by the second material in the pair.
//
//=============================================================================

#ifndef MATERIALSUB_H
#define MATERIALSUB_H
#ifdef _WIN32
#pragma once
#endif

extern bool g_ReplaceMaterials;

// Setup / Cleanup
void LoadMaterialReplacementKeys( const char *gamedir, const char *mapname );
void DeleteMaterialReplacementKeys( void );

// Takes a material name and returns it's replacement, if there is one.
// If there isn't a replacement, it returns the original.
const char* ReplaceMaterialName( const char *name );

#endif // MATERIALSUB_H
