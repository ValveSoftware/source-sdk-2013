//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shared VScript math functions.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// matrix3x4_t
// 
//=============================================================================

// 
// Exposes matrix3x4_t to VScript
// 
class CScriptMatrix3x4
{
public:
	CScriptMatrix3x4()
	{
		matrix = new matrix3x4_t();
		m_bFree = true;
	}

	~CScriptMatrix3x4()
	{
		if (m_bFree == true)
			delete matrix;
	}

	CScriptMatrix3x4( matrix3x4_t &inmatrix ) { matrix = &inmatrix; }

	matrix3x4_t *GetMatrix()					{ return matrix; }
	void SetMatrix( matrix3x4_t &inmatrix )		{ matrix = &inmatrix; }

	void Init( const Vector& xAxis, const Vector& yAxis, const Vector& zAxis, const Vector &vecOrigin )
	{
		matrix->Init( xAxis, yAxis, zAxis, vecOrigin );
	}

private:
	matrix3x4_t *matrix;
	bool m_bFree = false;
};

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptMatrix3x4, "matrix3x4_t", "A 3x4 matrix transform." )

	DEFINE_SCRIPT_CONSTRUCTOR()
	DEFINE_SCRIPTFUNC( Init, "Creates a matrix where the X axis = forward, the Y axis = left, and the Z axis = up." )

END_SCRIPTDESC();

matrix3x4_t *ToMatrix3x4( HSCRIPT hMat ) { return HScriptToClass<CScriptMatrix3x4>( hMat )->GetMatrix(); }

HSCRIPT ScriptCreateMatrixInstance( matrix3x4_t &matrix )
{
	CScriptMatrix3x4 *smatrix = new CScriptMatrix3x4( matrix );

	return g_pScriptVM->RegisterInstance( smatrix );
}

void ScriptFreeMatrixInstance( HSCRIPT hMat )
{
	CScriptMatrix3x4 *smatrix = HScriptToClass<CScriptMatrix3x4>( hMat );
	if (smatrix)
	{
		g_pScriptVM->RemoveInstance( hMat );
		delete smatrix;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void ScriptConcatTransforms( HSCRIPT hMat1, HSCRIPT hMat2, HSCRIPT hOut )
{
	if (!hMat1 || !hMat2 || !hOut)
		return;

	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );
	matrix3x4_t *pMat2 = ToMatrix3x4( hMat2 );
	matrix3x4_t *pOut = ToMatrix3x4( hOut );

	ConcatTransforms( *pMat1, *pMat2, *pOut );
}

void ScriptMatrixCopy( HSCRIPT hMat1, HSCRIPT hOut )
{
	if (!hMat1 || !hOut)
		return;

	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );
	matrix3x4_t *pOut = ToMatrix3x4( hOut );

	MatrixCopy( *pMat1, *pOut );
}

void ScriptMatrixInvert( HSCRIPT hMat1, HSCRIPT hOut )
{
	if (!hMat1 || !hOut)
		return;

	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );
	matrix3x4_t *pOut = ToMatrix3x4( hOut );

	MatrixInvert( *pMat1, *pOut );
}

void ScriptMatricesAreEqual( HSCRIPT hMat1, HSCRIPT hMat2 )
{
	if (!hMat1 || !hMat2)
		return;

	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );
	matrix3x4_t *pMat2 = ToMatrix3x4( hMat2 );

	MatricesAreEqual( *pMat1, *pMat2 );
}

const Vector& ScriptMatrixGetColumn( HSCRIPT hMat1, int column )
{
	static Vector outvec;
	outvec.Zero();
	if (!hMat1)
		return outvec;

	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );

	MatrixGetColumn( *pMat1, column, outvec );
	return outvec;
}

void ScriptMatrixSetColumn( const Vector& vecset, int column, HSCRIPT hMat1 )
{
	if (!hMat1)
		return;

	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );

	static Vector outvec;
	MatrixSetColumn( vecset, column, *pMat1 );
}

void ScriptMatrixAngles( HSCRIPT hMat1, const QAngle& angset, const Vector& vecset )
{
	if (!hMat1)
		return;

	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );

	MatrixAngles( *pMat1, *const_cast<QAngle*>(&angset), *const_cast<Vector*>(&vecset) );
}

void ScriptAngleMatrix( const QAngle& angset, const Vector& vecset, HSCRIPT hMat1 )
{
	if (!hMat1)
		return;

	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );

	AngleMatrix( angset, vecset, *pMat1 );
}

void ScriptAngleIMatrix( const QAngle& angset, const Vector& vecset, HSCRIPT hMat1 )
{
	if (!hMat1)
		return;

	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );

	AngleIMatrix( angset, vecset, *pMat1 );
}

void ScriptSetIdentityMatrix( HSCRIPT hMat1 )
{
	if (!hMat1)
		return;

	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );

	SetIdentityMatrix( *pMat1 );
}

void ScriptSetScaleMatrix( float x, float y, float z, HSCRIPT hMat1 )
{
	if (!hMat1)
		return;

	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );

	SetScaleMatrix( x, y, z, *pMat1 );
}

//=============================================================================
//
// Misc. Vector/QAngle functions
// 
//=============================================================================

const Vector& ScriptAngleVectors( const QAngle &angles )
{
	static Vector forward;
	AngleVectors( angles, &forward );
	return forward;
}

const QAngle& ScriptVectorAngles( const Vector &forward )
{
	static QAngle angles;
	VectorAngles( forward, angles );
	return angles;
}

const Vector& ScriptCalcClosestPointOnAABB( const Vector &mins, const Vector &maxs, const Vector &point )
{
	static Vector outvec;
	CalcClosestPointOnAABB( mins, maxs, point, outvec );
	return outvec;
}

const Vector& ScriptCalcClosestPointOnLine( const Vector &point, const Vector &vLineA, const Vector &vLineB )
{
	static Vector outvec;
	CalcClosestPointOnLine( point, vLineA, vLineB, outvec );
	return outvec;
}

float ScriptCalcDistanceToLine( const Vector &point, const Vector &vLineA, const Vector &vLineB )
{
	return CalcDistanceToLine( point, vLineA, vLineB );
}

const Vector& ScriptCalcClosestPointOnLineSegment( const Vector &point, const Vector &vLineA, const Vector &vLineB )
{
	static Vector outvec;
	CalcClosestPointOnLineSegment( point, vLineA, vLineB, outvec );
	return outvec;
}

float ScriptCalcDistanceToLineSegment( const Vector &point, const Vector &vLineA, const Vector &vLineB )
{
	return CalcDistanceToLineSegment( point, vLineA, vLineB );
}

#ifndef CLIENT_DLL
const Vector& ScriptPredictedPosition( HSCRIPT hTarget, float flTimeDelta )
{
	static Vector predicted;
	UTIL_PredictedPosition( ToEnt(hTarget), flTimeDelta, &predicted );
	return predicted;
}
#endif

void RegisterMathScriptFunctions()
{
	ScriptRegisterFunction( g_pScriptVM, RandomFloat, "Generate a random floating point number within a range, inclusive." );
	ScriptRegisterFunction( g_pScriptVM, RandomInt, "Generate a random integer within a range, inclusive." );

	ScriptRegisterFunction( g_pScriptVM, Approach, "Returns a value which approaches the target value from the input value with the specified speed." );
	ScriptRegisterFunction( g_pScriptVM, ApproachAngle, "Returns an angle which approaches the target angle from the input angle with the specified speed." );
	ScriptRegisterFunction( g_pScriptVM, AngleDiff, "Returns the degrees difference between two yaw angles." );
	ScriptRegisterFunction( g_pScriptVM, AngleDistance, "Returns the distance between two angles." );
	ScriptRegisterFunction( g_pScriptVM, AngleNormalize, "Clamps an angle to be in between -360 and 360." );
	ScriptRegisterFunction( g_pScriptVM, AngleNormalizePositive, "Clamps an angle to be in between 0 and 360." );
	ScriptRegisterFunction( g_pScriptVM, AnglesAreEqual, "Checks if two angles are equal based on a given tolerance value." );

	// 
	// matrix3x4_t
	// 
	g_pScriptVM->RegisterClass( GetScriptDescForClass( CScriptMatrix3x4 ) );

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptFreeMatrixInstance, "FreeMatrixInstance", "Frees an allocated matrix instance." );

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptConcatTransforms, "ConcatTransforms", "Concatenates two transformation matrices into another matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMatrixCopy, "MatrixCopy", "Copies a matrix to another matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMatrixInvert, "MatrixInvert", "Inverts a matrix and copies the result to another matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMatricesAreEqual, "MatricesAreEqual", "Checks if two matrices are equal." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMatrixGetColumn, "MatrixGetColumn", "Gets the column of a matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMatrixSetColumn, "MatrixSetColumn", "Sets the column of a matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMatrixAngles, "MatrixAngles", "Gets the angles and position of a matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptAngleMatrix, "AngleMatrix", "Sets the angles and position of a matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptAngleIMatrix, "AngleIMatrix", "Sets the inverted angles and position of a matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptSetIdentityMatrix, "SetIdentityMatrix", "Turns a matrix into an identity matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptSetScaleMatrix, "SetScaleMatrix", "Scales a matrix." );

	// 
	// Misc. Vector/QAngle functions
	// 
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptAngleVectors, "AngleVectors", "Turns an angle into a direction vector." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptVectorAngles, "VectorAngles", "Turns a direction vector into an angle." );

	ScriptRegisterFunction( g_pScriptVM, CalcSqrDistanceToAABB, "Returns the squared distance to a bounding box." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCalcClosestPointOnAABB, "CalcClosestPointOnAABB", "Returns the closest point on a bounding box." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCalcDistanceToLine, "CalcDistanceToLine", "Returns the distance to a line." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCalcClosestPointOnLine, "CalcClosestPointOnLine", "Returns the closest point on a line." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCalcDistanceToLineSegment, "CalcDistanceToLineSegment", "Returns the distance to a line segment." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCalcClosestPointOnLineSegment, "CalcClosestPointOnLineSegment", "Returns the closest point on a line segment." );

#ifndef CLIENT_DLL
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptPredictedPosition, "PredictedPosition", "Predicts what an entity's position will be in a given amount of time." );
#endif
}
