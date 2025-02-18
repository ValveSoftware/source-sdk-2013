//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  
//			
//=============================================================================

#ifndef TF_PROGRESSION_H
#define TF_PROGRESSION_H
#ifdef _WIN32
#pragma once
#endif

class IProgressionDesc;

enum EProgressionDesc
{
	k_eProgression_Casual,
	k_eProgression_Drillo,
	k_eProgression_Glicko
};

const IProgressionDesc* GetProgressionDesc( EProgressionDesc eType );
#endif //TF_PROGRESSION_H
