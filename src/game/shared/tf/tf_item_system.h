//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_ITEM_SYSTEM_H
#define TF_ITEM_SYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "econ_item_system.h"
#include "tf_item_constants.h"

//-----------------------------------------------------------------------------
// Criteria used by the system to generate a base item for a slot in a class's loadout
struct baseitemcriteria_t
{
	baseitemcriteria_t()
	{
		iClass = 0;
		iSlot = LOADOUT_POSITION_INVALID;
	}

	int					iClass;
	int					iSlot;
};


class CTFItemSystem : public CEconItemSystem
{
public:
	// Select and return the base item definition index for a class's load-out slot 
	virtual item_definition_index_t GenerateBaseItem( baseitemcriteria_t *pCriteria );
};

CTFItemSystem *TFItemSystem( void );

#endif // TF_ITEM_SYSTEM_H
