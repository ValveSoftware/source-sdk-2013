//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "model_types.h"
#include "clienteffectprecachesystem.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "beamdraw.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectCrossbow )
CLIENTEFFECT_MATERIAL( "effects/muzzleflash1" )
CLIENTEFFECT_REGISTER_END()

//
// Crossbow bolt
//

class C_CrossbowBolt : public C_BaseCombatCharacter
{
	DECLARE_CLASS( C_CrossbowBolt, C_BaseCombatCharacter );
	DECLARE_CLIENTCLASS();
public:
	
	C_CrossbowBolt( void );

	virtual RenderGroup_t GetRenderGroup( void )
	{
		// We want to draw translucent bits as well as our main model
		return RENDER_GROUP_TWOPASS;
	}

	virtual void	ClientThink( void );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual int		DrawModel( int flags );

private:

	C_CrossbowBolt( const C_CrossbowBolt & ); // not defined, not accessible

	Vector	m_vecLastOrigin;
	bool	m_bUpdated;
};

IMPLEMENT_CLIENTCLASS_DT( C_CrossbowBolt, DT_CrossbowBolt, CCrossbowBolt )
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_CrossbowBolt::C_CrossbowBolt( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_CrossbowBolt::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_bUpdated = false;
		m_vecLastOrigin = GetAbsOrigin();
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
// Output : int
//-----------------------------------------------------------------------------
int C_CrossbowBolt::DrawModel( int flags )
{
	// See if we're drawing the motion blur
	if ( flags & STUDIO_TRANSPARENCY )
	{
		float		color[3];
		IMaterial	*pBlurMaterial = materials->FindMaterial( "effects/muzzleflash1", NULL, false );

		Vector	vecDir = GetAbsOrigin() - m_vecLastOrigin;
		float	speed = VectorNormalize( vecDir );
		
		speed = clamp( speed, 0, 32 );
		
		if ( speed > 0 )
		{
			float	stepSize = MIN( ( speed * 0.5f ), 4.0f );

			Vector	spawnPos = GetAbsOrigin() + ( vecDir * 24.0f );
			Vector	spawnStep = -vecDir * stepSize;

			CMatRenderContextPtr pRenderContext( materials );
			pRenderContext->Bind( pBlurMaterial );

			float	alpha;

			// Draw the motion blurred trail
			for ( int i = 0; i < 20; i++ )
			{
				spawnPos += spawnStep;

				alpha = RemapValClamped( i, 5, 11, 0.25f, 0.05f );

				color[0] = color[1] = color[2] = alpha;

				DrawHalo( pBlurMaterial, spawnPos, 3.0f, color );
			}
		}

		if ( gpGlobals->frametime > 0.0f && !m_bUpdated)
		{
			m_bUpdated = true;
			m_vecLastOrigin = GetAbsOrigin();
		}

		return 1;
	}

	// Draw the normal portion
	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_CrossbowBolt::ClientThink( void )
{
	m_bUpdated = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CrosshairLoadCallback( const CEffectData &data )
{
	IClientRenderable *pRenderable = data.GetRenderable( );
	if ( !pRenderable )
		return;
	
	Vector	position;
	QAngle	angles;

	// If we found the attachment, emit sparks there
	if ( pRenderable->GetAttachment( data.m_nAttachmentIndex, position, angles ) )
	{
		FX_ElectricSpark( position, 1.0f, 1.0f, NULL );
	}
}

DECLARE_CLIENT_EFFECT( "CrossbowLoad", CrosshairLoadCallback );
