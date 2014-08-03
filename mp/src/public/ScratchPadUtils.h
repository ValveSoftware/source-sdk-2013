//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This module contains helper functions for use with scratch pads.
//
// $NoKeywords: $
//=============================================================================//

#ifndef SCRATCHPADUTILS_H
#define SCRATCHPADUTILS_H
#ifdef _WIN32
#pragma once
#endif


#include "iscratchpad3d.h"


// Use this to make a graph.
class CScratchPadGraph
{
public:

	typedef int LineID;

	CScratchPadGraph();

	// Initialze the orientation and scales of the two axes.
	// Axis indices are 0, 1, or 2 for x, y, and z.
	void Init( 
		IScratchPad3D *pPad,

		Vector vTimeAxis = Vector(0,-1,0),
		float flInchesPerSecond=1,
		Vector vTimeLineColor=Vector(0,0,1),
		float flTimeOrigin=0,				// Where the origin of the graph is.

		float flTimeLabelEveryNSeconds=1,
		
		Vector vValueAxis = Vector(0,0,1),
		float flInchesPerValue=1,
		Vector vValueLineColor=Vector(1,0,0),
		float flValueOrigin=0				// Where the origin of the graph is.

		);

	bool IsInitted() const;

	// Add another line into the graph.
	LineID AddLine( Vector vColor );
	void AddSample( LineID iLine, float flTime, float flValue );
	void AddVerticalLine( float flTime, float flMinValue, float flMaxValue, const CSPColor &vColor );
	
	// Get the 3D position of a sample on the graph (so you can draw other things there).
	Vector GetSamplePosition( float flTime, float flValue );


private:

	void UpdateTicksAndStuff( float flTime, float flValue );

		

private:
	class CLineInfo
	{
	public:
		bool m_bFirst;
		float m_flLastTime;
		float m_flLastValue;
		Vector m_vColor;
	};

	IScratchPad3D *m_pPad;

	CUtlVector<CLineInfo> m_LineInfos;

	Vector m_vTimeAxis;
	float m_flInchesPerSecond;

	Vector m_vValueAxis;
	float m_flInchesPerValue;

	// How often to make a time label.
	float m_flTimeLabelEveryNSeconds;
	int m_nTimeLabelsDrawn;

	Vector m_vTimeLineColor;
	Vector m_vValueLineColor;

	float m_flTimeOrigin;
	float m_flValueOrigin;
	
	// Used to extend the value border.
	float m_flHighestValue;
	float m_flHighestTime;
};



// Draw a cone.
void ScratchPad_DrawLitCone( 
	IScratchPad3D *pPad,
	const Vector &vBaseCenter,
	const Vector &vTip,
	const Vector &vBrightColor,
	const Vector &vDarkColor,
	const Vector &vLightDir,
	float baseWidth,
	int nSegments );


// Draw a cylinder.
void ScratchPad_DrawLitCylinder( 
	IScratchPad3D *pPad,
	const Vector &v1,
	const Vector &v2,
	const Vector &vBrightColor,
	const Vector &vDarkColor,
	const Vector &vLightDir,
	float width,
	int nSegments );


// Draw an arrow.
void ScratchPad_DrawArrow( 
	IScratchPad3D *pPad,
	const Vector &vPos, 
	const Vector &vDirection,
	const Vector &vColor, 
	float flLength=20, 
	float flLineWidth=3,
	float flHeadWidth=8,
	int nCylinderSegments=5,
	int nHeadSegments=8,
	float flArrowHeadPercentage = 0.3f	// How much of the line is the arrow head.
	);


// Draw an arrow with less parameters.. it generates parameters based on length
// automatically to make the arrow look good.
void ScratchPad_DrawArrowSimple( 
	IScratchPad3D *pPad,
	const Vector &vPos, 
	const Vector &vDirection,
	const Vector &vColor, 
	float flLength );

void ScratchPad_DrawSphere(
	IScratchPad3D *pPad,
	const Vector &vCenter,
	float flRadius,
	const Vector &vColor,
	int nSubDivs=7 );


void ScratchPad_DrawAABB(
	IScratchPad3D *pPad,
	const Vector &vMins,
	const Vector &vMaxs,
	const Vector &vColor = Vector( 1,1,1 ) );


#endif // SCRATCHPADUTILS_H
