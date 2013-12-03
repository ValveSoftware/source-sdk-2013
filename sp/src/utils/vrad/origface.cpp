//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "vrad.h"

bool bOrigFacesTouched[MAX_MAP_FACES];


//-----------------------------------------------------------------------------
// Pupose: clear (reset) the bOrigFacesTouched list -- parellels the original
//         face list allowing an original face to only be processed once in 
//         pairing edges!
//-----------------------------------------------------------------------------
void ResetOrigFacesTouched( void )
{
    for( int i = 0; i < MAX_MAP_FACES; i++ )
    {
        bOrigFacesTouched[i] = false;
    }
}


//-----------------------------------------------------------------------------
// Purpose: mark an original faces as touched (dirty)
//   Input: index - index of the original face touched
//-----------------------------------------------------------------------------
void SetOrigFaceTouched( int index )
{
    bOrigFacesTouched[index] = true;
}


//-----------------------------------------------------------------------------
// Purpose: return whether or not an original face has been touched
//   Input: index - index of the original face touched
//  Output: true/false
//-----------------------------------------------------------------------------
bool IsOrigFaceTouched( int index )
{
    return bOrigFacesTouched[index];
}
