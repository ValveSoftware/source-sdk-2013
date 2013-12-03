//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Rumble effects mixer for XBox
//
// $NoKeywords: $
//
//=============================================================================//
#pragma once
#ifndef C_RUMBLE_H
#define C_RUMBLE_H

extern void RumbleEffect( unsigned char effectIndex, unsigned char rumbleData, unsigned char rumbleFlags );
extern void UpdateRumbleEffects();
extern void UpdateScreenShakeRumble( float shake, float balance = 0 );
extern void EnableRumbleOutput( bool bEnable );

#endif//C_RUMBLE_H

