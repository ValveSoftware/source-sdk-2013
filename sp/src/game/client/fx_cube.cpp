//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "clientsideeffects.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class FX_Cube : public CClientSideEffect
{
public:
						FX_Cube(IMaterial *pMaterial) : CClientSideEffect("FX_Cube")
						{
							m_pMaterial = pMaterial;
							if(m_pMaterial)
								m_pMaterial->IncrementReferenceCount();
						}

	virtual				~FX_Cube()
						{
							if(m_pMaterial)
								m_pMaterial->DecrementReferenceCount();
								
						}

	void SetupVec(Vector& v, int dim1, int dim2, int fixedDim, float dim1Val, float dim2Val, float fixedDimVal)
	{
		v[dim1] = dim1Val;
		v[dim2] = dim2Val;
		v[fixedDim] = fixedDimVal;
	}

	void DrawBoxSide(
		int dim1, int dim2, int fixedDim, 
		float minX, float minY, 
		float maxX, float maxY, 
		float fixedDimVal, 
		bool bFlip, 
		float shade)
	{
		Vector v;
		Vector color;
		VectorScale( m_vColor, shade, color );

		CMatRenderContextPtr pRenderContext( materials );
		IMesh *pMesh = pRenderContext->GetDynamicMesh();
		
		CMeshBuilder builder;
		builder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

			SetupVec(v, dim1, dim2, fixedDim, minX, maxY, fixedDimVal);
			builder.Position3fv(v.Base());
			builder.Color3fv(color.Base());
			builder.AdvanceVertex();

			SetupVec(v, dim1, dim2, fixedDim, bFlip ? maxX : minX, bFlip ? maxY : minY, fixedDimVal);
			builder.Position3fv(v.Base());
			builder.Color3fv(color.Base());
			builder.AdvanceVertex();

			SetupVec(v, dim1, dim2, fixedDim, bFlip ? minX : maxX, bFlip ? minY : maxY, fixedDimVal);
			builder.Position3fv(v.Base());
			builder.Color3fv(color.Base());
			builder.AdvanceVertex();

			SetupVec(v, dim1, dim2, fixedDim, maxX, minY, fixedDimVal);
			builder.Position3fv(v.Base());
			builder.Color3fv(color.Base());
			builder.AdvanceVertex();

		builder.End();
		pMesh->Draw();
	}

	virtual void		Draw( double frametime )
	{
		CMatRenderContextPtr pRenderContext( materials );
		// Draw it.
		pRenderContext->Bind( m_pMaterial );
		
		Vector vLightDir(-1,-2,-3);
		VectorNormalize( vLightDir );

		DrawBoxSide(1, 2, 0, m_mins[1], m_mins[2], m_maxs[1], m_maxs[2], m_mins[0], false,  vLightDir.x * 0.5f + 0.5f);
		DrawBoxSide(1, 2, 0, m_mins[1], m_mins[2], m_maxs[1], m_maxs[2], m_maxs[0], true,  -vLightDir.x * 0.5f + 0.5f);

		DrawBoxSide(0, 2, 1, m_mins[0], m_mins[2], m_maxs[0], m_maxs[2], m_mins[1], true,   vLightDir.y * 0.5f + 0.5f);
		DrawBoxSide(0, 2, 1, m_mins[0], m_mins[2], m_maxs[0], m_maxs[2], m_maxs[1], false, -vLightDir.y * 0.5f + 0.5f);

		DrawBoxSide(0, 1, 2, m_mins[0], m_mins[1], m_maxs[0], m_maxs[1], m_mins[2], false,  vLightDir.z * 0.5f + 0.5f);
		DrawBoxSide(0, 1, 2, m_mins[0], m_mins[1], m_maxs[0], m_maxs[1], m_maxs[2], true,  -vLightDir.z * 0.5f + 0.5f);
		
		// Decrease lifetime.
		m_Life -= frametime;
	}

	bool IsActive( void )
	{
		return m_Life > 0.0;
	}

public:
	Vector		m_mins;
	Vector		m_maxs;
	Vector		m_vColor;
	float		m_Life;
	IMaterial	*m_pMaterial;
};


void FX_AddCube( const Vector &mins, const Vector &maxs, const Vector &vColor, float life, const char *materialName )
{
	IMaterial *mat = materials->FindMaterial(materialName, TEXTURE_GROUP_CLIENT_EFFECTS);

	FX_Cube *pCube = new FX_Cube(mat);
	pCube->m_mins = mins;
	pCube->m_maxs = maxs;
	pCube->m_vColor = vColor;
	pCube->m_Life = life;

	clienteffects->AddEffect(pCube);
}

void FX_AddCenteredCube( const Vector &center, float size, const Vector &vColor, float life, const char *materialName )
{
	FX_AddCube(center-Vector(size,size,size), center+Vector(size,size,size), vColor, life, materialName);
}


