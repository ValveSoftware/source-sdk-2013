//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The expression operator class - scalar math calculator
// for a good list of operators and simple functions, see:
//   \\fileserver\user\MarcS\boxweb\aliveDistLite\v4.2.0\doc\alive\functions.txt
// (although we'll want to implement elerp as the standard 3x^2 - 2x^3 with rescale)
//
//=============================================================================

#ifndef DMEEXPRESSIONOPERATOR_H
#define DMEEXPRESSIONOPERATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmeoperator.h"
#include "tier1/utlstack.h" // for calculator
#include "tier1/utlvector.h" // for calculator


//-----------------------------------------------------------------------------
// Calculator Parsing class
// precedence order:
//		unary operators: + - ! func var
//		* / %
//		+ -
//		< > <= >=
//		== !=
//		&&
//		||
//		?:
//-----------------------------------------------------------------------------
class CExpressionCalculator
{
public:
	CExpressionCalculator( const char *expr = NULL ) : m_expr( expr ) {}
	void SetExpression( const char *expr ) 
	{
		m_expr = expr;
	}

	void SetVariable( const char *var, float value );
	void SetVariable( int nVariableIndex, float value );
	int FindVariableIndex( const char *var );

	bool Evaluate( float &value );

	// Builds a list of variable names from the expression
	bool BuildVariableListFromExpression( );

	// Iterate over variables
	int VariableCount();
	const char *VariableName( int nIndex );

private:
	bool ParseExpr		 ( const char *&expr );
	bool ParseConditional( const char *&expr );
	bool ParseOr		 ( const char *&expr );
	bool ParseAnd		 ( const char *&expr );
	bool ParseEquality	 ( const char *&expr );
	bool ParseLessGreater( const char *&expr );
	bool ParseAddSub	 ( const char *&expr );
	bool ParseDivMul	 ( const char *&expr );
	bool ParseUnary		 ( const char *&expr );
	bool ParsePrimary	 ( const char *&expr );
	bool Parse1ArgFunc	 ( const char *&expr );
	bool Parse2ArgFunc	 ( const char *&expr );
	bool Parse3ArgFunc	 ( const char *&expr );
	//	bool Parse4ArgFunc	 ( const char *&expr );
	bool Parse5ArgFunc	 ( const char *&expr );

	CUtlString m_expr;
	CUtlVector< CUtlString > m_varNames;
	CUtlVector<float> m_varValues;
	CUtlStack<float> m_stack;
	bool m_bIsBuildingArgumentList;
};


//-----------------------------------------------------------------------------
// An operator which computes the value of expressions 
//-----------------------------------------------------------------------------
class CDmeExpressionOperator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmeExpressionOperator, CDmeOperator );

public:
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

	void		SetSpewResult( bool state );

protected:
	bool Parse( const char *expr );

	bool IsInputAttribute( CDmAttribute *pAttribute );

	CDmaVar< float > m_result;
	CDmaString    m_expr;
	CDmaVar< bool >  m_bSpewResult;
};


#endif // DMEEXPRESSIONOPERATOR_H
