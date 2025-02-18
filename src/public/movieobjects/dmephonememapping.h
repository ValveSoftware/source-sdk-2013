//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMEPHONEMEMAPPING_H
#define DMEPHONEMEMAPPING_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"

class CDmePhonemeMapping : public CDmElement
{
	DEFINE_ELEMENT( CDmePhonemeMapping, CDmElement );

public:
	CDmaString    m_Preset; // "preset"  // map this item to this preset
	CDmaVar< float > m_Weight; // "weight"  // using this weight
};

#endif // DMEPHONEMEMAPPING_H
