//========= Contributed by 4v4 PASS Time developers. ==========================//
//
// Purpose: HUD element to show the build date and time.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "pf_hud_buildinfo.h"

#include "vgui_controls/Panel.h"
#include "iclientmode.h"
#include "buildinfo/buildinfo.h"

DECLARE_HUDELEMENT( CHudBuildInfo );

CHudBuildInfo::CHudBuildInfo( const char *pElementName ) 
: CHudElement( pElementName )
, BaseClass( NULL, "HudBuildInfo" )
, m_pBuildTextLabel( NULL )
{
    Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( 0 );

    RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );
}


void CHudBuildInfo::ApplySchemeSettings( vgui::IScheme *pScheme ) 
{
	// Load from our .res file
	LoadControlSettings( GetResFilename() );

	BaseClass::ApplySchemeSettings( pScheme );
	m_pBuildTextLabel = dynamic_cast<CExLabel*>( FindChildByName("BuildTextLabel") );

	if (m_pBuildTextLabel)
	{
		wchar_t wszBuildTextWide[128];

		V_strtowcs(g_szbuildInfo, V_strlen(g_szbuildInfo), wszBuildTextWide, sizeof(wszBuildTextWide));
		m_pBuildTextLabel->SetText(wszBuildTextWide);
	}
}