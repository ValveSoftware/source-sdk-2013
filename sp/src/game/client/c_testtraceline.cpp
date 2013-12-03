//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "materialsystem/imesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------------------------------------------------------------------- //
// An entity used to test traceline
// -------------------------------------------------------------------------------- //
class C_TestTraceline : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_TestTraceline, C_BaseEntity );
	DECLARE_CLIENTCLASS();

					C_TestTraceline();
	virtual			~C_TestTraceline();

// IClientEntity overrides.
public:
	virtual int			DrawModel( int flags );
	virtual bool		ShouldDraw() { return true; }

private:
	void DrawCube( Vector& center, unsigned char* pColor );
	IMaterial* m_pWireframe;
};

// Expose it to the engine.
IMPLEMENT_CLIENTCLASS(C_TestTraceline, DT_TestTraceline, CTestTraceline);

BEGIN_RECV_TABLE_NOBASE(C_TestTraceline, DT_TestTraceline)
	RecvPropInt(RECVINFO(m_clrRender)),
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[0], m_angRotation[0] ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[1], m_angRotation[1] ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[2], m_angRotation[2] ) ),
	RecvPropInt( RECVINFO_NAME(m_hNetworkMoveParent, moveparent), 0, RecvProxy_IntToMoveParent ),
END_RECV_TABLE()


// -------------------------------------------------------------------------------- //
// Functions.
// -------------------------------------------------------------------------------- //

C_TestTraceline::C_TestTraceline()
{
	m_pWireframe = materials->FindMaterial("shadertest/wireframevertexcolor", TEXTURE_GROUP_OTHER);
}

C_TestTraceline::~C_TestTraceline()
{
}


enum
{
	CUBE_SIZE = 5
};

void C_TestTraceline::DrawCube( Vector& center, unsigned char* pColor )
{
	Vector facePoints[8];
	Vector bmins, bmaxs;

	bmins[0] = center[0] - CUBE_SIZE;
	bmins[1] = center[1] - CUBE_SIZE;
	bmins[2] = center[2] - CUBE_SIZE;

	bmaxs[0] = center[0] + CUBE_SIZE;
	bmaxs[1] = center[1] + CUBE_SIZE;
	bmaxs[2] = center[2] + CUBE_SIZE;

	facePoints[0][0] = bmins[0];
	facePoints[0][1] = bmins[1];
	facePoints[0][2] = bmins[2];

	facePoints[1][0] = bmins[0];
	facePoints[1][1] = bmins[1];
	facePoints[1][2] = bmaxs[2];

	facePoints[2][0] = bmins[0];
	facePoints[2][1] = bmaxs[1];
	facePoints[2][2] = bmins[2];

	facePoints[3][0] = bmins[0];
	facePoints[3][1] = bmaxs[1];
	facePoints[3][2] = bmaxs[2];

	facePoints[4][0] = bmaxs[0];
	facePoints[4][1] = bmins[1];
	facePoints[4][2] = bmins[2];

	facePoints[5][0] = bmaxs[0];
	facePoints[5][1] = bmins[1];
	facePoints[5][2] = bmaxs[2];

	facePoints[6][0] = bmaxs[0];
	facePoints[6][1] = bmaxs[1];
	facePoints[6][2] = bmins[2];

	facePoints[7][0] = bmaxs[0];
	facePoints[7][1] = bmaxs[1];
	facePoints[7][2] = bmaxs[2];

	int nFaces[6][4] =
	{
		{ 0, 2, 3, 1 },
		{ 0, 1, 5, 4 },
		{ 4, 5, 7, 6 },
		{ 2, 6, 7, 3 },
		{ 1, 3, 7, 5 },
		{ 0, 4, 6, 2 }
	};

	for (int nFace = 0; nFace < 6; nFace++)
	{
		int nP1, nP2, nP3, nP4;

		nP1 = nFaces[nFace][0];
		nP2 = nFaces[nFace][1];
		nP3 = nFaces[nFace][2];
		nP4 = nFaces[nFace][3];

		// Draw the face.
		CMeshBuilder meshBuilder;
		CMatRenderContextPtr pRenderContext( materials );
		IMesh* pMesh = pRenderContext->GetDynamicMesh();
		meshBuilder.DrawQuad( pMesh, facePoints[nP1].Base(), facePoints[nP2].Base(), 
			facePoints[nP3].Base(), facePoints[nP4].Base(), pColor, true );
	}
}

int C_TestTraceline::DrawModel( int flags )
{
	trace_t tr;
	Vector forward, right, up, endpos, hitpos;
	AngleVectors (GetAbsAngles(), &forward, &right, &up);
	endpos = GetAbsOrigin() + forward * MAX_TRACE_LENGTH;

	UTIL_TraceLine( GetAbsOrigin(), endpos, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	CMatRenderContextPtr pRenderContext( materials );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_pWireframe );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );

	meshBuilder.Position3fv( GetAbsOrigin().Base() );
	meshBuilder.Color3ub( 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( tr.endpos.Base() );
	meshBuilder.Color3ub( 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	// Didn't hit anything
	if ( tr.fraction != 1.0 )
	{
		unsigned char color[] = { 0, 255, 0 };
		DrawCube( tr.endpos, color );
	}

	if ( (!tr.allsolid) && (tr.fractionleftsolid != 0.0) )
	{
		unsigned char color[] = { 255, 0, 0 };
		DrawCube( tr.startpos, color );
	}

	return 1;
}
