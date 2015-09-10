//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "baseanimatedtextureproxy.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "tier0/minidump.h"
#include "tier0/stacktools.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );

//-----------------------------------------------------------------------------
// Constructor, destructor: 
//-----------------------------------------------------------------------------

CBaseAnimatedTextureProxy::CBaseAnimatedTextureProxy()
{
	Cleanup();
}

CBaseAnimatedTextureProxy::~CBaseAnimatedTextureProxy()
{
	Cleanup();
}


//-----------------------------------------------------------------------------
// Initialization, shutdown
//-----------------------------------------------------------------------------
bool CBaseAnimatedTextureProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	char const* pAnimatedTextureVarName = pKeyValues->GetString( "animatedTextureVar" );

	if( pAnimatedTextureVarName )
	{
		bool foundVar;

		m_AnimatedTextureVar = pMaterial->FindVar( pAnimatedTextureVarName, &foundVar, false );
		if( foundVar )
		{
			char const* pAnimatedTextureFrameNumVarName = pKeyValues->GetString( "animatedTextureFrameNumVar" );

			if( pAnimatedTextureFrameNumVarName )
			{
				m_AnimatedTextureFrameNumVar = pMaterial->FindVar( pAnimatedTextureFrameNumVarName, &foundVar, false );

				if( foundVar )
				{
					m_FrameRate = pKeyValues->GetFloat( "animatedTextureFrameRate", 15 );
					m_WrapAnimation = !pKeyValues->GetInt( "animationNoWrap", 0 );
					return true;
				}
			}
		}
	}

	// Error - null out pointers.
	Cleanup();
	return false;
}

void CBaseAnimatedTextureProxy::Cleanup()
{
	m_AnimatedTextureVar = NULL;
	m_AnimatedTextureFrameNumVar = NULL;
}


//-----------------------------------------------------------------------------
// Does the dirty deed
//-----------------------------------------------------------------------------
void CBaseAnimatedTextureProxy::OnBind( void *pEntity )
{
	Assert ( m_AnimatedTextureVar );

	if( m_AnimatedTextureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE )
	{
		return;
	}
	ITexture *pTexture;
	pTexture = m_AnimatedTextureVar->GetTextureValue();
	int numFrames = pTexture->GetNumAnimationFrames();

	if ( numFrames <= 0 )
	{
		Assert( !"0 frames in material calling animated texture proxy" );
		return;
	}

	// NOTE: Must not use relative time based methods here
	// because the bind proxy can be called many times per frame.
	// Prevent multiple Wrap callbacks to be sent for no wrap mode
	float startTime = GetAnimationStartTime(pEntity);
	float deltaTime = gpGlobals->curtime - startTime;
	float prevTime = deltaTime - gpGlobals->frametime;

	// Clamp..
	if (deltaTime < 0.0f)
		deltaTime = 0.0f;
	if (prevTime < 0.0f)
		prevTime = 0.0f;

	float frame = m_FrameRate * deltaTime;	
	float prevFrame = m_FrameRate * prevTime;

	int intFrame = ((int)frame) % numFrames; 
	int intPrevFrame = ((int)prevFrame) % numFrames;

	// Report wrap situation...
	if (intPrevFrame > intFrame)
	{
		if (m_WrapAnimation)
		{
			AnimationWrapped( pEntity );
		}
		else
		{
			// Only sent the wrapped message once.
			// when we're in non-wrapping mode
			if (prevFrame < numFrames)
				AnimationWrapped( pEntity );
			intFrame = numFrames - 1;
		}
	}

	m_AnimatedTextureFrameNumVar->SetIntValue( intFrame );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

IMaterial *CBaseAnimatedTextureProxy::GetMaterial()
{
	return m_AnimatedTextureVar->GetOwningMaterial();
}
