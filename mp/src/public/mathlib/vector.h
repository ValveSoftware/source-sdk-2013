//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef VECTOR_H
#define VECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include <math.h>
#include <float.h>

// For vec_t, put this somewhere else?
#include "tier0/basetypes.h"

// For rand(). We really need a library!
#include <stdlib.h>

#ifndef _X360
// For MMX intrinsics
#include <xmmintrin.h>
#endif

#include "tier0/dbg.h"
#include "tier0/threadtools.h"
#include "mathlib/vector2d.h"
#include "mathlib/math_pfns.h"
#include "minmax.h"

// Uncomment this to add extra Asserts to check for NANs, uninitialized vecs, etc.
//#define VECTOR_PARANOIA	1

// Uncomment this to make sure we don't do anything slow with our vectors
//#define VECTOR_NO_SLOW_OPERATIONS 1


// Used to make certain code easier to read.
#define X_INDEX	0
#define Y_INDEX	1
#define Z_INDEX	2


#ifdef VECTOR_PARANOIA
#define CHECK_VALID( _v)	Assert( (_v).IsValid() )
#else
#ifdef GNUC
#define CHECK_VALID( _v)
#else
#define CHECK_VALID( _v)	0
#endif
#endif

#define VecToString(v)	(static_cast<const char *>(CFmtStr("(%f, %f, %f)", (v).x, (v).y, (v).z))) // ** Note: this generates a temporary, don't hold reference!

class VectorByValue;

//=========================================================
// 3D Vector
//=========================================================
class Vector					
{
public:
	// Members
	vec_t x, y, z;

	// Construction/destruction:
	Vector(void); 
	Vector(vec_t X, vec_t Y, vec_t Z);
	explicit Vector(vec_t XYZ); ///< broadcast initialize

	// Initialization
	void Init(vec_t ix=0.0f, vec_t iy=0.0f, vec_t iz=0.0f);
	 // TODO (Ilya): Should there be an init that takes a single float for consistency?

	// Got any nasty NAN's?
	bool IsValid() const;
	void Invalidate();

	// array access...
	vec_t operator[](int i) const;
	vec_t& operator[](int i);

	// Base address...
	vec_t* Base();
	vec_t const* Base() const;

	// Cast to Vector2D...
	Vector2D& AsVector2D();
	const Vector2D& AsVector2D() const;

	// Initialization methods
	void Random( vec_t minVal, vec_t maxVal );
	inline void Zero(); ///< zero out a vector

	// equality
	bool operator==(const Vector& v) const;
	bool operator!=(const Vector& v) const;	

	// arithmetic operations
	FORCEINLINE Vector&	operator+=(const Vector &v);			
	FORCEINLINE Vector&	operator-=(const Vector &v);		
	FORCEINLINE Vector&	operator*=(const Vector &v);			
	FORCEINLINE Vector&	operator*=(float s);
	FORCEINLINE Vector&	operator/=(const Vector &v);		
	FORCEINLINE Vector&	operator/=(float s);	
	FORCEINLINE Vector&	operator+=(float fl) ; ///< broadcast add
	FORCEINLINE Vector&	operator-=(float fl) ; ///< broadcast sub			

// negate the vector components
	void	Negate(); 

	// Get the vector's magnitude.
	inline vec_t	Length() const;

	// Get the vector's magnitude squared.
	FORCEINLINE vec_t LengthSqr(void) const
	{ 
		CHECK_VALID(*this);
		return (x*x + y*y + z*z);		
	}

	// return true if this vector is (0,0,0) within tolerance
	bool IsZero( float tolerance = 0.01f ) const
	{
		return (x > -tolerance && x < tolerance &&
				y > -tolerance && y < tolerance &&
				z > -tolerance && z < tolerance);
	}

	vec_t	NormalizeInPlace();
	Vector	Normalized() const;
	bool	IsLengthGreaterThan( float val ) const;
	bool	IsLengthLessThan( float val ) const;

	// check if a vector is within the box defined by two other vectors
	FORCEINLINE bool WithinAABox( Vector const &boxmin, Vector const &boxmax);
 
	// Get the distance from this vector to the other one.
	vec_t	DistTo(const Vector &vOther) const;

	// Get the distance from this vector to the other one squared.
	// NJS: note, VC wasn't inlining it correctly in several deeply nested inlines due to being an 'out of line' inline.  
	// may be able to tidy this up after switching to VC7
	FORCEINLINE vec_t DistToSqr(const Vector &vOther) const
	{
		Vector delta;

		delta.x = x - vOther.x;
		delta.y = y - vOther.y;
		delta.z = z - vOther.z;

		return delta.LengthSqr();
	}

	// Copy
	void	CopyToArray(float* rgfl) const;	

	// Multiply, add, and assign to this (ie: *this = a + b * scalar). This
	// is about 12% faster than the actual vector equation (because it's done per-component
	// rather than per-vector).
	void	MulAdd(const Vector& a, const Vector& b, float scalar);	

	// Dot product.
	vec_t	Dot(const Vector& vOther) const;			

	// assignment
	Vector& operator=(const Vector &vOther);

	// 2d
	vec_t	Length2D(void) const;					
	vec_t	Length2DSqr(void) const;					

	operator VectorByValue &()				{ return *((VectorByValue *)(this)); }
	operator const VectorByValue &() const	{ return *((const VectorByValue *)(this)); }

#ifndef VECTOR_NO_SLOW_OPERATIONS
	// copy constructors
//	Vector(const Vector &vOther);

	// arithmetic operations
	Vector	operator-(void) const;
				
	Vector	operator+(const Vector& v) const;	
	Vector	operator-(const Vector& v) const;	
	Vector	operator*(const Vector& v) const;	
	Vector	operator/(const Vector& v) const;	
	Vector	operator*(float fl) const;
	Vector	operator/(float fl) const;			
	
	// Cross product between two vectors.
	Vector	Cross(const Vector &vOther) const;		

	// Returns a vector with the min or max in X, Y, and Z.
	Vector	Min(const Vector &vOther) const;
	Vector	Max(const Vector &vOther) const;

#else

private:
	// No copy constructors allowed if we're in optimal mode
	Vector(const Vector& vOther);
#endif
};

FORCEINLINE void NetworkVarConstruct( Vector &v ) { v.Zero(); }


#define USE_M64S ( ( !defined( _X360 ) ) )



//=========================================================
// 4D Short Vector (aligned on 8-byte boundary)
//=========================================================
class ALIGN8 ShortVector
{
public:

	short x, y, z, w;

	// Initialization
	void Init(short ix = 0, short iy = 0, short iz = 0, short iw = 0 );


#if USE_M64S
	__m64 &AsM64() { return *(__m64*)&x; }
	const __m64 &AsM64() const { return *(const __m64*)&x; } 
#endif

	// Setter
	void Set( const ShortVector& vOther );
	void Set( const short ix, const short iy, const short iz, const short iw );

	// array access...
	short operator[](int i) const;
	short& operator[](int i);

	// Base address...
	short* Base();
	short const* Base() const;

	// equality
	bool operator==(const ShortVector& v) const;
	bool operator!=(const ShortVector& v) const;	

	// Arithmetic operations
	FORCEINLINE ShortVector& operator+=(const ShortVector &v);			
	FORCEINLINE ShortVector& operator-=(const ShortVector &v);		
	FORCEINLINE ShortVector& operator*=(const ShortVector &v);			
	FORCEINLINE ShortVector& operator*=(float s);
	FORCEINLINE ShortVector& operator/=(const ShortVector &v);		
	FORCEINLINE ShortVector& operator/=(float s);					
	FORCEINLINE ShortVector operator*(float fl) const;

private:

	// No copy constructors allowed if we're in optimal mode
//	ShortVector(ShortVector const& vOther);

	// No assignment operators either...
//	ShortVector& operator=( ShortVector const& src );

} ALIGN8_POST;






//=========================================================
// 4D Integer Vector
//=========================================================
class IntVector4D
{
public:

	int x, y, z, w;

	// Initialization
	void Init(int ix = 0, int iy = 0, int iz = 0, int iw = 0 );

#if USE_M64S
	__m64 &AsM64() { return *(__m64*)&x; }
	const __m64 &AsM64() const { return *(const __m64*)&x; } 
#endif

	// Setter
	void Set( const IntVector4D& vOther );
	void Set( const int ix, const int iy, const int iz, const int iw );

	// array access...
	int operator[](int i) const;
	int& operator[](int i);

	// Base address...
	int* Base();
	int const* Base() const;

	// equality
	bool operator==(const IntVector4D& v) const;
	bool operator!=(const IntVector4D& v) const;	

	// Arithmetic operations
	FORCEINLINE IntVector4D& operator+=(const IntVector4D &v);			
	FORCEINLINE IntVector4D& operator-=(const IntVector4D &v);		
	FORCEINLINE IntVector4D& operator*=(const IntVector4D &v);			
	FORCEINLINE IntVector4D& operator*=(float s);
	FORCEINLINE IntVector4D& operator/=(const IntVector4D &v);		
	FORCEINLINE IntVector4D& operator/=(float s);					
	FORCEINLINE IntVector4D operator*(float fl) const;

private:

	// No copy constructors allowed if we're in optimal mode
	//	IntVector4D(IntVector4D const& vOther);

	// No assignment operators either...
	//	IntVector4D& operator=( IntVector4D const& src );

};



//-----------------------------------------------------------------------------
// Allows us to specifically pass the vector by value when we need to
//-----------------------------------------------------------------------------
class VectorByValue : public Vector
{
public:
	// Construction/destruction:
	VectorByValue(void) : Vector() {} 
	VectorByValue(vec_t X, vec_t Y, vec_t Z) : Vector( X, Y, Z ) {}
	VectorByValue(const VectorByValue& vOther) { *this = vOther; }
};


//-----------------------------------------------------------------------------
// Utility to simplify table construction. No constructor means can use
// traditional C-style initialization
//-----------------------------------------------------------------------------
class TableVector
{
public:
	vec_t x, y, z;

	operator Vector &()				{ return *((Vector *)(this)); }
	operator const Vector &() const	{ return *((const Vector *)(this)); }

	// array access...
	inline vec_t& operator[](int i)
	{
		Assert( (i >= 0) && (i < 3) );
		return ((vec_t*)this)[i];
	}

	inline vec_t operator[](int i) const
	{
		Assert( (i >= 0) && (i < 3) );
		return ((vec_t*)this)[i];
	}
};


//-----------------------------------------------------------------------------
// Here's where we add all those lovely SSE optimized routines
//-----------------------------------------------------------------------------

class ALIGN16 VectorAligned : public Vector
{
public:
	inline VectorAligned(void) {};
	inline VectorAligned(vec_t X, vec_t Y, vec_t Z) 
	{
		Init(X,Y,Z);
	}

#ifdef VECTOR_NO_SLOW_OPERATIONS

private:
	// No copy constructors allowed if we're in optimal mode
	VectorAligned(const VectorAligned& vOther);
	VectorAligned(const Vector &vOther);

#else
public:
	explicit VectorAligned(const Vector &vOther) 
	{
		Init(vOther.x, vOther.y, vOther.z);
	}
	
	VectorAligned& operator=(const Vector &vOther)	
	{
		Init(vOther.x, vOther.y, vOther.z);
		return *this;
	}
	
#endif
	float w;	// this space is used anyway
} ALIGN16_POST;

//-----------------------------------------------------------------------------
// Vector related operations
//-----------------------------------------------------------------------------

// Vector clear
FORCEINLINE void VectorClear( Vector& a );

// Copy
FORCEINLINE void VectorCopy( const Vector& src, Vector& dst );

// Vector arithmetic
FORCEINLINE void VectorAdd( const Vector& a, const Vector& b, Vector& result );
FORCEINLINE void VectorSubtract( const Vector& a, const Vector& b, Vector& result );
FORCEINLINE void VectorMultiply( const Vector& a, vec_t b, Vector& result );
FORCEINLINE void VectorMultiply( const Vector& a, const Vector& b, Vector& result );
FORCEINLINE void VectorDivide( const Vector& a, vec_t b, Vector& result );
FORCEINLINE void VectorDivide( const Vector& a, const Vector& b, Vector& result );
inline void VectorScale ( const Vector& in, vec_t scale, Vector& result );
// Don't mark this as inline in its function declaration. That's only necessary on its
// definition, and 'inline' here leads to gcc warnings.
void VectorMA( const Vector& start, float scale, const Vector& direction, Vector& dest );

// Vector equality with tolerance
bool VectorsAreEqual( const Vector& src1, const Vector& src2, float tolerance = 0.0f );

#define VectorExpand(v) (v).x, (v).y, (v).z


// Normalization
// FIXME: Can't use quite yet
//vec_t VectorNormalize( Vector& v );

// Length
inline vec_t VectorLength( const Vector& v );

// Dot Product
FORCEINLINE vec_t DotProduct(const Vector& a, const Vector& b);

// Cross product
void CrossProduct(const Vector& a, const Vector& b, Vector& result );

// Store the min or max of each of x, y, and z into the result.
void VectorMin( const Vector &a, const Vector &b, Vector &result );
void VectorMax( const Vector &a, const Vector &b, Vector &result );

// Linearly interpolate between two vectors
void VectorLerp(const Vector& src1, const Vector& src2, vec_t t, Vector& dest );
Vector VectorLerp(const Vector& src1, const Vector& src2, vec_t t );

FORCEINLINE Vector ReplicateToVector( float x )
{
	return Vector( x, x, x );
}

// check if a point is in the field of a view of an object. supports up to 180 degree fov.
FORCEINLINE bool PointWithinViewAngle( Vector const &vecSrcPosition, 
									   Vector const &vecTargetPosition, 
									   Vector const &vecLookDirection, float flCosHalfFOV )
{
	Vector vecDelta = vecTargetPosition - vecSrcPosition;
	float cosDiff = DotProduct( vecLookDirection, vecDelta );

	if ( cosDiff < 0 ) 
		return false;

	float flLen2 = vecDelta.LengthSqr();

	// a/sqrt(b) > c  == a^2 > b * c ^2
	return ( cosDiff * cosDiff > flLen2 * flCosHalfFOV * flCosHalfFOV );
	
}


#ifndef VECTOR_NO_SLOW_OPERATIONS

// Cross product
Vector CrossProduct( const Vector& a, const Vector& b );

// Random vector creation
Vector RandomVector( vec_t minVal, vec_t maxVal );

#endif

float RandomVectorInUnitSphere( Vector *pVector );
float RandomVectorInUnitCircle( Vector2D *pVector );


//-----------------------------------------------------------------------------
//
// Inlined Vector methods
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// constructors
//-----------------------------------------------------------------------------
inline Vector::Vector(void)									
{ 
#ifdef _DEBUG
#ifdef VECTOR_PARANOIA
	// Initialize to NAN to catch errors
	x = y = z = VEC_T_NAN;
#endif
#endif
}

inline Vector::Vector(vec_t X, vec_t Y, vec_t Z)						
{ 
	x = X; y = Y; z = Z;
	CHECK_VALID(*this);
}

inline Vector::Vector(vec_t XYZ)						
{ 
	x = y = z = XYZ;
	CHECK_VALID(*this);
}

//inline Vector::Vector(const float *pFloat)					
//{
//	Assert( pFloat );
//	x = pFloat[0]; y = pFloat[1]; z = pFloat[2];	
//	CHECK_VALID(*this);
//} 

#if 0
//-----------------------------------------------------------------------------
// copy constructor
//-----------------------------------------------------------------------------

inline Vector::Vector(const Vector &vOther)					
{ 
	CHECK_VALID(vOther);
	x = vOther.x; y = vOther.y; z = vOther.z;
}
#endif

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------

inline void Vector::Init( vec_t ix, vec_t iy, vec_t iz )    
{ 
	x = ix; y = iy; z = iz;
	CHECK_VALID(*this);
}

inline void Vector::Random( vec_t minVal, vec_t maxVal )
{
	x = minVal + ((float)rand() / VALVE_RAND_MAX) * (maxVal - minVal);
	y = minVal + ((float)rand() / VALVE_RAND_MAX) * (maxVal - minVal);
	z = minVal + ((float)rand() / VALVE_RAND_MAX) * (maxVal - minVal);
	CHECK_VALID(*this);
}

// This should really be a single opcode on the PowerPC (move r0 onto the vec reg)
inline void Vector::Zero()
{
	x = y = z = 0.0f;
}

inline void VectorClear( Vector& a )
{
	a.x = a.y = a.z = 0.0f;
}

//-----------------------------------------------------------------------------
// assignment
//-----------------------------------------------------------------------------

inline Vector& Vector::operator=(const Vector &vOther)	
{
	CHECK_VALID(vOther);
	x=vOther.x; y=vOther.y; z=vOther.z; 
	return *this; 
}


//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------
inline vec_t& Vector::operator[](int i)
{
	Assert( (i >= 0) && (i < 3) );
	return ((vec_t*)this)[i];
}

inline vec_t Vector::operator[](int i) const
{
	Assert( (i >= 0) && (i < 3) );
	return ((vec_t*)this)[i];
}


//-----------------------------------------------------------------------------
// Base address...
//-----------------------------------------------------------------------------
inline vec_t* Vector::Base()
{
	return (vec_t*)this;
}

inline vec_t const* Vector::Base() const
{
	return (vec_t const*)this;
}

//-----------------------------------------------------------------------------
// Cast to Vector2D...
//-----------------------------------------------------------------------------

inline Vector2D& Vector::AsVector2D()
{
	return *(Vector2D*)this;
}

inline const Vector2D& Vector::AsVector2D() const
{
	return *(const Vector2D*)this;
}

//-----------------------------------------------------------------------------
// IsValid?
//-----------------------------------------------------------------------------

inline bool Vector::IsValid() const
{
	return IsFinite(x) && IsFinite(y) && IsFinite(z);
}

//-----------------------------------------------------------------------------
// Invalidate
//-----------------------------------------------------------------------------

inline void Vector::Invalidate()
{
//#ifdef _DEBUG
//#ifdef VECTOR_PARANOIA
	x = y = z = VEC_T_NAN;
//#endif
//#endif
}

//-----------------------------------------------------------------------------
// comparison
//-----------------------------------------------------------------------------

inline bool Vector::operator==( const Vector& src ) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x == x) && (src.y == y) && (src.z == z);
}

inline bool Vector::operator!=( const Vector& src ) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x != x) || (src.y != y) || (src.z != z);
}


//-----------------------------------------------------------------------------
// Copy
//-----------------------------------------------------------------------------

FORCEINLINE void VectorCopy( const Vector& src, Vector& dst )
{
	CHECK_VALID(src);
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}

inline void	Vector::CopyToArray(float* rgfl) const		
{ 
	Assert( rgfl );
	CHECK_VALID(*this);
	rgfl[0] = x, rgfl[1] = y, rgfl[2] = z; 
}

//-----------------------------------------------------------------------------
// standard math operations
//-----------------------------------------------------------------------------
// #pragma message("TODO: these should be SSE")

inline void Vector::Negate()
{ 
	CHECK_VALID(*this);
	x = -x; y = -y; z = -z; 
} 

FORCEINLINE  Vector& Vector::operator+=(const Vector& v)	
{ 
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x+=v.x; y+=v.y; z += v.z;	
	return *this;
}

FORCEINLINE  Vector& Vector::operator-=(const Vector& v)	
{ 
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x-=v.x; y-=v.y; z -= v.z;	
	return *this;
}

FORCEINLINE  Vector& Vector::operator*=(float fl)	
{
	x *= fl;
	y *= fl;
	z *= fl;
	CHECK_VALID(*this);
	return *this;
}

FORCEINLINE  Vector& Vector::operator*=(const Vector& v)	
{ 
	CHECK_VALID(v);
	x *= v.x;
	y *= v.y;
	z *= v.z;
	CHECK_VALID(*this);
	return *this;
}

// this ought to be an opcode.
FORCEINLINE Vector&	Vector::operator+=(float fl) 
{
	x += fl;
	y += fl;
	z += fl;
	CHECK_VALID(*this);
	return *this;
}

FORCEINLINE Vector&	Vector::operator-=(float fl) 
{
	x -= fl;
	y -= fl;
	z -= fl;
	CHECK_VALID(*this);
	return *this;
}



FORCEINLINE  Vector& Vector::operator/=(float fl)	
{
	Assert( fl != 0.0f );
	float oofl = 1.0f / fl;
	x *= oofl;
	y *= oofl;
	z *= oofl;
	CHECK_VALID(*this);
	return *this;
}

FORCEINLINE  Vector& Vector::operator/=(const Vector& v)	
{ 
	CHECK_VALID(v);
	Assert( v.x != 0.0f && v.y != 0.0f && v.z != 0.0f );
	x /= v.x;
	y /= v.y;
	z /= v.z;
	CHECK_VALID(*this);
	return *this;
}



//-----------------------------------------------------------------------------
//
// Inlined Short Vector methods
//
//-----------------------------------------------------------------------------


inline void ShortVector::Init( short ix, short iy, short iz, short iw )    
{ 
	x = ix; y = iy; z = iz; w = iw;
}

FORCEINLINE void ShortVector::Set( const ShortVector& vOther )
{
   x = vOther.x;
   y = vOther.y;
   z = vOther.z;
   w = vOther.w;
}

FORCEINLINE void ShortVector::Set( const short ix, const short iy, const short iz, const short iw )
{
   x = ix;
   y = iy;
   z = iz;
   w = iw;
}


//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------
inline short ShortVector::operator[](int i) const
{
	Assert( (i >= 0) && (i < 4) );
	return ((short*)this)[i];
}

inline short& ShortVector::operator[](int i)
{
	Assert( (i >= 0) && (i < 4) );
	return ((short*)this)[i];
}

//-----------------------------------------------------------------------------
// Base address...
//-----------------------------------------------------------------------------
inline short* ShortVector::Base()
{
	return (short*)this;
}

inline short const* ShortVector::Base() const
{
	return (short const*)this;
}


//-----------------------------------------------------------------------------
// comparison
//-----------------------------------------------------------------------------

inline bool ShortVector::operator==( const ShortVector& src ) const
{
	return (src.x == x) && (src.y == y) && (src.z == z) && (src.w == w);
}

inline bool ShortVector::operator!=( const ShortVector& src ) const
{
	return (src.x != x) || (src.y != y) || (src.z != z) || (src.w != w);
}



//-----------------------------------------------------------------------------
// standard math operations
//-----------------------------------------------------------------------------

FORCEINLINE  ShortVector& ShortVector::operator+=(const ShortVector& v)	
{ 
	x+=v.x; y+=v.y; z += v.z; w += v.w;
	return *this;
}

FORCEINLINE  ShortVector& ShortVector::operator-=(const ShortVector& v)	
{ 
	x-=v.x; y-=v.y; z -= v.z; w -= v.w;
	return *this;
}

FORCEINLINE  ShortVector& ShortVector::operator*=(float fl)	
{
	x *= fl;
	y *= fl;
	z *= fl;
	w *= fl;
	return *this;
}

FORCEINLINE  ShortVector& ShortVector::operator*=(const ShortVector& v)	
{ 
	x *= v.x;
	y *= v.y;
	z *= v.z;
	w *= v.w;
	return *this;
}

FORCEINLINE  ShortVector& ShortVector::operator/=(float fl)	
{
	Assert( fl != 0.0f );
	float oofl = 1.0f / fl;
	x *= oofl;
	y *= oofl;
	z *= oofl;
	w *= oofl;
	return *this;
}

FORCEINLINE  ShortVector& ShortVector::operator/=(const ShortVector& v)	
{ 
	Assert( v.x != 0 && v.y != 0 && v.z != 0 && v.w != 0 );
	x /= v.x;
	y /= v.y;
	z /= v.z;
	w /= v.w;
	return *this;
}

FORCEINLINE void ShortVectorMultiply( const ShortVector& src, float fl, ShortVector& res )
{
	Assert( IsFinite(fl) );
	res.x = src.x * fl;
	res.y = src.y * fl;
	res.z = src.z * fl;
	res.w = src.w * fl;
}

FORCEINLINE ShortVector ShortVector::operator*(float fl) const
{ 
	ShortVector res;
	ShortVectorMultiply( *this, fl, res );
	return res;	
}






//-----------------------------------------------------------------------------
//
// Inlined Integer Vector methods
//
//-----------------------------------------------------------------------------


inline void IntVector4D::Init( int ix, int iy, int iz, int iw )    
{ 
	x = ix; y = iy; z = iz; w = iw;
}

FORCEINLINE void IntVector4D::Set( const IntVector4D& vOther )
{
	x = vOther.x;
	y = vOther.y;
	z = vOther.z;
	w = vOther.w;
}

FORCEINLINE void IntVector4D::Set( const int ix, const int iy, const int iz, const int iw )
{
	x = ix;
	y = iy;
	z = iz;
	w = iw;
}


//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------
inline int IntVector4D::operator[](int i) const
{
	Assert( (i >= 0) && (i < 4) );
	return ((int*)this)[i];
}

inline int& IntVector4D::operator[](int i)
{
	Assert( (i >= 0) && (i < 4) );
	return ((int*)this)[i];
}

//-----------------------------------------------------------------------------
// Base address...
//-----------------------------------------------------------------------------
inline int* IntVector4D::Base()
{
	return (int*)this;
}

inline int const* IntVector4D::Base() const
{
	return (int const*)this;
}


//-----------------------------------------------------------------------------
// comparison
//-----------------------------------------------------------------------------

inline bool IntVector4D::operator==( const IntVector4D& src ) const
{
	return (src.x == x) && (src.y == y) && (src.z == z) && (src.w == w);
}

inline bool IntVector4D::operator!=( const IntVector4D& src ) const
{
	return (src.x != x) || (src.y != y) || (src.z != z) || (src.w != w);
}



//-----------------------------------------------------------------------------
// standard math operations
//-----------------------------------------------------------------------------

FORCEINLINE  IntVector4D& IntVector4D::operator+=(const IntVector4D& v)	
{ 
	x+=v.x; y+=v.y; z += v.z; w += v.w;
	return *this;
}

FORCEINLINE  IntVector4D& IntVector4D::operator-=(const IntVector4D& v)	
{ 
	x-=v.x; y-=v.y; z -= v.z; w -= v.w;
	return *this;
}

FORCEINLINE  IntVector4D& IntVector4D::operator*=(float fl)	
{
	x *= fl;
	y *= fl;
	z *= fl;
	w *= fl;
	return *this;
}

FORCEINLINE  IntVector4D& IntVector4D::operator*=(const IntVector4D& v)	
{ 
	x *= v.x;
	y *= v.y;
	z *= v.z;
	w *= v.w;
	return *this;
}

FORCEINLINE  IntVector4D& IntVector4D::operator/=(float fl)	
{
	Assert( fl != 0.0f );
	float oofl = 1.0f / fl;
	x *= oofl;
	y *= oofl;
	z *= oofl;
	w *= oofl;
	return *this;
}

FORCEINLINE  IntVector4D& IntVector4D::operator/=(const IntVector4D& v)	
{ 
	Assert( v.x != 0 && v.y != 0 && v.z != 0 && v.w != 0 );
	x /= v.x;
	y /= v.y;
	z /= v.z;
	w /= v.w;
	return *this;
}

FORCEINLINE void IntVector4DMultiply( const IntVector4D& src, float fl, IntVector4D& res )
{
	Assert( IsFinite(fl) );
	res.x = src.x * fl;
	res.y = src.y * fl;
	res.z = src.z * fl;
	res.w = src.w * fl;
}

FORCEINLINE IntVector4D IntVector4D::operator*(float fl) const
{ 
	IntVector4D res;
	IntVector4DMultiply( *this, fl, res );
	return res;	
}



// =======================


FORCEINLINE void VectorAdd( const Vector& a, const Vector& b, Vector& c )
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	c.x = a.x + b.x;
	c.y = a.y + b.y;
	c.z = a.z + b.z;
}

FORCEINLINE void VectorSubtract( const Vector& a, const Vector& b, Vector& c )
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	c.z = a.z - b.z;
}

FORCEINLINE void VectorMultiply( const Vector& a, vec_t b, Vector& c )
{
	CHECK_VALID(a);
	Assert( IsFinite(b) );
	c.x = a.x * b;
	c.y = a.y * b;
	c.z = a.z * b;
}

FORCEINLINE void VectorMultiply( const Vector& a, const Vector& b, Vector& c )
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	c.x = a.x * b.x;
	c.y = a.y * b.y;
	c.z = a.z * b.z;
}

// for backwards compatability
inline void VectorScale ( const Vector& in, vec_t scale, Vector& result )
{
	VectorMultiply( in, scale, result );
}


FORCEINLINE void VectorDivide( const Vector& a, vec_t b, Vector& c )
{
	CHECK_VALID(a);
	Assert( b != 0.0f );
	vec_t oob = 1.0f / b;
	c.x = a.x * oob;
	c.y = a.y * oob;
	c.z = a.z * oob;
}

FORCEINLINE void VectorDivide( const Vector& a, const Vector& b, Vector& c )
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	Assert( (b.x != 0.0f) && (b.y != 0.0f) && (b.z != 0.0f) );
	c.x = a.x / b.x;
	c.y = a.y / b.y;
	c.z = a.z / b.z;
}

// FIXME: Remove
// For backwards compatability
inline void	Vector::MulAdd(const Vector& a, const Vector& b, float scalar)
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	x = a.x + b.x * scalar;
	y = a.y + b.y * scalar;
	z = a.z + b.z * scalar;
}

inline void VectorLerp(const Vector& src1, const Vector& src2, vec_t t, Vector& dest )
{
	CHECK_VALID(src1);
	CHECK_VALID(src2);
	dest.x = src1.x + (src2.x - src1.x) * t;
	dest.y = src1.y + (src2.y - src1.y) * t;
	dest.z = src1.z + (src2.z - src1.z) * t;
}

inline Vector VectorLerp(const Vector& src1, const Vector& src2, vec_t t )
{
	Vector result;
	VectorLerp( src1, src2, t, result );
	return result;
}

//-----------------------------------------------------------------------------
// Temporary storage for vector results so const Vector& results can be returned
//-----------------------------------------------------------------------------
inline Vector &AllocTempVector()
{
	static Vector s_vecTemp[128];
	static CInterlockedInt s_nIndex;

	int nIndex;
	for (;;)
	{
		int nOldIndex = s_nIndex;
		nIndex = ( (nOldIndex + 0x10001) & 0x7F );

		if ( s_nIndex.AssignIf( nOldIndex, nIndex ) )
		{
			break;
		}
		ThreadPause();
	} 
	return s_vecTemp[nIndex];
}



//-----------------------------------------------------------------------------
// dot, cross
//-----------------------------------------------------------------------------
FORCEINLINE vec_t DotProduct(const Vector& a, const Vector& b) 
{ 
	CHECK_VALID(a);
	CHECK_VALID(b);
	return( a.x*b.x + a.y*b.y + a.z*b.z ); 
}

// for backwards compatability
inline vec_t Vector::Dot( const Vector& vOther ) const
{
	CHECK_VALID(vOther);
	return DotProduct( *this, vOther );
}

inline void CrossProduct(const Vector& a, const Vector& b, Vector& result )
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	Assert( &a != &result );
	Assert( &b != &result );
	result.x = a.y*b.z - a.z*b.y;
	result.y = a.z*b.x - a.x*b.z;
	result.z = a.x*b.y - a.y*b.x;
}

inline vec_t DotProductAbs( const Vector &v0, const Vector &v1 )
{
	CHECK_VALID(v0);
	CHECK_VALID(v1);
	return FloatMakePositive(v0.x*v1.x) + FloatMakePositive(v0.y*v1.y) + FloatMakePositive(v0.z*v1.z);
}

inline vec_t DotProductAbs( const Vector &v0, const float *v1 )
{
	return FloatMakePositive(v0.x * v1[0]) + FloatMakePositive(v0.y * v1[1]) + FloatMakePositive(v0.z * v1[2]);
}

//-----------------------------------------------------------------------------
// length
//-----------------------------------------------------------------------------

inline vec_t VectorLength( const Vector& v )
{
	CHECK_VALID(v);
	return (vec_t)FastSqrt(v.x*v.x + v.y*v.y + v.z*v.z);		
}


inline vec_t Vector::Length(void) const	
{
	CHECK_VALID(*this);
	return VectorLength( *this );
}


//-----------------------------------------------------------------------------
// Normalization
//-----------------------------------------------------------------------------

/*
// FIXME: Can't use until we're un-macroed in mathlib.h
inline vec_t VectorNormalize( Vector& v )
{
	Assert( v.IsValid() );
	vec_t l = v.Length();
	if (l != 0.0f)
	{
		v /= l;
	}
	else
	{
		// FIXME: 
		// Just copying the existing implemenation; shouldn't res.z == 0?
		v.x = v.y = 0.0f; v.z = 1.0f;
	}
	return l;
}
*/


// check a point against a box
bool Vector::WithinAABox( Vector const &boxmin, Vector const &boxmax)
{
	return ( 
		( x >= boxmin.x ) && ( x <= boxmax.x) &&
		( y >= boxmin.y ) && ( y <= boxmax.y) &&
		( z >= boxmin.z ) && ( z <= boxmax.z)
		);
}

//-----------------------------------------------------------------------------
// Get the distance from this vector to the other one 
//-----------------------------------------------------------------------------
inline vec_t Vector::DistTo(const Vector &vOther) const
{
	Vector delta;
	VectorSubtract( *this, vOther, delta );
	return delta.Length();
}


//-----------------------------------------------------------------------------
// Vector equality with tolerance
//-----------------------------------------------------------------------------
inline bool VectorsAreEqual( const Vector& src1, const Vector& src2, float tolerance )
{
	if (FloatMakePositive(src1.x - src2.x) > tolerance)
		return false;
	if (FloatMakePositive(src1.y - src2.y) > tolerance)
		return false;
	return (FloatMakePositive(src1.z - src2.z) <= tolerance);
}


//-----------------------------------------------------------------------------
// Computes the closest point to vecTarget no farther than flMaxDist from vecStart
//-----------------------------------------------------------------------------
inline void ComputeClosestPoint( const Vector& vecStart, float flMaxDist, const Vector& vecTarget, Vector *pResult )
{
	Vector vecDelta;
	VectorSubtract( vecTarget, vecStart, vecDelta );
	float flDistSqr = vecDelta.LengthSqr();
	if ( flDistSqr <= flMaxDist * flMaxDist )
	{
		*pResult = vecTarget;
	}
	else
	{
		vecDelta /= FastSqrt( flDistSqr );
		VectorMA( vecStart, flMaxDist, vecDelta, *pResult );
	}
}


//-----------------------------------------------------------------------------
// Takes the absolute value of a vector
//-----------------------------------------------------------------------------
inline void VectorAbs( const Vector& src, Vector& dst )
{
	dst.x = FloatMakePositive(src.x);
	dst.y = FloatMakePositive(src.y);
	dst.z = FloatMakePositive(src.z);
}


//-----------------------------------------------------------------------------
//
// Slow methods
//
//-----------------------------------------------------------------------------

#ifndef VECTOR_NO_SLOW_OPERATIONS

//-----------------------------------------------------------------------------
// Returns a vector with the min or max in X, Y, and Z.
//-----------------------------------------------------------------------------
inline Vector Vector::Min(const Vector &vOther) const
{
	return Vector(x < vOther.x ? x : vOther.x, 
		y < vOther.y ? y : vOther.y, 
		z < vOther.z ? z : vOther.z);
}

inline Vector Vector::Max(const Vector &vOther) const
{
	return Vector(x > vOther.x ? x : vOther.x, 
		y > vOther.y ? y : vOther.y, 
		z > vOther.z ? z : vOther.z);
}


//-----------------------------------------------------------------------------
// arithmetic operations
//-----------------------------------------------------------------------------

inline Vector Vector::operator-(void) const
{ 
	return Vector(-x,-y,-z);				
}

inline Vector Vector::operator+(const Vector& v) const	
{ 
	Vector res;
	VectorAdd( *this, v, res );
	return res;	
}

inline Vector Vector::operator-(const Vector& v) const	
{ 
	Vector res;
	VectorSubtract( *this, v, res );
	return res;	
}

inline Vector Vector::operator*(float fl) const	
{ 
	Vector res;
	VectorMultiply( *this, fl, res );
	return res;	
}

inline Vector Vector::operator*(const Vector& v) const	
{ 
	Vector res;
	VectorMultiply( *this, v, res );
	return res;	
}

inline Vector Vector::operator/(float fl) const	
{ 
	Vector res;
	VectorDivide( *this, fl, res );
	return res;	
}

inline Vector Vector::operator/(const Vector& v) const	
{ 
	Vector res;
	VectorDivide( *this, v, res );
	return res;	
}

inline Vector operator*(float fl, const Vector& v)	
{ 
	return v * fl; 
}

//-----------------------------------------------------------------------------
// cross product
//-----------------------------------------------------------------------------

inline Vector Vector::Cross(const Vector& vOther) const
{ 
	Vector res;
	CrossProduct( *this, vOther, res );
	return res;
}

//-----------------------------------------------------------------------------
// 2D
//-----------------------------------------------------------------------------

inline vec_t Vector::Length2D(void) const
{ 
	return (vec_t)FastSqrt(x*x + y*y); 
}

inline vec_t Vector::Length2DSqr(void) const
{ 
	return (x*x + y*y); 
}

inline Vector CrossProduct(const Vector& a, const Vector& b) 
{ 
	return Vector( a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x ); 
}

inline void VectorMin( const Vector &a, const Vector &b, Vector &result )
{
	result.x = fpmin(a.x, b.x);
	result.y = fpmin(a.y, b.y);
	result.z = fpmin(a.z, b.z);
}

inline void VectorMax( const Vector &a, const Vector &b, Vector &result )
{
	result.x = fpmax(a.x, b.x);
	result.y = fpmax(a.y, b.y);
	result.z = fpmax(a.z, b.z);
}

inline float ComputeVolume( const Vector &vecMins, const Vector &vecMaxs )
{
	Vector vecDelta;
	VectorSubtract( vecMaxs, vecMins, vecDelta );
	return DotProduct( vecDelta, vecDelta );
}

// Get a random vector.
inline Vector RandomVector( float minVal, float maxVal )
{
	Vector random;
	random.Random( minVal, maxVal );
	return random;
}

#endif //slow

//-----------------------------------------------------------------------------
// Helper debugging stuff....
//-----------------------------------------------------------------------------

inline bool operator==( float const* f, const Vector& v )
{
	// AIIIEEEE!!!!
	Assert(0);
	return false;
}

inline bool operator==( const Vector& v, float const* f )
{
	// AIIIEEEE!!!!
	Assert(0);
	return false;
}

inline bool operator!=( float const* f, const Vector& v )
{
	// AIIIEEEE!!!!
	Assert(0);
	return false;
}

inline bool operator!=( const Vector& v, float const* f )
{
	// AIIIEEEE!!!!
	Assert(0);
	return false;
}


//-----------------------------------------------------------------------------
// AngularImpulse
//-----------------------------------------------------------------------------
// AngularImpulse are exponetial maps (an axis scaled by a "twist" angle in degrees)
typedef Vector AngularImpulse;

#ifndef VECTOR_NO_SLOW_OPERATIONS

inline AngularImpulse RandomAngularImpulse( float minVal, float maxVal )
{
	AngularImpulse	angImp;
	angImp.Random( minVal, maxVal );
	return angImp;
}

#endif


//-----------------------------------------------------------------------------
// Quaternion
//-----------------------------------------------------------------------------

class RadianEuler;

class Quaternion				// same data-layout as engine's vec4_t,
{								//		which is a vec_t[4]
public:
	inline Quaternion(void)	{ 
	
	// Initialize to NAN to catch errors
#ifdef _DEBUG
#ifdef VECTOR_PARANOIA
		x = y = z = w = VEC_T_NAN;
#endif
#endif
	}
	inline Quaternion(vec_t ix, vec_t iy, vec_t iz, vec_t iw) : x(ix), y(iy), z(iz), w(iw) { }
	inline Quaternion(RadianEuler const &angle);	// evil auto type promotion!!!

	inline void Init(vec_t ix=0.0f, vec_t iy=0.0f, vec_t iz=0.0f, vec_t iw=0.0f)	{ x = ix; y = iy; z = iz; w = iw; }

	bool IsValid() const;
	void Invalidate();

	bool operator==( const Quaternion &src ) const;
	bool operator!=( const Quaternion &src ) const;

	vec_t* Base() { return (vec_t*)this; }
	const vec_t* Base() const { return (vec_t*)this; }

	// array access...
	vec_t operator[](int i) const;
	vec_t& operator[](int i);

	vec_t x, y, z, w;
};


//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------
inline vec_t& Quaternion::operator[](int i)
{
	Assert( (i >= 0) && (i < 4) );
	return ((vec_t*)this)[i];
}

inline vec_t Quaternion::operator[](int i) const
{
	Assert( (i >= 0) && (i < 4) );
	return ((vec_t*)this)[i];
}


//-----------------------------------------------------------------------------
// Equality test
//-----------------------------------------------------------------------------
inline bool Quaternion::operator==( const Quaternion &src ) const
{
	return ( x == src.x ) && ( y == src.y ) && ( z == src.z ) && ( w == src.w );
}

inline bool Quaternion::operator!=( const Quaternion &src ) const
{
	return !operator==( src );
}


//-----------------------------------------------------------------------------
// Quaternion equality with tolerance
//-----------------------------------------------------------------------------
inline bool QuaternionsAreEqual( const Quaternion& src1, const Quaternion& src2, float tolerance )
{
	if (FloatMakePositive(src1.x - src2.x) > tolerance)
		return false;
	if (FloatMakePositive(src1.y - src2.y) > tolerance)
		return false;
	if (FloatMakePositive(src1.z - src2.z) > tolerance)
		return false;
	return (FloatMakePositive(src1.w - src2.w) <= tolerance);
}


//-----------------------------------------------------------------------------
// Here's where we add all those lovely SSE optimized routines
//-----------------------------------------------------------------------------
class ALIGN16 QuaternionAligned : public Quaternion
{
public:
	inline QuaternionAligned(void) {};
	inline QuaternionAligned(vec_t X, vec_t Y, vec_t Z, vec_t W) 
	{
		Init(X,Y,Z,W);
	}

#ifdef VECTOR_NO_SLOW_OPERATIONS

private:
	// No copy constructors allowed if we're in optimal mode
	QuaternionAligned(const QuaternionAligned& vOther);
	QuaternionAligned(const Quaternion &vOther);

#else
public:
	explicit QuaternionAligned(const Quaternion &vOther) 
	{
		Init(vOther.x, vOther.y, vOther.z, vOther.w);
	}

	QuaternionAligned& operator=(const Quaternion &vOther)	
	{
		Init(vOther.x, vOther.y, vOther.z, vOther.w);
		return *this;
	}

#endif
} ALIGN16_POST;


//-----------------------------------------------------------------------------
// Radian Euler angle aligned to axis (NOT ROLL/PITCH/YAW)
//-----------------------------------------------------------------------------
class QAngle;
class RadianEuler
{
public:
	inline RadianEuler(void)							{ }
	inline RadianEuler(vec_t X, vec_t Y, vec_t Z)		{ x = X; y = Y; z = Z; }
	inline RadianEuler(Quaternion const &q);	// evil auto type promotion!!!
	inline RadianEuler(QAngle const &angles);	// evil auto type promotion!!!

	// Initialization
	inline void Init(vec_t ix=0.0f, vec_t iy=0.0f, vec_t iz=0.0f)	{ x = ix; y = iy; z = iz; }

	//	conversion to qangle
	QAngle ToQAngle( void ) const;
	bool IsValid() const;
	void Invalidate();

	// array access...
	vec_t operator[](int i) const;
	vec_t& operator[](int i);

	vec_t x, y, z;
};


extern void AngleQuaternion( RadianEuler const &angles, Quaternion &qt );
extern void QuaternionAngles( Quaternion const &q, RadianEuler &angles );

FORCEINLINE void NetworkVarConstruct( Quaternion &q ) { q.x = q.y = q.z = q.w = 0.0f; }

inline Quaternion::Quaternion(RadianEuler const &angle)
{
	AngleQuaternion( angle, *this );
}

inline bool Quaternion::IsValid() const
{
	return IsFinite(x) && IsFinite(y) && IsFinite(z) && IsFinite(w);
}

inline void Quaternion::Invalidate()
{
//#ifdef _DEBUG
//#ifdef VECTOR_PARANOIA
	x = y = z = w = VEC_T_NAN;
//#endif
//#endif
}

inline RadianEuler::RadianEuler(Quaternion const &q)
{
	QuaternionAngles( q, *this );
}

inline void VectorCopy( RadianEuler const& src, RadianEuler &dst )
{
	CHECK_VALID(src);
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}

inline void VectorScale( RadianEuler const& src, float b, RadianEuler &dst )
{
	CHECK_VALID(src);
	Assert( IsFinite(b) );
	dst.x = src.x * b;
	dst.y = src.y * b;
	dst.z = src.z * b;
}

inline bool RadianEuler::IsValid() const
{
	return IsFinite(x) && IsFinite(y) && IsFinite(z);
}

inline void RadianEuler::Invalidate()
{
//#ifdef _DEBUG
//#ifdef VECTOR_PARANOIA
	x = y = z = VEC_T_NAN;
//#endif
//#endif
}


//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------
inline vec_t& RadianEuler::operator[](int i)
{
	Assert( (i >= 0) && (i < 3) );
	return ((vec_t*)this)[i];
}

inline vec_t RadianEuler::operator[](int i) const
{
	Assert( (i >= 0) && (i < 3) );
	return ((vec_t*)this)[i];
}


//-----------------------------------------------------------------------------
// Degree Euler QAngle pitch, yaw, roll
//-----------------------------------------------------------------------------
class QAngleByValue;

class QAngle					
{
public:
	// Members
	vec_t x, y, z;

	// Construction/destruction
	QAngle(void);
	QAngle(vec_t X, vec_t Y, vec_t Z);
//	QAngle(RadianEuler const &angles);	// evil auto type promotion!!!

	// Allow pass-by-value
	operator QAngleByValue &()				{ return *((QAngleByValue *)(this)); }
	operator const QAngleByValue &() const	{ return *((const QAngleByValue *)(this)); }

	// Initialization
	void Init(vec_t ix=0.0f, vec_t iy=0.0f, vec_t iz=0.0f);
	void Random( vec_t minVal, vec_t maxVal );

	// Got any nasty NAN's?
	bool IsValid() const;
	void Invalidate();

	// array access...
	vec_t operator[](int i) const;
	vec_t& operator[](int i);

	// Base address...
	vec_t* Base();
	vec_t const* Base() const;
	
	// equality
	bool operator==(const QAngle& v) const;
	bool operator!=(const QAngle& v) const;	

	// arithmetic operations
	QAngle&	operator+=(const QAngle &v);
	QAngle&	operator-=(const QAngle &v);
	QAngle&	operator*=(float s);
	QAngle&	operator/=(float s);

	// Get the vector's magnitude.
	vec_t	Length() const;
	vec_t	LengthSqr() const;

	// negate the QAngle components
	//void	Negate(); 

	// No assignment operators either...
	QAngle& operator=( const QAngle& src );

#ifndef VECTOR_NO_SLOW_OPERATIONS
	// copy constructors

	// arithmetic operations
	QAngle	operator-(void) const;
	
	QAngle	operator+(const QAngle& v) const;
	QAngle	operator-(const QAngle& v) const;
	QAngle	operator*(float fl) const;
	QAngle	operator/(float fl) const;
#else

private:
	// No copy constructors allowed if we're in optimal mode
	QAngle(const QAngle& vOther);

#endif
};

FORCEINLINE void NetworkVarConstruct( QAngle &q ) { q.x = q.y = q.z = 0.0f; }

//-----------------------------------------------------------------------------
// Allows us to specifically pass the vector by value when we need to
//-----------------------------------------------------------------------------
class QAngleByValue : public QAngle
{
public:
	// Construction/destruction:
	QAngleByValue(void) : QAngle() {} 
	QAngleByValue(vec_t X, vec_t Y, vec_t Z) : QAngle( X, Y, Z ) {}
	QAngleByValue(const QAngleByValue& vOther) { *this = vOther; }
};


inline void VectorAdd( const QAngle& a, const QAngle& b, QAngle& result )
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
}

inline void VectorMA( const QAngle &start, float scale, const QAngle &direction, QAngle &dest )
{
	CHECK_VALID(start);
	CHECK_VALID(direction);
	dest.x = start.x + scale * direction.x;
	dest.y = start.y + scale * direction.y;
	dest.z = start.z + scale * direction.z;
}


//-----------------------------------------------------------------------------
// constructors
//-----------------------------------------------------------------------------
inline QAngle::QAngle(void)									
{ 
#ifdef _DEBUG
#ifdef VECTOR_PARANOIA
	// Initialize to NAN to catch errors
	x = y = z = VEC_T_NAN;
#endif
#endif
}

inline QAngle::QAngle(vec_t X, vec_t Y, vec_t Z)						
{ 
	x = X; y = Y; z = Z;
	CHECK_VALID(*this);
}


//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------
inline void QAngle::Init( vec_t ix, vec_t iy, vec_t iz )    
{ 
	x = ix; y = iy; z = iz;
	CHECK_VALID(*this);
}

inline void QAngle::Random( vec_t minVal, vec_t maxVal )
{
	x = minVal + ((float)rand() / VALVE_RAND_MAX) * (maxVal - minVal);
	y = minVal + ((float)rand() / VALVE_RAND_MAX) * (maxVal - minVal);
	z = minVal + ((float)rand() / VALVE_RAND_MAX) * (maxVal - minVal);
	CHECK_VALID(*this);
}

#ifndef VECTOR_NO_SLOW_OPERATIONS

inline QAngle RandomAngle( float minVal, float maxVal )
{
	Vector random;
	random.Random( minVal, maxVal );
	QAngle ret( random.x, random.y, random.z );
	return ret;
}

#endif


inline RadianEuler::RadianEuler(QAngle const &angles)
{
	Init(
		angles.z * 3.14159265358979323846f / 180.f,
		angles.x * 3.14159265358979323846f / 180.f, 
		angles.y * 3.14159265358979323846f / 180.f );
}




inline QAngle RadianEuler::ToQAngle( void) const
{
	return QAngle(
		y * 180.f / 3.14159265358979323846f,
		z * 180.f / 3.14159265358979323846f,
		x * 180.f / 3.14159265358979323846f );
}


//-----------------------------------------------------------------------------
// assignment
//-----------------------------------------------------------------------------
inline QAngle& QAngle::operator=(const QAngle &vOther)	
{
	CHECK_VALID(vOther);
	x=vOther.x; y=vOther.y; z=vOther.z; 
	return *this; 
}


//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------
inline vec_t& QAngle::operator[](int i)
{
	Assert( (i >= 0) && (i < 3) );
	return ((vec_t*)this)[i];
}

inline vec_t QAngle::operator[](int i) const
{
	Assert( (i >= 0) && (i < 3) );
	return ((vec_t*)this)[i];
}


//-----------------------------------------------------------------------------
// Base address...
//-----------------------------------------------------------------------------
inline vec_t* QAngle::Base()
{
	return (vec_t*)this;
}

inline vec_t const* QAngle::Base() const
{
	return (vec_t const*)this;
}


//-----------------------------------------------------------------------------
// IsValid?
//-----------------------------------------------------------------------------
inline bool QAngle::IsValid() const
{
	return IsFinite(x) && IsFinite(y) && IsFinite(z);
}

//-----------------------------------------------------------------------------
// Invalidate
//-----------------------------------------------------------------------------

inline void QAngle::Invalidate()
{
//#ifdef _DEBUG
//#ifdef VECTOR_PARANOIA
	x = y = z = VEC_T_NAN;
//#endif
//#endif
}

//-----------------------------------------------------------------------------
// comparison
//-----------------------------------------------------------------------------
inline bool QAngle::operator==( const QAngle& src ) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x == x) && (src.y == y) && (src.z == z);
}

inline bool QAngle::operator!=( const QAngle& src ) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x != x) || (src.y != y) || (src.z != z);
}


//-----------------------------------------------------------------------------
// Copy
//-----------------------------------------------------------------------------
inline void VectorCopy( const QAngle& src, QAngle& dst )
{
	CHECK_VALID(src);
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}


//-----------------------------------------------------------------------------
// standard math operations
//-----------------------------------------------------------------------------
inline QAngle& QAngle::operator+=(const QAngle& v)	
{ 
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x+=v.x; y+=v.y; z += v.z;	
	return *this;
}

inline QAngle& QAngle::operator-=(const QAngle& v)	
{ 
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x-=v.x; y-=v.y; z -= v.z;	
	return *this;
}

inline QAngle& QAngle::operator*=(float fl)	
{
	x *= fl;
	y *= fl;
	z *= fl;
	CHECK_VALID(*this);
	return *this;
}

inline QAngle& QAngle::operator/=(float fl)	
{
	Assert( fl != 0.0f );
	float oofl = 1.0f / fl;
	x *= oofl;
	y *= oofl;
	z *= oofl;
	CHECK_VALID(*this);
	return *this;
}


//-----------------------------------------------------------------------------
// length
//-----------------------------------------------------------------------------
inline vec_t QAngle::Length( ) const
{
	CHECK_VALID(*this);
	return (vec_t)FastSqrt( LengthSqr( ) );		
}


inline vec_t QAngle::LengthSqr( ) const
{
	CHECK_VALID(*this);
	return x * x + y * y + z * z;
}
	

//-----------------------------------------------------------------------------
// Vector equality with tolerance
//-----------------------------------------------------------------------------
inline bool QAnglesAreEqual( const QAngle& src1, const QAngle& src2, float tolerance = 0.0f )
{
	if (FloatMakePositive(src1.x - src2.x) > tolerance)
		return false;
	if (FloatMakePositive(src1.y - src2.y) > tolerance)
		return false;
	return (FloatMakePositive(src1.z - src2.z) <= tolerance);
}


//-----------------------------------------------------------------------------
// arithmetic operations (SLOW!!)
//-----------------------------------------------------------------------------
#ifndef VECTOR_NO_SLOW_OPERATIONS

inline QAngle QAngle::operator-(void) const
{ 
	QAngle ret(-x,-y,-z);
	return ret;
}

inline QAngle QAngle::operator+(const QAngle& v) const	
{ 
	QAngle res;
	res.x = x + v.x;
	res.y = y + v.y;
	res.z = z + v.z;
	return res;	
}

inline QAngle QAngle::operator-(const QAngle& v) const	
{ 
	QAngle res;
	res.x = x - v.x;
	res.y = y - v.y;
	res.z = z - v.z;
	return res;	
}

inline QAngle QAngle::operator*(float fl) const	
{ 
	QAngle res;
	res.x = x * fl;
	res.y = y * fl;
	res.z = z * fl;
	return res;	
}

inline QAngle QAngle::operator/(float fl) const	
{ 
	QAngle res;
	res.x = x / fl;
	res.y = y / fl;
	res.z = z / fl;
	return res;	
}

inline QAngle operator*(float fl, const QAngle& v)	
{ 
        QAngle ret( v * fl );
	return ret;
}

#endif // VECTOR_NO_SLOW_OPERATIONS


//-----------------------------------------------------------------------------
// NOTE: These are not completely correct.  The representations are not equivalent
// unless the QAngle represents a rotational impulse along a coordinate axis (x,y,z)
inline void QAngleToAngularImpulse( const QAngle &angles, AngularImpulse &impulse )
{
	impulse.x = angles.z;
	impulse.y = angles.x;
	impulse.z = angles.y;
}

inline void AngularImpulseToQAngle( const AngularImpulse &impulse, QAngle &angles )
{
	angles.x = impulse.y;
	angles.y = impulse.z;
	angles.z = impulse.x;
}

#if !defined( _X360 )

FORCEINLINE vec_t InvRSquared( float const *v )
{
#if defined(__i386__) || defined(_M_IX86)
	float sqrlen = v[0]*v[0]+v[1]*v[1]+v[2]*v[2] + 1.0e-10f, result;
	_mm_store_ss(&result, _mm_rcp_ss( _mm_max_ss( _mm_set_ss(1.0f), _mm_load_ss(&sqrlen) ) ));
	return result;
#else
	return 1.f/fpmax(1.f, v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
#endif
}

FORCEINLINE vec_t InvRSquared( const Vector &v )
{
	return InvRSquared(&v.x);
}

#if defined(__i386__) || defined(_M_IX86)
inline void _SSE_RSqrtInline( float a, float* out )
{
	__m128  xx = _mm_load_ss( &a );
	__m128  xr = _mm_rsqrt_ss( xx );
	__m128  xt;
	xt = _mm_mul_ss( xr, xr );
	xt = _mm_mul_ss( xt, xx );
	xt = _mm_sub_ss( _mm_set_ss(3.f), xt );
	xt = _mm_mul_ss( xt, _mm_set_ss(0.5f) );
	xr = _mm_mul_ss( xr, xt );
	_mm_store_ss( out, xr );
}
#endif

// FIXME: Change this back to a #define once we get rid of the vec_t version
FORCEINLINE float VectorNormalize( Vector& vec )
{
#ifndef DEBUG // stop crashing my edit-and-continue!
	#if defined(__i386__) || defined(_M_IX86)
		#define DO_SSE_OPTIMIZATION
	#endif
#endif

#if defined( DO_SSE_OPTIMIZATION )
	float sqrlen = vec.LengthSqr() + 1.0e-10f, invlen;
	_SSE_RSqrtInline(sqrlen, &invlen);
	vec.x *= invlen;
	vec.y *= invlen;
	vec.z *= invlen;
	return sqrlen * invlen;
#else
	extern float (FASTCALL *pfVectorNormalize)(Vector& v);
	return (*pfVectorNormalize)(vec);
#endif
}

// FIXME: Obsolete version of VectorNormalize, once we remove all the friggin float*s
FORCEINLINE float VectorNormalize( float * v )
{
	return VectorNormalize(*(reinterpret_cast<Vector *>(v)));
}

FORCEINLINE void VectorNormalizeFast( Vector &vec )
{
	VectorNormalize(vec);
}

#else

FORCEINLINE float _VMX_InvRSquared( const Vector &v )
{
	XMVECTOR xmV = XMVector3ReciprocalLength( XMLoadVector3( v.Base() ) );
	xmV = XMVector3Dot( xmV, xmV );
	return xmV.x;
}

// call directly
FORCEINLINE float _VMX_VectorNormalize( Vector &vec )
{
	float mag = XMVector3Length( XMLoadVector3( vec.Base() ) ).x;
	float den = 1.f / (mag + FLT_EPSILON );
	vec.x *= den;
	vec.y *= den;
	vec.z *= den;
	return mag;
}

#define InvRSquared(x) _VMX_InvRSquared(x)

// FIXME: Change this back to a #define once we get rid of the vec_t version
FORCEINLINE float VectorNormalize( Vector& v )
{
	return _VMX_VectorNormalize( v );
}
// FIXME: Obsolete version of VectorNormalize, once we remove all the friggin float*s
FORCEINLINE float VectorNormalize( float *pV )
{
	return _VMX_VectorNormalize(*(reinterpret_cast<Vector*>(pV)));
}

// call directly
FORCEINLINE void VectorNormalizeFast( Vector &vec )
{
	XMVECTOR xmV = XMVector3LengthEst( XMLoadVector3( vec.Base() ) );
	float den = 1.f / (xmV.x + FLT_EPSILON);
	vec.x *= den;
	vec.y *= den;
	vec.z *= den;
}

#endif // _X360


inline vec_t Vector::NormalizeInPlace()
{
	return VectorNormalize( *this );
}

inline Vector Vector::Normalized() const
{
	Vector norm = *this;
	VectorNormalize( norm );
	return norm;
}

inline bool Vector::IsLengthGreaterThan( float val ) const
{
	return LengthSqr() > val*val;
}

inline bool Vector::IsLengthLessThan( float val ) const
{
	return LengthSqr() < val*val;
}

#endif

