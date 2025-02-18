//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DRAWING_PANEL_H
#define DRAWING_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>

#define DRAWN_LINE_SOLID_TIME 15.0f
#define DRAWN_LINE_FADE_TIME 3.0f
							   
class MapLine
{
public:
	MapLine()
	{
		worldpos.Init( 0, 0 );
		linkpos.Init( 0, 0 );
		created_time = 0;
		bSetLinkBlipCentre = false;
		bSetBlipCentre = false;
		blipcentre.Init( 0, 0 );
		linkblipcentre.Init( 0, 0 );
		bLink = false;
	}

	Vector2D worldpos;		// blip in world space
	Vector2D blipcentre;	// blip in map texture space
	Vector2D linkpos;			// link blip in world space
	Vector2D linkblipcentre;	// link blip in map texture space
	bool bLink;
	bool bSetBlipCentre;	// have we calculated the blip in map texture space yet?
	bool bSetLinkBlipCentre;	// have we calculated the link blip in map texture space yet?
	float created_time;
};


class CDrawingPanel : public vgui::Panel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CDrawingPanel, vgui::Panel );
public:
	CDrawingPanel( Panel *parent, const char *name );

	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;

	void SendMapLine( int x, int y, bool bInitial );
	virtual void OnMouseReleased( vgui::MouseCode code );
	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void OnCursorExited();
	virtual void OnCursorMoved( int x, int y );
	virtual void Paint();
	virtual void OnThink();
	virtual void SetVisible( bool bState ) OVERRIDE;
	void ClearLines( int iIndex );
	void ClearAllLines();
	const CUtlVector<MapLine>& GetLines( int iIndex ) const { return m_vecDrawnLines[iIndex]; }
	void SetType( int iPanelType ){ m_iPanelType = iPanelType; }

	virtual void FireGameEvent( IGameEvent *event );

private:
	void ReadColor( const char* pszToken, Color& color );

	bool m_bDrawingLines;
	float m_fLastMapLine;
	int m_iMouseX, m_iMouseY;
	int m_nWhiteTexture;

	Color m_colorLine;

	CUtlVector<MapLine> m_vecDrawnLines[MAX_PLAYERS_ARRAY_SAFE];

	int m_iPanelType;	
	bool m_bTeamColors;
};

#endif // DRAWING_PANEL_H
