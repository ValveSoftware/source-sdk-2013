//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#if !defined(_STATIC_LINKED) || defined(_SHARED_LIB)

#include "iscratchpad3d.h"
#include "mathlib/mathlib.h"
#include "ScratchPadUtils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// --------------------------------------------------------------------------------------------------------------------- //
// CScratchPadGraph implementation.
// --------------------------------------------------------------------------------------------------------------------- //

CScratchPadGraph::CScratchPadGraph()
{
	m_pPad = NULL;
}


void CScratchPadGraph::Init( 
	IScratchPad3D *pPad,

	Vector vTimeAxis, 
	float flInchesPerSecond,
	Vector vTimeLineColor,
	float flTimeOrigin,

	float flTimeLabelEveryNSeconds,
	
	Vector vValueAxis, 
	float flInchesPerValue,
	Vector vValueLineColor,
	float flValueOrigin

	)
{
	m_pPad = pPad;
	m_vTimeAxis = vTimeAxis;
	m_flInchesPerSecond = flInchesPerSecond;
	m_vValueAxis = vValueAxis;
	m_flInchesPerValue = flInchesPerValue;
	m_flTimeLabelEveryNSeconds = flTimeLabelEveryNSeconds;

	m_vTimeLineColor = vTimeLineColor;
	m_vValueLineColor = vValueLineColor;

	m_flTimeOrigin = flTimeOrigin;
	m_flValueOrigin = flValueOrigin;

	m_nTimeLabelsDrawn = 0;
	m_flHighestTime = flTimeOrigin;
	m_flHighestValue = flValueOrigin;
}


bool CScratchPadGraph::IsInitted() const
{
	return m_pPad != NULL;
}


CScratchPadGraph::LineID CScratchPadGraph::AddLine( Vector vColor )
{
	CScratchPadGraph::CLineInfo info;
	info.m_bFirst = true;
	info.m_vColor = vColor;
	return m_LineInfos.AddToTail( info );
}


void CScratchPadGraph::AddSample( LineID iLine, float flTime, float flValue )
{
	CScratchPadGraph::CLineInfo *pInfo = &m_LineInfos[iLine];

	UpdateTicksAndStuff( flTime, flValue );

	if ( !pInfo->m_bFirst )
	{
		// Draw a line from the last value to the current one.
		Vector vStart = GetSamplePosition( pInfo->m_flLastTime, pInfo->m_flLastValue );
		Vector vEnd   = GetSamplePosition( flTime, flValue );

		m_pPad->DrawLine(
			CSPVert( vStart, pInfo->m_vColor ),
			CSPVert( vEnd, pInfo->m_vColor ) 
			);
	}
	
	pInfo->m_flLastTime = flTime;
	pInfo->m_flLastValue = flValue;
	pInfo->m_bFirst = false;
}


void CScratchPadGraph::AddVerticalLine( float flTime, float flMinValue, float flMaxValue, const CSPColor &vColor )
{
	Vector v1 = GetSamplePosition( flTime, flMinValue );
	Vector v2 = GetSamplePosition( flTime, flMaxValue );
	m_pPad->DrawLine(
		CSPVert( v1, vColor ),
		CSPVert( v2, vColor ) );
}


void CScratchPadGraph::UpdateTicksAndStuff( float flTime, float flValue )
{
	if ( flTime > m_flHighestTime )
	{
		// Update the left part of the time axis.
		Vector vStart = GetSamplePosition( m_flHighestTime, m_flValueOrigin );
		Vector vEnd = GetSamplePosition( flTime, m_flValueOrigin );

		m_pPad->DrawLine(
			CSPVert( vStart, m_vTimeLineColor ),
			CSPVert( vEnd, m_vTimeLineColor )
			);

		m_flHighestTime = flTime;
	}
	
	if ( flValue > m_flHighestValue )
	{
		// Update the left part of the time axis.
		Vector vStart = GetSamplePosition( m_flTimeOrigin, m_flHighestValue );
		Vector vEnd = GetSamplePosition( m_flTimeOrigin, flValue );

		m_pPad->DrawLine(
			CSPVert( vStart, m_vValueLineColor ),
			CSPVert( vEnd, m_vValueLineColor )
			);

		// Extend the lines attached to the time labels.
		for ( int i=0; i < m_nTimeLabelsDrawn; i++ )
		{
			float flTime = m_flTimeOrigin + m_nTimeLabelsDrawn * m_flTimeLabelEveryNSeconds;

			m_pPad->DrawLine(
				CSPVert((const Vector&) GetSamplePosition( flTime, m_flHighestValue )),
				CSPVert((const Vector&) GetSamplePosition( flTime, flValue ) )	
				);
		}

		m_flHighestValue = flValue;
	}

	// More text labels?
	int iHighestTextLabel = (int)ceil( (flTime - m_flTimeOrigin) / m_flTimeLabelEveryNSeconds + 0.5f );
	while ( m_nTimeLabelsDrawn < iHighestTextLabel )
	{
		CTextParams params;
		
		float flTime = m_flTimeOrigin + m_nTimeLabelsDrawn * m_flTimeLabelEveryNSeconds;

		params.m_bSolidBackground = true;
		params.m_vPos = GetSamplePosition( flTime, m_flValueOrigin-5 );
		params.m_bTwoSided = true;
		
		char str[512];
		Q_snprintf( str, sizeof( str ), "time: %.2f", flTime );
		m_pPad->DrawText( str, params );


		// Now draw the vertical line for the value..
		m_pPad->DrawLine(
			CSPVert(  (const Vector&)GetSamplePosition( flTime, m_flValueOrigin ) ),
			CSPVert( (const Vector&)GetSamplePosition( flTime, m_flHighestValue ) )
			);
		

		m_nTimeLabelsDrawn++;
	}
}


Vector CScratchPadGraph::GetSamplePosition( float flTime, float flValue )
{
	Vector vRet = 
		m_vTimeAxis * ((flTime - m_flTimeOrigin) * m_flInchesPerSecond) + 
		m_vValueAxis * ((flValue - m_flValueOrigin) * m_flInchesPerValue);
	
	return vRet;
}



// --------------------------------------------------------------------------------------------------------------------- //
// Global functions.
// --------------------------------------------------------------------------------------------------------------------- //

void ScratchPad_DrawLitCone( 
	IScratchPad3D *pPad,
	const Vector &vBaseCenter,
	const Vector &vTip,
	const Vector &vBrightColor,
	const Vector &vDarkColor,
	const Vector &vLightDir,
	float baseWidth,
	int nSegments )
{
	// Make orthogonal vectors.
	Vector vDir = vTip - vBaseCenter;
	VectorNormalize( vDir );

	Vector vRight, vUp;
	VectorVectors( vDir, vRight, vUp );
	vRight *= baseWidth;
	vUp *= baseWidth;

	// Setup the top and bottom caps.
	CSPVertList bottomCap, tri;
	bottomCap.m_Verts.SetSize( nSegments );
	tri.m_Verts.SetSize( 3 );

	float flDot = -vLightDir.Dot( vDir );
	Vector topColor, bottomColor;
	VectorLerp( vDarkColor, vBrightColor, RemapVal( -flDot, -1, 1, 0, 1 ), bottomColor );

	
	// Draw each quad.
	Vector vPrevBottom = vBaseCenter + vRight;
	
	for ( int i=0; i < nSegments; i++ )
	{
		float flAngle = (float)(i+1) * M_PI * 2.0 / nSegments;
		Vector vOffset = vRight * cos( flAngle ) + vUp * sin( flAngle );
		Vector vCurBottom = vBaseCenter + vOffset;

		const Vector &v1 = vTip;
		const Vector &v2 = vPrevBottom;
		const Vector &v3 = vCurBottom;
		Vector vFaceNormal = (v2 - v1).Cross( v3 - v1 );
		VectorNormalize( vFaceNormal );

		// Now light it.
		flDot = -vLightDir.Dot( vFaceNormal );
		Vector vColor;
		VectorLerp( vDarkColor, vBrightColor, RemapVal( flDot,  -1, 1, 0, 1 ), vColor );

		// Draw the quad.
		tri.m_Verts[0] = CSPVert( v1, vColor );
		tri.m_Verts[1] = CSPVert( v2, vColor );
		tri.m_Verts[2] = CSPVert( v3, vColor );
		pPad->DrawPolygon( tri );

		bottomCap.m_Verts[i] = CSPVert( vCurBottom, bottomColor );
	}

	pPad->DrawPolygon( bottomCap );
}


void ScratchPad_DrawLitCylinder( 
	IScratchPad3D *pPad,
	const Vector &v1,
	const Vector &v2,
	const Vector &vBrightColor,
	const Vector &vDarkColor,
	const Vector &vLightDir,
	float width,
	int nSegments )
{
	// Make orthogonal vectors.
	Vector vDir = v2 - v1;
	VectorNormalize( vDir );

	Vector vRight, vUp;
	VectorVectors( vDir, vRight, vUp );
	vRight *= width;
	vUp *= width;

	// Setup the top and bottom caps.
	CSPVertList topCap, bottomCap, quad;
	
	topCap.m_Verts.SetSize( nSegments );
	bottomCap.m_Verts.SetSize( nSegments );
	quad.m_Verts.SetSize( 4 );

	float flDot = -vLightDir.Dot( vDir );
	Vector topColor, bottomColor;

	VectorLerp( vDarkColor, vBrightColor, RemapVal( flDot,  -1, 1, 0, 1 ), topColor );
	VectorLerp( vDarkColor, vBrightColor, RemapVal( -flDot, -1, 1, 0, 1 ), bottomColor );

	
	// Draw each quad.
	Vector vPrevTop = v1 + vRight;
	Vector vPrevBottom = v2 + vRight;
	
	for ( int i=0; i < nSegments; i++ )
	{
		float flAngle = (float)(i+1) * M_PI * 2.0 / nSegments;
		Vector vOffset = vRight * cos( flAngle ) + vUp * sin( flAngle );
		Vector vCurTop = v1 + vOffset;
		Vector vCurBottom = v2 + vOffset;

		// Now light it.
		VectorNormalize( vOffset );
		flDot = -vLightDir.Dot( vOffset );
		Vector vColor;
		VectorLerp( vDarkColor, vBrightColor, RemapVal( flDot,  -1, 1, 0, 1 ), vColor );

		// Draw the quad.
		quad.m_Verts[0] = CSPVert( vPrevTop, vColor );
		quad.m_Verts[1] = CSPVert( vPrevBottom, vColor );
		quad.m_Verts[2] = CSPVert( vCurBottom, vColor );
		quad.m_Verts[3] = CSPVert( vCurTop, vColor );
		pPad->DrawPolygon( quad );

		topCap.m_Verts[i] = CSPVert( vCurTop, topColor );
		bottomCap.m_Verts[i] = CSPVert( vCurBottom, bottomColor );
	}

	pPad->DrawPolygon( topCap );
	pPad->DrawPolygon( bottomCap );
}


void ScratchPad_DrawArrow( 
	IScratchPad3D *pPad,
	const Vector &vPos, 
	const Vector &vDirection,
	const Vector &vColor, 
	float flLength, 
	float flLineWidth,
	float flHeadWidth,
	int nCylinderSegments,
	int nHeadSegments,
	float flArrowHeadPercentage
	)
{
	Vector vNormDir = vDirection;
	VectorNormalize( vNormDir );
	
	Vector vConeBase = vPos + vNormDir * (flLength * ( 1 - flArrowHeadPercentage ) );
	Vector vConeEnd = vPos + vNormDir * flLength;
	
	Vector vLightDir( -1, -1, -1 );
	VectorNormalize( vLightDir ); // could precalculate this

	pPad->SetRenderState( IScratchPad3D::RS_FillMode, IScratchPad3D::FillMode_Solid );
	pPad->SetRenderState( IScratchPad3D::RS_ZRead, true );

	ScratchPad_DrawLitCylinder( pPad, vPos, vConeBase, vColor, vColor*0.25f, vLightDir, flLineWidth, nCylinderSegments );
	ScratchPad_DrawLitCone( pPad, vConeBase, vConeEnd, vColor, vColor*0.25f, vLightDir, flHeadWidth, nHeadSegments );
}


void ScratchPad_DrawArrowSimple( 
	IScratchPad3D *pPad,
	const Vector &vPos, 
	const Vector &vDirection,
	const Vector &vColor, 
	float flLength )
{
	ScratchPad_DrawArrow(
		pPad, 
		vPos,
		vDirection,
		vColor,
		flLength,
		flLength * 1.0/15,
		flLength * 3.0/15,
		4,
		4 );
}


void ScratchPad_DrawSphere(
	IScratchPad3D *pPad,
	const Vector &vCenter,
	float flRadius,
	const Vector &vColor,
	int nSubDivs )
{
	CUtlVector<Vector> prevPoints;
	prevPoints.SetSize( nSubDivs );
	
	// For each vertical slice.. (the top and bottom ones are just a single point).
	for ( int iSlice=0; iSlice < nSubDivs; iSlice++ )
	{
		float flHalfSliceAngle = M_PI * (float)iSlice / (nSubDivs - 1);

		if ( iSlice == 0 )
		{
			prevPoints[0] = vCenter + Vector( 0, 0, flRadius );
			for ( int z=1; z < prevPoints.Count(); z++ )
				prevPoints[z] = prevPoints[0];
		}
		else
		{
			for ( int iSubPt=0; iSubPt < nSubDivs; iSubPt++ )
			{
				float flHalfAngle = M_PI * (float)iSubPt / (nSubDivs - 1);
				float flAngle = flHalfAngle * 2;
				
				Vector pt;
				if ( iSlice == (nSubDivs - 1) )
				{
					pt = vCenter - Vector( 0, 0, flRadius );
				}
				else
				{
					pt.x = cos( flAngle ) * sin( flHalfSliceAngle );
					pt.y = sin( flAngle ) * sin( flHalfSliceAngle );
					pt.z = cos( flHalfSliceAngle );
					
					pt *= flRadius;
					pt += vCenter;
				}
				
				pPad->DrawLine( CSPVert( pt, vColor ), CSPVert( prevPoints[iSubPt], vColor ) );
				prevPoints[iSubPt] = pt;
			}
			
			if ( iSlice != (nSubDivs - 1) )
			{
				for ( int i=0; i < nSubDivs; i++ )
					pPad->DrawLine( CSPVert( prevPoints[i], vColor ), CSPVert( prevPoints[(i+1)%nSubDivs], vColor ) );
			}
		}
	}
}


void ScratchPad_DrawAABB(
	IScratchPad3D *pPad,
	const Vector &vMins,
	const Vector &vMaxs,
	const Vector &vColor )
{
	int vertOrder[4][2] = {{0,0},{1,0},{1,1},{0,1}};
	const Vector *vecs[2] = {&vMins, &vMaxs};
	
	Vector vTop, vBottom, vPrevTop, vPrevBottom;
	vTop.z = vPrevTop.z = vMaxs.z;
	vBottom.z = vPrevBottom.z = vMins.z;

	vPrevTop.x = vPrevBottom.x = vecs[vertOrder[3][0]]->x;
	vPrevTop.y = vPrevBottom.y = vecs[vertOrder[3][1]]->y;
	
	for ( int i=0; i < 4; i++ )
	{
		vTop.x = vBottom.x = vecs[vertOrder[i][0]]->x;
		vTop.y = vBottom.y = vecs[vertOrder[i][1]]->y;

		// Draw the top line.
		pPad->DrawLine( CSPVert( vPrevTop, vColor ), CSPVert( vTop, vColor ) );
		pPad->DrawLine( CSPVert( vPrevBottom, vColor ), CSPVert( vBottom, vColor ) );
		pPad->DrawLine( CSPVert( vTop, vColor ), CSPVert( vBottom, vColor ) );
		
		vPrevTop = vTop;
		vPrevBottom = vBottom;
	}
}


#endif // !_STATIC_LINKED || _SHARED_LIB

