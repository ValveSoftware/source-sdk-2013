//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef _3DNOW_H
#define _3DNOW_H

float _3DNow_Sqrt(float x);
float _3DNow_RSqrt(float x);
float FASTCALL _3DNow_VectorNormalize (Vector& vec);
void FASTCALL _3DNow_VectorNormalizeFast (Vector& vec);
float _3DNow_InvRSquared(const float* v);

#endif // _3DNOW_H
