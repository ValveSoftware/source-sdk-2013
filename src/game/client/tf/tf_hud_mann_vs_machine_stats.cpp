//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Stat Ui Elements for MvM
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_hud_mann_vs_machine_stats.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// CCreditDisplayPanel
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CCreditDisplayPanel );

CCreditDisplayPanel::CCreditDisplayPanel( Panel *parent, const char *pName ): vgui::EditablePanel( parent, pName )
{

}

//-----------------------------------------------------------------------------
void CCreditDisplayPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/MvMCreditSubPanel.res" );
}

//-----------------------------------------------------------------------------
// CCreditSpendPanel
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CCreditSpendPanel );

CCreditSpendPanel::CCreditSpendPanel( Panel *parent, const char *pName ): vgui::EditablePanel( parent, pName )
{

}

//-----------------------------------------------------------------------------
void CCreditSpendPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/MvMCreditSpendPanel.res" );
}