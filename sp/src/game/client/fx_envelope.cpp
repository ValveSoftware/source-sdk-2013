//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "fx_envelope.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_EnvelopeFX::C_EnvelopeFX( void )
{
	m_active = false;
}

C_EnvelopeFX::~C_EnvelopeFX()
{
	RemoveRenderable();
}

const matrix3x4_t & C_EnvelopeFX::RenderableToWorldTransform()
{
	static matrix3x4_t mat;
	SetIdentityMatrix( mat );
	PositionMatrix( GetRenderOrigin(), mat );
	return mat;
}

void C_EnvelopeFX::RemoveRenderable()
{
	ClientLeafSystem()->RemoveRenderable( m_hRenderHandle );
}

//-----------------------------------------------------------------------------
// Purpose: Updates the envelope being in the client's known entity list
//-----------------------------------------------------------------------------
void C_EnvelopeFX::Update( void )
{
	if ( m_active )
	{
		if ( m_hRenderHandle == INVALID_CLIENT_RENDER_HANDLE )
		{
			ClientLeafSystem()->AddRenderable( this, RENDER_GROUP_TRANSLUCENT_ENTITY );
		}
		else
		{
			ClientLeafSystem()->RenderableChanged( m_hRenderHandle );
		}
	}
	else
	{
		RemoveRenderable();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Starts up the effect
// Input  : entityIndex - entity to be attached to
//			attachment - attachment point (if any) to be attached to
//-----------------------------------------------------------------------------
void C_EnvelopeFX::EffectInit( int entityIndex, int attachment )
{
	m_entityIndex = entityIndex;
	m_attachment = attachment;

	m_active = 1;
	m_t = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Shuts down the effect
//-----------------------------------------------------------------------------
void C_EnvelopeFX::EffectShutdown( void ) 
{
	m_active = 0;
	m_t = 0;
}
