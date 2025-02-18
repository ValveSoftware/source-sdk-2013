//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

// light structure definitions.
#ifndef LIGHTDESC_H
#define LIGHTDESC_H

#include <mathlib/ssemath.h>
#include <mathlib/vector.h>

//-----------------------------------------------------------------------------
// Light structure
//-----------------------------------------------------------------------------

enum LightType_t
{
	MATERIAL_LIGHT_DISABLE = 0,
	MATERIAL_LIGHT_POINT,
	MATERIAL_LIGHT_DIRECTIONAL,
	MATERIAL_LIGHT_SPOT,
};

enum LightType_OptimizationFlags_t
{
	LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0 = 1,
	LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1 = 2,
	LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2 = 4,
	LIGHTTYPE_OPTIMIZATIONFLAGS_DERIVED_VALUES_CALCED = 8,
};

struct LightDesc_t 
{
    LightType_t m_Type;										//< MATERIAL_LIGHT_xxx
	Vector m_Color;											//< color+intensity 
    Vector m_Position;										//< light source center position
    Vector m_Direction;										//< for SPOT, direction it is pointing
    float  m_Range;											//< distance range for light.0=infinite
    float m_Falloff;										//< angular falloff exponent for spot lights
    float m_Attenuation0;									//< constant distance falloff term
    float m_Attenuation1;									//< linear term of falloff
    float m_Attenuation2;									//< quadatic term of falloff
    float m_Theta;											//< inner cone angle. no angular falloff 
															//< within this cone
    float m_Phi;											//< outer cone angle

	// the values below are derived from the above settings for optimizations
	// These aren't used by DX8. . used for software lighting.
	float m_ThetaDot;
	float m_PhiDot;
	unsigned int m_Flags;
protected:
	float OneOver_ThetaDot_Minus_PhiDot;
	float m_RangeSquared;
public:

	void RecalculateDerivedValues(void);			 // calculate m_xxDot, m_Type for changed parms

	LightDesc_t(void)
	{
	}

	// constructors for various useful subtypes

	// a point light with infinite range
	LightDesc_t( const Vector &pos, const Vector &color )
	{
		InitPoint( pos, color );
	}
	
	/// a simple light. cone boundaries in radians. you pass a look_at point and the
	/// direciton is derived from that.
	LightDesc_t( const Vector &pos, const Vector &color, const Vector &point_at,
				float inner_cone_boundary, float outer_cone_boundary )
	{
		InitSpot( pos, color, point_at, inner_cone_boundary, outer_cone_boundary );
	}

	void InitPoint( const Vector &pos, const Vector &color );
	void InitDirectional( const Vector &dir, const Vector &color );
	void InitSpot(const Vector &pos, const Vector &color, const Vector &point_at,
		float inner_cone_boundary, float outer_cone_boundary );

	/// Given 4 points and 4 normals, ADD lighting from this light into "color".
	void ComputeLightAtPoints( const FourVectors &pos, const FourVectors &normal,
							   FourVectors &color, bool DoHalfLambert=false ) const;
	void ComputeNonincidenceLightAtPoints( const FourVectors &pos, FourVectors &color ) const;
	void ComputeLightAtPointsForDirectional( const FourVectors &pos,
											 const FourVectors &normal,
											 FourVectors &color, bool DoHalfLambert=false ) const;

	// warning - modifies color!!! set color first!!
	void SetupOldStyleAttenuation( float fQuadatricAttn, float fLinearAttn, float fConstantAttn );

	void SetupNewStyleAttenuation( float fFiftyPercentDistance, float fZeroPercentDistance );


/// given a direction relative to the light source position, is this ray within the
	/// light cone (for spotlights..non spots consider all rays to be within their cone)
	bool IsDirectionWithinLightCone(const Vector &rdir) const
	{
		return ((m_Type!=MATERIAL_LIGHT_SPOT) || (rdir.Dot(m_Direction)>=m_PhiDot));
	}

	float OneOverThetaDotMinusPhiDot() const
	{
		return OneOver_ThetaDot_Minus_PhiDot;
	}
};


//-----------------------------------------------------------------------------
// a point light with infinite range
//-----------------------------------------------------------------------------
inline void LightDesc_t::InitPoint( const Vector &pos, const Vector &color )
{
	m_Type=MATERIAL_LIGHT_POINT;
	m_Color=color;
	m_Position=pos;
	m_Range=0.0;									// infinite
	m_Attenuation0=1.0;
	m_Attenuation1=0;
	m_Attenuation2=0;
	RecalculateDerivedValues();
}


//-----------------------------------------------------------------------------
// a directional light with infinite range
//-----------------------------------------------------------------------------
inline void LightDesc_t::InitDirectional( const Vector &dir, const Vector &color )
{
	m_Type=MATERIAL_LIGHT_DIRECTIONAL;
	m_Color=color;
	m_Direction=dir;
	m_Range=0.0;									// infinite
	m_Attenuation0=1.0;
	m_Attenuation1=0;
	m_Attenuation2=0;
	RecalculateDerivedValues();
}


//-----------------------------------------------------------------------------
// a simple light. cone boundaries in radians. you pass a look_at point and the
// direciton is derived from that.
//-----------------------------------------------------------------------------
inline void LightDesc_t::InitSpot(const Vector &pos, const Vector &color, const Vector &point_at,
	float inner_cone_boundary, float outer_cone_boundary)
{
	m_Type=MATERIAL_LIGHT_SPOT;
	m_Color=color;
	m_Position=pos;
	m_Direction=point_at;
	m_Direction-=pos;
	VectorNormalizeFast(m_Direction);
	m_Falloff=5.0;										// linear angle falloff
	m_Theta=inner_cone_boundary;
	m_Phi=outer_cone_boundary;

	m_Range=0.0;										// infinite

	m_Attenuation0=1.0;
	m_Attenuation1=0;
	m_Attenuation2=0;
	RecalculateDerivedValues();
}


#endif

