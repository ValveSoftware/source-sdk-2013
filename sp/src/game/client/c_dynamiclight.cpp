//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Dynamic light
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "dlight.h"
#include "iefx.h"
#include "iviewrender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#if HL2_EPISODIC
// In Episodic we unify the NO_WORLD_ILLUMINATION lights to use 
// the more efficient elight structure instead. This should theoretically
// be extended to other projects but may have unintended consequences
// and bears more thorough testing.
//
// For an earlier iteration on this technique see changelist 214433,
// which had a specific flag for use of elights.
#define DLIGHT_NO_WORLD_USES_ELIGHT 1
#endif


//-----------------------------------------------------------------------------
// A dynamic light, with the goofy hack needed for spotlights
//-----------------------------------------------------------------------------
class C_DynamicLight : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_DynamicLight, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_DynamicLight();

public:
	void	OnDataChanged(DataUpdateType_t updateType);
	bool	ShouldDraw();
	void	ClientThink( void );
	void	Release( void );

	unsigned char	m_Flags;
	unsigned char	m_LightStyle;

	float	m_Radius;
	int		m_Exponent;
	float	m_InnerAngle;
	float	m_OuterAngle;
	float	m_SpotRadius;

private:
	dlight_t*	m_pDynamicLight;
	dlight_t*	m_pSpotlightEnd;


	inline bool ShouldBeElight() { return (m_Flags & DLIGHT_NO_WORLD_ILLUMINATION); }
};

IMPLEMENT_CLIENTCLASS_DT(C_DynamicLight, DT_DynamicLight, CDynamicLight)
	RecvPropInt		(RECVINFO(m_Flags)),
	RecvPropInt		(RECVINFO(m_LightStyle)),
	RecvPropFloat	(RECVINFO(m_Radius)),
	RecvPropInt		(RECVINFO(m_Exponent)),
	RecvPropFloat	(RECVINFO(m_InnerAngle)),
	RecvPropFloat	(RECVINFO(m_OuterAngle)),
	RecvPropFloat	(RECVINFO(m_SpotRadius)),
END_RECV_TABLE()


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
C_DynamicLight::C_DynamicLight(void) : m_pSpotlightEnd(0), m_pDynamicLight(0)
{
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void C_DynamicLight::OnDataChanged(DataUpdateType_t updateType)
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink(gpGlobals->curtime + 0.05);
	}

	BaseClass::OnDataChanged( updateType );
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
bool C_DynamicLight::ShouldDraw()
{
	return false;
}

//------------------------------------------------------------------------------
// Purpose : Disable drawing of this light when entity perishes
//------------------------------------------------------------------------------
void C_DynamicLight::Release()
{
	if (m_pDynamicLight)
	{
		m_pDynamicLight->die = gpGlobals->curtime;
		m_pDynamicLight = 0;
	}
	
	if (m_pSpotlightEnd)
	{
		m_pSpotlightEnd->die = gpGlobals->curtime;
		m_pSpotlightEnd = 0;
	}

	BaseClass::Release();
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void C_DynamicLight::ClientThink(void)
{
	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );

	if ( (m_Flags & DLIGHT_NO_MODEL_ILLUMINATION) == 0 )
	{
		// Deal with the model light
 		if ( !m_pDynamicLight || (m_pDynamicLight->key != index) )
		{
#if DLIGHT_NO_WORLD_USES_ELIGHT
			m_pDynamicLight = ShouldBeElight() != 0
				? effects->CL_AllocElight( index )
				: effects->CL_AllocDlight( index );
#else
			m_pDynamicLight = effects->CL_AllocDlight( index );
#endif
			Assert (m_pDynamicLight);
			m_pDynamicLight->minlight = 0;
		}

		m_pDynamicLight->style = m_LightStyle;
		m_pDynamicLight->radius = m_Radius;
		m_pDynamicLight->flags = m_Flags;
		if ( m_OuterAngle > 0 )
			m_pDynamicLight->flags |= DLIGHT_NO_WORLD_ILLUMINATION;
		m_pDynamicLight->color.r = m_clrRender->r;
		m_pDynamicLight->color.g = m_clrRender->g;
		m_pDynamicLight->color.b = m_clrRender->b;
		m_pDynamicLight->color.exponent	= m_Exponent;	// this makes it match the world
		m_pDynamicLight->origin		= GetAbsOrigin();
		m_pDynamicLight->m_InnerAngle = m_InnerAngle;
		m_pDynamicLight->m_OuterAngle = m_OuterAngle;
		m_pDynamicLight->die = gpGlobals->curtime + 1e6;
		m_pDynamicLight->m_Direction = forward;
	}
	else
	{
		// In this case, the m_Flags could have changed; which is how we turn the light off
		if (m_pDynamicLight)
		{
			m_pDynamicLight->die = gpGlobals->curtime;
			m_pDynamicLight = 0;
		}
	}
	
#if DLIGHT_NO_WORLD_USES_ELIGHT
	if (( m_OuterAngle > 0 ) && !ShouldBeElight())
#else
	if (( m_OuterAngle > 0 ) && ((m_Flags & DLIGHT_NO_WORLD_ILLUMINATION) == 0))
#endif
	{
		// Raycast to where the endpoint goes
		// Deal with the environment light
		if ( !m_pSpotlightEnd || (m_pSpotlightEnd->key != -index) )
		{
			m_pSpotlightEnd = effects->CL_AllocDlight( -index );
			Assert (m_pSpotlightEnd);
		}
				  
		// Trace a line outward, don't use hitboxes (too slow)
		Vector end;
		VectorMA( GetAbsOrigin(), m_Radius, forward, end );

		trace_t		pm;
		C_BaseEntity::PushEnableAbsRecomputations( false );	 // HACK don't recompute positions while doing RayTrace
		UTIL_TraceLine( GetAbsOrigin(), end, MASK_NPCWORLDSTATIC, NULL, COLLISION_GROUP_NONE, &pm );
		C_BaseEntity::PopEnableAbsRecomputations();
		VectorCopy( pm.endpos, m_pSpotlightEnd->origin );
		
		if (pm.fraction == 1.0f)
		{
			m_pSpotlightEnd->die = gpGlobals->curtime;
			m_pSpotlightEnd = 0;
		}
		else
		{
			float falloff = 1.0 - pm.fraction;
			falloff *= falloff;

			m_pSpotlightEnd->style = m_LightStyle;
			m_pSpotlightEnd->flags = DLIGHT_NO_MODEL_ILLUMINATION | (m_Flags & DLIGHT_DISPLACEMENT_MASK);
			m_pSpotlightEnd->radius		= m_SpotRadius; // * falloff;
			m_pSpotlightEnd->die		= gpGlobals->curtime + 1e6;
			m_pSpotlightEnd->color.r	= m_clrRender->r * falloff;
			m_pSpotlightEnd->color.g	= m_clrRender->g * falloff;
			m_pSpotlightEnd->color.b	= m_clrRender->b * falloff;
			m_pSpotlightEnd->color.exponent	= m_Exponent;

			// For bumped lighting
			m_pSpotlightEnd->m_Direction = forward;

			// Update list of surfaces we influence
			render->TouchLight( m_pSpotlightEnd );
		}
	}
	else
	{
		// In this case, the m_Flags could have changed; which is how we turn the light off
		if (m_pSpotlightEnd)
		{
			m_pSpotlightEnd->die = gpGlobals->curtime;
			m_pSpotlightEnd = 0;
		}
	}

	SetNextClientThink(gpGlobals->curtime + 0.001);
}

