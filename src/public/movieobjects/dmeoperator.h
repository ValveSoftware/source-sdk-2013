//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The abstract base operator class - all actions within the scenegraph happen via operators
//
//=============================================================================

#ifndef DMEOPERATOR_H
#define DMEOPERATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"

#include "tier1/utlvector.h"


//-----------------------------------------------------------------------------
// A class representing a camera
//-----------------------------------------------------------------------------
class CDmeOperator : public IDmeOperator, public CDmElement
{
	DEFINE_ELEMENT( CDmeOperator, CDmElement );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate() {}

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs ) {}
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs ) {}
};


#endif // DMEOPERATOR_H
