//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PREDICTED_VIEWMODEL_H
#define PREDICTED_VIEWMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "predictable_entity.h"
#include "utlvector.h"
#include "baseplayer_shared.h"
#include "shared_classnames.h"

#if defined( CLIENT_DLL )
#define CPredictedViewModel C_PredictedViewModel
#endif

class CPredictedViewModel : public CBaseViewModel
{
	DECLARE_CLASS( CPredictedViewModel, CBaseViewModel );
public:

	DECLARE_NETWORKCLASS();

	CPredictedViewModel( void );
	virtual ~CPredictedViewModel( void );
							
	virtual void CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles );

#if defined( CLIENT_DLL )
	virtual bool ShouldPredict( void )
	{
		if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
			return true;

		return BaseClass::ShouldPredict();
	}

	virtual bool PredictionErrorShouldResetLatchedForAllPredictables( void ) OVERRIDE
	{
#ifdef HL2MP
		// misyl: If viewmodel mispred's on HL2MP don't reset all the player's variables.
		return false;
#else
		// Not changing this behaviour for other games without testing. They also don't have the same issues.
		return BaseClass::PredictionErrorShouldResetLatchedForAllPredictables();
#endif
	}
#endif

private:
	
#if defined( CLIENT_DLL )

	// This is used to lag the angles.
	CInterpolatedVar<QAngle> m_LagAnglesHistory;
	QAngle m_vLagAngles;
	Vector	m_vPredictedOffset;

	CPredictedViewModel( const CPredictedViewModel & ); // not defined, not accessible

#endif
};

#endif // PREDICTED_VIEWMODEL_H
