//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TFBACKGROUND_H
#define TFBACKGROUND_H

#include <vgui_controls/Frame.h>
#include <vgui_controls/EditablePanel.h>

//-----------------------------------------------------------------------------
// Purpose: Creates background image panels
//-----------------------------------------------------------------------------
void CreateBackground( vgui::EditablePanel *pWindow );

//-----------------------------------------------------------------------------
// Purpose: Resizes windows to fit completely on-screen (for 1280x1024), and
//          centers them on the screen.  Sub-controls are also resized and moved.
//-----------------------------------------------------------------------------
void LayoutBackgroundPanel( vgui::EditablePanel *pWindow );

//-----------------------------------------------------------------------------
// Purpose: Sets colors etc for background image panels
//-----------------------------------------------------------------------------
void ApplyBackgroundSchemeSettings( vgui::EditablePanel *pWindow, vgui::IScheme *pScheme );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ResizeWindowControls( vgui::EditablePanel *pWindow, int tall, int wide, int offsetX, int offsetY );

//-----------------------------------------------------------------------------
// Purpose: transform a standard scaled value into one that is scaled based the minimum
//          of the horizontal and vertical ratios
//-----------------------------------------------------------------------------
int GetAlternateProportionalValueFromScaled( vgui::HScheme hScheme, int scaledValue );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DrawRoundedBackground( Color bgColor, int wide, int tall );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DrawRoundedBorder( Color borderColor, int wide, int tall );

//-----------------------------------------------------------------------------

#endif // TFBACKGROUND_H
