//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef GAME_ITEM_SCHEMA_H
#define GAME_ITEM_SCHEMA_H
#ifdef _WIN32
#pragma once
#endif

#if defined(TF_CLIENT_DLL) || defined(TF_DLL) || defined(TF_GC_DLL)
// TF
	class CTFItemSchema;
	class CTFItemDefinition;
	class CTFItemSystem;
	
	typedef CTFItemSchema		GameItemSchema_t;
	typedef CTFItemDefinition	GameItemDefinition_t;
	typedef CTFItemSystem		GameItemSystem_t;

	#include "tf_item_schema.h"
#elif defined( DOTA_CLIENT_DLL ) || defined( DOTA_DLL ) || defined ( DOTA_GC_DLL ) 
// DOTA
	class CDOTAItemSchema;
	class CDOTAItemDefinition;
	class CDOTAItemSystem;

	typedef CDOTAItemSchema		GameItemSchema_t;
	typedef CDOTAItemDefinition	GameItemDefinition_t;
	typedef CDOTAItemSystem		GameItemSystem_t;

	#include "econ/dota_item_schema.h"
#else
	// Fallback Case
	class CEconItemSchema;
	class CEconItemDefinition;
	class CEconItemSystem;

	typedef CEconItemSchema		GameItemSchema_t;
	typedef CEconItemDefinition	GameItemDefinition_t;
	typedef CEconItemSystem		GameItemSystem_t;

	#include "econ_item_schema.h"
#endif

extern GameItemSchema_t *GetItemSchema();

#endif // GAME_ITEM_SYSTEM_H
