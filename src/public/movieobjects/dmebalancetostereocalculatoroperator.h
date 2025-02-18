//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMEBALANCETOSTEREOCALCULATOROPERATOR_H
#define DMEBALANCETOSTEREOCALCULATOROPERATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmeoperator.h"

class CDmeBalanceToStereoCalculatorOperator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmeBalanceToStereoCalculatorOperator, CDmeOperator );

public:
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

	void		SetSpewResult( bool state );
	 
protected:
	float ComputeDefaultValue();

	CDmaVar< float > m_result_left;
	CDmaVar< float > m_result_right;
	CDmaVar< float > m_result_multi;

	CDmaVar< float > m_value;		// input value
	CDmaVar< float > m_balance;		// balance value
	CDmaVar< float > m_multilevel;	// multilevel value

	// Debuggin
	CDmaVar< bool >  m_bSpewResult;

	float m_flDefaultValue;
};


#endif // DMEBALANCETOSTEREOCALCULATOROPERATOR_H
