//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef IHASATTRIBUTES_H
#define IHASATTRIBUTES_H
#ifdef _WIN32
#pragma once
#endif

//#include "attribute_manager.h"

class CAttributeManager;
class CAttributeContainer;
class CBaseEntity;
class CAttributeList;

// To allow an entity to have attributes, derive it from IHasAttributes and 
// contain an CAttributeManager in it. Then:
//		- Call InitializeAttributes() before your entity's Spawn()
//		- Call AddAttribute() to add attributes to the entity
//		- Call all the CAttributeManager hooks at the appropriate times in your entity.
// To get networking of the attributes to work on your entity:
//		- Add this to your entity's send table:
//			SendPropDataTable( SENDINFO_DT( m_AttributeManager ), &REFERENCE_SEND_TABLE(DT_AttributeManager) ),
//		- Call this inside your entity's OnDataChanged():
//			GetAttributeManager()->OnDataChanged( updateType );

//-----------------------------------------------------------------------------
// Purpose: Derive from this if your entity wants to contain attributes.
//-----------------------------------------------------------------------------
class IHasAttributes
{
public:
	virtual CAttributeManager	*GetAttributeManager( void ) = 0;
	virtual CAttributeContainer	*GetAttributeContainer( void ) = 0;
	virtual CBaseEntity			*GetAttributeOwner( void ) = 0;
	virtual CAttributeList		*GetAttributeList( void ) = 0;

	// Reapply yourself to whoever you should be providing attributes to.
	virtual void				ReapplyProvision( void ) = 0;
};

#endif // IHASATTRIBUTES_H
