//============ Copyright (c) Valve Corporation, All rights reserved. ==========
//
//=============================================================================

#ifndef DELTAUTILS_H
#define DELTAUTILS_H


// Valve includes
#include "tier1/utlstring.h"
#include "tier1/utlsymbol.h"
#include "tier1/utlvector.h"


//=============================================================================
//
//=============================================================================
class CDeltaUtils
{
public:
	enum Corrector_t
	{
		CT_NO_CORRECTORS,
		CT_SINGLE_UNDERSCORE,
		CT_DOUBLE_UNDERSCORES
	};

	enum DeltaSortForwardBackward_t
	{
		DS_FORWARD,
		DS_BACKWARD
	};

	static void SplitDeltaName(
		CUtlVector< CUtlSymbol > &splitDeltaName,
		CUtlSymbolTable &deltaSymbolTable,
		const CUtlString &sDeltaName,
		Corrector_t nCorrectorType );

	static void ComputeDependentDeltas(
		CUtlVector< CUtlVector< int > > &dependentDeltaList,
		const CUtlVector< CUtlString > &deltaList,
		Corrector_t nCorrectorType );

	static void ComputeDeltaOrder(
		CUtlVector< int > &deltaOrder,
		const CUtlVector< CUtlVector< int > > &dependentDeltaList,
		DeltaSortForwardBackward_t eForwardBackward );

	static void MakeDeltaVerticesAbsolute(
		CUtlVector< CUtlVector< Vector > > &vDeltaVerticesList,
		const CUtlVector< CUtlString > &deltaList,
		Corrector_t nCorrectorType );

	static void MakeDeltaVerticesRelative(
		CUtlVector< CUtlVector< Vector > > &vDeltaVerticesList,
		const CUtlVector< CUtlString > &deltaList,
		Corrector_t nCorrectorType );
};


#endif // DELTAUTILS_H
