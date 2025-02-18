//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include <ssemath.h>
#include <lightdesc.h>
#include "mathlib.h"

void LightDesc_t::RecalculateDerivedValues(void)
{
	m_Flags = LIGHTTYPE_OPTIMIZATIONFLAGS_DERIVED_VALUES_CALCED;
	if (m_Attenuation0)
		m_Flags|=LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0;
	if (m_Attenuation1)
		m_Flags|=LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1;
	if (m_Attenuation2)
		m_Flags|=LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2;
	
	if (m_Type==MATERIAL_LIGHT_SPOT)
	{
		m_ThetaDot=cos(m_Theta);
		m_PhiDot=cos(m_Phi);
		float spread=m_ThetaDot-m_PhiDot;
		if (spread>1.0e-10)
		{
			// note - this quantity is very sensitive to round off error. the sse
			// reciprocal approximation won't cut it here.
			OneOver_ThetaDot_Minus_PhiDot=1.0/spread;
		}
		else
		{
			// hard falloff instead of divide by zero
			OneOver_ThetaDot_Minus_PhiDot=1.0;
		}				
	}	
	if (m_Type==MATERIAL_LIGHT_DIRECTIONAL)
	{
		// set position to be real far away in the right direction
		m_Position=m_Direction;
		m_Position *= 2.0e6;
	}
	
	m_RangeSquared=m_Range*m_Range;

}

void LightDesc_t::ComputeLightAtPointsForDirectional(
	const FourVectors &pos, const FourVectors &normal,
	FourVectors &color, bool DoHalfLambert ) const
{
	FourVectors delta;
	delta.DuplicateVector(m_Direction);
//	delta.VectorNormalizeFast();
	fltx4 strength=delta*normal;
	if (DoHalfLambert)
	{
		strength=AddSIMD(MulSIMD(strength,Four_PointFives),Four_PointFives);
	}
	else
		strength=MaxSIMD(Four_Zeros,delta*normal);
		
	color.x=AddSIMD(color.x,MulSIMD(strength,ReplicateX4(m_Color.x)));
	color.y=AddSIMD(color.y,MulSIMD(strength,ReplicateX4(m_Color.y)));
	color.z=AddSIMD(color.z,MulSIMD(strength,ReplicateX4(m_Color.z)));
}


void LightDesc_t::ComputeLightAtPoints( const FourVectors &pos, const FourVectors &normal,
										FourVectors &color, bool DoHalfLambert ) const
{
	FourVectors delta;
	Assert((m_Type==MATERIAL_LIGHT_POINT) || (m_Type==MATERIAL_LIGHT_SPOT) || (m_Type==MATERIAL_LIGHT_DIRECTIONAL));
	switch (m_Type)
	{
		default:
		case MATERIAL_LIGHT_POINT:
		case MATERIAL_LIGHT_SPOT:
			delta.DuplicateVector(m_Position);
			delta-=pos;
			break;
				
		case MATERIAL_LIGHT_DIRECTIONAL:
			ComputeLightAtPointsForDirectional( pos, normal, color, DoHalfLambert );
			return;
	}

	fltx4 dist2 = delta*delta;

	dist2=MaxSIMD( Four_Ones, dist2 );

	fltx4 falloff;

	if( m_Flags & LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0 )
	{
		falloff = ReplicateX4(m_Attenuation0);
	}
	else
		falloff= Four_Epsilons;

	if( m_Flags & LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1 )
	{
		falloff=AddSIMD(falloff,MulSIMD(ReplicateX4(m_Attenuation1),SqrtEstSIMD(dist2)));
	}

	if( m_Flags & LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2 )
	{
		falloff=AddSIMD(falloff,MulSIMD(ReplicateX4(m_Attenuation2),dist2));
	}

	falloff=ReciprocalEstSIMD(falloff);
	// Cull out light beyond this radius
	// now, zero out elements for which dist2 was > range^2. !!speed!! lights should store dist^2 in sse format
	if (m_Range != 0.f)
	{
		fltx4 RangeSquared=ReplicateX4(m_RangeSquared); // !!speed!!
		falloff=AndSIMD(falloff,CmpLtSIMD(dist2,RangeSquared));
	}

	delta.VectorNormalizeFast();
	fltx4 strength=delta*normal;
	if (DoHalfLambert)
	{
		strength=AddSIMD(MulSIMD(strength,Four_PointFives),Four_PointFives);
	}
	else
		strength=MaxSIMD(Four_Zeros,delta*normal);
		
	switch(m_Type)
	{
		case MATERIAL_LIGHT_POINT:
			// half-lambert
			break;
				
		case MATERIAL_LIGHT_SPOT:
		{
			fltx4 dot2=SubSIMD(Four_Zeros,delta*m_Direction); // dot position with spot light dir for cone falloff


			fltx4 cone_falloff_scale=MulSIMD(ReplicateX4(OneOver_ThetaDot_Minus_PhiDot),
												 SubSIMD(dot2,ReplicateX4(m_PhiDot)));
			cone_falloff_scale=MinSIMD(cone_falloff_scale,Four_Ones);
			
			if ((m_Falloff!=0.0) && (m_Falloff!=1.0))
			{
				// !!speed!! could compute integer exponent needed by powsimd and store in light
				cone_falloff_scale=PowSIMD(cone_falloff_scale,m_Falloff);
			}
			strength=MulSIMD(cone_falloff_scale,strength);

			// now, zero out lighting where dot2<phidot. This will mask out any invalid results
			// from pow function, etc
			fltx4 OutsideMask=CmpGtSIMD(dot2,ReplicateX4(m_PhiDot)); // outside light cone?
			strength=AndSIMD(OutsideMask,strength);
		}
		break;
			

	}
	strength=MulSIMD(strength,falloff);
	color.x=AddSIMD(color.x,MulSIMD(strength,ReplicateX4(m_Color.x)));
	color.y=AddSIMD(color.y,MulSIMD(strength,ReplicateX4(m_Color.y)));
	color.z=AddSIMD(color.z,MulSIMD(strength,ReplicateX4(m_Color.z)));
}



void LightDesc_t::ComputeNonincidenceLightAtPoints( const FourVectors &pos, FourVectors &color ) const
{
	FourVectors delta;
	Assert((m_Type==MATERIAL_LIGHT_POINT) || (m_Type==MATERIAL_LIGHT_SPOT) || (m_Type==MATERIAL_LIGHT_DIRECTIONAL));
	switch (m_Type)
	{
		default:
		case MATERIAL_LIGHT_POINT:
		case MATERIAL_LIGHT_SPOT:
			delta.DuplicateVector(m_Position);
			delta-=pos;
			break;
				
		case MATERIAL_LIGHT_DIRECTIONAL:
			return;
	}

	fltx4 dist2 = delta*delta;

	dist2=MaxSIMD( Four_Ones, dist2 );

	fltx4 falloff;

	if( m_Flags & LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0 )
	{
		falloff = ReplicateX4(m_Attenuation0);
	}
	else
		falloff= Four_Epsilons;

	if( m_Flags & LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1 )
	{
		falloff=AddSIMD(falloff,MulSIMD(ReplicateX4(m_Attenuation1),SqrtEstSIMD(dist2)));
	}

	if( m_Flags & LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2 )
	{
		falloff=AddSIMD(falloff,MulSIMD(ReplicateX4(m_Attenuation2),dist2));
	}

	falloff=ReciprocalEstSIMD(falloff);
	// Cull out light beyond this radius
	// now, zero out elements for which dist2 was > range^2. !!speed!! lights should store dist^2 in sse format
	if (m_Range != 0.f)
	{
		fltx4 RangeSquared=ReplicateX4(m_RangeSquared); // !!speed!!
		falloff=AndSIMD(falloff,CmpLtSIMD(dist2,RangeSquared));
	}

	delta.VectorNormalizeFast();
	fltx4 strength = Four_Ones;
	//fltx4 strength=delta;
	//fltx4 strength = MaxSIMD(Four_Zeros,delta);
		
	switch(m_Type)
	{
		case MATERIAL_LIGHT_POINT:
			// half-lambert
			break;
				
		case MATERIAL_LIGHT_SPOT:
		{
			fltx4 dot2=SubSIMD(Four_Zeros,delta*m_Direction); // dot position with spot light dir for cone falloff


			fltx4 cone_falloff_scale=MulSIMD(ReplicateX4(OneOver_ThetaDot_Minus_PhiDot),
												 SubSIMD(dot2,ReplicateX4(m_PhiDot)));
			cone_falloff_scale=MinSIMD(cone_falloff_scale,Four_Ones);
			
			if ((m_Falloff!=0.0) && (m_Falloff!=1.0))
			{
				// !!speed!! could compute integer exponent needed by powsimd and store in light
				cone_falloff_scale=PowSIMD(cone_falloff_scale,m_Falloff);
			}
			strength=MulSIMD(cone_falloff_scale,strength);

			// now, zero out lighting where dot2<phidot. This will mask out any invalid results
			// from pow function, etc
			fltx4 OutsideMask=CmpGtSIMD(dot2,ReplicateX4(m_PhiDot)); // outside light cone?
			strength=AndSIMD(OutsideMask,strength);
		}
		break;
			

	}
	strength=MulSIMD(strength,falloff);
	color.x=AddSIMD(color.x,MulSIMD(strength,ReplicateX4(m_Color.x)));
	color.y=AddSIMD(color.y,MulSIMD(strength,ReplicateX4(m_Color.y)));
	color.z=AddSIMD(color.z,MulSIMD(strength,ReplicateX4(m_Color.z)));
}



void LightDesc_t::SetupOldStyleAttenuation( float fQuadraticAttn, float fLinearAttn, float fConstantAttn )
{
	// old-style manually typed quadrtiac coefficients
	if ( fQuadraticAttn < EQUAL_EPSILON )
		fQuadraticAttn = 0;
	
	if ( fLinearAttn < EQUAL_EPSILON)
		fLinearAttn = 0;
	
	if ( fConstantAttn < EQUAL_EPSILON)
		fConstantAttn = 0;
	
	if ( ( fConstantAttn < EQUAL_EPSILON ) && 
		 ( fLinearAttn < EQUAL_EPSILON ) && 
		 ( fQuadraticAttn < EQUAL_EPSILON ) )
		fConstantAttn = 1;

	m_Attenuation2=fQuadraticAttn;
	m_Attenuation1=fLinearAttn;
	m_Attenuation0=fConstantAttn;
	float fScaleFactor = fQuadraticAttn * 10000 + fLinearAttn * 100 + fConstantAttn;
	
	if ( fScaleFactor > 0 )
		m_Color *= fScaleFactor;
}

void LightDesc_t::SetupNewStyleAttenuation( float fFiftyPercentDistance, 
											float fZeroPercentDistance )
{
	// new style storing 50% and 0% distances
	float d50=fFiftyPercentDistance;
	float d0=fZeroPercentDistance;
	if (d0<d50)
	{
		// !!warning in lib code???!!!
		Warning("light has _fifty_percent_distance of %f but no zero_percent_distance\n",d50);
		d0=2.0*d50;
	}
	float a=0,b=1,c=0;
	if (! SolveInverseQuadraticMonotonic(0,1.0,d50,2.0,d0,256.0,a,b,c))
	{
		Warning("can't solve quadratic for light %f %f\n",d50,d0);
	}
	float v50=c+d50*(b+d50*a);
	float scale=2.0/v50;
	a*=scale;
	b*=scale;
	c*=scale;
	m_Attenuation2=a;
	m_Attenuation1=b;
	m_Attenuation0=c;
}

