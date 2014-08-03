//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PANELANIMATIONVAR_H
#define PANELANIMATIONVAR_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlvector.h"
#include <vgui_controls/Panel.h>

#define DECLARE_PANELANIMATION( className )												\
	static void AddToAnimationMap( char const *scriptname, char const *type, char const *var,	\
		char const *defaultvalue, bool array, PANELLOOKUPFUNC func )					\
	{																					\
		PanelAnimationMap *map = FindOrAddPanelAnimationMap( GetPanelClassName() );		\
																						\
		PanelAnimationMapEntry entry;													\
		entry.m_pszScriptName = scriptname;												\
		entry.m_pszVariable = var;														\
		entry.m_pszType = type;															\
		entry.m_pszDefaultValue = defaultvalue;											\
		entry.m_pfnLookup = func;														\
		entry.m_bArray = array;															\
																						\
		map->entries.AddToTail( entry );												\
	}																					\
																						\
	static void ChainToAnimationMap( void )														\
	{																					\
		static bool chained = false;													\
		if ( chained )																	\
			return;																		\
		chained = true;																	\
		PanelAnimationMap *map = FindOrAddPanelAnimationMap( GetPanelClassName() );		\
		map->pfnClassName = GetPanelClassName;											\
		if ( map && GetPanelBaseClassName() && GetPanelBaseClassName()[0] )				\
		{																				\
			map->baseMap = FindOrAddPanelAnimationMap( GetPanelBaseClassName() );		\
		}																				\
	}																					\
																						\
	class className##_Register;															\
	friend class className##_Register;													\
	class className##_Register															\
	{																					\
	public:																				\
		className##_Register()															\
		{																				\
			className::ChainToAnimationMap();													\
		}																				\
	};																					\
	className##_Register m_RegisterAnimationClass;												\
																						\
	virtual PanelAnimationMap *GetAnimMap()												\
	{																					\
		return FindOrAddPanelAnimationMap( GetPanelClassName() );						\
	}

typedef void *( *PANELLOOKUPFUNC )( vgui::Panel *panel );

// Use this macro to define a variable which hudanimations.txt and hudlayout.res scripts can access
#define CPanelAnimationVarAliasType( type, name, scriptname, defaultvalue, typealias ) \
	class PanelAnimationVar_##name; \
	friend class PanelAnimationVar_##name; \
	static void *GetVar_##name( vgui::Panel *panel ) \
	{								\
		return &(( ThisClass *)panel)->name;	\
	}								\
	class PanelAnimationVar_##name \
	{ \
	public: \
		static void InitVar() \
		{ \
			static bool bAdded = false; \
			if ( !bAdded ) \
			{ \
				bAdded = true; \
				AddToAnimationMap( scriptname, typealias, #name, defaultvalue, false, ThisClass::GetVar_##name ); \
			} \
		}												\
		PanelAnimationVar_##name()						\
		{												\
			PanelAnimationVar_##name::InitVar();		\
		}												\
	};													\
	PanelAnimationVar_##name m_##name##_register;		\
	type name;

#define CPanelAnimationVar( type, name, scriptname, defaultvalue )	\
	CPanelAnimationVarAliasType( type, name, scriptname, defaultvalue, #type )

// Use this macro to define a variable which hudanimations.txt and hudlayout.res scripts can access
#define CPanelAnimationStringVarAliasType( count, name, scriptname, defaultvalue, typealias ) \
	class PanelAnimationVar_##name; \
	friend class PanelAnimationVar_##name; \
	static void *GetVar_##name( vgui::Panel *panel ) \
	{								\
		return &(( ThisClass *)panel)->name;	\
	}								\
	class PanelAnimationVar_##name \
	{ \
	public: \
		static void InitVar() \
		{ \
			static bool bAdded = false; \
			if ( !bAdded ) \
			{ \
				bAdded = true; \
				AddToAnimationMap( scriptname, typealias, #name, defaultvalue, true, ThisClass::GetVar_##name ); \
			} \
		}												\
		PanelAnimationVar_##name()						\
		{												\
			PanelAnimationVar_##name::InitVar();		\
		}												\
	};													\
	PanelAnimationVar_##name m_##name##_register;		\
	char name[ count ];

#define CPanelAnimationStringVar( count, name, scriptname, defaultvalue )	\
	CPanelAnimationStringVarAliasType( count, name, scriptname, defaultvalue, "string" )

struct PanelAnimationMapEntry
{
	char const *name() { return m_pszScriptName; }
	char const *type() { return m_pszType; }
	char const *defaultvalue() { return m_pszDefaultValue; }
	bool		isarray() { return m_bArray; }

	char const *m_pszScriptName;
	char const *m_pszVariable;
	char const *m_pszType;
	char const *m_pszDefaultValue;
	bool		m_bArray;

	PANELLOOKUPFUNC	m_pfnLookup;
};

struct PanelAnimationMap
{
	PanelAnimationMap()
	{
		baseMap = NULL;
		pfnClassName = NULL;
	}

	CUtlVector< PanelAnimationMapEntry > entries;
	PanelAnimationMap *baseMap;
	char const *(*pfnClassName)( void );
};

PanelAnimationMap *FindPanelAnimationMap( char const *className );
PanelAnimationMap *FindOrAddPanelAnimationMap( char const *className );
void PanelAnimationDumpVars( char const *className );

#endif // PANELANIMATIONVAR_H
