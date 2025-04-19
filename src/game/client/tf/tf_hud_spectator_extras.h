//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_SPECTATOR_EXTRAS_H
#define TF_HUD_SPECTATOR_EXTRAS_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/IBorder.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFHudSpectatorExtras : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudSpectatorExtras, vgui::EditablePanel );
public:
	CTFHudSpectatorExtras( const char *pElementName );
	virtual ~CTFHudSpectatorExtras(){}

	virtual bool	ShouldDraw( void ) OVERRIDE;
	virtual void	OnTick() OVERRIDE;
	virtual void	Paint() OVERRIDE;
	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme) OVERRIDE;

	void RemoveEntity( int nRemove );
	void GetNameAndBorderColor( C_BaseEntity *pEntity, Color &outColor );

private:
	void Reset( void );
	void DrawBorder(int x, int y, int w, int h, Color color); // Helper function to draw consistent borders
	int m_nArrowTextureID;
	int m_iTexture;
	vgui::IBorder *m_pBorder;
	typedef struct
	{
		int m_nEntIndex;
		wchar_t m_wszName[MAX_ID_STRING];
		int m_nNameWidth;
		float m_flHealth;
		Color m_clrGlowColor;
		bool m_bDrawName;
		int m_nOffset;
	} spec_extra_t;

	CUtlVector< spec_extra_t > m_vecEntitiesToDraw;
	CPanelAnimationVar( vgui::HFont, m_hNameFont, "player_name_font", "NametagSmall" );
};

#endif // TF_HUD_SPECTATOR_EXTRAS_H
