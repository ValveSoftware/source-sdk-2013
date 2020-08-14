//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Shared VScript math functions.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "vscript_funcs_math.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// matrix3x4_t
// 
//=============================================================================
BEGIN_SCRIPTDESC_ROOT_NAMED( matrix3x4_t, "matrix3x4_t", "A 3x4 matrix transform." )

	DEFINE_SCRIPT_CONSTRUCTOR()
	DEFINE_SCRIPTFUNC( Init, "Creates a matrix where the X axis = forward, the Y axis = left, and the Z axis = up." )

END_SCRIPTDESC();

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
// Quaternion
// 
//=============================================================================
CScriptQuaternionInstanceHelper g_QuaternionScriptInstanceHelper;

BEGIN_SCRIPTDESC_ROOT_NAMED( Quaternion, "Quaternion", "A quaternion." )

	DEFINE_SCRIPT_CONSTRUCTOR()
	DEFINE_SCRIPT_INSTANCE_HELPER( &g_QuaternionScriptInstanceHelper )
	DEFINE_SCRIPTFUNC_NAMED( ScriptInit, "Init", "Creates a quaternion with the given values." )

END_SCRIPTDESC();

//-----------------------------------------------------------------------------

bool CScriptQuaternionInstanceHelper::ToString( void *p, char *pBuf, int bufSize )
{
	Quaternion *pQuat = ((Quaternion *)p);
	V_snprintf( pBuf, bufSize, "(quaternion: (%f, %f, %f, %f))", pQuat->x, pQuat->y, pQuat->z, pQuat->w );
	return true; 
}

bool CScriptQuaternionInstanceHelper::Get( void *p, const char *pszKey, ScriptVariant_t &variant )
{
	Quaternion *pQuat = ((Quaternion *)p);
	if ( strlen(pszKey) == 1 )
	{
		switch (pszKey[0])
		{
			case 'x':
				variant = pQuat->x;
				return true;
			case 'y':
				variant = pQuat->y;
				return true;
			case 'z':
				variant = pQuat->z;
				return true;
			case 'w':
				variant = pQuat->w;
				return true;
		}
	}
	return false;
}

bool CScriptQuaternionInstanceHelper::Set( void *p, const char *pszKey, ScriptVariant_t &variant )
{
	Quaternion *pQuat = ((Quaternion *)p);
	if ( strlen(pszKey) == 1 )
	{
		switch (pszKey[0])
		{
			case 'x':
				variant.AssignTo( &pQuat->x );
				return true;
			case 'y':
				variant.AssignTo( &pQuat->y );
				return true;
			case 'z':
				variant.AssignTo( &pQuat->z );
				return true;
			case 'w':
				variant.AssignTo( &pQuat->w );
				return true;
		}
	}
	return false;
}

ScriptVariant_t *CScriptQuaternionInstanceHelper::Add( void *p, ScriptVariant_t &variant )
{
	Quaternion *pQuat = ((Quaternion *)p);
	DevMsg("Adding %i is an interesting choice\n", variant.m_int);

	float flAdd;
	variant.AssignTo( &flAdd );

	(*pQuat)[0] += flAdd;
	(*pQuat)[1] += flAdd;
	(*pQuat)[2] += flAdd;
	(*pQuat)[3] += flAdd;

	static ScriptVariant_t result;
	result = (HSCRIPT)p;
	return &result;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void ScriptQuaternionAdd( HSCRIPT hQuat1, HSCRIPT hQuat2, HSCRIPT hOut )
{
	if (!hQuat1 || !hQuat2 || !hOut)
		return;

	Quaternion *pQuat1 = ToQuaternion( hQuat1 );
	Quaternion *pQuat2 = ToQuaternion( hQuat2 );
	Quaternion *pOut = ToQuaternion( hOut );

	QuaternionAdd( *pQuat1, *pQuat2, *pOut );
}

void ScriptMatrixQuaternion( HSCRIPT hMat1, HSCRIPT hQuat1 )
{
	if (!hMat1 || !hQuat1)
		return;

	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );
	Quaternion *pQuat1 = ToQuaternion( hQuat1 );

	MatrixQuaternion( *pMat1, *pQuat1 );
}

void ScriptQuaternionMatrix( HSCRIPT hQuat1, HSCRIPT hMat1 )
{
	if (!hQuat1 || !hMat1)
		return;

	Quaternion *pQuat1 = ToQuaternion( hQuat1 );
	matrix3x4_t *pMat1 = ToMatrix3x4( hMat1 );

	QuaternionMatrix( *pQuat1, *pMat1 );
}

QAngle ScriptQuaternionAngles( HSCRIPT hQuat1 )
{
	if (!hQuat1)
		return QAngle();

	Quaternion *pQuat1 = ToQuaternion( hQuat1 );

	QAngle angles;
	QuaternionAngles( *pQuat1, angles );
	return angles;
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

const Vector& ScriptVectorRotate( const Vector &in, HSCRIPT hMat )
{
	if (ToMatrix3x4(hMat) == NULL)
		return vec3_origin;

	static Vector out;
	VectorRotate( in, *ToMatrix3x4(hMat), out );
	return out;
}

const Vector& ScriptVectorIRotate( const Vector &in, HSCRIPT hMat )
{
	if (ToMatrix3x4(hMat) == NULL)
		return vec3_origin;

	static Vector out;
	VectorIRotate( in, *ToMatrix3x4(hMat), out );
	return out;
}

const Vector& ScriptVectorTransform( const Vector &in, HSCRIPT hMat )
{
	if (ToMatrix3x4(hMat) == NULL)
		return vec3_origin;

	static Vector out;
	VectorTransform( in, *ToMatrix3x4( hMat ), out );
	return out;
}

const Vector& ScriptVectorITransform( const Vector &in, HSCRIPT hMat )
{
	if (ToMatrix3x4(hMat) == NULL)
		return vec3_origin;

	static Vector out;
	VectorITransform( in, *ToMatrix3x4( hMat ), out );
	return out;
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
	g_pScriptVM->RegisterClass( GetScriptDescForClass( matrix3x4_t ) );

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
	// Quaternion
	// 
	g_pScriptVM->RegisterClass( GetScriptDescForClass( Quaternion ) );

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptFreeQuaternionInstance, "FreeQuaternionInstance", "Frees an allocated quaternion instance." );

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptQuaternionAdd, "QuaternionAdd", "Adds two quaternions together into another quaternion." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMatrixQuaternion, "MatrixQuaternion", "Converts a matrix to a quaternion." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptQuaternionMatrix, "QuaternionMatrix", "Converts a quaternion to a matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptQuaternionAngles, "QuaternionAngles", "Converts a quaternion to angles." );

	// 
	// Misc. Vector/QAngle functions
	// 
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptAngleVectors, "AngleVectors", "Turns an angle into a direction vector." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptVectorAngles, "VectorAngles", "Turns a direction vector into an angle." );

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptVectorRotate, "VectorRotate", "Rotates a vector with a matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptVectorIRotate, "VectorIRotate", "Rotates a vector with the inverse of a matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptVectorTransform, "VectorTransform", "Transforms a vector with a matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptVectorITransform, "VectorITransform", "Transforms a vector with the inverse of a matrix." );

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
