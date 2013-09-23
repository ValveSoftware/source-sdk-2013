
#include "cbase.h"
#include "Gstring/clensflare_base.h"
#include "tier1/KeyValues.h"

#ifdef CLIENT_DLL

#include "filesystem.h"
#include "cdll_client_int.h"
#include "hud.h"
#include "Gstring/vgui/hud_lensflarefx.h"
#include "Gstring/vgui/vLensflare.h"

#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CLensflareBase )

	DEFINE_KEYFIELD( m_strLensFlareScript,		FIELD_STRING,	"LensFlareScript" ),

	DEFINE_FIELD( m_bLensflareEnabled,		FIELD_BOOLEAN ),

END_DATADESC()
#endif

#ifdef GAME_DLL
IMPLEMENT_SERVERCLASS_ST_NOBASE( CLensflareBase, CLensflareBase_DT )
#else
IMPLEMENT_CLIENTCLASS_DT_NOBASE( CLensflareBase, CLensflareBase_DT, CLensflareBase )
#endif

#ifdef GAME_DLL
	SendPropString( SENDINFO( m_netStr_LensFlareScript ) ),
#else
	RecvPropString( RECVINFO( m_netStr_LensFlareScript ) ),
#endif

END_NETWORK_TABLE();


CLensflareBase::CLensflareBase()
{
#ifdef CLIENT_DLL
	m_pLensFlare = NULL;
#else
	m_bLensflareEnabled = true;
#endif
}

#ifdef GAME_DLL

void CLensflareBase::Activate()
{
	BaseClass::Activate();

	Q_strncpy( m_netStr_LensFlareScript.GetForModify(), STRING( m_strLensFlareScript ), MAX_PATH );
}

void CLensflareBase::SetLensflareEnabled( bool bEnabled )
{
	m_bLensflareEnabled = bEnabled;
}

#else

void CLensflareBase::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		//CreateLensFlare();
	}
}

void CLensflareBase::UpdateOnRemove()
{
	DestroyLensFlare();
}

void CLensflareBase::CreateLensFlare()
{
	Assert( m_pLensFlare == NULL );

	if ( !m_netStr_LensFlareScript.Get() || Q_strlen( m_netStr_LensFlareScript.Get() ) < 1 )
		return;

	KeyValues *pData = new KeyValues("");
	if ( !pData->LoadFromFile( filesystem, m_netStr_LensFlareScript.Get() ) )
	{
		Warning( "unable to load lens flare script: %s\n", m_netStr_LensFlareScript.Get() );
		pData->deleteThis();
		return;
	}

	CHudLensflareEffects *pLensFlareFX = GET_HUDELEMENT( CHudLensflareEffects );
	Assert( pLensFlareFX != NULL );

	if ( pLensFlareFX != NULL )
	{
		m_pLensFlare = pLensFlareFX->LoadLensflare( GetGlowSource(), pData );
	}

	pData->deleteThis();
}

void CLensflareBase::DestroyLensFlare()
{
	if ( m_pLensFlare == NULL )
		return;

	CHudLensflareEffects *pLensFlareFX = GET_HUDELEMENT( CHudLensflareEffects );
	Assert( pLensFlareFX != NULL );

	if ( pLensFlareFX != NULL )
	{
		pLensFlareFX->FreeLensflare( &m_pLensFlare );
	}
}

CGlowOverlay *CLensflareBase::GetGlowSource()
{
	Assert( 0 );
	return NULL;
}
#endif