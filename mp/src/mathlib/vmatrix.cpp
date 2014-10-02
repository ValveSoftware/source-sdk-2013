//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#if !defined(_STATIC_LINKED) || defined(_SHARED_LIB)

#include "basetypes.h"
#include "mathlib/vmatrix.h"
#include "mathlib/mathlib.h"
#include <string.h>
#include "mathlib/vector4d.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#pragma warning (disable : 4700) // local variable 'x' used without having been initialized

// ------------------------------------------------------------------------------------------- //
// Helper functions.
// ------------------------------------------------------------------------------------------- //

#ifndef VECTOR_NO_SLOW_OPERATIONS

VMatrix SetupMatrixIdentity()
{
	return VMatrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

VMatrix SetupMatrixTranslation(const Vector &vTranslation)
{
	return VMatrix(
		1.0f, 0.0f, 0.0f, vTranslation.x,
		0.0f, 1.0f, 0.0f, vTranslation.y,
		0.0f, 0.0f, 1.0f, vTranslation.z,
		0.0f, 0.0f, 0.0f, 1.0f
		);
}

VMatrix SetupMatrixScale(const Vector &vScale)
{
	return VMatrix(
		vScale.x, 0.0f,     0.0f,     0.0f,
		0.0f,     vScale.y, 0.0f,     0.0f,
		0.0f,     0.0f,     vScale.z, 0.0f,
		0.0f,     0.0f,     0.0f,     1.0f
		);
}

VMatrix SetupMatrixReflection(const VPlane &thePlane)
{
	VMatrix mReflect, mBack, mForward;
	Vector vOrigin, N;

	N = thePlane.m_Normal;

	mReflect.Init( 
		-2.0f*N.x*N.x + 1.0f,	-2.0f*N.x*N.y,			-2.0f*N.x*N.z,			0.0f,
		-2.0f*N.y*N.x,			-2.0f*N.y*N.y + 1.0f,	-2.0f*N.y*N.z,			0.0f,
		-2.0f*N.z*N.x,			-2.0f*N.z*N.y,			-2.0f*N.z*N.z + 1.0f,	0.0f,
		0.0f,					0.0f,					0.0f,					1.0f
		);

	vOrigin = thePlane.GetPointOnPlane();

	mBack.Identity();
	mBack.SetTranslation(-vOrigin);

	mForward.Identity();
	mForward.SetTranslation(vOrigin);

	// (multiplied in reverse order, so it translates to the origin point,
	// reflects, and translates back).
	return mForward * mReflect * mBack;
}

VMatrix SetupMatrixProjection(const Vector &vOrigin, const VPlane &thePlane)
{
	vec_t dot;
	VMatrix mRet;


	#define PN thePlane.m_Normal
	#define PD thePlane.m_Dist;

		dot = PN[0]*vOrigin.x + PN[1]*vOrigin.y + PN[2]*vOrigin.z - PD;

		mRet.m[0][0] = dot - vOrigin.x * PN[0];
		mRet.m[0][1] = -vOrigin.x * PN[1];
		mRet.m[0][2] = -vOrigin.x * PN[2];
		mRet.m[0][3] = -vOrigin.x * -PD;

		mRet.m[1][0] = -vOrigin.y * PN[0];
		mRet.m[1][1] = dot - vOrigin.y * PN[1];
		mRet.m[1][2] = -vOrigin.y * PN[2];
		mRet.m[1][3] = -vOrigin.y * -PD;

		mRet.m[2][0] = -vOrigin.z * PN[0];
		mRet.m[2][1] = -vOrigin.z * PN[1];
		mRet.m[2][2] = dot - vOrigin.z * PN[2];
		mRet.m[2][3] = -vOrigin.z * -PD;

		mRet.m[3][0] = -PN[0];
		mRet.m[3][1] = -PN[1];
		mRet.m[3][2] = -PN[2];
		mRet.m[3][3] = dot + PD;

	#undef PN
	#undef PD	

	return mRet;
}

VMatrix SetupMatrixAxisRot(const Vector &vAxis, vec_t fDegrees)
{
	vec_t s, c, t;
	vec_t tx, ty, tz;
	vec_t sx, sy, sz;
	vec_t fRadians;


	fRadians = fDegrees * (M_PI / 180.0f);
	
	s = (vec_t)sin(fRadians);
	c = (vec_t)cos(fRadians);
	t = 1.0f - c;

	tx = t * vAxis.x;	ty = t * vAxis.y;	tz = t * vAxis.z;
	sx = s * vAxis.x;	sy = s * vAxis.y;	sz = s * vAxis.z;

	return VMatrix(
		tx*vAxis.x + c,  tx*vAxis.y - sz, tx*vAxis.z + sy, 0.0f,
		tx*vAxis.y + sz, ty*vAxis.y + c,  ty*vAxis.z - sx, 0.0f,
		tx*vAxis.z - sy, ty*vAxis.z + sx, tz*vAxis.z + c,  0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

VMatrix SetupMatrixAngles(const QAngle &vAngles)
{
	VMatrix mRet;
	MatrixFromAngles( vAngles, mRet );
	return mRet;
}

VMatrix SetupMatrixOrgAngles(const Vector &origin, const QAngle &vAngles)
{
	VMatrix mRet;
	mRet.SetupMatrixOrgAngles( origin, vAngles );
	return mRet;
}

#endif // VECTOR_NO_SLOW_OPERATIONS


bool PlaneIntersection( const VPlane &vp1, const VPlane &vp2, const VPlane &vp3, Vector &vOut )
{
	VMatrix mMat, mInverse;

	mMat.Init(
		vp1.m_Normal.x, vp1.m_Normal.y, vp1.m_Normal.z, -vp1.m_Dist,
		vp2.m_Normal.x, vp2.m_Normal.y, vp2.m_Normal.z, -vp2.m_Dist,
		vp3.m_Normal.x, vp3.m_Normal.y, vp3.m_Normal.z, -vp3.m_Dist,
		0.0f, 0.0f, 0.0f, 1.0f
		);
	
	if(mMat.InverseGeneral(mInverse))
	{
		//vOut = mInverse * Vector(0.0f, 0.0f, 0.0f);
		mInverse.GetTranslation( vOut );
		return true;
	}
	else
	{
		return false;
	}
}



// ------------------------------------------------------------------------------------------- //
// VMatrix functions.
// ------------------------------------------------------------------------------------------- //

VMatrix& VMatrix::operator=(const VMatrix &mOther)
{
	m[0][0] = mOther.m[0][0];
	m[0][1] = mOther.m[0][1];
	m[0][2] = mOther.m[0][2];
	m[0][3] = mOther.m[0][3];

	m[1][0] = mOther.m[1][0];
	m[1][1] = mOther.m[1][1];
	m[1][2] = mOther.m[1][2];
	m[1][3] = mOther.m[1][3];

	m[2][0] = mOther.m[2][0];
	m[2][1] = mOther.m[2][1];
	m[2][2] = mOther.m[2][2];
	m[2][3] = mOther.m[2][3];

	m[3][0] = mOther.m[3][0];
	m[3][1] = mOther.m[3][1];
	m[3][2] = mOther.m[3][2];
	m[3][3] = mOther.m[3][3];

	return *this;
}

bool VMatrix::operator==( const VMatrix& src ) const
{
	return !memcmp( src.m, m, sizeof(m) );
}

void VMatrix::MatrixMul( const VMatrix &vm, VMatrix &out ) const
{
	out.Init(
		m[0][0]*vm.m[0][0] + m[0][1]*vm.m[1][0] + m[0][2]*vm.m[2][0] + m[0][3]*vm.m[3][0],
		m[0][0]*vm.m[0][1] + m[0][1]*vm.m[1][1] + m[0][2]*vm.m[2][1] + m[0][3]*vm.m[3][1],
		m[0][0]*vm.m[0][2] + m[0][1]*vm.m[1][2] + m[0][2]*vm.m[2][2] + m[0][3]*vm.m[3][2],
		m[0][0]*vm.m[0][3] + m[0][1]*vm.m[1][3] + m[0][2]*vm.m[2][3] + m[0][3]*vm.m[3][3],

		m[1][0]*vm.m[0][0] + m[1][1]*vm.m[1][0] + m[1][2]*vm.m[2][0] + m[1][3]*vm.m[3][0],
		m[1][0]*vm.m[0][1] + m[1][1]*vm.m[1][1] + m[1][2]*vm.m[2][1] + m[1][3]*vm.m[3][1],
		m[1][0]*vm.m[0][2] + m[1][1]*vm.m[1][2] + m[1][2]*vm.m[2][2] + m[1][3]*vm.m[3][2],
		m[1][0]*vm.m[0][3] + m[1][1]*vm.m[1][3] + m[1][2]*vm.m[2][3] + m[1][3]*vm.m[3][3],

		m[2][0]*vm.m[0][0] + m[2][1]*vm.m[1][0] + m[2][2]*vm.m[2][0] + m[2][3]*vm.m[3][0],
		m[2][0]*vm.m[0][1] + m[2][1]*vm.m[1][1] + m[2][2]*vm.m[2][1] + m[2][3]*vm.m[3][1],
		m[2][0]*vm.m[0][2] + m[2][1]*vm.m[1][2] + m[2][2]*vm.m[2][2] + m[2][3]*vm.m[3][2],
		m[2][0]*vm.m[0][3] + m[2][1]*vm.m[1][3] + m[2][2]*vm.m[2][3] + m[2][3]*vm.m[3][3],

		m[3][0]*vm.m[0][0] + m[3][1]*vm.m[1][0] + m[3][2]*vm.m[2][0] + m[3][3]*vm.m[3][0],
		m[3][0]*vm.m[0][1] + m[3][1]*vm.m[1][1] + m[3][2]*vm.m[2][1] + m[3][3]*vm.m[3][1],
		m[3][0]*vm.m[0][2] + m[3][1]*vm.m[1][2] + m[3][2]*vm.m[2][2] + m[3][3]*vm.m[3][2],
		m[3][0]*vm.m[0][3] + m[3][1]*vm.m[1][3] + m[3][2]*vm.m[2][3] + m[3][3]*vm.m[3][3]
		);
}

#ifndef VECTOR_NO_SLOW_OPERATIONS

VMatrix VMatrix::operator*(const VMatrix &vm) const
{
	VMatrix ret;
	MatrixMul( vm, ret );
	return ret;
}

#endif

bool VMatrix::InverseGeneral(VMatrix &vInverse) const
{
	return MatrixInverseGeneral( *this, vInverse );
}


bool MatrixInverseGeneral(const VMatrix& src, VMatrix& dst)
{
	int iRow, i, j, iTemp, iTest;
	vec_t mul, fTest, fLargest;
	vec_t mat[4][8];
	int rowMap[4], iLargest;
	vec_t *pOut, *pRow, *pScaleRow;


	// How it's done.
	// AX = I
	// A = this
	// X = the matrix we're looking for
	// I = identity

	// Setup AI
	for(i=0; i < 4; i++)
	{
		const vec_t *pIn = src[i];
		pOut = mat[i];

		for(j=0; j < 4; j++)
		{
			pOut[j] = pIn[j];
		}

		pOut[4] = 0.0f;
		pOut[5] = 0.0f;
		pOut[6] = 0.0f;
		pOut[7] = 0.0f;
		pOut[i+4] = 1.0f;

		rowMap[i] = i;
	}

	// Use row operations to get to reduced row-echelon form using these rules:
	// 1. Multiply or divide a row by a nonzero number.
	// 2. Add a multiple of one row to another.
	// 3. Interchange two rows.

	for(iRow=0; iRow < 4; iRow++)
	{
		// Find the row with the largest element in this column.
		fLargest = 0.00001f;
		iLargest = -1;
		for(iTest=iRow; iTest < 4; iTest++)
		{
			fTest = (vec_t)FloatMakePositive(mat[rowMap[iTest]][iRow]);
			if(fTest > fLargest)
			{
				iLargest = iTest;
				fLargest = fTest;
			}
		}

		// They're all too small.. sorry.
		if(iLargest == -1)
		{
			return false;
		}

		// Swap the rows.
		iTemp = rowMap[iLargest];
		rowMap[iLargest] = rowMap[iRow];
		rowMap[iRow] = iTemp;

		pRow = mat[rowMap[iRow]];

		// Divide this row by the element.
		mul = 1.0f / pRow[iRow];
		for(j=0; j < 8; j++)
			pRow[j] *= mul;

		pRow[iRow] = 1.0f; // Preserve accuracy...
		
		// Eliminate this element from the other rows using operation 2.
		for(i=0; i < 4; i++)
		{
			if(i == iRow)
				continue;

			pScaleRow = mat[rowMap[i]];
		
			// Multiply this row by -(iRow*the element).
			mul = -pScaleRow[iRow];
			for(j=0; j < 8; j++)
			{
				pScaleRow[j] += pRow[j] * mul;
			}

			pScaleRow[iRow] = 0.0f; // Preserve accuracy...
		}
	}

	// The inverse is on the right side of AX now (the identity is on the left).
	for(i=0; i < 4; i++)
	{
		const vec_t *pIn = mat[rowMap[i]] + 4;
		pOut = dst.m[i];

		for(j=0; j < 4; j++)
		{
			pOut[j] = pIn[j];
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Does a fast inverse, assuming the matrix only contains translation and rotation.
//-----------------------------------------------------------------------------
void MatrixInverseTR( const VMatrix& src, VMatrix &dst )
{
	Vector vTrans, vNewTrans;

	// Transpose the upper 3x3.
	dst.m[0][0] = src.m[0][0];  dst.m[0][1] = src.m[1][0]; dst.m[0][2] = src.m[2][0];
	dst.m[1][0] = src.m[0][1];  dst.m[1][1] = src.m[1][1]; dst.m[1][2] = src.m[2][1];
	dst.m[2][0] = src.m[0][2];  dst.m[2][1] = src.m[1][2]; dst.m[2][2] = src.m[2][2];

	// Transform the translation.
	vTrans.Init( -src.m[0][3], -src.m[1][3], -src.m[2][3] );
	Vector3DMultiply( dst, vTrans, vNewTrans );
	MatrixSetColumn( dst, 3, vNewTrans );

	// Fill in the bottom row.
	dst.m[3][0] = dst.m[3][1] = dst.m[3][2] = 0.0f;
	dst.m[3][3] = 1.0f;
}


void VMatrix::InverseTR( VMatrix &ret ) const
{
	MatrixInverseTR( *this, ret );
}

void MatrixInverseTranspose( const VMatrix& src, VMatrix& dst )
{
	src.InverseGeneral( dst );
	MatrixTranspose( dst, dst );
}

//-----------------------------------------------------------------------------
// Computes the inverse transpose
//-----------------------------------------------------------------------------
void MatrixInverseTranspose( const matrix3x4_t& src, matrix3x4_t& dst )
{
	VMatrix tmp, out;
	tmp.CopyFrom3x4( src );
	::MatrixInverseTranspose( tmp, out );
	out.Set3x4( dst );
}


#ifndef VECTOR_NO_SLOW_OPERATIONS

VMatrix VMatrix::InverseTR() const
{
	VMatrix ret;
	MatrixInverseTR( *this, ret );
	return ret;
}

Vector VMatrix::GetScale() const
{
	Vector vecs[3];

	GetBasisVectors(vecs[0], vecs[1], vecs[2]);

	return Vector(
		vecs[0].Length(),
		vecs[1].Length(),
		vecs[2].Length()
		);
}

VMatrix VMatrix::Scale(const Vector &vScale)
{
	return VMatrix(
		m[0][0]*vScale.x, m[0][1]*vScale.y, m[0][2]*vScale.z, m[0][3],
		m[1][0]*vScale.x, m[1][1]*vScale.y, m[1][2]*vScale.z, m[1][3],
		m[2][0]*vScale.x, m[2][1]*vScale.y, m[2][2]*vScale.z, m[2][3],
		m[3][0]*vScale.x, m[3][1]*vScale.y, m[3][2]*vScale.z, 1.0f
		);
}

VMatrix VMatrix::NormalizeBasisVectors() const
{
	Vector vecs[3];
	VMatrix mRet;


	GetBasisVectors(vecs[0], vecs[1], vecs[2]);
	
	VectorNormalize( vecs[0] );
	VectorNormalize( vecs[1] );
	VectorNormalize( vecs[2] );

	mRet.SetBasisVectors(vecs[0], vecs[1], vecs[2]);
	
	// Set everything but basis vectors to identity.
	mRet.m[3][0] = mRet.m[3][1] = mRet.m[3][2] = 0.0f;
	mRet.m[3][3] = 1.0f;

	return mRet;
}

VMatrix VMatrix::Transpose() const
{
	return VMatrix(
		m[0][0], m[1][0], m[2][0], m[3][0],
		m[0][1], m[1][1], m[2][1], m[3][1],
		m[0][2], m[1][2], m[2][2], m[3][2],
		m[0][3], m[1][3], m[2][3], m[3][3]);
}

// Transpose upper-left 3x3.
VMatrix VMatrix::Transpose3x3() const
{
	return VMatrix(
		m[0][0], m[1][0], m[2][0], m[0][3],
		m[0][1], m[1][1], m[2][1], m[1][3],
		m[0][2], m[1][2], m[2][2], m[2][3],
		m[3][0], m[3][1], m[3][2], m[3][3]);
}

#endif // VECTOR_NO_SLOW_OPERATIONS


bool VMatrix::IsRotationMatrix() const
{
	Vector &v1 = (Vector&)m[0][0];
	Vector &v2 = (Vector&)m[1][0];
	Vector &v3 = (Vector&)m[2][0];

	return 
		FloatMakePositive( 1 - v1.Length() ) < 0.01f && 
		FloatMakePositive( 1 - v2.Length() ) < 0.01f && 
		FloatMakePositive( 1 - v3.Length() ) < 0.01f && 
		FloatMakePositive( v1.Dot(v2) ) < 0.01f &&
		FloatMakePositive( v1.Dot(v3) ) < 0.01f &&
		FloatMakePositive( v2.Dot(v3) ) < 0.01f;
}

static void SetupMatrixAnglesInternal( vec_t m[4][4], const QAngle & vAngles )
{
	float		sr, sp, sy, cr, cp, cy;

	SinCos( DEG2RAD( vAngles[YAW] ), &sy, &cy );
	SinCos( DEG2RAD( vAngles[PITCH] ), &sp, &cp );
	SinCos( DEG2RAD( vAngles[ROLL] ), &sr, &cr );

	// matrix = (YAW * PITCH) * ROLL
	m[0][0] = cp*cy;
	m[1][0] = cp*sy;
	m[2][0] = -sp;
	m[0][1] = sr*sp*cy+cr*-sy;
	m[1][1] = sr*sp*sy+cr*cy;
	m[2][1] = sr*cp;
	m[0][2] = (cr*sp*cy+-sr*-sy);
	m[1][2] = (cr*sp*sy+-sr*cy);
	m[2][2] = cr*cp;
	m[0][3] = 0.f;
	m[1][3] = 0.f;
	m[2][3] = 0.f;
}

void VMatrix::SetupMatrixOrgAngles( const Vector &origin, const QAngle &vAngles )
{
	SetupMatrixAnglesInternal( m, vAngles );
	
	// Add translation
	m[0][3] = origin.x;
	m[1][3] = origin.y;
	m[2][3] = origin.z;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
}


void	VMatrix::SetupMatrixAngles( const QAngle &vAngles )
{
	SetupMatrixAnglesInternal( m, vAngles );

	// Zero everything else
	m[0][3] = 0.0f;
	m[1][3] = 0.0f;
	m[2][3] = 0.0f;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
}


//-----------------------------------------------------------------------------
// Sets matrix to identity
//-----------------------------------------------------------------------------
void MatrixSetIdentity( VMatrix &dst )
{
	dst[0][0] = 1.0f; dst[0][1] = 0.0f; dst[0][2] = 0.0f; dst[0][3] = 0.0f;
	dst[1][0] = 0.0f; dst[1][1] = 1.0f; dst[1][2] = 0.0f; dst[1][3] = 0.0f;
	dst[2][0] = 0.0f; dst[2][1] = 0.0f; dst[2][2] = 1.0f; dst[2][3] = 0.0f;
	dst[3][0] = 0.0f; dst[3][1] = 0.0f; dst[3][2] = 0.0f; dst[3][3] = 1.0f;
}


//-----------------------------------------------------------------------------
// Setup a matrix from euler angles. 
//-----------------------------------------------------------------------------
void MatrixFromAngles( const QAngle& vAngles, VMatrix& dst )
{
	dst.SetupMatrixOrgAngles( vec3_origin, vAngles );
}


//-----------------------------------------------------------------------------
// Creates euler angles from a matrix 
//-----------------------------------------------------------------------------
void MatrixToAngles( const VMatrix& src, QAngle& vAngles )
{
	float forward[3];
	float left[3];
	float up[3];

	// Extract the basis vectors from the matrix. Since we only need the Z
	// component of the up vector, we don't get X and Y.
	forward[0] = src[0][0];
	forward[1] = src[1][0];
	forward[2] = src[2][0];
	left[0] = src[0][1];
	left[1] = src[1][1];
	left[2] = src[2][1];
	up[2] = src[2][2];

	float xyDist = sqrtf( forward[0] * forward[0] + forward[1] * forward[1] );
	
	// enough here to get angles?
	if ( xyDist > 0.001f )
	{
		// (yaw)	y = ATAN( forward.y, forward.x );		-- in our space, forward is the X axis
		vAngles[1] = RAD2DEG( atan2f( forward[1], forward[0] ) );

		// The engine does pitch inverted from this, but we always end up negating it in the DLL
		// UNDONE: Fix the engine to make it consistent
		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		vAngles[0] = RAD2DEG( atan2f( -forward[2], xyDist ) );

		// (roll)	z = ATAN( left.z, up.z );
		vAngles[2] = RAD2DEG( atan2f( left[2], up[2] ) );
	}
	else	// forward is mostly Z, gimbal lock-
	{
		// (yaw)	y = ATAN( -left.x, left.y );			-- forward is mostly z, so use right for yaw
		vAngles[1] = RAD2DEG( atan2f( -left[0], left[1] ) );

		// The engine does pitch inverted from this, but we always end up negating it in the DLL
		// UNDONE: Fix the engine to make it consistent
		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		vAngles[0] = RAD2DEG( atan2f( -forward[2], xyDist ) );

		// Assume no roll in this case as one degree of freedom has been lost (i.e. yaw == roll)
		vAngles[2] = 0;
	}
}


//-----------------------------------------------------------------------------
// Transpose
//-----------------------------------------------------------------------------
inline void Swap( float& a, float& b )
{
	float tmp = a;
	a = b;
	b = tmp;
}

void MatrixTranspose( const VMatrix& src, VMatrix& dst )
{
	if (&src == &dst)
	{
		Swap( dst[0][1], dst[1][0] );
		Swap( dst[0][2], dst[2][0] );
		Swap( dst[0][3], dst[3][0] );
		Swap( dst[1][2], dst[2][1] );
		Swap( dst[1][3], dst[3][1] );
		Swap( dst[2][3], dst[3][2] );
	}
	else
	{
		dst[0][0] = src[0][0]; dst[0][1] = src[1][0]; dst[0][2] = src[2][0]; dst[0][3] = src[3][0];
		dst[1][0] = src[0][1]; dst[1][1] = src[1][1]; dst[1][2] = src[2][1]; dst[1][3] = src[3][1];
		dst[2][0] = src[0][2]; dst[2][1] = src[1][2]; dst[2][2] = src[2][2]; dst[2][3] = src[3][2];
		dst[3][0] = src[0][3]; dst[3][1] = src[1][3]; dst[3][2] = src[2][3]; dst[3][3] = src[3][3];
	}
}


//-----------------------------------------------------------------------------
// Matrix copy
//-----------------------------------------------------------------------------

void MatrixCopy( const VMatrix& src, VMatrix& dst )
{
	if (&src != &dst)
	{
		memcpy( dst.m, src.m, 16 * sizeof(float) );
	}
}

//-----------------------------------------------------------------------------
// Matrix multiply
//-----------------------------------------------------------------------------
typedef float VMatrixRaw_t[4];

void MatrixMultiply( const VMatrix& src1, const VMatrix& src2, VMatrix& dst )
{
	// Make sure it works if src1 == dst or src2 == dst
	VMatrix tmp1, tmp2;
	const VMatrixRaw_t* s1 = (&src1 == &dst) ? tmp1.m : src1.m;
	const VMatrixRaw_t* s2 = (&src2 == &dst) ? tmp2.m : src2.m;

	if (&src1 == &dst)
	{
		MatrixCopy( src1, tmp1 );
	}
	if (&src2 == &dst)
	{
		MatrixCopy( src2, tmp2 );
	}

	dst[0][0] = s1[0][0] * s2[0][0] + s1[0][1] * s2[1][0] + s1[0][2] * s2[2][0] + s1[0][3] * s2[3][0];
	dst[0][1] = s1[0][0] * s2[0][1] + s1[0][1] * s2[1][1] + s1[0][2] * s2[2][1] + s1[0][3] * s2[3][1];
	dst[0][2] = s1[0][0] * s2[0][2] + s1[0][1] * s2[1][2] + s1[0][2] * s2[2][2] + s1[0][3] * s2[3][2];
	dst[0][3] = s1[0][0] * s2[0][3] + s1[0][1] * s2[1][3] + s1[0][2] * s2[2][3] + s1[0][3] * s2[3][3];

	dst[1][0] = s1[1][0] * s2[0][0] + s1[1][1] * s2[1][0] + s1[1][2] * s2[2][0] + s1[1][3] * s2[3][0];
	dst[1][1] = s1[1][0] * s2[0][1] + s1[1][1] * s2[1][1] + s1[1][2] * s2[2][1] + s1[1][3] * s2[3][1];
	dst[1][2] = s1[1][0] * s2[0][2] + s1[1][1] * s2[1][2] + s1[1][2] * s2[2][2] + s1[1][3] * s2[3][2];
	dst[1][3] = s1[1][0] * s2[0][3] + s1[1][1] * s2[1][3] + s1[1][2] * s2[2][3] + s1[1][3] * s2[3][3];

	dst[2][0] = s1[2][0] * s2[0][0] + s1[2][1] * s2[1][0] + s1[2][2] * s2[2][0] + s1[2][3] * s2[3][0];
	dst[2][1] = s1[2][0] * s2[0][1] + s1[2][1] * s2[1][1] + s1[2][2] * s2[2][1] + s1[2][3] * s2[3][1];
	dst[2][2] = s1[2][0] * s2[0][2] + s1[2][1] * s2[1][2] + s1[2][2] * s2[2][2] + s1[2][3] * s2[3][2];
	dst[2][3] = s1[2][0] * s2[0][3] + s1[2][1] * s2[1][3] + s1[2][2] * s2[2][3] + s1[2][3] * s2[3][3];

	dst[3][0] = s1[3][0] * s2[0][0] + s1[3][1] * s2[1][0] + s1[3][2] * s2[2][0] + s1[3][3] * s2[3][0];
	dst[3][1] = s1[3][0] * s2[0][1] + s1[3][1] * s2[1][1] + s1[3][2] * s2[2][1] + s1[3][3] * s2[3][1];
	dst[3][2] = s1[3][0] * s2[0][2] + s1[3][1] * s2[1][2] + s1[3][2] * s2[2][2] + s1[3][3] * s2[3][2];
	dst[3][3] = s1[3][0] * s2[0][3] + s1[3][1] * s2[1][3] + s1[3][2] * s2[2][3] + s1[3][3] * s2[3][3];
}

//-----------------------------------------------------------------------------
// Matrix/vector multiply
//-----------------------------------------------------------------------------

void Vector4DMultiply( const VMatrix& src1, Vector4D const& src2, Vector4D& dst )
{
	// Make sure it works if src2 == dst
	Vector4D tmp;
	Vector4D const&v = (&src2 == &dst) ? tmp : src2;

	if (&src2 == &dst)
	{
		Vector4DCopy( src2, tmp );
	}

	dst[0] = src1[0][0] * v[0] + src1[0][1] * v[1] + src1[0][2] * v[2] + src1[0][3] * v[3];
	dst[1] = src1[1][0] * v[0] + src1[1][1] * v[1] + src1[1][2] * v[2] + src1[1][3] * v[3];
	dst[2] = src1[2][0] * v[0] + src1[2][1] * v[1] + src1[2][2] * v[2] + src1[2][3] * v[3];
	dst[3] = src1[3][0] * v[0] + src1[3][1] * v[1] + src1[3][2] * v[2] + src1[3][3] * v[3];
}

//-----------------------------------------------------------------------------
// Matrix/vector multiply
//-----------------------------------------------------------------------------

void Vector4DMultiplyPosition( const VMatrix& src1, Vector const& src2, Vector4D& dst )
{
	// Make sure it works if src2 == dst
	Vector tmp;
	Vector const&v = ( &src2 == &dst.AsVector3D() ) ? static_cast<const Vector&>(tmp) : src2;

	if (&src2 == &dst.AsVector3D())
	{
		VectorCopy( src2, tmp );
	}

	dst[0] = src1[0][0] * v[0] + src1[0][1] * v[1] + src1[0][2] * v[2] + src1[0][3];
	dst[1] = src1[1][0] * v[0] + src1[1][1] * v[1] + src1[1][2] * v[2] + src1[1][3];
	dst[2] = src1[2][0] * v[0] + src1[2][1] * v[1] + src1[2][2] * v[2] + src1[2][3];
	dst[3] = src1[3][0] * v[0] + src1[3][1] * v[1] + src1[3][2] * v[2] + src1[3][3];
}



//-----------------------------------------------------------------------------
// Matrix/vector multiply
//-----------------------------------------------------------------------------

void Vector3DMultiply( const VMatrix &src1, const Vector &src2, Vector &dst )
{
	// Make sure it works if src2 == dst
	Vector tmp;
	const Vector &v = (&src2 == &dst) ?  static_cast<const Vector&>(tmp) : src2;

	if( &src2 == &dst )
	{
		VectorCopy( src2, tmp );
	}

	dst[0] = src1[0][0] * v[0] + src1[0][1] * v[1] + src1[0][2] * v[2];
	dst[1] = src1[1][0] * v[0] + src1[1][1] * v[1] + src1[1][2] * v[2];
	dst[2] = src1[2][0] * v[0] + src1[2][1] * v[1] + src1[2][2] * v[2];
}


//-----------------------------------------------------------------------------
// Vector3DMultiplyPositionProjective treats src2 as if it's a point 
// and does the perspective divide at the end
//-----------------------------------------------------------------------------
void Vector3DMultiplyPositionProjective( const VMatrix& src1, const Vector &src2, Vector& dst )
{
	// Make sure it works if src2 == dst
	Vector tmp;
	const Vector &v = (&src2 == &dst) ? static_cast<const Vector&>(tmp): src2;
	if( &src2 == &dst )
	{
		VectorCopy( src2, tmp );
	}

	float w = src1[3][0] * v[0] + src1[3][1] * v[1] + src1[3][2] * v[2] + src1[3][3];
	if ( w != 0.0f ) 
	{
		w = 1.0f / w;
	}

	dst[0] = src1[0][0] * v[0] + src1[0][1] * v[1] + src1[0][2] * v[2] + src1[0][3];
	dst[1] = src1[1][0] * v[0] + src1[1][1] * v[1] + src1[1][2] * v[2] + src1[1][3];
	dst[2] = src1[2][0] * v[0] + src1[2][1] * v[1] + src1[2][2] * v[2] + src1[2][3];
	dst *= w;
}


//-----------------------------------------------------------------------------
// Vector3DMultiplyProjective treats src2 as if it's a direction 
// and does the perspective divide at the end
//-----------------------------------------------------------------------------
void Vector3DMultiplyProjective( const VMatrix& src1, const Vector &src2, Vector& dst )
{
	// Make sure it works if src2 == dst
	Vector tmp;
	const Vector &v = (&src2 == &dst) ? static_cast<const Vector&>(tmp) : src2;
	if( &src2 == &dst )
	{
		VectorCopy( src2, tmp );
	}

	float w;
	dst[0] = src1[0][0] * v[0] + src1[0][1] * v[1] + src1[0][2] * v[2];
	dst[1] = src1[1][0] * v[0] + src1[1][1] * v[1] + src1[1][2] * v[2];
	dst[2] = src1[2][0] * v[0] + src1[2][1] * v[1] + src1[2][2] * v[2];
	w = src1[3][0] * v[0] + src1[3][1] * v[1] + src1[3][2] * v[2];
	if (w != 0.0f)
	{
		dst /= w;
	}
	else
	{
		dst = vec3_origin;
	}
}


//-----------------------------------------------------------------------------
// Multiplies the vector by the transpose of the matrix
//-----------------------------------------------------------------------------
void Vector4DMultiplyTranspose( const VMatrix& src1, Vector4D const& src2, Vector4D& dst )
{
	// Make sure it works if src2 == dst
	bool srcEqualsDst = (&src2 == &dst);

	Vector4D tmp;
	Vector4D const&v = srcEqualsDst ? tmp : src2;

	if (srcEqualsDst)
	{
		Vector4DCopy( src2, tmp );
	}

	dst[0] = src1[0][0] * v[0] + src1[1][0] * v[1] + src1[2][0] * v[2] + src1[3][0] * v[3];
	dst[1] = src1[0][1] * v[0] + src1[1][1] * v[1] + src1[2][1] * v[2] + src1[3][1] * v[3];
	dst[2] = src1[0][2] * v[0] + src1[1][2] * v[1] + src1[2][2] * v[2] + src1[3][2] * v[3];
	dst[3] = src1[0][3] * v[0] + src1[1][3] * v[1] + src1[2][3] * v[2] + src1[3][3] * v[3];
}

//-----------------------------------------------------------------------------
// Multiplies the vector by the transpose of the matrix
//-----------------------------------------------------------------------------
void Vector3DMultiplyTranspose( const VMatrix& src1, const Vector& src2, Vector& dst )
{
	// Make sure it works if src2 == dst
	bool srcEqualsDst = (&src2 == &dst);

	Vector tmp;
	const Vector&v = srcEqualsDst ? static_cast<const Vector&>(tmp) : src2;

	if (srcEqualsDst)
	{
		VectorCopy( src2, tmp );
	}

	dst[0] = src1[0][0] * v[0] + src1[1][0] * v[1] + src1[2][0] * v[2];
	dst[1] = src1[0][1] * v[0] + src1[1][1] * v[1] + src1[2][1] * v[2];
	dst[2] = src1[0][2] * v[0] + src1[1][2] * v[1] + src1[2][2] * v[2];
}


//-----------------------------------------------------------------------------
// Transform a plane
//-----------------------------------------------------------------------------
void MatrixTransformPlane( const VMatrix &src, const cplane_t &inPlane, cplane_t &outPlane )
{
	// What we want to do is the following:
	// 1) transform the normal into the new space.
	// 2) Determine a point on the old plane given by plane dist * plane normal
	// 3) Transform that point into the new space
	// 4) Plane dist = DotProduct( new normal, new point )

	// An optimized version, which works if the plane is orthogonal.
	// 1) Transform the normal into the new space
	// 2) Realize that transforming the old plane point into the new space
	// is given by [ d * n'x + Tx, d * n'y + Ty, d * n'z + Tz ]
	// where d = old plane dist, n' = transformed normal, Tn = translational component of transform
	// 3) Compute the new plane dist using the dot product of the normal result of #2

	// For a correct result, this should be an inverse-transpose matrix
	// but that only matters if there are nonuniform scale or skew factors in this matrix.
	Vector vTrans;
	Vector3DMultiply( src, inPlane.normal, outPlane.normal );
	outPlane.dist = inPlane.dist * DotProduct( outPlane.normal, outPlane.normal );
	outPlane.dist += DotProduct( outPlane.normal, src.GetTranslation(vTrans) );
}


#ifndef VECTOR_NO_SLOW_OPERATIONS

VPlane VMatrix::operator*(const VPlane &thePlane) const
{
	VPlane ret;
	TransformPlane( thePlane, ret );
	return ret;
}

#endif


//-----------------------------------------------------------------------------
// Builds a rotation matrix that rotates one direction vector into another
//-----------------------------------------------------------------------------
void MatrixBuildTranslation( VMatrix& dst, float x, float y, float z )
{
	MatrixSetIdentity( dst );
	dst[0][3] = x;
	dst[1][3] = y;
	dst[2][3] = z;
}

void MatrixBuildTranslation( VMatrix& dst, const Vector &translation )
{
	MatrixSetIdentity( dst );
	dst[0][3] = translation[0];
	dst[1][3] = translation[1];
	dst[2][3] = translation[2];
}


//-----------------------------------------------------------------------------
// Purpose: Builds the matrix for a counterclockwise rotation about an arbitrary axis.
//
//		   | ax2 + (1 - ax2)cosQ		axay(1 - cosQ) - azsinQ		azax(1 - cosQ) + aysinQ |
// Ra(Q) = | axay(1 - cosQ) + azsinQ	ay2 + (1 - ay2)cosQ			ayaz(1 - cosQ) - axsinQ |
//		   | azax(1 - cosQ) - aysinQ	ayaz(1 - cosQ) + axsinQ		az2 + (1 - az2)cosQ     |
//          
// Input  : mat - 
//			vAxisOrRot - 
//			angle - 
//-----------------------------------------------------------------------------
void MatrixBuildRotationAboutAxis( VMatrix &dst, const Vector &vAxisOfRot, float angleDegrees )
{
	MatrixBuildRotationAboutAxis( vAxisOfRot, angleDegrees, const_cast< matrix3x4_t &> ( dst.As3x4() ) );
	dst[3][0] = 0;
	dst[3][1] = 0;
	dst[3][2] = 0;
	dst[3][3] = 1;
}


//-----------------------------------------------------------------------------
// Builds a rotation matrix that rotates one direction vector into another
//-----------------------------------------------------------------------------
void MatrixBuildRotation( VMatrix &dst, const Vector& initialDirection, const Vector& finalDirection )
{
	float angle = DotProduct( initialDirection, finalDirection );
	Assert( IsFinite(angle) );
	
	Vector axis;

	// No rotation required
	if (angle - 1.0 > -1e-3)
	{
		// parallel case
		MatrixSetIdentity(dst);
		return;
	}
	else if (angle + 1.0 < 1e-3)
	{
		// antiparallel case, pick any axis in the plane
		// perpendicular to the final direction. Choose the direction (x,y,z)
		// which has the minimum component of the final direction, use that
		// as an initial guess, then subtract out the component which is 
		// parallel to the final direction
		int idx = 0;
		if (FloatMakePositive(finalDirection[1]) < FloatMakePositive(finalDirection[idx]))
			idx = 1;
		if (FloatMakePositive(finalDirection[2]) < FloatMakePositive(finalDirection[idx]))
			idx = 2;

		axis.Init( 0, 0, 0 );
		axis[idx] = 1.0f;
		VectorMA( axis, -DotProduct( axis, finalDirection ), finalDirection, axis );
		VectorNormalize(axis);
		angle = 180.0f;
	}
	else
	{
		CrossProduct( initialDirection, finalDirection, axis );
		VectorNormalize( axis );
		angle = acos(angle) * 180 / M_PI;
	}

	MatrixBuildRotationAboutAxis( dst, axis, angle );

#ifdef _DEBUG
	Vector test;
	Vector3DMultiply( dst, initialDirection, test );
	test -= finalDirection;
	Assert( test.LengthSqr() < 1e-3 );
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void MatrixBuildRotateZ( VMatrix &dst, float angleDegrees )
{
	float radians = angleDegrees * ( M_PI / 180.0f );

	float fSin = ( float )sin( radians );
	float fCos = ( float )cos( radians );

	dst[0][0] = fCos; dst[0][1] = -fSin; dst[0][2] = 0.0f; dst[0][3] = 0.0f;
	dst[1][0] = fSin; dst[1][1] =  fCos; dst[1][2] = 0.0f; dst[1][3] = 0.0f;
	dst[2][0] = 0.0f; dst[2][1] =  0.0f; dst[2][2] = 1.0f; dst[2][3] = 0.0f;
	dst[3][0] = 0.0f; dst[3][1] =  0.0f; dst[3][2] = 0.0f; dst[3][3] = 1.0f;
}

// Builds a scale matrix
void MatrixBuildScale( VMatrix &dst, float x, float y, float z )
{
	dst[0][0] = x;		dst[0][1] = 0.0f;	dst[0][2] = 0.0f;	dst[0][3] = 0.0f;
	dst[1][0] = 0.0f;	dst[1][1] = y;		dst[1][2] = 0.0f;	dst[1][3] = 0.0f;
	dst[2][0] = 0.0f;	dst[2][1] = 0.0f;	dst[2][2] = z;		dst[2][3] = 0.0f;
	dst[3][0] = 0.0f;	dst[3][1] = 0.0f;	dst[3][2] = 0.0f;	dst[3][3] = 1.0f;
}

void MatrixBuildScale( VMatrix &dst, const Vector& scale )
{
	MatrixBuildScale( dst, scale.x, scale.y, scale.z );
}

void MatrixBuildPerspective( VMatrix &dst, float fovX, float fovY, float zNear, float zFar )
{
	// FIXME: collapse all of this into one matrix after we figure out what all should be in here.
	float width = 2 * zNear * tan( fovX * ( M_PI/180.0f ) * 0.5f );
	float height = 2 * zNear * tan( fovY * ( M_PI/180.0f ) * 0.5f );

	memset( dst.Base(), 0, sizeof( dst ) );
	dst[0][0]  = 2.0F * zNear / width;
	dst[1][1]  = 2.0F * zNear / height;
	dst[2][2] = -zFar / ( zNear - zFar );
	dst[3][2] = 1.0f;
	dst[2][3] = zNear * zFar / ( zNear - zFar );

	// negate X and Y so that X points right, and Y points up.
	VMatrix negateXY;
	negateXY.Identity();
	negateXY[0][0] = -1.0f;
	negateXY[1][1] = -1.0f;
	MatrixMultiply( negateXY, dst, dst );
	
	VMatrix addW;
	addW.Identity();
	addW[0][3] = 1.0f;
	addW[1][3] = 1.0f;
	addW[2][3] = 0.0f;
	MatrixMultiply( addW, dst, dst );
	
	VMatrix scaleHalf;
	scaleHalf.Identity();
	scaleHalf[0][0] = 0.5f;
	scaleHalf[1][1] = 0.5f;
	MatrixMultiply( scaleHalf, dst, dst );
}

static inline void CalculateAABBForNormalizedFrustum_Helper( float x, float y, float z, const VMatrix &volumeToWorld, Vector &mins, Vector &maxs )
{
	Vector volumeSpacePos( x, y, z );

	// Make sure it's been clipped
	Assert( volumeSpacePos[0] >= -1e-3f );
	Assert( volumeSpacePos[0] - 1.0f <= 1e-3f );
	Assert( volumeSpacePos[1] >= -1e-3f );
	Assert( volumeSpacePos[1] - 1.0f <= 1e-3f );
	Assert( volumeSpacePos[2] >= -1e-3f );
	Assert( volumeSpacePos[2] - 1.0f <= 1e-3f );

	Vector worldPos;
	Vector3DMultiplyPositionProjective( volumeToWorld, volumeSpacePos, worldPos );
	AddPointToBounds( worldPos, mins, maxs );
}

//-----------------------------------------------------------------------------
// Given an inverse projection matrix, take the extremes of the space in transformed into world space and
// get a bounding box.
//-----------------------------------------------------------------------------
void CalculateAABBFromProjectionMatrixInverse( const VMatrix &volumeToWorld, Vector *pMins, Vector *pMaxs )
{
	// FIXME: Could maybe do better than the compile with all of these multiplies by 0 and 1.
	ClearBounds( *pMins, *pMaxs );
	CalculateAABBForNormalizedFrustum_Helper( 0, 0, 0, volumeToWorld, *pMins, *pMaxs );
	CalculateAABBForNormalizedFrustum_Helper( 0, 0, 1, volumeToWorld, *pMins, *pMaxs );
	CalculateAABBForNormalizedFrustum_Helper( 0, 1, 0, volumeToWorld, *pMins, *pMaxs );
	CalculateAABBForNormalizedFrustum_Helper( 0, 1, 1, volumeToWorld, *pMins, *pMaxs );
	CalculateAABBForNormalizedFrustum_Helper( 1, 0, 0, volumeToWorld, *pMins, *pMaxs );
	CalculateAABBForNormalizedFrustum_Helper( 1, 0, 1, volumeToWorld, *pMins, *pMaxs );
	CalculateAABBForNormalizedFrustum_Helper( 1, 1, 0, volumeToWorld, *pMins, *pMaxs );
	CalculateAABBForNormalizedFrustum_Helper( 1, 1, 1, volumeToWorld, *pMins, *pMaxs );
}

void CalculateAABBFromProjectionMatrix( const VMatrix &worldToVolume, Vector *pMins, Vector *pMaxs )
{
	VMatrix volumeToWorld;
	MatrixInverseGeneral( worldToVolume, volumeToWorld );
	CalculateAABBFromProjectionMatrixInverse( volumeToWorld, pMins, pMaxs );
}

//-----------------------------------------------------------------------------
// Given an inverse projection matrix, take the extremes of the space in transformed into world space and
// get a bounding sphere.
//-----------------------------------------------------------------------------
void CalculateSphereFromProjectionMatrixInverse( const VMatrix &volumeToWorld, Vector *pCenter, float *pflRadius )
{
	// FIXME: Could maybe do better than the compile with all of these multiplies by 0 and 1.

	// Need 3 points: the endpoint of the line through the center of the near + far planes,
	// and one point on the far plane. From that, we can derive a point somewhere on the center	line
	// which would produce the smallest bounding sphere.
	Vector vecCenterNear, vecCenterFar, vecNearEdge, vecFarEdge;
	Vector3DMultiplyPositionProjective( volumeToWorld, Vector( 0.5f, 0.5f, 0.0f ), vecCenterNear );
	Vector3DMultiplyPositionProjective( volumeToWorld, Vector( 0.5f, 0.5f, 1.0f ), vecCenterFar );
	Vector3DMultiplyPositionProjective( volumeToWorld, Vector( 0.0f, 0.0f, 0.0f ), vecNearEdge );
	Vector3DMultiplyPositionProjective( volumeToWorld, Vector( 0.0f, 0.0f, 1.0f ), vecFarEdge );

	// Let the distance between the near + far center points = l
	// Let the distance between the near center point + near edge point = h1
	// Let the distance between the far center point + far edge point = h2
	// Let the distance along the center line from the near point to the sphere center point = x
	// Then let the distance between the sphere center point + near edge point == 
	//	the distance between the sphere center point + far edge point == r == radius of sphere
	// Then h1^2 + x^2 == r^2 == (l-x)^2 + h2^2
	// h1^x + x^2 = l^2 - 2 * l * x + x^2 + h2^2
	// 2 * l * x = l^2 + h2^2 - h1^2
	// x = (l^2 + h2^2 - h1^2) / (2 * l)
	// r = sqrt( hl^1 + x^2 )
	Vector vecDelta;
	VectorSubtract( vecCenterFar, vecCenterNear, vecDelta );
	float l = vecDelta.Length();
	float h1Sqr = vecCenterNear.DistToSqr( vecNearEdge );
	float h2Sqr = vecCenterFar.DistToSqr( vecFarEdge );
	float x = (l*l + h2Sqr - h1Sqr) / (2.0f * l);
	VectorMA( vecCenterNear, (x / l), vecDelta, *pCenter );
	*pflRadius = sqrt( h1Sqr + x*x );
}

//-----------------------------------------------------------------------------
// Given a projection matrix, take the extremes of the space in transformed into world space and
// get a bounding sphere.
//-----------------------------------------------------------------------------
void CalculateSphereFromProjectionMatrix( const VMatrix &worldToVolume, Vector *pCenter, float *pflRadius )
{
	VMatrix volumeToWorld;
	MatrixInverseGeneral( worldToVolume, volumeToWorld );
	CalculateSphereFromProjectionMatrixInverse( volumeToWorld, pCenter, pflRadius );
}


static inline void FrustumPlanesFromMatrixHelper( const VMatrix &shadowToWorld, const Vector &p1, const Vector &p2, const Vector &p3, 
												 Vector &normal, float &dist )
{
	Vector world1, world2, world3;
	Vector3DMultiplyPositionProjective( shadowToWorld, p1, world1 );
	Vector3DMultiplyPositionProjective( shadowToWorld, p2, world2 );
	Vector3DMultiplyPositionProjective( shadowToWorld, p3, world3 );

	Vector v1, v2;
	VectorSubtract( world2, world1, v1 );
	VectorSubtract( world3, world1, v2 );

	CrossProduct( v1, v2, normal );
	VectorNormalize( normal );
	dist = DotProduct( normal, world1 );	
}

void FrustumPlanesFromMatrix( const VMatrix &clipToWorld, Frustum_t &frustum )
{
	Vector normal;
	float dist;

	FrustumPlanesFromMatrixHelper( clipToWorld, 
		Vector( 0.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f ), Vector( 0.0f, 1.0f, 0.0f ), normal, dist );
	frustum.SetPlane( FRUSTUM_NEARZ, PLANE_ANYZ, normal, dist );

	FrustumPlanesFromMatrixHelper( clipToWorld, 
		Vector( 0.0f, 0.0f, 1.0f ), Vector( 0.0f, 1.0f, 1.0f ), Vector( 1.0f, 0.0f, 1.0f ), normal, dist );
	frustum.SetPlane( FRUSTUM_FARZ, PLANE_ANYZ, normal, dist );

	FrustumPlanesFromMatrixHelper( clipToWorld, 
		Vector( 1.0f, 0.0f, 0.0f ), Vector( 1.0f, 1.0f, 1.0f ), Vector( 1.0f, 1.0f, 0.0f ), normal, dist );
	frustum.SetPlane( FRUSTUM_RIGHT, PLANE_ANYZ, normal, dist );

	FrustumPlanesFromMatrixHelper( clipToWorld, 
		Vector( 0.0f, 0.0f, 0.0f ), Vector( 0.0f, 1.0f, 1.0f ), Vector( 0.0f, 0.0f, 1.0f ), normal, dist );
	frustum.SetPlane( FRUSTUM_LEFT, PLANE_ANYZ, normal, dist );

	FrustumPlanesFromMatrixHelper( clipToWorld, 
		Vector( 1.0f, 1.0f, 0.0f ), Vector( 1.0f, 1.0f, 1.0f ), Vector( 0.0f, 1.0f, 1.0f ), normal, dist );
	frustum.SetPlane( FRUSTUM_TOP, PLANE_ANYZ, normal, dist );

	FrustumPlanesFromMatrixHelper( clipToWorld, 
		Vector( 1.0f, 0.0f, 0.0f ), Vector( 0.0f, 0.0f, 1.0f ), Vector( 1.0f, 0.0f, 1.0f ), normal, dist );
	frustum.SetPlane( FRUSTUM_BOTTOM, PLANE_ANYZ, normal, dist );
}

void MatrixBuildOrtho( VMatrix& dst, double left, double top, double right, double bottom, double zNear, double zFar )
{
	// FIXME: This is being used incorrectly! Should read:
	// D3DXMatrixOrthoOffCenterRH( &matrix, left, right, bottom, top, zNear, zFar );
	// Which is certainly why we need these extra -1 scales in y. Bleah

	// NOTE: The camera can be imagined as the following diagram:
	//		/z
	//	   /
	//	  /____ x	Z is going into the screen
	//	  |
	//	  |
	//	  |y
	//
	// (0,0,z) represents the upper-left corner of the screen.
	// Our projection transform needs to transform from this space to a LH coordinate
	// system that looks thusly:
	// 
	//	y|  /z
	//	 | /
	//	 |/____ x	Z is going into the screen
	//
	// Where x,y lies between -1 and 1, and z lies from 0 to 1
	// This is because the viewport transformation from projection space to pixels
	// introduces a -1 scale in the y coordinates
	//		D3DXMatrixOrthoOffCenterRH( &matrix, left, right, top, bottom, zNear, zFar );

	dst.Init(	 2.0f / ( right - left ),						0.0f,						0.0f, ( left + right ) / ( left - right ),
				0.0f,	 2.0f / ( bottom - top ),						0.0f, ( bottom + top ) / ( top - bottom ),
				0.0f,						0.0f,	 1.0f / ( zNear - zFar ),			 zNear / ( zNear - zFar ),
				0.0f,						0.0f,						0.0f,								1.0f );
}

void MatrixBuildPerspectiveZRange( VMatrix& dst, double flZNear, double flZFar )
{
	dst.m[2][0] = 0.0f;
	dst.m[2][1] = 0.0f;
	dst.m[2][2] = flZFar / ( flZNear - flZFar );
	dst.m[2][3] = flZNear * flZFar / ( flZNear - flZFar );
}

void MatrixBuildPerspectiveX( VMatrix& dst, double flFovX, double flAspect, double flZNear, double flZFar )
{
	float flWidthScale = 1.0f / tanf( flFovX * M_PI / 360.0f );
	float flHeightScale = flAspect * flWidthScale;
	dst.Init(   flWidthScale,				0.0f,							0.0f,										0.0f,
				0.0f,						flHeightScale,					0.0f,										0.0f,
				0.0f,						0.0f,							0.0f,										0.0f,
				0.0f,						0.0f,						   -1.0f,										0.0f );

	MatrixBuildPerspectiveZRange ( dst, flZNear, flZFar );
}

void MatrixBuildPerspectiveOffCenterX( VMatrix& dst, double flFovX, double flAspect, double flZNear, double flZFar, double bottom, double top, double left, double right )
{
	float flWidth = tanf( flFovX * M_PI / 360.0f );
	float flHeight = flWidth / flAspect;

	// bottom, top, left, right are 0..1 so convert to -<val>/2..<val>/2
	float flLeft   = -(flWidth/2.0f)  * (1.0f - left)   + left   * (flWidth/2.0f);
	float flRight  = -(flWidth/2.0f)  * (1.0f - right)  + right  * (flWidth/2.0f);
	float flBottom = -(flHeight/2.0f) * (1.0f - bottom) + bottom * (flHeight/2.0f);
	float flTop    = -(flHeight/2.0f) * (1.0f - top)    + top    * (flHeight/2.0f);

	dst.Init(   1.0f / (flRight-flLeft),        0.0f,                           (flLeft+flRight)/(flRight-flLeft),  0.0f,
				0.0f,                           1.0f /(flTop-flBottom),         (flTop+flBottom)/(flTop-flBottom),  0.0f,
				0.0f,                           0.0f,							0.0f,								0.0f,
				0.0f,                           0.0f,                           -1.0f,								0.0f );

	MatrixBuildPerspectiveZRange ( dst, flZNear, flZFar );
}
#endif // !_STATIC_LINKED || _SHARED_LIB

