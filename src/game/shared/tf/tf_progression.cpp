//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_progression.h"

#include "tf_progression_description_casual.h"
#include "tf_progression_description_comp.h"

const IProgressionDesc* GetProgressionDesc( EProgressionDesc eType )
{
	static CCasualProgressionDesc progressionCasual;
	static CDrilloProgressionDesc progressionDrillo;
	static CGlickoProgressionDesc progressionGlicko;

	switch ( eType )
	{
	case k_eProgression_Casual:
		return &progressionCasual;
	case k_eProgression_Drillo:
		return &progressionDrillo;
	case k_eProgression_Glicko:
		return &progressionGlicko;
	}

	Assert( false );
	return NULL;
}