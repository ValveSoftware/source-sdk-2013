//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "beam_shared.h"
#include "spotlightend.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Spawnflags
#define SF_SPOTLIGHT_START_LIGHT_ON			0x1
#define SF_SPOTLIGHT_NO_DYNAMIC_LIGHT		0x2


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointSpotlight : public CPointEntity
{
	DECLARE_CLASS( CPointSpotlight, CPointEntity );
public:
	DECLARE_DATADESC();

	CPointSpotlight();

	void	Precache(void);
	void	Spawn(void);
	virtual void Activate();

	virtual void OnEntityEvent( EntityEvent_t event, void *pEventData );

private:
	int 	UpdateTransmitState();
	void	SpotlightThink(void);
	void	SpotlightUpdate(void);
	Vector	SpotlightCurrentPos(void);
	void	SpotlightCreate(void);
	void	SpotlightDestroy(void);

	// ------------------------------
	//  Inputs
	// ------------------------------
	void InputLightOn( inputdata_t &inputdata );
	void InputLightOff( inputdata_t &inputdata );

	// Creates the efficient spotlight 
	void CreateEfficientSpotlight();

	// Computes render info for a spotlight
	void ComputeRenderInfo();

private:
	bool	m_bSpotlightOn;
	bool	m_bEfficientSpotlight;
	bool	m_bIgnoreSolid;
	Vector	m_vSpotlightTargetPos;
	Vector	m_vSpotlightCurrentPos;
	Vector	m_vSpotlightDir;
	int		m_nHaloSprite;
	CHandle<CBeam>			m_hSpotlight;
	CHandle<CSpotlightEnd>	m_hSpotlightTarget;
	
	float	m_flSpotlightMaxLength;
	float	m_flSpotlightCurLength;
	float	m_flSpotlightGoalWidth;
	float	m_flHDRColorScale;
	int		m_nMinDXLevel;

public:
	COutputEvent m_OnOn, m_OnOff;     ///< output fires when turned on, off
};

BEGIN_DATADESC( CPointSpotlight )
	DEFINE_FIELD( m_flSpotlightCurLength, FIELD_FLOAT ),

	DEFINE_FIELD( m_bSpotlightOn,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bEfficientSpotlight,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vSpotlightTargetPos,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vSpotlightCurrentPos,	FIELD_POSITION_VECTOR ),

	// Robin: Don't Save, recreated after restore/transition
	//DEFINE_FIELD( m_hSpotlight,			FIELD_EHANDLE ),
	//DEFINE_FIELD( m_hSpotlightTarget,		FIELD_EHANDLE ),

	DEFINE_FIELD( m_vSpotlightDir,			FIELD_VECTOR ),
	DEFINE_FIELD( m_nHaloSprite,			FIELD_INTEGER ),

	DEFINE_KEYFIELD( m_bIgnoreSolid, FIELD_BOOLEAN, "IgnoreSolid" ),
	DEFINE_KEYFIELD( m_flSpotlightMaxLength,FIELD_FLOAT, "SpotlightLength"),
	DEFINE_KEYFIELD( m_flSpotlightGoalWidth,FIELD_FLOAT, "SpotlightWidth"),
	DEFINE_KEYFIELD( m_flHDRColorScale, FIELD_FLOAT, "HDRColorScale" ),
	DEFINE_KEYFIELD( m_nMinDXLevel, FIELD_INTEGER, "mindxlevel" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,		"LightOn",		InputLightOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"LightOff",		InputLightOff ),
	DEFINE_OUTPUT( m_OnOn, "OnLightOn" ),
	DEFINE_OUTPUT( m_OnOff, "OnLightOff" ),

	DEFINE_THINKFUNC( SpotlightThink ),

END_DATADESC()


LINK_ENTITY_TO_CLASS(point_spotlight, CPointSpotlight);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPointSpotlight::CPointSpotlight()
{
#ifdef _DEBUG
	m_vSpotlightTargetPos.Init();
	m_vSpotlightCurrentPos.Init();
	m_vSpotlightDir.Init();
#endif
	m_flHDRColorScale = 1.0f;
	m_nMinDXLevel = 0;
	m_bIgnoreSolid = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointSpotlight::Precache(void)
{
	BaseClass::Precache();

	// Sprites.
	m_nHaloSprite = PrecacheModel("sprites/light_glow03.vmt");
	PrecacheModel( "sprites/glow_test02.vmt" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointSpotlight::Spawn(void)
{
	Precache();

	UTIL_SetSize( this,vec3_origin,vec3_origin );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	m_bEfficientSpotlight = true;

	// Check for user error
	if (m_flSpotlightMaxLength <= 0)
	{
		DevMsg("%s (%s) has an invalid spotlight length <= 0, setting to 500\n", GetClassname(), GetDebugName() );
		m_flSpotlightMaxLength = 500;
	}
	if (m_flSpotlightGoalWidth <= 0)
	{
		DevMsg("%s (%s) has an invalid spotlight width <= 0, setting to 10\n", GetClassname(), GetDebugName() );
		m_flSpotlightGoalWidth = 10;
	}
	
	if (m_flSpotlightGoalWidth > MAX_BEAM_WIDTH )
	{
		DevMsg("%s (%s) has an invalid spotlight width %.1f (max %.1f).\n", GetClassname(), GetDebugName(), m_flSpotlightGoalWidth, MAX_BEAM_WIDTH );
		m_flSpotlightGoalWidth = MAX_BEAM_WIDTH; 
	}

	// ------------------------------------
	//	Init all class vars 
	// ------------------------------------
	m_vSpotlightTargetPos	= vec3_origin;
	m_vSpotlightCurrentPos	= vec3_origin;
	m_hSpotlight			= NULL;
	m_hSpotlightTarget		= NULL;
	m_vSpotlightDir			= vec3_origin;
	m_flSpotlightCurLength	= m_flSpotlightMaxLength;

	m_bSpotlightOn = HasSpawnFlags( SF_SPOTLIGHT_START_LIGHT_ON );

	SetThink( &CPointSpotlight::SpotlightThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}


//-----------------------------------------------------------------------------
// Computes render info for a spotlight
//-----------------------------------------------------------------------------
void CPointSpotlight::ComputeRenderInfo()
{
	// Fade out spotlight end if past max length.  
	if ( m_flSpotlightCurLength > 2*m_flSpotlightMaxLength )
	{
		m_hSpotlightTarget->SetRenderColorA( 0 );
		m_hSpotlight->SetFadeLength( m_flSpotlightMaxLength );
	}
	else if ( m_flSpotlightCurLength > m_flSpotlightMaxLength )		
	{
		m_hSpotlightTarget->SetRenderColorA( (1-((m_flSpotlightCurLength-m_flSpotlightMaxLength)/m_flSpotlightMaxLength)) );
		m_hSpotlight->SetFadeLength( m_flSpotlightMaxLength );
	}
	else
	{
		m_hSpotlightTarget->SetRenderColorA( 1.0 );
		m_hSpotlight->SetFadeLength( m_flSpotlightCurLength );
	}

	// Adjust end width to keep beam width constant
	float flNewWidth = m_flSpotlightGoalWidth * (m_flSpotlightCurLength / m_flSpotlightMaxLength);
	flNewWidth = clamp(flNewWidth, 0.f, MAX_BEAM_WIDTH );
	m_hSpotlight->SetEndWidth(flNewWidth);

	// Adjust width of light on the end.  
	if ( FBitSet (m_spawnflags, SF_SPOTLIGHT_NO_DYNAMIC_LIGHT) )
	{
		m_hSpotlightTarget->m_flLightScale = 0.0;
	}
	else
	{
		// <<TODO>> - magic number 1.8 depends on sprite size
		m_hSpotlightTarget->m_flLightScale = 1.8*flNewWidth;
	}
}


//-----------------------------------------------------------------------------
// Creates the efficient spotlight 
//-----------------------------------------------------------------------------
void CPointSpotlight::CreateEfficientSpotlight()
{
	if ( m_hSpotlightTarget.Get() != NULL )
		return;

	SpotlightCreate();
	m_vSpotlightCurrentPos = SpotlightCurrentPos();
	m_hSpotlightTarget->SetAbsOrigin( m_vSpotlightCurrentPos );
	m_hSpotlightTarget->m_vSpotlightOrg = GetAbsOrigin();
	VectorSubtract( m_hSpotlightTarget->GetAbsOrigin(), m_hSpotlightTarget->m_vSpotlightOrg, m_hSpotlightTarget->m_vSpotlightDir );
	m_flSpotlightCurLength = VectorNormalize( m_hSpotlightTarget->m_vSpotlightDir );
	m_hSpotlightTarget->SetMoveType( MOVETYPE_NONE );
	ComputeRenderInfo();

	m_OnOn.FireOutput( this, this );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointSpotlight::Activate(void)
{
	BaseClass::Activate();

	if ( GetMoveParent() )
	{
		m_bEfficientSpotlight = false;
	}

	if ( m_bEfficientSpotlight )
	{
		if ( m_bSpotlightOn )
		{
			CreateEfficientSpotlight();
		}

		// Don't think
		SetThink( NULL );
	}
}


//-------------------------------------------------------------------------------------
// Optimization to deal with spotlights
//-------------------------------------------------------------------------------------
void CPointSpotlight::OnEntityEvent( EntityEvent_t event, void *pEventData )
{
	if ( event == ENTITY_EVENT_PARENT_CHANGED )
	{
		if ( GetMoveParent() )
		{
			m_bEfficientSpotlight = false;
			if ( m_hSpotlightTarget )
			{
				m_hSpotlightTarget->SetMoveType( MOVETYPE_FLY );
			}
			SetThink( &CPointSpotlight::SpotlightThink );
			SetNextThink( gpGlobals->curtime + 0.1f );
		}
	}

	BaseClass::OnEntityEvent( event, pEventData );
}

	
//-------------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model so spotlight gets proper position
// Input   :
// Output  :
//-------------------------------------------------------------------------------------
int CPointSpotlight::UpdateTransmitState()
{
	if ( m_bEfficientSpotlight )
		return SetTransmitState( FL_EDICT_DONTSEND );

	return SetTransmitState( FL_EDICT_PVSCHECK );
}

//-----------------------------------------------------------------------------
// Purpose: Plays the engine sound.
//-----------------------------------------------------------------------------
void CPointSpotlight::SpotlightThink( void )
{
	if ( GetMoveParent() )
	{
		SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
	}

	SpotlightUpdate();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CPointSpotlight::SpotlightCreate(void)
{
	if ( m_hSpotlightTarget.Get() != NULL )
		return;

	AngleVectors( GetAbsAngles(), &m_vSpotlightDir );

	Vector vTargetPos;
	if ( m_bIgnoreSolid )
	{
		vTargetPos = GetAbsOrigin() + m_vSpotlightDir * m_flSpotlightMaxLength;
	}
	else
	{
		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + m_vSpotlightDir * m_flSpotlightMaxLength, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		vTargetPos = tr.endpos;
	}

	m_hSpotlightTarget = (CSpotlightEnd*)CreateEntityByName( "spotlight_end" );
	m_hSpotlightTarget->Spawn();
	m_hSpotlightTarget->SetAbsOrigin( vTargetPos );
	m_hSpotlightTarget->SetOwnerEntity( this );
	m_hSpotlightTarget->m_clrRender = m_clrRender;
	m_hSpotlightTarget->m_Radius = m_flSpotlightMaxLength;

	if ( FBitSet (m_spawnflags, SF_SPOTLIGHT_NO_DYNAMIC_LIGHT) )
	{
		m_hSpotlightTarget->m_flLightScale = 0.0;
	}

	//m_hSpotlight = CBeam::BeamCreate( "sprites/spotlight.vmt", m_flSpotlightGoalWidth );
	m_hSpotlight = CBeam::BeamCreate( "sprites/glow_test02.vmt", m_flSpotlightGoalWidth );
	// Set the temporary spawnflag on the beam so it doesn't save (we'll recreate it on restore)
	m_hSpotlight->SetHDRColorScale( m_flHDRColorScale );
	m_hSpotlight->AddSpawnFlags( SF_BEAM_TEMPORARY );
	m_hSpotlight->SetColor( m_clrRender->r, m_clrRender->g, m_clrRender->b ); 
	m_hSpotlight->SetHaloTexture(m_nHaloSprite);
	m_hSpotlight->SetHaloScale(60);
	m_hSpotlight->SetEndWidth(m_flSpotlightGoalWidth);
	m_hSpotlight->SetBeamFlags( (FBEAM_SHADEOUT|FBEAM_NOTILE) );
	m_hSpotlight->SetBrightness( 64 );
	m_hSpotlight->SetNoise( 0 );
	m_hSpotlight->SetMinDXLevel( m_nMinDXLevel );

	if ( m_bEfficientSpotlight )
	{
		m_hSpotlight->PointsInit( GetAbsOrigin(), m_hSpotlightTarget->GetAbsOrigin() );
	}
	else
	{
		m_hSpotlight->EntsInit( this, m_hSpotlightTarget );
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CPointSpotlight::SpotlightCurrentPos(void)
{
	AngleVectors( GetAbsAngles(), &m_vSpotlightDir );

	//	Get beam end point.  Only collide with solid objects, not npcs
	Vector vEndPos = GetAbsOrigin() + ( m_vSpotlightDir * 2 * m_flSpotlightMaxLength );
	if ( m_bIgnoreSolid )
	{
		return vEndPos;
	}
	else
	{
		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin(), vEndPos, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		return tr.endpos;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CPointSpotlight::SpotlightDestroy(void)
{
	if ( m_hSpotlight )
	{
		m_OnOff.FireOutput( this, this );

		UTIL_Remove(m_hSpotlight);
		UTIL_Remove(m_hSpotlightTarget);
	}
}

//------------------------------------------------------------------------------
// Purpose : Update the direction and position of my spotlight
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CPointSpotlight::SpotlightUpdate(void)
{
	// ---------------------------------------------------
	//  If I don't have a spotlight attempt to create one
	// ---------------------------------------------------
	if ( !m_hSpotlight )
	{
		if ( m_bSpotlightOn )
		{
			// Make the spotlight
			SpotlightCreate();
		}
		else
		{
			return;
		}
	}
	else if ( !m_bSpotlightOn )
	{
		SpotlightDestroy();
		return;
	}
	
	if ( !m_hSpotlightTarget )
	{
		DevWarning( "**Attempting to update point_spotlight but target ent is NULL\n" );
		SpotlightDestroy();
		SpotlightCreate();
		if ( !m_hSpotlightTarget )
			return;
	}

	m_vSpotlightCurrentPos = SpotlightCurrentPos();

	//  Update spotlight target velocity
	Vector vTargetDir;
	VectorSubtract( m_vSpotlightCurrentPos, m_hSpotlightTarget->GetAbsOrigin(), vTargetDir );
	float vTargetDist = vTargetDir.Length();

	// If we haven't moved at all, don't recompute
	if ( vTargetDist < 1 )
	{
		m_hSpotlightTarget->SetAbsVelocity( vec3_origin );
		return;
	}

	Vector vecNewVelocity = vTargetDir;
	VectorNormalize(vecNewVelocity);
	vecNewVelocity *= (10 * vTargetDist);

	// If a large move is requested, just jump to final spot as we probably hit a discontinuity
	if (vecNewVelocity.Length() > 200)
	{
		VectorNormalize(vecNewVelocity);
		vecNewVelocity *= 200;
		VectorNormalize(vTargetDir);
		m_hSpotlightTarget->SetAbsOrigin( m_vSpotlightCurrentPos );
	}
	m_hSpotlightTarget->SetAbsVelocity( vecNewVelocity );
	m_hSpotlightTarget->m_vSpotlightOrg = GetAbsOrigin();

	// Avoid sudden change in where beam fades out when cross disconinuities
	VectorSubtract( m_hSpotlightTarget->GetAbsOrigin(), m_hSpotlightTarget->m_vSpotlightOrg, m_hSpotlightTarget->m_vSpotlightDir );
	float flBeamLength	= VectorNormalize( m_hSpotlightTarget->m_vSpotlightDir );
	m_flSpotlightCurLength = (0.60*m_flSpotlightCurLength) + (0.4*flBeamLength);

	ComputeRenderInfo();

	//NDebugOverlay::Cross3D(GetAbsOrigin(),Vector(-5,-5,-5),Vector(5,5,5),0,255,0,true,0.1);
	//NDebugOverlay::Cross3D(m_vSpotlightCurrentPos,Vector(-5,-5,-5),Vector(5,5,5),0,255,0,true,0.1);
	//NDebugOverlay::Cross3D(m_vSpotlightTargetPos,Vector(-5,-5,-5),Vector(5,5,5),255,0,0,true,0.1);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointSpotlight::InputLightOn( inputdata_t &inputdata )
{
	if ( !m_bSpotlightOn )
	{
		m_bSpotlightOn = true;
		if ( m_bEfficientSpotlight )
		{
			CreateEfficientSpotlight();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointSpotlight::InputLightOff( inputdata_t &inputdata )
{
	if ( m_bSpotlightOn )
	{
		m_bSpotlightOn = false;
		if ( m_bEfficientSpotlight )
		{
			SpotlightDestroy();
		}
	}
}
