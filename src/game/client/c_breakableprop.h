//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_BREAKABLEPROP_H
#define C_BREAKABLEPROP_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_BreakableProp : public C_BaseAnimating
{
	typedef C_BaseAnimating BaseClass;
public:
	DECLARE_CLIENTCLASS();

	C_BreakableProp();
	
	virtual void SetFadeMinMax( float fademin, float fademax );

	//virtual bool	ShouldPredict( void ) OVERRIDE;
	//virtual C_BasePlayer *GetPredictionOwner( void ) OVERRIDE;
	virtual bool PredictionErrorShouldResetLatchedForAllPredictables( void ) OVERRIDE { return false; }

	// Copy fade from another breakable prop
	void CopyFadeFrom( C_BreakableProp *pSource );
};

#endif // C_BREAKABLEPROP_H
