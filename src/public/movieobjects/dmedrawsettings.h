//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing draw settings for Dme things
//
//=============================================================================

#ifndef DMEDRAWSETTINGS_H
#define DMEDRAWSETTINGS_H

#ifdef _WIN32
#pragma once
#endif


#include "datamodel/dmelement.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "tier1/utlstack.h"


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CDmeDag;


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CDmeDrawSettings : public CDmElement
{
	DEFINE_ELEMENT( CDmeDrawSettings, CDmElement );

public:

	enum DrawType_t
	{
		DRAW_INVALID = -1,

		DRAW_SMOOTH,
		DRAW_FLAT,
		DRAW_WIREFRAME,
		DRAW_BOUNDINGBOX,

		STANDARD_DRAW_COUNT
	};

	// resolve internal data from changed attributes
	virtual void Resolve();

	DrawType_t GetDrawType() const;
	DrawType_t SetDrawType( int drawType );
	void PushDrawType();
	void PopDrawType();

	bool Shaded() const {
		const DrawType_t drawType = GetDrawType();
		return drawType == DRAW_SMOOTH || drawType == DRAW_FLAT;
	}

	bool GetBackfaceCulling() const { return m_bBackfaceCulling.Get(); }
	void SetBackfaceCulling( bool val ) { m_bBackfaceCulling.Set( val ); }

	bool GetWireframeOnShaded() const { return m_bWireframeOnShaded.Get(); }
	void SetWireframeOnShaded( bool val ) { m_bWireframeOnShaded.Set( val ); }

	bool GetXRay() const { return m_bXRay.Get(); }
	void SetXRay( bool val ) { m_bXRay.Set( val ); }

	bool GetGrayShade() const { return m_bGrayShade.Get(); }
	void SetGrayShade( bool val ) { m_bGrayShade.Set( val ); }

	bool GetNormals() const { return m_bNormals.Get(); }
	void SetNormals( bool val ) { m_bNormals.Set( val ); }

	float GetNormalLength() const { return m_NormalLength.Get(); }
	void SetNormalLength( float val ) { m_NormalLength.Set( val ); }

	const Color &GetColor() const { return m_Color.Get(); }
	void SetColor( Color val ) { m_Color.Set( val ); }

	bool Drawable( CDmElement *pElement );

	void BindWireframe();

	void BindWireframeOnShaded();

	void BindGray();

	void BindUnlitGray();

	bool GetDeltaHighlight() const { return m_bDeltaHighlight; }
	void SetDeltaHighlight( bool bDeltaHighlight ) { m_bDeltaHighlight.Set( bDeltaHighlight ); }

	bool IsAMaterialBound() const {
		return m_IsAMaterialBound;
	}

	void DrawDag( CDmeDag *pDag );

	CUtlVector< Vector > &GetHighlightPoints() { return m_vHighlightPoints; }

public:

	CDmaVar< int > m_DrawType;
	CDmaVar< bool > m_bBackfaceCulling;
	CDmaVar< bool > m_bWireframeOnShaded;
	CDmaVar< bool > m_bXRay;
	CDmaVar< bool > m_bGrayShade;
	CDmaVar< bool > m_bNormals;
	CDmaVar< float > m_NormalLength;
	CDmaVar< Color > m_Color;
	CDmaVar< bool > m_bDeltaHighlight;
	CDmaVar< float > m_flHighlightSize;
	CDmaVar< Color > m_cHighlightColor;

protected:

	void BuildKnownDrawableTypes();

	static bool s_bWireframeMaterialInitialized;
	static CMaterialReference s_WireframeMaterial;

	static bool s_bWireframeOnShadedMaterialInitialized;
	static CMaterialReference s_WireframeOnShadedMaterial;

	static bool s_bFlatGrayMaterial;
	static CMaterialReference s_FlatGrayMaterial;

	static bool s_bUnlitGrayMaterial;
	static CMaterialReference s_UnlitGrayMaterial;

	static CUtlRBTree< CUtlSymbol > s_KnownDrawableTypes;
	CUtlRBTree< CUtlSymbol > m_NotDrawable;

	CUtlStack< DrawType_t > m_drawTypeStack;
	bool m_IsAMaterialBound;

	// Points to highlight
	CUtlVector< Vector > m_vHighlightPoints;
};


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
inline CDmeDrawSettings::DrawType_t CDmeDrawSettings::SetDrawType( int drawType )
{
	if ( drawType < 0 || drawType >= STANDARD_DRAW_COUNT )
	{
		drawType = DRAW_SMOOTH;
	}

	m_DrawType.Set( drawType );

	return GetDrawType();
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
inline CDmeDrawSettings::DrawType_t CDmeDrawSettings::GetDrawType() const
{
	const int drawType( m_DrawType.Get() );
	if ( drawType < 0 || drawType >= STANDARD_DRAW_COUNT )
		return DRAW_SMOOTH;

	return static_cast< DrawType_t >( drawType );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
inline void CDmeDrawSettings::PushDrawType()
{
	m_drawTypeStack.Push( GetDrawType() );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
inline void CDmeDrawSettings::PopDrawType()
{
	if ( m_drawTypeStack.Count() )
	{
		DrawType_t drawType = GetDrawType();
		m_drawTypeStack.Pop( drawType );
		SetDrawType( drawType );
	}
}


#endif // DMEDRAWSETTINGS_H
