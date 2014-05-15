//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements visual effects entities: sprites, beams, bubbles, etc.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "shake.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CEnvFade : public CLogicalEntity
{
private:

	float m_Duration;
	float m_HoldTime;

	COutputEvent m_OnBeginFade;

	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CEnvFade, CLogicalEntity );

	virtual void Spawn( void );

	inline float Duration( void ) { return m_Duration; }
	inline float HoldTime( void ) { return m_HoldTime; }

	inline void SetDuration( float duration ) { m_Duration = duration; }
	inline void SetHoldTime( float hold ) { m_HoldTime = hold; }

	int DrawDebugTextOverlays(void);

	// Inputs
	void InputFade( inputdata_t &inputdata );
};

LINK_ENTITY_TO_CLASS( env_fade, CEnvFade );

BEGIN_DATADESC( CEnvFade )

	DEFINE_KEYFIELD( m_Duration, FIELD_FLOAT, "duration" ),
	DEFINE_KEYFIELD( m_HoldTime, FIELD_FLOAT, "holdtime" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Fade", InputFade ),

	DEFINE_OUTPUT( m_OnBeginFade, "OnBeginFade"),

END_DATADESC()



#define SF_FADE_IN				0x0001		// Fade in, not out
#define SF_FADE_MODULATE		0x0002		// Modulate, don't blend
#define SF_FADE_ONLYONE			0x0004
#define SF_FADE_STAYOUT			0x0008

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvFade::Spawn( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that does the screen fade.
//-----------------------------------------------------------------------------
void CEnvFade::InputFade( inputdata_t &inputdata )
{
	int fadeFlags = 0;

	if ( m_spawnflags & SF_FADE_IN )
	{
		fadeFlags |= FFADE_IN;
	}
	else
	{
		fadeFlags |= FFADE_OUT;
	}

	if ( m_spawnflags & SF_FADE_MODULATE )
	{
		fadeFlags |= FFADE_MODULATE;
	}

	if ( m_spawnflags & SF_FADE_STAYOUT )
	{
		fadeFlags |= FFADE_STAYOUT;
	}

	if ( m_spawnflags & SF_FADE_ONLYONE )
	{
		if ( inputdata.pActivator && inputdata.pActivator->IsNetClient() )
		{
			UTIL_ScreenFade( inputdata.pActivator, m_clrRender, Duration(), HoldTime(), fadeFlags );
		}
	}
	else
	{
		UTIL_ScreenFadeAll( m_clrRender, Duration(), HoldTime(), fadeFlags|FFADE_PURGE );
	}

	m_OnBeginFade.FireOutput( inputdata.pActivator, this );
}


//-----------------------------------------------------------------------------
// Purpose: Fetches the arguments from the command line for the fadein and fadeout
//			console commands.
// Input  : flTime - Returns the fade time in seconds (the time to fade in or out)
//			clrFade - Returns the color to fade to or from.
//-----------------------------------------------------------------------------
static void GetFadeParms( const CCommand &args, float &flTime, color32 &clrFade)
{
	flTime = 2.0f;

	if ( args.ArgC() > 1 )
	{
		flTime = atof( args[1] );
	}
	
	clrFade.r = 0;
	clrFade.g = 0;
	clrFade.b = 0;
	clrFade.a = 255;

	if ( args.ArgC() > 4 )
	{
		clrFade.r = atoi( args[2] );
		clrFade.g = atoi( args[3] );
		clrFade.b = atoi( args[4] );

		if ( args.ArgC() == 5 )
		{
			clrFade.a = atoi( args[5] );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Console command to fade out to a given color.
//-----------------------------------------------------------------------------
static void CC_FadeOut( const CCommand &args )
{
	float flTime;
	color32 clrFade;
	GetFadeParms( args, flTime, clrFade );

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	UTIL_ScreenFade( pPlayer, clrFade, flTime, 0, FFADE_OUT | FFADE_PURGE | FFADE_STAYOUT );
}
static ConCommand fadeout("fadeout", CC_FadeOut, "fadeout {time r g b}: Fades the screen to black or to the specified color over the given number of seconds.", FCVAR_CHEAT );


//-----------------------------------------------------------------------------
// Purpose: Console command to fade in from a given color.
//-----------------------------------------------------------------------------
static void CC_FadeIn( const CCommand &args )
{
	float flTime;
	color32 clrFade;
	GetFadeParms( args, flTime, clrFade );

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	UTIL_ScreenFade( pPlayer, clrFade, flTime, 0, FFADE_IN | FFADE_PURGE );
}

static ConCommand fadein("fadein", CC_FadeIn, "fadein {time r g b}: Fades the screen in from black or from the specified color over the given number of seconds.", FCVAR_CHEAT );


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CEnvFade::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		// print duration
		Q_snprintf(tempstr,sizeof(tempstr),"    duration: %f", m_Duration);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		// print hold time
		Q_snprintf(tempstr,sizeof(tempstr),"    hold time: %f", m_HoldTime);
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}