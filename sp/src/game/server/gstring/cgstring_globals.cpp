
#include "cbase.h"
#include "cgstring_globals.h"
#include "cgstring_player.h"


#define GSTRINGGLOBALSFLAGS_NIGHTVISION_ENABLED 0x01

CGstringGlobals *g_pGstringGlobals;

BEGIN_DATADESC( CGstringGlobals )

	DEFINE_FIELD( m_bNightvisionEnabled, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "nightvision_enable", InputNightvisionEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "nightvision_disable", InputNightvisionDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "nightvision_toggle", InputNightvisionToggle ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( gstring_globals, CGstringGlobals );

CGstringGlobals::CGstringGlobals()
{
	m_bNightvisionEnabled = true;

	Assert( g_pGstringGlobals == NULL );

	if ( g_pGstringGlobals == NULL )
	{
		g_pGstringGlobals = this;
	}
}

CGstringGlobals::~CGstringGlobals()
{
	Assert( g_pGstringGlobals == this );

	if ( g_pGstringGlobals == this )
	{
		g_pGstringGlobals = NULL;
	}
}

void CGstringGlobals::Spawn()
{
	BaseClass::Spawn();

	SetNightvisionEnabled( HasSpawnFlags( GSTRINGGLOBALSFLAGS_NIGHTVISION_ENABLED ) );
}

void CGstringGlobals::SetNightvisionEnabled( bool bEnabled )
{
	m_bNightvisionEnabled = bEnabled;

	if ( !bEnabled )
	{
		CGstringPlayer *pPlayer = LocalGstringPlayer();

		if ( pPlayer )
		{
			pPlayer->SetNightvisionActive( false );
		}
	}
}

bool CGstringGlobals::IsNightvisionEnabled() const
{
	return m_bNightvisionEnabled;
}

void CGstringGlobals::InputNightvisionEnable( inputdata_t &inputdata )
{
	SetNightvisionEnabled( true );
}

void CGstringGlobals::InputNightvisionDisable( inputdata_t &inputdata )
{
	SetNightvisionEnabled( false );
}

void CGstringGlobals::InputNightvisionToggle( inputdata_t &inputdata )
{
	SetNightvisionEnabled( !IsNightvisionEnabled() );
}
