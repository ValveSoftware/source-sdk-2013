//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Helper for the CHudElement class to add themselves to the list of hud elements
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_ELEMENT_HELPER_H
#define HUD_ELEMENT_HELPER_H
#ifdef _WIN32
#pragma once
#endif

class CHudElement;

//-----------------------------------------------------------------------------
// Purpose: Used by DECLARE_HUDELEMENT macro to create a linked list of
//  instancing functions
//-----------------------------------------------------------------------------
class CHudElementHelper
{
public:
	// Static list of helpers
	static CHudElementHelper *m_sHelpers;
	// Create all the hud elements
	static void CreateAllElements( void );

public:
	// Construction
	CHudElementHelper( CHudElement *( *pfnCreate )( void ), int depth );

	// Accessors
	CHudElementHelper *GetNext( void );

private:
	// Next factory in list
	CHudElementHelper	*m_pNext;
	// Creation function to use for this technology
	CHudElement			*( *m_pfnCreate )( void );

	//Depth used to determine hud panel ordering
	int					m_iDepth;
};

// This is the macro which implements creation of each hud element
// It creates a function which instances an object of the specified type
// It them hooks that function up to the helper list so that the CHud objects can create
//  the elements by name, with no header file dependency, etc.
#define DECLARE_HUDELEMENT( className )											\
	static CHudElement *Create_##className( void )							\
		{																		\
			return new className( #className );									\
		};																		\
	static CHudElementHelper g_##className##_Helper( Create_##className, 50 );

#define DECLARE_HUDELEMENT_DEPTH( className, depth )											\
	static CHudElement *Create_##className( void )							\
		{																		\
			return new className( #className );									\
		};																		\
	static CHudElementHelper g_##className##_Helper( Create_##className, depth );

#define DECLARE_NAMED_HUDELEMENT( className, panelName )						\
	static CHudElement *Create_##panelName( void )							\
		{																		\
			return new className( #panelName );									\
		};																		\
	static CHudElementHelper g_##panelName##_Helper( Create_##panelName, 50 );

// This macro can be used to get a pointer to a specific hud element
#define GET_HUDELEMENT( className )												\
	( className *)gHUD.FindElement( #className )

#define GET_NAMED_HUDELEMENT( className, panelName )							\
	( className *)gHUD.FindElement( #panelName )


// Things that inherit from vgui::Panel, too, will have ambiguous new operators
//  so this should disambiguate them
#define DECLARE_MULTIPLY_INHERITED()							\
    void *operator new( size_t stAllocateBlock )				\
	{															\
		return CHudElement::operator new ( stAllocateBlock );	\
	}															\
	void* operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine )	\
	{ 																								\
		return CHudElement::operator new ( stAllocateBlock, nBlockUse, pFileName, nLine );	\
	}															\
	void operator delete( void *pMem )							\
	{															\
		CHudElement::operator delete ( pMem );					\
	}															\
	void operator delete( void *pMem, int nBlockUse, const char *pFileName, int nLine )	\
	{															\
		CHudElement::operator delete ( pMem, nBlockUse, pFileName, nLine );	\
	}

// Alias for base hud element
#define IMPLEMENT_OPERATORS_NEW_AND_DELETE			DECLARE_MULTIPLY_INHERITED

#endif // HUD_ELEMENT_HELPER_H
