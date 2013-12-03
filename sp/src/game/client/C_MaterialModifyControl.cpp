//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Material Modify control entity.
//
//=============================================================================//

#include "cbase.h"
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "iviewrender.h"
#include "texture_group_names.h"
#include "baseanimatedtextureproxy.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MATERIAL_MODIFY_STRING_SIZE			255
#define MATERIAL_MODIFY_ANIMATION_UNSET		-1

// Must match MaterialModifyControl.cpp
enum MaterialModifyMode_t
{
	MATERIAL_MODIFY_MODE_NONE = 0,
	MATERIAL_MODIFY_MODE_SETVAR = 1,
	MATERIAL_MODIFY_MODE_ANIM_SEQUENCE = 2,
	MATERIAL_MODIFY_MODE_FLOAT_LERP = 3,
};

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );

ConVar debug_materialmodifycontrol_client( "debug_materialmodifycontrol_client", "0" );

struct materialanimcommands_t
{
	int iFrameStart;
	int iFrameEnd;
	bool bWrap;
	float flFrameRate;
};

struct materialfloatlerpcommands_t
{
	int flStartValue;
	int flEndValue;
	float flTransitionTime;
};

//------------------------------------------------------------------------------
// FIXME: This really should inherit from something	more lightweight
//------------------------------------------------------------------------------

class C_MaterialModifyControl : public C_BaseEntity
{
public:

	DECLARE_CLASS( C_MaterialModifyControl, C_BaseEntity );

	C_MaterialModifyControl();

	void OnPreDataChanged( DataUpdateType_t updateType );
	void OnDataChanged( DataUpdateType_t updateType );
	bool ShouldDraw();

	IMaterial *GetMaterial( void )					{ return m_pMaterial; }
	const char *GetMaterialVariableName( void )		{ return m_szMaterialVar; }
	const char *GetMaterialVariableValue( void )	{ return m_szMaterialVarValue; }

	DECLARE_CLIENTCLASS();

	// Animated texture and Float Lerp usage
	bool	HasNewAnimationCommands( void )			{ return m_bHasNewAnimationCommands; }
	void	ClearAnimationCommands( void )			{ m_bHasNewAnimationCommands = false; }

	// Animated texture usage
	void	GetAnimationCommands( materialanimcommands_t *pCommands );

	// FloatLerp usage
	void	GetFloatLerpCommands( materialfloatlerpcommands_t *pCommands );

	void	SetAnimationStartTime( float flTime )
	{
		m_flAnimationStartTime = flTime;
	}
	float	GetAnimationStartTime( void ) const
	{
		return m_flAnimationStartTime;
	}

	MaterialModifyMode_t GetModifyMode( void ) const
	{
		return ( MaterialModifyMode_t)m_nModifyMode;
	}
private:

	char m_szMaterialName[MATERIAL_MODIFY_STRING_SIZE];
	char m_szMaterialVar[MATERIAL_MODIFY_STRING_SIZE];
	char m_szMaterialVarValue[MATERIAL_MODIFY_STRING_SIZE];
	IMaterial		*m_pMaterial;

	bool	m_bHasNewAnimationCommands;

	// Animation commands from the server
	int		m_iFrameStart;
	int		m_iFrameEnd;
	bool	m_bWrap;
	float	m_flFramerate;
	bool	m_bNewAnimCommandsSemaphore;
	bool	m_bOldAnimCommandsSemaphore;

	// Float lerp commands from the server
	float	m_flFloatLerpStartValue;
	float	m_flFloatLerpEndValue;
	float	m_flFloatLerpTransitionTime;
	bool	m_bFloatLerpWrap;
	float	m_flAnimationStartTime;

	int		m_nModifyMode;
};

IMPLEMENT_CLIENTCLASS_DT(C_MaterialModifyControl, DT_MaterialModifyControl, CMaterialModifyControl)
	RecvPropString( RECVINFO( m_szMaterialName ) ),
	RecvPropString( RECVINFO( m_szMaterialVar ) ),
	RecvPropString( RECVINFO( m_szMaterialVarValue ) ),
	RecvPropInt( RECVINFO(m_iFrameStart) ),
	RecvPropInt( RECVINFO(m_iFrameEnd) ),
	RecvPropInt( RECVINFO(m_bWrap) ),
	RecvPropFloat( RECVINFO(m_flFramerate) ),
	RecvPropInt( RECVINFO(m_bNewAnimCommandsSemaphore) ),
	RecvPropFloat( RECVINFO(m_flFloatLerpStartValue) ),
	RecvPropFloat( RECVINFO(m_flFloatLerpEndValue) ),
	RecvPropFloat( RECVINFO(m_flFloatLerpTransitionTime) ),
	RecvPropInt( RECVINFO(m_bFloatLerpWrap) ),
	RecvPropInt( RECVINFO(m_nModifyMode) ),
END_RECV_TABLE()

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
C_MaterialModifyControl::C_MaterialModifyControl()
{
	m_pMaterial = NULL;
	m_bOldAnimCommandsSemaphore = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_MaterialModifyControl::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bOldAnimCommandsSemaphore = m_bNewAnimCommandsSemaphore;
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void C_MaterialModifyControl::OnDataChanged( DataUpdateType_t updateType )
{
	if( updateType == DATA_UPDATE_CREATED )
	{
		m_pMaterial = materials->FindMaterial( m_szMaterialName, TEXTURE_GROUP_OTHER );

		// Clear out our variables
		m_bHasNewAnimationCommands = true;
	}

	// Detect changes in the anim commands
	if ( m_bNewAnimCommandsSemaphore != m_bOldAnimCommandsSemaphore )
	{
		m_bHasNewAnimationCommands = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_MaterialModifyControl::GetAnimationCommands( materialanimcommands_t *pCommands )
{
	pCommands->iFrameStart = m_iFrameStart;
	pCommands->iFrameEnd = m_iFrameEnd;
	pCommands->bWrap = m_bWrap;
	pCommands->flFrameRate = m_flFramerate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_MaterialModifyControl::GetFloatLerpCommands( materialfloatlerpcommands_t *pCommands )
{
	pCommands->flStartValue = m_flFloatLerpStartValue;
	pCommands->flEndValue = m_flFloatLerpEndValue;
	pCommands->flTransitionTime = m_flFloatLerpTransitionTime;
}

//------------------------------------------------------------------------------
// Purpose: We don't draw.
//------------------------------------------------------------------------------
bool C_MaterialModifyControl::ShouldDraw()
{
	return false;
}

//=============================================================================
//
// THE MATERIALMODIFYPROXY ITSELF
//
class CMaterialModifyProxy : public CBaseAnimatedTextureProxy
{
public:
	CMaterialModifyProxy();
	virtual ~CMaterialModifyProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pEntity );
	virtual IMaterial *GetMaterial();

private:
	void OnBindSetVar( C_MaterialModifyControl *pControl );
	void OnBindAnimatedTexture( C_MaterialModifyControl *pControl );
	void OnBindFloatLerp( C_MaterialModifyControl *pControl );
	float GetAnimationStartTime( void* pArg );
	void AnimationWrapped( void* pArg );

	IMaterial	*m_pMaterial;
	
	// texture animation stuff
	int			m_iFrameStart;
	int			m_iFrameEnd;
	bool		m_bReachedEnd;
	bool		m_bCustomWrap;
	float		m_flCustomFramerate;

	// float lerp stuff
	IMaterialVar *m_pMaterialVar;
	int			m_flStartValue;
	int			m_flEndValue;
	float		m_flStartTime;
	float		m_flTransitionTime;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CMaterialModifyProxy::CMaterialModifyProxy()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CMaterialModifyProxy::~CMaterialModifyProxy()
{
}

bool CMaterialModifyProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// set var stuff
	m_pMaterial = pMaterial;

	// float lerp stuff
	m_flStartValue = MATERIAL_MODIFY_ANIMATION_UNSET;
	m_flEndValue = MATERIAL_MODIFY_ANIMATION_UNSET;

	// animated stuff
//	m_pMaterial = pMaterial;
//	m_iFrameStart = MATERIAL_MODIFY_ANIMATION_UNSET;
//	m_iFrameEnd = MATERIAL_MODIFY_ANIMATION_UNSET;
//	m_bReachedEnd = false;
//	return CBaseAnimatedTextureProxy::Init( pMaterial, pKeyValues );

	return true;
}

void CMaterialModifyProxy::OnBind( void *pEntity )
{
	// Get the modified material vars from the entity input
	IClientRenderable *pRend = (IClientRenderable *)pEntity;
	if ( pRend )
	{
		C_BaseEntity *pBaseEntity = pRend->GetIClientUnknown()->GetBaseEntity();
		
		if ( pBaseEntity )
		{
			if( debug_materialmodifycontrol_client.GetBool() )
			{
//				DevMsg( 1, "%s\n", pBaseEntity->GetDebugName() );
			}
			int numChildren = 0;
			bool gotOne = false;
			for ( C_BaseEntity *pChild = pBaseEntity->FirstMoveChild(); pChild; pChild = pChild->NextMovePeer() )
			{
				numChildren++;
				C_MaterialModifyControl *pControl = dynamic_cast<C_MaterialModifyControl*>( pChild );
				if ( !pControl )
					continue;

				if( debug_materialmodifycontrol_client.GetBool() )
				{
//					DevMsg( 1, "pControl: 0x%p\n", pControl );
				}
				
				switch( pControl->GetModifyMode() )
				{
				case MATERIAL_MODIFY_MODE_NONE:
					break;
				case MATERIAL_MODIFY_MODE_SETVAR:
					gotOne = true;
					OnBindSetVar( pControl );
					break;
				case MATERIAL_MODIFY_MODE_ANIM_SEQUENCE:
					OnBindAnimatedTexture( pControl );
					break;
				case MATERIAL_MODIFY_MODE_FLOAT_LERP:
					OnBindFloatLerp( pControl );
					break;
				default:
					Assert( 0 );
					break;
				}
			}
			if( gotOne )
			{
//				DevMsg( 1, "numChildren: %d\n", numChildren );
			}
		}
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

IMaterial *CMaterialModifyProxy::GetMaterial()
{
	return m_pMaterial;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMaterialModifyProxy::OnBindSetVar( C_MaterialModifyControl *pControl )
{
	IMaterial *pMaterial = pControl->GetMaterial();
	if( !pMaterial )
	{
		Assert( 0 );
		return;
	}

	if ( pMaterial != m_pMaterial )
	{
//		Warning( "\t%s!=%s\n", pMaterial->GetName(), m_pMaterial->GetName() );
		return;
	}

	bool bFound;
	IMaterialVar *pMaterialVar = pMaterial->FindVar( pControl->GetMaterialVariableName(), &bFound, false );
	if ( !bFound )
		return;

	if( Q_strcmp( pControl->GetMaterialVariableValue(), "" ) )
	{
//		const char *pMaterialName = m_pMaterial->GetName();
//		const char *pMaterialVarName = pMaterialVar->GetName();
//		const char *pMaterialVarValue = pControl->GetMaterialVariableValue();
//		if( debug_materialmodifycontrol_client.GetBool() 
//			&& Q_stristr( m_pMaterial->GetName(), "faceandhair" )
//			&& Q_stristr( pMaterialVar->GetName(), "self" )
//			)
//		{
//			static int count = 0;
//			DevMsg( 1, "CMaterialModifyProxy::OnBindSetVar \"%s\" %s=%s %d pControl=0x%p\n", 
//				m_pMaterial->GetName(), pMaterialVar->GetName(), pControl->GetMaterialVariableValue(), count++, pControl );
//		}
		pMaterialVar->SetValueAutodetectType( pControl->GetMaterialVariableValue() );
	}
}


//-----------------------------------------------------------------------------
// Does the dirty deed
//-----------------------------------------------------------------------------
void CMaterialModifyProxy::OnBindAnimatedTexture( C_MaterialModifyControl *pControl )
{
	assert ( m_AnimatedTextureVar );
	if( m_AnimatedTextureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE )
		return;

	ITexture *pTexture;
	pTexture = m_AnimatedTextureVar->GetTextureValue();

	if ( !pControl )
		return;

	if ( pControl->HasNewAnimationCommands() )
	{
		// Read the data from the modify entity
		materialanimcommands_t sCommands;
		pControl->GetAnimationCommands( &sCommands );

		m_iFrameStart = sCommands.iFrameStart;
		m_iFrameEnd = sCommands.iFrameEnd;
		m_bCustomWrap = sCommands.bWrap;
		m_flCustomFramerate = sCommands.flFrameRate;
		m_bReachedEnd = false;

		m_flStartTime = gpGlobals->curtime;

		pControl->ClearAnimationCommands();
	}

	// Init all the vars based on whether we're using the base material settings, 
	// or the custom ones from the entity input.
	int numFrames;
	bool bWrapAnimation;
	float flFrameRate;
	int iLastFrame;

	// Do we have a custom frame section from the server?
	if ( m_iFrameStart != MATERIAL_MODIFY_ANIMATION_UNSET )
	{
		if ( m_iFrameEnd == MATERIAL_MODIFY_ANIMATION_UNSET )
		{
			m_iFrameEnd = pTexture->GetNumAnimationFrames();
		}

		numFrames = (m_iFrameEnd - m_iFrameStart) + 1;
		bWrapAnimation = m_bCustomWrap;
		flFrameRate = m_flCustomFramerate;
		iLastFrame = (m_iFrameEnd - 1);
	}
	else
	{
		numFrames = pTexture->GetNumAnimationFrames();
		bWrapAnimation = m_WrapAnimation;
		flFrameRate = m_FrameRate;
		iLastFrame = (numFrames - 1);
	}

	// Have we already reached the end? If so, stay there.
	if ( m_bReachedEnd && !bWrapAnimation )
	{
		m_AnimatedTextureFrameNumVar->SetIntValue( iLastFrame );
		return;
	}

	// NOTE: Must not use relative time based methods here
	// because the bind proxy can be called many times per frame.
	// Prevent multiple Wrap callbacks to be sent for no wrap mode
	float startTime;
	if ( m_iFrameStart != MATERIAL_MODIFY_ANIMATION_UNSET )
	{
		startTime = m_flStartTime;
	}
	else
	{
		startTime = GetAnimationStartTime(pControl);
	}
	float deltaTime = gpGlobals->curtime - startTime;
	float prevTime = deltaTime - gpGlobals->frametime;

	// Clamp..
	if (deltaTime < 0.0f)
		deltaTime = 0.0f;
	if (prevTime < 0.0f)
		prevTime = 0.0f;

	float frame = flFrameRate * deltaTime;	
	float prevFrame = flFrameRate * prevTime;

	int intFrame = ((int)frame) % numFrames; 
	int intPrevFrame = ((int)prevFrame) % numFrames;

	if ( m_iFrameStart != MATERIAL_MODIFY_ANIMATION_UNSET )
	{
		intFrame += m_iFrameStart;
		intPrevFrame += m_iFrameStart;
	}

	// Report wrap situation...
	if (intPrevFrame > intFrame)
	{
		m_bReachedEnd = true;

		if (bWrapAnimation)
		{
			AnimationWrapped( pControl );
		}
		else
		{
			// Only sent the wrapped message once.
			// when we're in non-wrapping mode
			if (prevFrame < numFrames)
				AnimationWrapped( pControl );
			intFrame = numFrames - 1;
		}
	}

	m_AnimatedTextureFrameNumVar->SetIntValue( intFrame );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CMaterialModifyProxy::GetAnimationStartTime( void* pArg )
{
	IClientRenderable *pRend = (IClientRenderable *)pArg;
	if (!pRend)
		return 0.0f;

	C_BaseEntity* pEntity = pRend->GetIClientUnknown()->GetBaseEntity();
	if (pEntity)
	{
		return pEntity->GetTextureAnimationStartTime();
	}
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMaterialModifyProxy::AnimationWrapped( void* pArg )
{
	IClientRenderable *pRend = (IClientRenderable *)pArg;
	if (!pRend)
		return;

	C_BaseEntity* pEntity = pRend->GetIClientUnknown()->GetBaseEntity();
	if (pEntity)
	{
		pEntity->TextureAnimationWrapped();
	}
}

//-----------------------------------------------------------------------------
// Does the dirty deed
//-----------------------------------------------------------------------------
void CMaterialModifyProxy::OnBindFloatLerp( C_MaterialModifyControl *pControl )
{
	if ( !pControl )
		return;

	if ( pControl->HasNewAnimationCommands() )
	{
		pControl->SetAnimationStartTime( gpGlobals->curtime );
		pControl->ClearAnimationCommands();
	}

	// Read the data from the modify entity
	materialfloatlerpcommands_t sCommands;
	pControl->GetFloatLerpCommands( &sCommands );

	m_flStartValue = sCommands.flStartValue;
	m_flEndValue = sCommands.flEndValue;
	m_flTransitionTime = sCommands.flTransitionTime;
	m_flStartTime = pControl->GetAnimationStartTime();
	bool bFound;
	m_pMaterialVar = m_pMaterial->FindVar( pControl->GetMaterialVariableName(), &bFound, false );

	if( bFound )
	{
		float currentValue;
		if( m_flTransitionTime > 0.0f )
		{
			currentValue = m_flStartValue + ( m_flEndValue - m_flStartValue ) * clamp( ( ( gpGlobals->curtime - m_flStartTime ) / m_flTransitionTime ), 0.0f, 1.0f );
		}
		else
		{
			currentValue = m_flEndValue;
		}

		if( debug_materialmodifycontrol_client.GetBool() && Q_stristr( m_pMaterial->GetName(), "faceandhair" ) && Q_stristr( m_pMaterialVar->GetName(), "warp" ) )
		{
			static int count = 0;
			DevMsg( 1, "CMaterialFloatLerpProxy::OnBind \"%s\" %s=%f %d\n", m_pMaterial->GetName(), m_pMaterialVar->GetName(), currentValue, count++ );
		}
		m_pMaterialVar->SetFloatValue( currentValue );
	}
}

//=============================================================================
//
// MATERIALMODIFYANIMATED PROXY 
//
class CMaterialModifyAnimatedProxy : public CBaseAnimatedTextureProxy
{
public:
	CMaterialModifyAnimatedProxy() {};
	virtual ~CMaterialModifyAnimatedProxy() {};
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pEntity );

	virtual float GetAnimationStartTime( void* pBaseEntity );
	virtual void AnimationWrapped( void* pC_BaseEntity );

private:
	IMaterial	*m_pMaterial;
	int			m_iFrameStart;
	int			m_iFrameEnd;
	bool		m_bReachedEnd;
	float		m_flStartTime;
	bool		m_bCustomWrap;
	float		m_flCustomFramerate;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMaterialModifyAnimatedProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	m_pMaterial = pMaterial;
	m_iFrameStart = MATERIAL_MODIFY_ANIMATION_UNSET;
	m_iFrameEnd = MATERIAL_MODIFY_ANIMATION_UNSET;
	m_bReachedEnd = false;
	return CBaseAnimatedTextureProxy::Init( pMaterial, pKeyValues );
}

//-----------------------------------------------------------------------------
// Does the dirty deed
//-----------------------------------------------------------------------------
void CMaterialModifyAnimatedProxy::OnBind( void *pEntity )
{
	assert ( m_AnimatedTextureVar );
	if( m_AnimatedTextureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE )
		return;

	ITexture *pTexture;
	pTexture = m_AnimatedTextureVar->GetTextureValue();

	// Get the modified material vars from the entity input
	IClientRenderable *pRend = (IClientRenderable *)pEntity;
	if ( pRend )
	{
		C_BaseEntity *pBaseEntity = pRend->GetIClientUnknown()->GetBaseEntity();
		if ( pBaseEntity )
		{
			for ( C_BaseEntity *pChild = pBaseEntity->FirstMoveChild(); pChild; pChild = pChild->NextMovePeer() )
			{
				C_MaterialModifyControl *pControl = dynamic_cast<C_MaterialModifyControl*>( pChild );
				if ( !pControl )
					continue;

				if ( !pControl->HasNewAnimationCommands() )
					continue;

				// Read the data from the modify entity
				materialanimcommands_t sCommands;
				pControl->GetAnimationCommands( &sCommands );

				m_iFrameStart = sCommands.iFrameStart;
				m_iFrameEnd = sCommands.iFrameEnd;
				m_bCustomWrap = sCommands.bWrap;
				m_flCustomFramerate = sCommands.flFrameRate;
				m_bReachedEnd = false;

				m_flStartTime = gpGlobals->curtime;

				pControl->ClearAnimationCommands();
			}
		}
	}

	// Init all the vars based on whether we're using the base material settings, 
	// or the custom ones from the entity input.
	int numFrames;
	bool bWrapAnimation;
	float flFrameRate;
	int iLastFrame;

	// Do we have a custom frame section from the server?
	if ( m_iFrameStart != MATERIAL_MODIFY_ANIMATION_UNSET )
	{
		if ( m_iFrameEnd == MATERIAL_MODIFY_ANIMATION_UNSET )
		{
			m_iFrameEnd = pTexture->GetNumAnimationFrames();
		}

		numFrames = (m_iFrameEnd - m_iFrameStart) + 1;
		bWrapAnimation = m_bCustomWrap;
		flFrameRate = m_flCustomFramerate;
		iLastFrame = (m_iFrameEnd - 1);
	}
	else
	{
		numFrames = pTexture->GetNumAnimationFrames();
		bWrapAnimation = m_WrapAnimation;
		flFrameRate = m_FrameRate;
		iLastFrame = (numFrames - 1);
	}

	// Have we already reached the end? If so, stay there.
	if ( m_bReachedEnd && !bWrapAnimation )
	{
		m_AnimatedTextureFrameNumVar->SetIntValue( iLastFrame );
		return;
	}

	// NOTE: Must not use relative time based methods here
	// because the bind proxy can be called many times per frame.
	// Prevent multiple Wrap callbacks to be sent for no wrap mode
	float startTime;
	if ( m_iFrameStart != MATERIAL_MODIFY_ANIMATION_UNSET )
	{
		startTime = m_flStartTime;
	}
	else
	{
		startTime = GetAnimationStartTime(pEntity);
	}
	float deltaTime = gpGlobals->curtime - startTime;
	float prevTime = deltaTime - gpGlobals->frametime;

	// Clamp..
	if (deltaTime < 0.0f)
		deltaTime = 0.0f;
	if (prevTime < 0.0f)
		prevTime = 0.0f;

	float frame = flFrameRate * deltaTime;	
	float prevFrame = flFrameRate * prevTime;

	int intFrame = ((int)frame) % numFrames; 
	int intPrevFrame = ((int)prevFrame) % numFrames;

	if ( m_iFrameStart != MATERIAL_MODIFY_ANIMATION_UNSET )
	{
		intFrame += m_iFrameStart;
		intPrevFrame += m_iFrameStart;
	}

	// Report wrap situation...
	if (intPrevFrame > intFrame)
	{
		m_bReachedEnd = true;

		if (bWrapAnimation)
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CMaterialModifyAnimatedProxy::GetAnimationStartTime( void* pArg )
{
	IClientRenderable *pRend = (IClientRenderable *)pArg;
	if (!pRend)
		return 0.0f;

	C_BaseEntity* pEntity = pRend->GetIClientUnknown()->GetBaseEntity();
	if (pEntity)
	{
		return pEntity->GetTextureAnimationStartTime();
	}
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMaterialModifyAnimatedProxy::AnimationWrapped( void* pArg )
{
	IClientRenderable *pRend = (IClientRenderable *)pArg;
	if (!pRend)
		return;

	C_BaseEntity* pEntity = pRend->GetIClientUnknown()->GetBaseEntity();
	if (pEntity)
	{
		pEntity->TextureAnimationWrapped();
	}
}


EXPOSE_INTERFACE( CMaterialModifyProxy, IMaterialProxy, "MaterialModify" IMATERIAL_PROXY_INTERFACE_VERSION );
EXPOSE_INTERFACE( CMaterialModifyAnimatedProxy, IMaterialProxy, "MaterialModifyAnimated" IMATERIAL_PROXY_INTERFACE_VERSION );
