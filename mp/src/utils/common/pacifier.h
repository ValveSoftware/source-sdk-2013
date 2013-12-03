//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PACIFIER_H
#define PACIFIER_H
#ifdef _WIN32
#pragma once
#endif


// Use these to display a pacifier like:
// ProcessBlock_Thread: 0...1...2...3...4...5...6...7...8...9... (0)
void StartPacifier( char const *pPrefix );			// Prints the prefix and resets the pacifier
void UpdatePacifier( float flPercent );				// percent value between 0 and 1.
void EndPacifier( bool bCarriageReturn = true );	// Completes pacifier as if 100% was done
void SuppressPacifier( bool bSuppress = true );		// Suppresses pacifier updates if another thread might still be firing them


#endif // PACIFIER_H
