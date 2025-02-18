//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "particles_simple.h"
#include "tempent.h"
#include "iefx.h"
#include "decals.h"
#include "iviewrender.h"
#include "engine/ivmodelinfo.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	NUM_CHILD_FLAMES	6
#define	CHILD_SPREAD		40

//==================================================
// C_Plasma
//==================================================

//NOTENOTE: Mirrored in dlls/fire_smoke.h
#define	bitsFIRESMOKE_NONE				0x00000000
#define	bitsFIRESMOKE_ACTIVE			0x00000001


class C_PlasmaSprite : public C_Sprite
{
	DECLARE_CLASS( C_PlasmaSprite, C_Sprite );

public:
	Vector	m_vecMoveDir;
};


class C_Plasma : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_Plasma, C_BaseEntity );

	C_Plasma();
	~C_Plasma();

	void	AddEntity( void );

protected:
	void	Update( void );
	void	UpdateAnimation( void );
	void	UpdateScale( void );
	void	UpdateFlames( void );
	void	AddFlames( void );
	void	Start( void );
	
	float	GetFlickerScale( void );

//C_BaseEntity
public:

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw();

//From the server
public:
	float	m_flStartScale;
	float	m_flScale;
	float	m_flScaleTime;
	int		m_nFlags;
	int		m_nPlasmaModelIndex;
	int		m_nPlasmaModelIndex2;
	int		m_nGlowModelIndex;

//Client-side only
public:
	float	m_flScaleRegister;
	float	m_flScaleStart;
	float	m_flScaleEnd;
	float	m_flScaleTimeStart;
	float	m_flScaleTimeEnd;

	VPlane	m_planeClip;
	bool	m_bClipTested;

protected:
	C_PlasmaSprite		m_entFlames[NUM_CHILD_FLAMES];
	float				m_entFlameScales[NUM_CHILD_FLAMES];

	C_Sprite			m_entGlow;
	float				m_flGlowScale;

	TimedEvent			m_tParticleSpawn;
	TimedEvent			m_tDecalSpawn;

private:
	C_Plasma( const C_Plasma & );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pRecvProp - 
//			*pStruct - 
//			*pVarData - 
//			*pIn - 
//			objectID - 
//-----------------------------------------------------------------------------
void RecvProxy_PlasmaScale( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_Plasma	*pPlasmaSmoke	= (C_Plasma	*) pStruct;
	float scale				= pData->m_Value.m_Float;

	//If changed, update our internal information
	if ( pPlasmaSmoke->m_flScale != scale )
	{
		pPlasmaSmoke->m_flScaleStart	= pPlasmaSmoke->m_flScaleRegister;
		pPlasmaSmoke->m_flScaleEnd		= scale;			

		pPlasmaSmoke->m_flScale = scale;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pRecvProp - 
//			*pStruct - 
//			*pVarData - 
//			*pIn - 
//			objectID - 
//-----------------------------------------------------------------------------
void RecvProxy_PlasmaScaleTime( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_Plasma	*pPlasmaSmoke	= (C_Plasma	*) pStruct;
	float time				= pData->m_Value.m_Float;

	//If changed, update our internal information
	if ( pPlasmaSmoke->m_flScaleTime != time )
	{
		if ( time == -1.0f )
		{
			pPlasmaSmoke->m_flScaleTimeStart	= gpGlobals->curtime-1.0f;
			pPlasmaSmoke->m_flScaleTimeEnd	= pPlasmaSmoke->m_flScaleTimeStart;
		}
		else
		{
			pPlasmaSmoke->m_flScaleTimeStart	= gpGlobals->curtime;
			pPlasmaSmoke->m_flScaleTimeEnd	= gpGlobals->curtime + time;
		}

		pPlasmaSmoke->m_flScaleTime = time;
	}

	
}

//Receive datatable
IMPLEMENT_CLIENTCLASS_DT( C_Plasma, DT_Plasma, CPlasma )
	RecvPropFloat( RECVINFO( m_flStartScale )),
	RecvPropFloat( RECVINFO( m_flScale ), 0, RecvProxy_PlasmaScale ),
	RecvPropFloat( RECVINFO( m_flScaleTime ), 0, RecvProxy_PlasmaScaleTime ),
	RecvPropInt( RECVINFO( m_nFlags ) ),
	RecvPropInt( RECVINFO( m_nPlasmaModelIndex ) ),
	RecvPropInt( RECVINFO( m_nPlasmaModelIndex2 ) ),
	RecvPropInt( RECVINFO( m_nGlowModelIndex ) ),
END_RECV_TABLE()

//==================================================
// C_Plasma
//==================================================

C_Plasma::C_Plasma()
{
	//Server-side
	m_flStartScale		= 0.0f;
	m_flScale			= 0.0f;
	m_flScaleTime		= 0.0f;
	m_nFlags			= bitsFIRESMOKE_NONE;
	m_nPlasmaModelIndex	= 0;
	m_nPlasmaModelIndex2	= 0;
	m_nGlowModelIndex	= 0;

	//Client-side
	m_flScaleRegister	= 0.0f;
	m_flScaleStart		= 0.0f;
	m_flScaleEnd		= 0.0f;
	m_flScaleTimeStart	= 0.0f;
	m_flScaleTimeEnd	= 0.0f;
	m_flGlowScale		= 0.0f;
	m_bClipTested		= false;
	
	m_entGlow.Clear();
	
	//Clear all child flames
	for ( int i = 0; i < NUM_CHILD_FLAMES; i++ )
	{
		m_entFlames[i].Clear();
	}
}

C_Plasma::~C_Plasma()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float C_Plasma::GetFlickerScale( void )
{
	float	result = 0.0f;

	result = sin( gpGlobals->curtime * 10000.0f );
	result += 0.5f * sin( gpGlobals->curtime * 2000.0f );
	result -= 0.5f * cos( gpGlobals->curtime * 8000.0f );
	
	return result * 0.1f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Plasma::AddEntity( void )
{
	//Only do this if we're active
	if ( ( m_nFlags & bitsFIRESMOKE_ACTIVE ) == false )
		return;

	Update();
	AddFlames();

	float	dScale = m_flScaleRegister - m_flGlowScale;
	m_flGlowScale = m_flScaleRegister;

	// Note: Sprite renderer assumes scale of 0.0 is 1.0
	m_entGlow.SetScale( MAX( 0.0000001f, (m_flScaleRegister*1.5f) + GetFlickerScale() ) );
	m_entGlow.SetLocalOriginDim( Z_INDEX, m_entGlow.GetLocalOriginDim( Z_INDEX ) + ( dScale * 32.0f ) );
}

#define	FLAME_ALPHA_START	0.8f
#define FLAME_ALPHA_END		1.0f

#define	FLAME_TRANS_START	0.75f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Plasma::AddFlames( void )
{
	Vector	viewDir = GetAbsOrigin() - CurrentViewOrigin();
	VectorNormalize(viewDir);
	float	dot		= viewDir.Dot( Vector( 0, 0, 1 ) );	//NOTENOTE: Flames always point up
	float	alpha	= 1.0f;

	dot = fabs( dot );

	if ( dot < FLAME_ALPHA_START )
	{
		alpha = 1.0f;
	}

	for ( int i = 0; i < NUM_CHILD_FLAMES; i++ )
	{
		if ( m_entFlames[i].GetScale() > 0.0f )
		{
			m_entFlames[i].SetRenderColor( ( 255.0f * alpha ), ( 255.0f * alpha ), ( 255.0f * alpha ) );
			m_entFlames[i].SetBrightness( 255.0f * alpha );
		}

		m_entFlames[i].AddToLeafSystem();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_Plasma::OnDataChanged( DataUpdateType_t updateType )
{
	C_BaseEntity::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_Plasma::ShouldDraw()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Plasma::Start( void )
{
	//Various setup info
	m_tParticleSpawn.Init( 10.0f );
	m_tDecalSpawn.Init( 20.0f);

	QAngle	offset;
	int		maxFrames;

	// Setup the child flames
	int i;
	for ( i = 0; i < NUM_CHILD_FLAMES; i++ )
	{
		//Setup our offset angles
		offset[0] = 0.0f;
		offset[1] = random->RandomFloat( 0, 360 );
		offset[2] = 0.0f;
	
  		AngleVectors( offset, &m_entFlames[i].m_vecMoveDir );
		
		int	nModelIndex = ( i % 2 ) ? m_nPlasmaModelIndex : m_nPlasmaModelIndex2;

		model_t *pModel	= (model_t *) modelinfo->GetModel( nModelIndex );
		maxFrames	= modelinfo->GetModelFrameCount( pModel );

		// Setup all the information for the client entity
		m_entFlames[i].SetModelByIndex( nModelIndex );
		m_entFlames[i].SetLocalOrigin( GetLocalOrigin() );
		m_entFlames[i].m_flFrame			= random->RandomInt( 0.0f, maxFrames );
		m_entFlames[i].m_flSpriteFramerate	= (float) random->RandomInt( 15, 20 );
		m_entFlames[i].SetScale( m_flStartScale );
		m_entFlames[i].SetRenderMode( kRenderTransAddFrameBlend );
		m_entFlames[i].m_nRenderFX			= kRenderFxNone;
		m_entFlames[i].SetRenderColor( 255, 255, 255, 255 );
		m_entFlames[i].SetBrightness( 255 );
		m_entFlames[i].index				= -1;
		
		if ( i == 0 )
		{
			m_entFlameScales[i] = 1.0f;
		}
		else
		{
			//Keep a scale offset
			m_entFlameScales[i] = 1.0f - ( ( (float) i / (float) NUM_CHILD_FLAMES ) );
		}
	}

	// Setup the glow
	m_entGlow.SetModelByIndex( m_nGlowModelIndex );
	m_entGlow.SetLocalOrigin( GetLocalOrigin() );
	m_entGlow.SetScale( m_flStartScale );
	m_entGlow.SetRenderMode( kRenderTransAdd );
	m_entGlow.m_nRenderFX		= kRenderFxNone;
	m_entGlow.SetRenderColor( 255, 255, 255, 255 );
	m_entGlow.SetBrightness( 255 );
	m_entGlow.index				= -1;
	
	m_flGlowScale				= m_flStartScale;

	m_entGlow.AddToLeafSystem( RENDER_GROUP_TRANSLUCENT_ENTITY );

	for( i=0; i < NUM_CHILD_FLAMES; i++ )
		m_entFlames[i].AddToLeafSystem( RENDER_GROUP_TRANSLUCENT_ENTITY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Plasma::UpdateAnimation( void )
{
	int		numFrames;
	float	frametime	= gpGlobals->frametime;

	for ( int i = 0; i < NUM_CHILD_FLAMES; i++ )
	{
		m_entFlames[i].m_flFrame += m_entFlames[i].m_flSpriteFramerate * frametime;

		numFrames = modelinfo->GetModelFrameCount( m_entFlames[i].GetModel() );

		if ( m_entFlames[i].m_flFrame >= numFrames )
		{
			m_entFlames[i].m_flFrame = m_entFlames[i].m_flFrame - (int)(m_entFlames[i].m_flFrame);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Plasma::UpdateFlames( void )
{
	for ( int i = 0; i < NUM_CHILD_FLAMES; i++ )
	{
		float	newScale = m_flScaleRegister * m_entFlameScales[i];
		float	dScale = newScale - m_entFlames[i].GetScale();

		Vector dir;

		dir[2] = 0.0f;
		VectorNormalize( dir );
		dir[2] = 0.0f;

		Vector	offset = GetAbsOrigin();
		offset[2] = m_entFlames[i].GetAbsOrigin()[2];

		// Note: Sprite render assumes 0 scale means 1.0
		m_entFlames[i].SetScale ( MAX(0.000001,newScale) );
		
		if ( i != 0 )
		{
			m_entFlames[i].SetLocalOrigin( offset + ( m_entFlames[i].m_vecMoveDir * ((m_entFlames[i].GetScale())*CHILD_SPREAD) ) );
		}

		Assert( !m_entFlames[i].GetMoveParent() );
		m_entFlames[i].SetLocalOriginDim( Z_INDEX, m_entFlames[i].GetLocalOriginDim( Z_INDEX ) + ( dScale * 64.0f ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Plasma::UpdateScale( void )
{
	float	time = gpGlobals->curtime;

	if ( m_flScaleRegister != m_flScaleEnd )
	{
		//See if we're done scaling
		if ( time > m_flScaleTimeEnd )
		{
			m_flScaleRegister = m_flStartScale = m_flScaleEnd;
		}
		else
		{
			//Lerp the scale and set it
			float	timeFraction = 1.0f - ( m_flScaleTimeEnd - time ) / ( m_flScaleTimeEnd - m_flScaleTimeStart );	
			float	newScale = m_flScaleStart + ( ( m_flScaleEnd - m_flScaleStart ) * timeFraction );

			m_flScaleRegister = m_flStartScale = newScale;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_Plasma::Update( void )
{
	//Update all our parts
	UpdateScale();
	UpdateAnimation();
	UpdateFlames();

	if (m_flScaleRegister > 0.1)
	{
		float tempDelta = gpGlobals->frametime;
		while( m_tDecalSpawn.NextEvent( tempDelta ) )
		{
			// Add decal to floor
			C_BaseEntity *ent = cl_entitylist->GetEnt( 0 );
			if ( ent )
			{
				int iDecal = decalsystem->GetDecalIndexForName( "PlasmaGlowFade" );
				if ( iDecal >= 0 )
				{
					effects->DecalShoot( iDecal, 0, ent->GetModel(), ent->GetAbsOrigin(), ent->GetAbsAngles(), GetAbsOrigin(), 0, 0 );
				}
			}
		}
	}
}

