//========= Copyright Valve Corporation, All rights reserved. ============//

#pragma once

#include "materialsystem/MaterialSystemUtil.h"

//------------------------------------------------------------------------------
// Purpose: Text message in world space
//------------------------------------------------------------------------------
class C_PointWorldText : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_PointWorldText, C_BaseEntity );
		
	DECLARE_CLIENTCLASS();

	C_PointWorldText();

	virtual ~C_PointWorldText();

	virtual void Spawn() OVERRIDE;
	virtual bool ShouldDraw() OVERRIDE;
	virtual void ClientThink() OVERRIDE;
	virtual int DrawModel( int flags ) OVERRIDE;
	virtual void GetRenderBounds( Vector& mins, Vector& maxs ) OVERRIDE;
	virtual bool ValidateEntityAttachedToPlayer( bool &bShouldRetry );
	virtual void PostDataUpdate( DataUpdateType_t updateType ) OVERRIDE;

	void SetText( const char* pszText );
	void SetFont( int nFont );

	bool IsTransparent( void );
	 
private:
	void UpdateRenderBounds();
	void ComputeCornerVertices( const QAngle &angles, const Vector &origin, Vector *pVerts ) const;

	void CalcTextTotalSize(float &outWidth, float &outHeight);
	void UpdateTextWorldSize();

	float GetTextWorldWidth() const;
	float GetTextWorldHeight() const;
	float GetTextSpacingX() const;
	float GetTextSpacingY() const;

	Vector m_localBBMin;
	Vector m_localBBMax;

	char m_szText[ MAX_PATH ];
	float m_flTextSize;
	float m_flTextSpacingX;
	float m_flTextSpacingY;
	color32 m_colTextColor;
	int m_nOrientation;
	int m_nTextLength;
	int m_nFont;
	bool m_bRainbow;

	float m_flTextWorldWidth;
	float m_flTextWorldHeight;

	CMaterialReference m_Font;
};
