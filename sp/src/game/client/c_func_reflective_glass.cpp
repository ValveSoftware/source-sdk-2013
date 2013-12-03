//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "view_shared.h"

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

	C_FuncReflectiveGlass	*m_pNext;
};

IMPLEMENT_CLIENTCLASS_DT( C_FuncReflectiveGlass, DT_FuncReflectiveGlass, CFuncReflectiveGlass )
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



