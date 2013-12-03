//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "dlight.h"
#include "iefx.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//##################################################################
//
// PlasmaBeamNode - generates plasma embers
//
//##################################################################
class C_SpotlightEnd : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_SpotlightEnd, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_SpotlightEnd();

public:
	void	OnDataChanged(DataUpdateType_t updateType);
	bool	ShouldDraw();
	void	ClientThink( void );

	virtual bool ShouldInterpolate();


//	Vector	m_vSpotlightOrg;
//	Vector	m_vSpotlightDir;
	float	m_flLightScale;
	float	m_Radius;

private:
	dlight_t*	m_pDynamicLight;

	//dlight_t*	m_pModelLight;
};

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
C_SpotlightEnd::C_SpotlightEnd(void) : /*m_pModelLight(0), */m_pDynamicLight(0)
{
	m_flLightScale	= 100;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_SpotlightEnd::OnDataChanged(DataUpdateType_t updateType)
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool C_SpotlightEnd::ShouldDraw()
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: YWB:  This is a hack, BaseClass::Interpolate skips this entity because model == NULL
//   We could do something like model = (model_t *)0x00000001, but that's probably more evil.
// Input  : currentTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SpotlightEnd::ShouldInterpolate()
{
	return true;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_SpotlightEnd::ClientThink(void)
{
	// If light scale is zero, don't draw light
	if ( m_flLightScale <= 0 )
		return;

	ColorRGBExp32 color;
	color.r	= m_clrRender->r * m_clrRender->a;
	color.g	= m_clrRender->g * m_clrRender->a;
	color.b	= m_clrRender->b * m_clrRender->a;
	color.exponent = 0;
	if ( color.r == 0 && color.g == 0 && color.b == 0 )
		return;

	// Deal with the environment light
	if ( !m_pDynamicLight || (m_pDynamicLight->key != index) )
	{
		m_pDynamicLight = effects->CL_AllocDlight( index );
		assert (m_pDynamicLight);
	}

	//m_pDynamicLight->flags = DLIGHT_NO_MODEL_ILLUMINATION;
	m_pDynamicLight->radius		= m_flLightScale*3.0f;
	m_pDynamicLight->origin		= GetAbsOrigin() + Vector(0,0,5);
	m_pDynamicLight->die		= gpGlobals->curtime + 0.05f;
	m_pDynamicLight->color		= color;

	/*
	// For bumped lighting
	VectorCopy (m_vSpotlightDir,  m_pDynamicLight->m_Direction);

	// Deal with the model light
 	if ( !m_pModelLight || (m_pModelLight->key != -index) )
	{
		m_pModelLight = effects->CL_AllocDlight( -index );
		assert (m_pModelLight);
	}

	m_pModelLight->radius = m_Radius;
	m_pModelLight->flags = DLIGHT_NO_WORLD_ILLUMINATION;
	m_pModelLight->color.r = m_clrRender->r * m_clrRender->a;
	m_pModelLight->color.g = m_clrRender->g * m_clrRender->a;
	m_pModelLight->color.b = m_clrRender->b * m_clrRender->a;
	m_pModelLight->color.exponent	= 1;
	m_pModelLight->origin		= m_vSpotlightOrg;
	m_pModelLight->m_InnerAngle = 6;
	m_pModelLight->m_OuterAngle = 8;
	m_pModelLight->die = gpGlobals->curtime + 0.05;
	VectorCopy( m_vSpotlightDir, m_pModelLight->m_Direction );
	*/

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

IMPLEMENT_CLIENTCLASS_DT(C_SpotlightEnd, DT_SpotlightEnd, CSpotlightEnd)
	RecvPropFloat	(RECVINFO(m_flLightScale)),
	RecvPropFloat	(RECVINFO(m_Radius)),
//	RecvPropVector	(RECVINFO(m_vSpotlightOrg)),
//	RecvPropVector	(RECVINFO(m_vSpotlightDir)),
END_RECV_TABLE()
