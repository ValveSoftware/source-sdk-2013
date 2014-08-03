//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Helper for the CHudElement class to add themselves to the list of hud elements
//
// $NoKeywords: $
//=============================================================================//
#include "vgui/IVGui.h"
#include "vgui_controls/MessageMap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// Start with empty list
CBuildFactoryHelper *CBuildFactoryHelper::m_sHelpers = NULL;

//-----------------------------------------------------------------------------
// Purpose: Constructs a panel  factory
// Input  : pfnCreate - fn Ptr to a function which generates a panel
//-----------------------------------------------------------------------------
CBuildFactoryHelper::CBuildFactoryHelper( char const *className, PANELCREATEFUNC func )
{
	// Make this fatal
	if ( HasFactory( className ) )
	{
		Error( "CBuildFactoryHelper:  Factory for '%s' already exists!!!!\n", className );
	}

	//List is empty, or element belongs at front, insert here
	m_pNext			= m_sHelpers;
	m_sHelpers		= this;

	Assert( func );
	m_CreateFunc = func;
	Assert( className );
	m_pClassName = className;
}

//-----------------------------------------------------------------------------
// Purpose: Returns next object in list
// Output : CBuildFactoryHelper
//-----------------------------------------------------------------------------
CBuildFactoryHelper *CBuildFactoryHelper::GetNext( void )
{ 
	return m_pNext;
}

char const *CBuildFactoryHelper::GetClassName() const
{
	return m_pClassName;
}

vgui::Panel *CBuildFactoryHelper::CreatePanel()
{
	if ( !m_CreateFunc )
		return NULL;

	return ( *m_CreateFunc )();
}

// private static meethod
bool CBuildFactoryHelper::HasFactory( char const *className )
{
	CBuildFactoryHelper *p = m_sHelpers;
	while ( p )
	{
		if ( !Q_stricmp( className, p->GetClassName() ) )
			return true;

		p = p->GetNext();
	}
	return false;
}

// static method
vgui::Panel *CBuildFactoryHelper::InstancePanel( char const *className )
{
	CBuildFactoryHelper *p = m_sHelpers;
	while ( p )
	{
		if ( !Q_stricmp( className, p->GetClassName() ) )
			return p->CreatePanel();

		p = p->GetNext();
	}
	return NULL;
}

// static method
void CBuildFactoryHelper::GetFactoryNames( CUtlVector< char const * >& list )
{
	list.RemoveAll();

	CBuildFactoryHelper *p = m_sHelpers;
	while ( p )
	{
		list.AddToTail( p->GetClassName() );
		p = p->GetNext();
	}
}



