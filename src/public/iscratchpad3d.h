//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISCRATCHPAD3D_H
#define ISCRATCHPAD3D_H
#ifdef _WIN32
#pragma once
#endif


// IScratchPad3D will store drawing commands in a file to be viewed by ScratchPad3DViewer.
// It can be used while stepping through geometry code to visualize what is going on as
// drawing commands will be immediately visible in ScratchPad3DViewer even while you're stuck
// in the debugger

// ScratchPad3DViewer initially orbits 100 inches from the origin, so it can be useful
// to call SetMapping to map what you're drawing input into this cube.


#include "mathlib/vector.h"
#include "mathlib/vector2d.h"
#include "utlvector.h"

class IFileSystem;


class CSPColor
{
public:
	CSPColor()
	{
		m_flAlpha = 1;
	}
	CSPColor( const Vector &vColor, float flAlpha=1 )
	{
		m_vColor = vColor;
		m_flAlpha = flAlpha;
	}
	CSPColor( float r, float g, float b, float a=1 )
	{
		m_vColor.Init( r, g, b );
		m_flAlpha = a;
	}
	
	Vector m_vColor;
	float m_flAlpha;
};


class CSPVert
{
public:
				CSPVert();
				CSPVert( Vector const &vPos, const CSPColor &vColor=CSPColor( Vector(1, 1, 1), 1 ) );
	
	void		Init( Vector const &vPos, const CSPColor &vColor=CSPColor( Vector(1, 1, 1), 1 ) );

public:
	Vector		m_vPos;
	CSPColor	m_vColor;
};


class CSPVertList
{
public:
				CSPVertList( int nVerts = 0 );
				CSPVertList(CSPVert const *pVerts, int nVerts);
				CSPVertList(Vector const *pVerts, int nVerts, CSPColor vColor=CSPColor(1,1,1) );

				CSPVertList(Vector const *pVerts, Vector const *pColors, int nVerts);
				CSPVertList(Vector const *pVerts, CSPColor const *pColors, int nVerts);
				
				CSPVertList(Vector const &vert1, CSPColor const &color1, 
					Vector const &vert2, CSPColor const &color2, 
					Vector const &vert3, CSPColor const &color3);

	CUtlVector<CSPVert>	m_Verts;
};

class SPRGBA
{
public:
	unsigned char r,g,b,a;
};


class CTextParams
{
public:
	CTextParams();


	Vector	m_vColor;				// Color of the string (starting color.. at some point,
												// we can embed commands in the text itself to change the color).
	float	m_flAlpha;				// Alpha of the whole thing.

	bool	m_bSolidBackground;		// Should the background be solid or alpha'd?

	// Draw an outline around the text?
	bool	m_bOutline;

	Vector	m_vPos;					// Where to render the text.
	bool	m_bCentered;			// Centered on m_vPos, or is m_vPos the upper-left corner?

	QAngle	m_vAngles;				// Orientation of the text.
	bool	m_bTwoSided;			// Render the text from both sides?
	
	float	m_flLetterWidth;		// Letter width in world space.
};


abstract_class IScratchPad3D
{
protected:

	virtual				~IScratchPad3D() {}


// Types.
public:

	enum RenderState
	{
		RS_FillMode=0,	// val = one of the FillMode enums
		RS_ZRead,
		RS_ZBias		// val = 0 - 16 to push Z towards viewer
	};

	enum FillMode
	{
		FillMode_Wireframe=0,
		FillMode_Solid
	};


public:
	
	virtual void		Release() = 0;

	// This sets up a mapping between input coordinates and output coordinates.
	// This can be used to zoom into an area of interest where you'll be drawing things.
	// An alternative is to press Z while in VisLibViewer to have it center and zoom on
	// everything that has been drawn.
	virtual void		SetMapping( 
		Vector const &vInputMin, 
		Vector const &vInputMax,
		Vector const &vOutputMin,
		Vector const &vOutputMax ) = 0;

	// Enable/disable auto flush. When set to true (the default), all drawing commands
	// are immediately written to the file and will show up in VisLibViewer right away.
	// If you want to draw a lot of things, you can set this to false and call Flush() 
	// manually when you want the file written out.
	// When you set auto flush to true, it calls Flush().
	virtual bool		GetAutoFlush() = 0;
	virtual void		SetAutoFlush( bool bAutoFlush ) = 0;

	// Draw a point. Point size is (roughly) in world coordinates, so points
	// get smaller as the viewer moves away.
	virtual void		DrawPoint( CSPVert const &v, float flPointSize ) = 0;

	// Draw a line.
	virtual void		DrawLine( CSPVert const &v1, CSPVert const &v2 ) = 0;

	// Draw a polygon.
	virtual void		DrawPolygon( CSPVertList const &verts ) = 0;

	// Draw 2D rectangles.
	virtual void		DrawRectYZ( float xPos, Vector2D const &vMin, Vector2D const &vMax, const CSPColor &vColor ) = 0;
	virtual void		DrawRectXZ( float yPos, Vector2D const &vMin, Vector2D const &vMax, const CSPColor &vColor ) = 0;
	virtual void		DrawRectXY( float zPos, Vector2D const &vMin, Vector2D const &vMax, const CSPColor &vColor ) = 0;

	// Draw a wireframe box.
	virtual void		DrawWireframeBox( Vector const &vMin, Vector const &vMax, Vector const &vColor ) = 0;

	// Draw some text.
	virtual void		DrawText( const char *pStr, const CTextParams &params ) = 0;

	// Wireframe on/off.	
	virtual void		SetRenderState( RenderState state, unsigned long val ) = 0;

	// Clear all the drawing commands.
	virtual void		Clear() = 0;

	// Calling this writes all the commands to the file. If AutoFlush is true, this is called
	// automatically in all the drawing commands.
	virtual void		Flush() = 0;


// Primitives that build on the atomic primitives.
public:
	
	// Draw a black and white image.
	// Corners are in this order: bottom-left, top-left, top-right, bottom-right.
	// If the corners are NULL, then the image is drawn in the XY plane from (-100,-100) to (100,100).
	virtual void		DrawImageBW( 
		unsigned char const *pData, 
		int width, 
		int height, 
		int pitchInBytes, 
		bool bOutlinePixels=true, 
		bool bOutlineImage=false,
		Vector *vCorners=NULL ) = 0;
	
	// Draw an RGBA image.
	// Corners are in this order: bottom-left, top-left, top-right, bottom-right.
	virtual void		DrawImageRGBA( 
		SPRGBA *pData, 
		int width, 
		int height, 
		int pitchInBytes, 
		bool bOutlinePixels=true,
		bool bOutlineImage=false,
		Vector *vCorners=NULL ) = 0;
};


// Just a helper for functions where you want to have a CScratchPad3D around 
// and release it automatically when the function exits.
class CScratchPadAutoRelease
{
public:
			CScratchPadAutoRelease( IScratchPad3D *pPad )	{ m_pPad = pPad; }
			~CScratchPadAutoRelease()						{ if( m_pPad ) m_pPad->Release(); }

	IScratchPad3D *m_pPad;
};



IScratchPad3D* ScratchPad3D_Create( char const *pFilename = "scratch.pad" );



// ------------------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------------------ //

inline CTextParams::CTextParams()
{
	m_vColor.Init( 1, 1, 1 );
	m_flAlpha = 1;
	m_bSolidBackground = true;
	m_bOutline = true;
	m_vPos.Init();
	m_bCentered = true;
	m_vAngles.Init();
	m_bTwoSided = true;
	m_flLetterWidth = 3;
}

inline CSPVert::CSPVert()
{
}

inline CSPVert::CSPVert( Vector const &vPos, const CSPColor &vColor )
{
	Init( vPos, vColor );
}

inline void CSPVert::Init( Vector const &vPos, const CSPColor &vColor )
{
	m_vPos = vPos;
	m_vColor = vColor;
}


inline CSPVertList::CSPVertList( int nVerts )
{
	if( nVerts )
		m_Verts.AddMultipleToTail( nVerts );
}

inline CSPVertList::CSPVertList(CSPVert const *pVerts, int nVerts )
{
	m_Verts.CopyArray( pVerts, nVerts );
}

inline CSPVertList::CSPVertList(Vector const *pVerts, int nVerts, CSPColor vColor )
{
	m_Verts.AddMultipleToTail( nVerts );
	for( int i=0; i < nVerts; i++ )
	{
		m_Verts[i].m_vPos = pVerts[i];
		m_Verts[i].m_vColor = vColor;
	}
}

inline CSPVertList::CSPVertList( Vector const *pVerts, Vector const *pColors, int nVerts )
{
	m_Verts.AddMultipleToTail( nVerts );
	for( int i=0; i < nVerts; i++ )
	{
		m_Verts[i].m_vPos = pVerts[i];
		m_Verts[i].m_vColor = pColors[i];
	}
}

inline CSPVertList::CSPVertList( Vector const *pVerts, CSPColor const *pColors, int nVerts )
{
	m_Verts.AddMultipleToTail( nVerts );
	for( int i=0; i < nVerts; i++ )
	{
		m_Verts[i].m_vPos = pVerts[i];
		m_Verts[i].m_vColor = pColors[i];
	}
}

inline CSPVertList::CSPVertList( 
	Vector const &vert1, CSPColor const &color1, 
	Vector const &vert2, CSPColor const &color2, 
	Vector const &vert3, CSPColor const &color3 )
{
	m_Verts.AddMultipleToTail( 3 );
	m_Verts[0].Init( vert1, color1 );
	m_Verts[1].Init( vert2, color2 );
	m_Verts[2].Init( vert3, color3 );
}


#endif // ISCRATCHPAD3D_H
