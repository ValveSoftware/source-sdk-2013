//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef KEYBINDINGMAP_H
#define KEYBINDINGMAP_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlvector.h"

// more flexible than default pointers to members code required for casting member function pointers
//#pragma pointers_to_members( full_generality, virtual_inheritance )

namespace vgui
{

class Panel;

enum
{
    MODIFIER_SHIFT		= ( 1 << 0 ),
	MODIFIER_CONTROL	= ( 1 << 1 ),
	MODIFIER_ALT		= ( 1 << 2 ),
};

//-----------------------------------------------------------------------------
// Purpose: An actual keybinding, where bindingname references a bindingmap mentioned below
//-----------------------------------------------------------------------------
struct BoundKey_t
{
	BoundKey_t();
	BoundKey_t( const BoundKey_t& src );
	~BoundKey_t();
	BoundKey_t& operator =( const BoundKey_t& src );

	bool				isbuiltin;		// whether this was by the #DECLARE macros or via code/parsing a config file
	char const			*bindingname; // what it's bound to
	int					keycode;	// vgui keycode
	int					modifiers;  // which modifiers
};

//-----------------------------------------------------------------------------
// Purpose: Single item in a message map
//			Contains the information to map a string message name with parameters
//			to a function call
//-----------------------------------------------------------------------------
struct KeyBindingMap_t
{
	KeyBindingMap_t();
	KeyBindingMap_t( const KeyBindingMap_t& src );
	~KeyBindingMap_t();

	char const				*bindingname; // for the script file
	ALIGN16 MessageFunc_t	func;
	char const 				*helpstring; // e.g., #KeybindingPasteHelp
	char const 				*docstring;  // e.g., #KeybindingPasteHelp
	bool					passive; // dispatch command, but still chain
};

#define DECLARE_KEYBINDINGMAP( className )												\
	static void KB_AddToMap																\
	(																					\
		char const			*bindingname,												\
		vgui::KeyCode		defaultcode,												\
		int					default_modifiers,											\
		vgui::MessageFunc_t function,													\
		char const			*helpstring,												\
		char const			*docstring,													\
		bool				passive														\
	) 																					\
	{																					\
		vgui::PanelKeyBindingMap *map = vgui::FindOrAddPanelKeyBindingMap( GetPanelClassName() ); \
																						\
		vgui::KeyBindingMap_t entry;													\
		entry.bindingname = bindingname;												\
																						\
		entry.func = function;															\
																						\
		entry.helpstring = helpstring;													\
		entry.docstring = docstring;													\
																						\
		entry.passive = passive;														\
																						\
		map->entries.AddToTail( entry );												\
																						\
		vgui::BoundKey_t kb;															\
		kb.isbuiltin = true;															\
		kb.bindingname = bindingname;													\
		kb.keycode = defaultcode;														\
		kb.modifiers = default_modifiers;												\
		map->defaultkeys.AddToTail( kb );													\
		map->boundkeys.AddToTail( kb );													\
	}																					\
																						\
	static void KB_ChainToMap( void )													\
	{																					\
		static bool chained = false;													\
		if ( chained )																	\
			return;																		\
		chained = true;																	\
		vgui::PanelKeyBindingMap *map = vgui::FindOrAddPanelKeyBindingMap( GetPanelClassName() );	\
		map->pfnClassName = &GetPanelClassName;											\
		if ( map && GetPanelBaseClassName() && GetPanelBaseClassName()[0] )				\
		{																				\
			map->baseMap = vgui::FindOrAddPanelKeyBindingMap( GetPanelBaseClassName() );	\
		}																				\
	}																					\
																						\
	static void KB_AddBoundKey															\
	(																					\
		char const			*bindingname,												\
		int					keycode,													\
		int					modifiers													\
	)																					\
	{																					\
		vgui::PanelKeyBindingMap *map = vgui::FindOrAddPanelKeyBindingMap( GetPanelClassName() );	\
		vgui::BoundKey_t kb;															\
		kb.isbuiltin = true;															\
		kb.bindingname = bindingname;													\
		kb.keycode = keycode;															\
		kb.modifiers = modifiers;														\
		map->defaultkeys.AddToTail( kb );													\
		map->boundkeys.AddToTail( kb );													\
	}																					\
																						\
	class className##_RegisterKBMap;													\
	friend class className##_RegisterKBMap;												\
	class className##_RegisterKBMap														\
	{																					\
	public:																				\
		className##_RegisterKBMap()														\
		{																				\
			className::KB_ChainToMap();													\
		}																				\
	};																					\
	className##_RegisterKBMap m_RegisterClassKB;										\
																						\
	virtual vgui::PanelKeyBindingMap *GetKBMap()										\
	{																					\
		static vgui::PanelKeyBindingMap *s_pMap = vgui::FindOrAddPanelKeyBindingMap( GetPanelClassName() );	\
		return s_pMap;																	\
	}

#define _KBMapFuncCommonFunc( name, keycode, modifiers, function, help, doc, passive )	\
	class PanelKBMapFunc_##name; \
	friend class PanelKBMapFunc_##name; \
	class PanelKBMapFunc_##name \
	{ \
	public: \
		static void InitVar() \
		{ \
			static bool bAdded = false; \
			if ( !bAdded ) \
			{ \
				bAdded = true; \
				KB_AddToMap( #name, keycode, modifiers, (vgui::MessageFunc_t)&ThisClass::function, help, doc, passive ); \
			}											\
		}												\
		PanelKBMapFunc_##name()							\
		{												\
			PanelKBMapFunc_##name::InitVar();			\
		}												\
	};													\
	PanelKBMapFunc_##name m_##name##_register;		

#define _KBBindKeyCommon( name, keycode, modifiers, _classname )	\
	class PanelKBBindFunc_##_classname; \
	friend class PanelKBBindFunc_##_classname; \
	class PanelKBBindFunc_##_classname \
	{ \
	public: \
		static void InitVar() \
		{ \
			static bool bAdded = false; \
			if ( !bAdded ) \
			{ \
				bAdded = true; \
				KB_AddBoundKey( #name, keycode, modifiers ); \
			}											\
		}												\
		PanelKBBindFunc_##_classname()					\
		{												\
			PanelKBBindFunc_##_classname::InitVar();	\
		}												\
	};													\
	PanelKBBindFunc_##_classname m_##_classname##_bindkey_register;	

#define KEYBINDING_FUNC( name, keycode, modifiers, function, help, doc )				_KBMapFuncCommonFunc( name, keycode, modifiers, function, help, doc, false ); virtual void function()
#define KEYBINDING_FUNC_NODECLARE( name, keycode, modifiers, function, help, doc )		_KBMapFuncCommonFunc( name, keycode, modifiers, function, help, doc, false );
#define KEYBINDING_FUNC_PASSIVE( name, keycode, modifiers, function, help, doc )		_KBMapFuncCommonFunc( name, keycode, modifiers, function, help, doc, true );	virtual void function()
#define KEYBINDING_FUNC_PASSIVE_NODECLARE( name, keycode, modifiers, function, help, doc )		_KBMapFuncCommonFunc( name, keycode, modifiers, function, help, doc, true );

// For definding additional (non-default) keybindings
#define KEYBINDING_ADDBINDING( name, keycode, modifiers )									_KBBindKeyCommon( name, keycode, modifiers, name );
#define KEYBINDING_ADDBINDING_MULTIPLE( name, keycode, modifiers, _classname )				_KBBindKeyCommon( name, keycode, modifiers, _classname );

// mapping, one per class
struct PanelKeyBindingMap
{
	PanelKeyBindingMap()
	{
		baseMap = NULL;
		pfnClassName = NULL;
		processed = false;
	}

	CUtlVector< KeyBindingMap_t > entries;
	bool processed;
	PanelKeyBindingMap *baseMap;
	CUtlVector< BoundKey_t > defaultkeys;
	CUtlVector< BoundKey_t > boundkeys;
	char const *(*pfnClassName)( void );
};

PanelKeyBindingMap *FindPanelKeyBindingMap( char const *className );
PanelKeyBindingMap *FindOrAddPanelKeyBindingMap( char const *className );

} // namespace vgui


#endif // KEYBINDINGMAP_H
