//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a panel which is rendered image on top of an entity
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "TeamBitmapImage.h"
#include <KeyValues.h>
#include "vgui_bitmapimage.h"
#include "panelmetaclassmgr.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include <vgui_controls/Panel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// A multiplexer bitmap that chooses a bitmap based on team
//-----------------------------------------------------------------------------
CTeamBitmapImage::CTeamBitmapImage() : m_Alpha(1.0f)
{
	memset( m_ppImage, 0, BITMAP_COUNT * sizeof(BitmapImage*) );
	m_pEntity = NULL;
	m_bRelativeTeams = 0;
}

CTeamBitmapImage::~CTeamBitmapImage()
{
	int i;
	for ( i = 0; i < BITMAP_COUNT; ++i )
	{
		if (m_ppImage[i])
			delete m_ppImage[i];
	}
}


//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------
bool CTeamBitmapImage::Init( vgui::Panel *pParent, KeyValues* pInitData, C_BaseEntity* pEntity )
{
	static const char *pRelativeTeamNames[BITMAP_COUNT] = 
	{
		"NoTeam",
		"MyTeam",
		"EnemyTeam",
	};

	static const char *pAbsoluteTeamNames[BITMAP_COUNT] = 
	{
		"Team0",
		"Team1",
		"Team2",
	};

	m_pEntity = pEntity;
	m_bRelativeTeams = (pInitData->GetInt( "relativeteam" ) != 0);

	const char **ppTeamNames = m_bRelativeTeams ? pRelativeTeamNames : pAbsoluteTeamNames;

	int i;
	for ( i = 0 ; i < BITMAP_COUNT; ++i )
	{
		// Default to null
		m_ppImage[i] = NULL;

		// Look for team section
		KeyValues *pTeamKV = pInitData->FindKey( ppTeamNames[i] );
		if ( !pTeamKV )
			continue;

		char const* pClassImage = pTeamKV->GetString( "material" );
		if ( !pClassImage || !pClassImage[ 0 ] )
			return false;

		// modulation color
		Color color;
		if (!ParseRGBA( pTeamKV, "color", color ))
			color.SetColor( 255, 255, 255, 255 );

		// hook in the bitmap
		m_ppImage[i] = new BitmapImage( pParent->GetVPanel(), pClassImage );
		m_ppImage[i]->SetColor( color );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Alpha modulate...
//-----------------------------------------------------------------------------
void CTeamBitmapImage::SetAlpha( float alpha )
{
	m_Alpha = clamp( alpha, 0.0f, 1.0f );
}


//-----------------------------------------------------------------------------
// draw
//-----------------------------------------------------------------------------
void CTeamBitmapImage::Paint( float yaw /*= 0.0f*/ )
{
	if (m_Alpha == 0.0f)
		return;

	int team = 0;
	if (m_bRelativeTeams)
	{
		if (GetEntity())
		{
			if (GetEntity()->GetTeamNumber() != 0)
			{
				team = GetEntity()->InLocalTeam() ? 1 : 2;
			}
		}
	}
	else
	{
		if (GetEntity())
			team = GetEntity()->GetTeamNumber();
	}

	// Paint the image for the current team
	if (m_ppImage[team])
	{
		// Modulate the color based on the alpha....
		Color color = m_ppImage[team]->GetColor();
		int alpha = color[3];
		color[3] = (alpha * m_Alpha);
		m_ppImage[team]->SetColor( color );

		if ( yaw != 0.0f )
		{
			g_pMatSystemSurface->DisableClipping( true );

			m_ppImage[team]->DoPaint( m_ppImage[team]->GetRenderSizePanel(), yaw );

			g_pMatSystemSurface->DisableClipping( false );
		}
		else
		{
			// Paint
			m_ppImage[team]->Paint();
		}

		// restore previous color
		color[3] = alpha;
		m_ppImage[team]->SetColor( color );
	}
}


//-----------------------------------------------------------------------------
// Helper method to initialize a team image from KeyValues data..
//-----------------------------------------------------------------------------
bool InitializeTeamImage( KeyValues *pInitData, const char* pSectionName, vgui::Panel *pParent, C_BaseEntity *pEntity, CTeamBitmapImage* pTeamImage )
{
	KeyValues *pTeamImageSection = pInitData;
	if (pSectionName)
	{
		pTeamImageSection = pInitData->FindKey( pSectionName );
		if ( !pTeamImageSection )
			return false;
	}

	return pTeamImage->Init( pParent, pTeamImageSection, pEntity );
}

