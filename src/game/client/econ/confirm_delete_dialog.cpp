//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "confirm_delete_dialog.h"
#include "vgui_controls/TextImage.h"
#include "econ_controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConfirmDeleteDialog::CConfirmDeleteDialog( vgui::Panel *parent )
:	BaseClass(parent)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDeleteDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// Set the X to be bright, and the rest dull
	if ( m_pConfirmButton )
	{
		m_pConfirmButton->SetText( "#X_DeleteConfirmButton" );
		SetXToRed( m_pConfirmButton );
	}
}