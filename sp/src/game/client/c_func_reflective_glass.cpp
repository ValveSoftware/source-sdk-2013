//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "view_shared.h"
#ifdef MAPBASE
#include "viewrender.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_FuncReflectiveGlass : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_FuncReflectiveGlass, C_BaseEntity );
	DECLARE_CLIENTCLASS();

// C_BaseEntity.
public:
	C_FuncReflectiveGlass();
	virtual ~C_FuncReflectiveGlass();

	virtual bool	ShouldDraw();

#ifdef MAPBASE
	virtual void	OnDataChanged( DataUpdateType_t type );
	ITexture		*ReflectionRenderTarget();
	ITexture		*RefractionRenderTarget();

	char m_iszReflectRenderTarget[64];
	char m_iszRefractRenderTarget[64];
	ITexture *m_pReflectRenderTarget;
	ITexture *m_pRefractRenderTarget;
#endif

	C_FuncReflectiveGlass	*m_pNext;
};

IMPLEMENT_CLIENTCLASS_DT( C_FuncReflectiveGlass, DT_FuncReflectiveGlass, CFuncReflectiveGlass )

#ifdef MAPBASE
	RecvPropString( RECVINFO( m_iszReflectRenderTarget ) ),
	RecvPropString( RECVINFO( m_iszRefractRenderTarget ) ),
#endif

END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
C_EntityClassList<C_FuncReflectiveGlass> g_ReflectiveGlassList;
template<> C_FuncReflectiveGlass *C_EntityClassList<C_FuncReflectiveGlass>::m_pClassList = NULL;

C_FuncReflectiveGlass* GetReflectiveGlassList()
{
	return g_ReflectiveGlassList.m_pClassList;
}


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
C_FuncReflectiveGlass::C_FuncReflectiveGlass()
{
#ifdef MAPBASE
	m_iszReflectRenderTarget[0] = '\0';
	m_iszRefractRenderTarget[0] = '\0';
#endif

	g_ReflectiveGlassList.Insert( this );
}

C_FuncReflectiveGlass::~C_FuncReflectiveGlass()
{
	g_ReflectiveGlassList.Remove( this );
}


bool C_FuncReflectiveGlass::ShouldDraw()
{
	return true;
}


//-----------------------------------------------------------------------------
// Do we have reflective glass in view?
//-----------------------------------------------------------------------------
bool IsReflectiveGlassInView( const CViewSetup& view, cplane_t &plane )
{
	// Early out if no cameras
	C_FuncReflectiveGlass *pReflectiveGlass = GetReflectiveGlassList();
	if ( !pReflectiveGlass )
		return false;

	Frustum_t frustum;
	GeneratePerspectiveFrustum( view.origin, view.angles, view.zNear, view.zFar, view.fov, view.m_flAspectRatio, frustum );

	cplane_t localPlane;
	Vector vecOrigin, vecWorld, vecDelta, vecForward;
	AngleVectors( view.angles, &vecForward, NULL, NULL );

	for ( ; pReflectiveGlass != NULL; pReflectiveGlass = pReflectiveGlass->m_pNext )
	{
		if ( pReflectiveGlass->IsDormant() )
			continue;

		Vector vecMins, vecMaxs;
		pReflectiveGlass->GetRenderBoundsWorldspace( vecMins, vecMaxs );
		if ( R_CullBox( vecMins, vecMaxs, frustum ) )
			continue;

		const model_t *pModel = pReflectiveGlass->GetModel();
		const matrix3x4_t& mat = pReflectiveGlass->EntityToWorldTransform();

		int nCount = modelinfo->GetBrushModelPlaneCount( pModel );
		for ( int i = 0; i < nCount; ++i )
		{
			modelinfo->GetBrushModelPlane( pModel, i, localPlane, &vecOrigin );

			MatrixTransformPlane( mat, localPlane, plane );			// Transform to world space
			VectorTransform( vecOrigin, mat, vecWorld );
					 
			if ( view.origin.Dot( plane.normal ) <= plane.dist )	// Check for view behind plane
				continue;
			
			VectorSubtract( vecWorld, view.origin, vecDelta );		// Backface cull
			if ( vecDelta.Dot( plane.normal ) >= 0 )
				continue;

			return true;
		}
	}

	return false;
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Iterates through reflective glass instead of just picking one
//-----------------------------------------------------------------------------
C_BaseEntity *NextReflectiveGlass( C_BaseEntity *pStart, const CViewSetup& view, cplane_t &plane,
	const Frustum_t &frustum, ITexture **pRenderTargets )
{
	// Early out if no cameras
	C_FuncReflectiveGlass *pReflectiveGlass = NULL;
	if (!pStart)
		pReflectiveGlass = GetReflectiveGlassList();
	else
		pReflectiveGlass = ((C_FuncReflectiveGlass*)pStart)->m_pNext;

	cplane_t localPlane;
	Vector vecOrigin, vecWorld, vecDelta;
	for ( ; pReflectiveGlass != NULL; pReflectiveGlass = pReflectiveGlass->m_pNext )
	{
		if ( pReflectiveGlass->IsDormant() )
			continue;

		if ( pReflectiveGlass->m_iViewHideFlags & (1 << CurrentViewID()) )
			continue;

		Vector vecMins, vecMaxs;
		pReflectiveGlass->GetRenderBoundsWorldspace( vecMins, vecMaxs );
		if ( R_CullBox( vecMins, vecMaxs, frustum ) )
			continue;

		const model_t *pModel = pReflectiveGlass->GetModel();
		const matrix3x4_t& mat = pReflectiveGlass->EntityToWorldTransform();

		int nCount = modelinfo->GetBrushModelPlaneCount( pModel );
		for ( int i = 0; i < nCount; ++i )
		{
			modelinfo->GetBrushModelPlane( pModel, i, localPlane, &vecOrigin );

			MatrixTransformPlane( mat, localPlane, plane );			// Transform to world space
			VectorTransform( vecOrigin, mat, vecWorld );
					 
			if ( view.origin.Dot( plane.normal ) <= plane.dist )	// Check for view behind plane
				continue;
			
			VectorSubtract( vecWorld, view.origin, vecDelta );		// Backface cull
			if ( vecDelta.Dot( plane.normal ) >= 0 )
				continue;

			if (pRenderTargets != NULL)
			{
				pRenderTargets[0] = pReflectiveGlass->ReflectionRenderTarget();
				pRenderTargets[1] = pReflectiveGlass->RefractionRenderTarget();
			}

			return pReflectiveGlass;
		}
	}

	return NULL;
}

void C_FuncReflectiveGlass::OnDataChanged( DataUpdateType_t type )
{
	// Reset render textures
	m_pReflectRenderTarget = NULL;
	m_pRefractRenderTarget = NULL;

	return BaseClass::OnDataChanged( type );
}

ITexture *C_FuncReflectiveGlass::ReflectionRenderTarget()
{
	if (m_iszReflectRenderTarget[0] != '\0')
	{
		if (!m_pReflectRenderTarget)
		{
			// We don't use a CTextureReference for this because we don't want to shut down the texture on removal/change
			m_pReflectRenderTarget = materials->FindTexture( m_iszReflectRenderTarget, TEXTURE_GROUP_RENDER_TARGET );
		}

		if (m_pReflectRenderTarget)
			return m_pReflectRenderTarget;
	}

	return NULL;
	//return GetWaterReflectionTexture();
}

ITexture *C_FuncReflectiveGlass::RefractionRenderTarget()
{
	if (m_iszRefractRenderTarget[0] != '\0')
	{
		if (!m_pRefractRenderTarget)
		{
			// We don't use a CTextureReference for this because we don't want to shut down the texture on removal/change
			m_pRefractRenderTarget = materials->FindTexture( m_iszRefractRenderTarget, TEXTURE_GROUP_RENDER_TARGET );
		}

		if (m_pRefractRenderTarget)
			return m_pRefractRenderTarget;
	}

	return NULL;
	//return GetWaterRefractionTexture();
}
#endif



