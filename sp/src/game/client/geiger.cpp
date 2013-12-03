//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// Geiger.cpp
//
// implementation of CHudAmmo class
//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "iclientmode.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudGeiger: public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudGeiger, vgui::Panel );
public:
	CHudGeiger( const char *pElementName );
	void Init( void );
	void VidInit( void );
	bool ShouldDraw( void );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );
	void MsgFunc_Geiger(bf_read &msg);
	
private:
	int m_iGeigerRange;
	float m_flLastSoundTestTime;
};

DECLARE_HUDELEMENT( CHudGeiger );
DECLARE_HUD_MESSAGE( CHudGeiger, Geiger );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudGeiger::CHudGeiger( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudGeiger" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	m_flLastSoundTestTime = -9999;

	SetHiddenBits( HIDEHUD_HEALTH );
}

void CHudGeiger::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGeiger::Init(void)
{
	HOOK_HUD_MESSAGE( CHudGeiger, Geiger );

	m_iGeigerRange = 0;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGeiger::VidInit(void)
{
	m_iGeigerRange = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGeiger::MsgFunc_Geiger( bf_read &msg )
{
	// update geiger data
	m_iGeigerRange = msg.ReadByte();
	m_iGeigerRange = m_iGeigerRange << 2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudGeiger::ShouldDraw( void )
{
	return ( ( m_iGeigerRange > 0 && m_iGeigerRange < 1000 ) && CHudElement::ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGeiger::Paint()
{
	int pct;
	float flvol=0;
	bool highsound = false;
	
	if ( gpGlobals->curtime - m_flLastSoundTestTime < 0.06 )
	{
		return;
	}

	m_flLastSoundTestTime = gpGlobals->curtime;

	// piecewise linear is better than continuous formula for this
	if (m_iGeigerRange > 800)
	{
		pct = 0;			//Msg ( "range > 800\n");
	}
	else if (m_iGeigerRange > 600)
	{
		pct = 2;
		flvol = 0.2;		//Msg ( "range > 600\n");
	}
	else if (m_iGeigerRange > 500)
	{
		pct = 4;
		flvol = 0.25;		//Msg ( "range > 500\n");
	}
	else if (m_iGeigerRange > 400)
	{
		pct = 8;
		flvol = 0.3;		//Msg ( "range > 400\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 300)
	{
		pct = 8;
		flvol = 0.35;		//Msg ( "range > 300\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 200)
	{
		pct = 28;
		flvol = 0.39;		//Msg ( "range > 200\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 150)
	{
		pct = 40;
		flvol = 0.40;		//Msg ( "range > 150\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 100)
	{
		pct = 60;
		flvol = 0.425;		//Msg ( "range > 100\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 75)
	{
		pct = 80;
		flvol = 0.45;		//Msg ( "range > 75\n");
		//gflGeigerDelay = cl.time + GEIGERDELAY * 0.75;
		highsound = true;
	}
	else if (m_iGeigerRange > 50)
	{
		pct = 90;
		flvol = 0.475;		//Msg ( "range > 50\n");
	}
	else
	{
		pct = 95;
		flvol = 0.5;		//Msg ( "range < 50\n");
	}

	flvol = (flvol * (random->RandomInt(0,127)) / 255) + 0.25;

	if ( random->RandomInt(0,127) < pct )
	{
		char sz[256];
		if ( highsound )
		{
			Q_strncpy( sz, "Geiger.BeepHigh", sizeof( sz ) );
		}
		else
		{
			Q_strncpy( sz, "Geiger.BeepLow", sizeof( sz ) );
		}

		CSoundParameters params;

		if ( C_BaseEntity::GetParametersForSound( sz, params, NULL ) )
		{
			CLocalPlayerFilter filter;

			EmitSound_t ep;
			ep.m_nChannel = params.channel;
			ep.m_pSoundName =  params.soundname;
			ep.m_flVolume = flvol;
			ep.m_SoundLevel = params.soundlevel;
			ep.m_nPitch = params.pitch;

			C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, ep ); 
		}
	}
}
