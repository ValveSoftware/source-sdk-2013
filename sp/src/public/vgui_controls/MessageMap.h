//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MESSAGEMAP_H
#define MESSAGEMAP_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlvector.h"

// more flexible than default pointers to members code required for casting member function pointers
//#pragma pointers_to_members( full_generality, virtual_inheritance )

namespace vgui
{

////////////// MESSAGEMAP DEFINITIONS //////////////


//-----------------------------------------------------------------------------
// Purpose: parameter data type enumeration
//			used internal but the shortcut macros require this to be exposed
//-----------------------------------------------------------------------------
enum DataType_t
{
	DATATYPE_VOID,
	DATATYPE_CONSTCHARPTR,
	DATATYPE_INT,
	DATATYPE_FLOAT,
	DATATYPE_PTR,	
	DATATYPE_BOOL,
	DATATYPE_KEYVALUES,
	DATATYPE_CONSTWCHARPTR,
	DATATYPE_UINT64,
	DATATYPE_HANDLE,  // It's an int, really
};

#ifdef WIN32
class __virtual_inheritance Panel;
#else
class Panel;
#endif
typedef unsigned int VPANEL;

typedef void (Panel::*MessageFunc_t)(void);

//-----------------------------------------------------------------------------
// Purpose: Single item in a message map
//			Contains the information to map a string message name with parameters
//			to a function call
//-----------------------------------------------------------------------------
#pragma warning(disable:4121)
struct MessageMapItem_t
{
	const char *name;
	// VC6 aligns this to 16-bytes.  Since some of the code has been compiled with VC6,
	// we need to enforce the alignment on later compilers to remain compatible.
	ALIGN16 MessageFunc_t func;

	int numParams;

	DataType_t firstParamType;
	const char *firstParamName;

	DataType_t secondParamType;
	const char *secondParamName;

	int nameSymbol;
	int firstParamSymbol;
	int secondParamSymbol;
};

#define DECLARE_PANELMESSAGEMAP( className )												\
	static void AddToMap( char const *scriptname, vgui::MessageFunc_t function, int paramCount, int p1type, const char *p1name, int p2type, const char *p2name ) 	\
	{																					\
		vgui::PanelMessageMap *map = vgui::FindOrAddPanelMessageMap( GetPanelClassName() );			\
																						\
		vgui::MessageMapItem_t entry;															\
		entry.name = scriptname;														\
		entry.func = function;															\
		entry.numParams = paramCount;													\
		entry.firstParamType = (vgui::DataType_t)p1type;								\
		entry.firstParamName = p1name;													\
		entry.secondParamType = (vgui::DataType_t)p2type;								\
		entry.secondParamName = p2name;													\
		entry.nameSymbol = 0;															\
		entry.firstParamSymbol = 0;														\
		entry.secondParamSymbol = 0;													\
																						\
		map->entries.AddToTail( entry );												\
	}																					\
																						\
	static void ChainToMap( void )														\
	{																					\
		static bool chained = false;													\
		if ( chained )																	\
			return;																		\
		chained = true;																	\
		vgui::PanelMessageMap *map = vgui::FindOrAddPanelMessageMap( GetPanelClassName() );			\
		map->pfnClassName = &GetPanelClassName;											\
		if ( map && GetPanelBaseClassName() && GetPanelBaseClassName()[0] )				\
		{																				\
			map->baseMap = vgui::FindOrAddPanelMessageMap( GetPanelBaseClassName() );			\
		}																				\
	}																					\
																						\
	class className##_RegisterMap;															\
	friend class className##_RegisterMap;													\
	class className##_RegisterMap															\
	{																					\
	public:																				\
		className##_RegisterMap()															\
		{																				\
			className::ChainToMap();													\
		}																				\
	};																					\
	className##_RegisterMap m_RegisterClass;												\
																						\
	virtual vgui::PanelMessageMap *GetMessageMap()											\
	{																					\
		static vgui::PanelMessageMap *s_pMap = vgui::FindOrAddPanelMessageMap( GetPanelClassName() );	\
		return s_pMap;																	\
	}

#if !defined( _XBOX )
#define VGUI_USEKEYBINDINGMAPS	1
#endif

#if defined( VGUI_USEKEYBINDINGMAPS )

#define DECLARE_CLASS_SIMPLE( className, baseClassName ) \
	typedef baseClassName BaseClass; \
	typedef className ThisClass;	\
public:								\
	DECLARE_PANELMESSAGEMAP( className ); \
	DECLARE_PANELANIMATION( className ); \
	DECLARE_KEYBINDINGMAP( className ); \
	static char const *GetPanelClassName() { return #className; } \
	static char const *GetPanelBaseClassName() { return #baseClassName; }

#define DECLARE_CLASS_SIMPLE_NOBASE( className ) \
	typedef className ThisClass;	\
public:							\
	DECLARE_PANELMESSAGEMAP( className ); \
	DECLARE_PANELANIMATION( className ); \
	DECLARE_KEYBINDINGMAP( className ); \
	static char const *GetPanelClassName() { return #className; } \
	static char const *GetPanelBaseClassName() { return NULL; }

#else // no keybinding maps

#define DECLARE_CLASS_SIMPLE( className, baseClassName ) \
	typedef baseClassName BaseClass; \
	typedef className ThisClass;	\
public:								\
	DECLARE_PANELMESSAGEMAP( className ); \
	DECLARE_PANELANIMATION( className ); \
	static char const *GetPanelClassName() { return #className; } \
	static char const *GetPanelBaseClassName() { return #baseClassName; }

#define DECLARE_CLASS_SIMPLE_NOBASE( className ) \
	typedef className ThisClass;	\
public:							\
	DECLARE_PANELMESSAGEMAP( className ); \
	DECLARE_PANELANIMATION( className ); \
	static char const *GetPanelClassName() { return #className; } \
	static char const *GetPanelBaseClassName() { return NULL; }

#endif // !VGUI_USEKEYBINDINGMAPS

#define _MessageFuncCommon( name, scriptname, paramCount, p1type, p1name, p2type, p2name )	\
	class PanelMessageFunc_##name; \
	friend class PanelMessageFunc_##name; \
	class PanelMessageFunc_##name \
	{ \
	public: \
		static void InitVar() \
		{ \
			static bool bAdded = false; \
			if ( !bAdded ) \
			{ \
				bAdded = true; \
				AddToMap( scriptname, (vgui::MessageFunc_t)&ThisClass::name, paramCount, p1type, p1name, p2type, p2name ); \
			} \
		}												\
		PanelMessageFunc_##name()						\
		{												\
			PanelMessageFunc_##name::InitVar();			\
		}												\
	};													\
	PanelMessageFunc_##name m_##name##_register;		\

// Use this macro to define a message mapped function
// must end with a semicolon ';', or with a function
// no parameter
#define MESSAGE_FUNC( name, scriptname )			_MessageFuncCommon( name, scriptname, 0, 0, 0, 0, 0 );	virtual void name( void )

// one parameter
#define MESSAGE_FUNC_INT( name, scriptname, p1 )	_MessageFuncCommon( name, scriptname, 1, vgui::DATATYPE_INT, #p1, 0, 0 );	virtual void name( int p1 )
#define MESSAGE_FUNC_UINT64( name, scriptname, p1 )	_MessageFuncCommon( name, scriptname, 1, vgui::DATATYPE_UINT64, #p1, 0, 0 );	virtual void name( uint64 p1 )
#define MESSAGE_FUNC_PTR( name, scriptname, p1 )	_MessageFuncCommon( name, scriptname, 1, vgui::DATATYPE_PTR, #p1, 0, 0 );	virtual void name( vgui::Panel *p1 )
#define MESSAGE_FUNC_HANDLE( name, scriptname, p1 )	_MessageFuncCommon( name, scriptname, 1, vgui::DATATYPE_HANDLE, #p1, 0, 0 );	virtual void name( vgui::VPANEL p1 )
#define MESSAGE_FUNC_ENUM( name, scriptname, t1, p1 )	_MessageFuncCommon( name, scriptname, 1, vgui::DATATYPE_INT, #p1, 0, 0 );	virtual void name( t1 p1 )
#define MESSAGE_FUNC_FLOAT( name, scriptname, p1 )	_MessageFuncCommon( name, scriptname, 1, vgui::DATATYPE_FLOAT, #p1, 0, 0 );	virtual void name( float p1 )
#define MESSAGE_FUNC_CHARPTR( name, scriptname, p1 )	_MessageFuncCommon( name, scriptname, 1, vgui::DATATYPE_CONSTCHARPTR, #p1, 0, 0 );	virtual void name( const char *p1 )
#define MESSAGE_FUNC_WCHARPTR( name, scriptname, p1 )	_MessageFuncCommon( name, scriptname, 1, vgui::DATATYPE_CONSTWCHARPTR, #p1, 0, 0 ); virtual void name( const wchar_t *p1 )

// two parameters
#define MESSAGE_FUNC_INT_INT( name, scriptname, p1, p2 )	_MessageFuncCommon( name, scriptname, 2, vgui::DATATYPE_INT, #p1, vgui::DATATYPE_INT, #p2 );	virtual void name( int p1, int p2 )
#define MESSAGE_FUNC_PTR_INT( name, scriptname, p1, p2 )	_MessageFuncCommon( name, scriptname, 2, vgui::DATATYPE_PTR, #p1, vgui::DATATYPE_INT, #p2 );	virtual void name( vgui::Panel *p1, int p2 )
#define MESSAGE_FUNC_HANDLE_INT( name, scriptname, p1, p2 )	_MessageFuncCommon( name, scriptname, 2, vgui::DATATYPE_HANDLE, #p1, vgui::DATATYPE_INT, #p2 );	virtual void name( vgui::VPANEL p1, int p2 )
#define MESSAGE_FUNC_ENUM_ENUM( name, scriptname, t1, p1, t2, p2 )	_MessageFuncCommon( name, scriptname, 2, vgui::DATATYPE_INT, #p1, vgui::DATATYPE_INT, #p2 );	virtual void name( t1 p1, t2 p2 )
#define MESSAGE_FUNC_INT_CHARPTR( name, scriptname, p1, p2 )	_MessageFuncCommon( name, scriptname, 2, vgui::DATATYPE_INT, #p1, vgui::DATATYPE_CONSTCHARPTR, #p2 );	virtual void name( int p1, const char *p2 )
#define MESSAGE_FUNC_PTR_CHARPTR( name, scriptname, p1, p2 )	_MessageFuncCommon( name, scriptname, 2, vgui::DATATYPE_PTR, #p1, vgui::DATATYPE_CONSTCHARPTR, #p2 );	virtual void name( vgui::Panel *p1, const char *p2 )
#define MESSAGE_FUNC_HANDLE_CHARPTR( name, scriptname, p1, p2 )	_MessageFuncCommon( name, scriptname, 2, vgui::DATATYPE_HANDLE, #p1, vgui::DATATYPE_CONSTCHARPTR, #p2 );	virtual void name( vgui::VPANEL p1, const char *p2 )
#define MESSAGE_FUNC_PTR_WCHARPTR( name, scriptname, p1, p2 )	_MessageFuncCommon( name, scriptname, 2, vgui::DATATYPE_PTR, #p1, vgui::DATATYPE_CONSTWCHARPTR, #p2 );	virtual void name( vgui::Panel *p1, const wchar_t *p2 )
#define MESSAGE_FUNC_HANDLE_WCHARPTR( name, scriptname, p1, p2 )	_MessageFuncCommon( name, scriptname, 2, vgui::DATATYPE_HANDLE, #p1, vgui::DATATYPE_CONSTWCHARPTR, #p2 );	virtual void name( vgui::VPANEL p1, const wchar_t *p2 )
#define MESSAGE_FUNC_CHARPTR_CHARPTR( name, scriptname, p1, p2 )	_MessageFuncCommon( name, scriptname, 2, vgui::DATATYPE_CONSTCHARPTR, #p1, vgui::DATATYPE_CONSTCHARPTR, #p2 );	virtual void name( const char *p1, const char *p2 )

// unlimited parameters (passed in the whole KeyValues)
#define MESSAGE_FUNC_PARAMS( name, scriptname, p1 )	_MessageFuncCommon( name, scriptname, 1, vgui::DATATYPE_KEYVALUES, NULL, 0, 0 );	virtual void name( KeyValues *p1 )

// no-virtual function version
#define MESSAGE_FUNC_NV( name, scriptname )			_MessageFuncCommon( name, scriptname, 0, 0, 0, 0, 0 );	void name( void )
#define MESSAGE_FUNC_NV_INT( name, scriptname, p1 )	_MessageFuncCommon( name, scriptname, 1, vgui::DATATYPE_INT, #p1, 0, 0 );	void name( int p1 )
#define MESSAGE_FUNC_NV_INT_INT( name, scriptname, p1, p2 )	_MessageFuncCommon( name, scriptname, 2, vgui::DATATYPE_INT, #p1, vgui::DATATYPE_INT, #p2 );	void name( int p1, int p2 )


// mapping, one per class
struct PanelMessageMap
{
	PanelMessageMap()
	{
		baseMap = NULL;
		pfnClassName = NULL;
		processed = false;
	}

	CUtlVector< MessageMapItem_t > entries;
	bool processed;
	PanelMessageMap *baseMap;
	char const *(*pfnClassName)( void );
};

PanelMessageMap *FindPanelMessageMap( char const *className );
PanelMessageMap *FindOrAddPanelMessageMap( char const *className );



///////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// OBSELETE MAPPING FUNCTIONS, USE ABOVE
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

// no parameters
#define MAP_MESSAGE( type, name, func )						{ name, (vgui::MessageFunc_t)(&type::func), 0 }

// implicit single parameter (params is the data store)
#define MAP_MESSAGE_PARAMS( type, name, func )				{ name, (vgui::MessageFunc_t)(&type::func), 1, vgui::DATATYPE_KEYVALUES, NULL }

// single parameter
#define MAP_MESSAGE_PTR( type, name, func, param1 )			{ name, (vgui::MessageFunc_t)(&type::func), 1, vgui::DATATYPE_PTR, param1 }
#define MAP_MESSAGE_INT( type, name, func, param1 )			{ name, (vgui::MessageFunc_t)(&type::func), 1, vgui::DATATYPE_INT, param1 }
#define MAP_MESSAGE_BOOL( type, name, func, param1 )		{ name, (vgui::MessageFunc_t)(&type::func), 1, vgui::DATATYPE_BOOL, param1 }
#define MAP_MESSAGE_FLOAT( type, name, func, param1 )		{ name, (vgui::MessageFunc_t)(&type::func), 1, vgui::DATATYPE_FLOAT, param1 }
#define MAP_MESSAGE_PTR( type, name, func, param1 )			{ name, (vgui::MessageFunc_t)(&type::func), 1, vgui::DATATYPE_PTR, param1 }
#define MAP_MESSAGE_CONSTCHARPTR( type, name, func, param1) { name, (vgui::MessageFunc_t)(&type::func), 1, vgui::DATATYPE_CONSTCHARPTR, param1 }
#define MAP_MESSAGE_CONSTWCHARPTR( type, name, func, param1) { name, (vgui::MessageFunc_t)(&type::func), 1, vgui::DATATYPE_CONSTWCHARPTR, param1 }

// two parameters
#define MAP_MESSAGE_INT_INT( type, name, func, param1, param2 ) { name, (vgui::MessageFunc_t)&type::func, 2, vgui::DATATYPE_INT, param1, vgui::DATATYPE_INT, param2 }
#define MAP_MESSAGE_PTR_INT( type, name, func, param1, param2 ) { name, (vgui::MessageFunc_t)&type::func, 2, vgui::DATATYPE_PTR, param1, vgui::DATATYPE_INT, param2 }
#define MAP_MESSAGE_INT_CONSTCHARPTR( type, name, func, param1, param2 ) { name, (vgui::MessageFunc_t)&type::func, 2, vgui::DATATYPE_INT, param1, vgui::DATATYPE_CONSTCHARPTR, param2 }
#define MAP_MESSAGE_PTR_CONSTCHARPTR( type, name, func, param1, param2 ) { name, (vgui::MessageFunc_t)&type::func, 2, vgui::DATATYPE_PTR, param1, vgui::DATATYPE_CONSTCHARPTR, param2 }
#define MAP_MESSAGE_PTR_CONSTWCHARPTR( type, name, func, param1, param2 ) { name, (vgui::MessageFunc_t)&type::func, 2, vgui::DATATYPE_PTR, param1, vgui::DATATYPE_CONSTWCHARPTR, param2 }
#define MAP_MESSAGE_CONSTCHARPTR_CONSTCHARPTR( type, name, func, param1, param2 ) { name, (vgui::MessageFunc_t)&type::func, 2, vgui::DATATYPE_CONSTCHARPTR, param1, vgui::DATATYPE_CONSTCHARPTR, param2 }

// if more parameters are needed, just use MAP_MESSAGE_PARAMS() and pass the keyvalue set into the function

//-----------------------------------------------------------------------------
// Purpose: stores the list of objects in the hierarchy
//			used to iterate through an object's message maps
//-----------------------------------------------------------------------------
struct PanelMap_t
{
	MessageMapItem_t *dataDesc;
	int dataNumFields;
	const char *dataClassName;
	PanelMap_t *baseMap;
	int processed;
};

// for use in class declarations
// declares the static variables and functions needed for the data description iteration
#define DECLARE_PANELMAP() \
	static vgui::PanelMap_t m_PanelMap; \
	static vgui::MessageMapItem_t m_MessageMap[]; \
	virtual vgui::PanelMap_t *GetPanelMap( void );

// could embed typeid() into here as well?
#define IMPLEMENT_PANELMAP( derivedClass, baseClass ) \
	vgui::PanelMap_t derivedClass::m_PanelMap = { derivedClass::m_MessageMap, ARRAYSIZE(derivedClass::m_MessageMap), #derivedClass, &baseClass::m_PanelMap }; \
	vgui::PanelMap_t *derivedClass::GetPanelMap( void ) { return &m_PanelMap; }

typedef vgui::Panel *( *PANELCREATEFUNC )( void );

//-----------------------------------------------------------------------------
// Purpose: Used by DECLARE_BUILD_FACTORY macro to create a linked list of
//  instancing functions
//-----------------------------------------------------------------------------
class CBuildFactoryHelper
{
public:
	// Static list of helpers
	static CBuildFactoryHelper *m_sHelpers;

public:
	// Construction
	CBuildFactoryHelper( char const *className, PANELCREATEFUNC func );

	// Accessors
	CBuildFactoryHelper *GetNext( void );

	char const	*GetClassName() const;

	vgui::Panel *CreatePanel();

	static vgui::Panel *InstancePanel( char const *className );
	static void GetFactoryNames( CUtlVector< char const * >& list );
private:

	static bool HasFactory( char const *className );

	// Next factory in list
	CBuildFactoryHelper	*m_pNext;

	int					m_Type;
	PANELCREATEFUNC		m_CreateFunc;
	char const			*m_pClassName;
};

// This is the macro which implements creation of each type of panel
// It creates a function which instances an object of the specified type
// It them hooks that function up to the helper list so that the CHud objects can create
//  the elements by name, with no header file dependency, etc.
#define DECLARE_BUILD_FACTORY( className )										\
	static vgui::Panel *Create_##className( void )							\
		{																		\
			return new className( NULL, NULL );									\
		};																		\
		static vgui::CBuildFactoryHelper g_##className##_Helper( #className, Create_##className );\
	className *g_##className##LinkerHack = NULL;

#define DECLARE_BUILD_FACTORY_DEFAULT_TEXT( className, defaultText )			\
	static vgui::Panel *Create_##className( void )							\
		{																		\
			return new className( NULL, NULL, #defaultText );					\
		};																		\
	static vgui::CBuildFactoryHelper g_##className##_Helper( #className, Create_##className );\
	className *g_##className##LinkerHack = NULL;

// This one allows passing in a special function with calls new panel( xxx ) with arbitrary default parameters
#define DECLARE_BUILD_FACTORY_CUSTOM( className, createFunc )					\
	static vgui::CBuildFactoryHelper g_##className##_Helper( #className, createFunc );\
	className *g_##className##LinkerHack = NULL;

#define DECLARE_BUILD_FACTORY_CUSTOM_ALIAS( className, factoryName, createFunc )					\
	static vgui::CBuildFactoryHelper g_##factoryName##_Helper( #factoryName, createFunc );\
	className *g_##factoryName##LinkerHack = NULL;

} // namespace vgui


#endif // MESSAGEMAP_H
