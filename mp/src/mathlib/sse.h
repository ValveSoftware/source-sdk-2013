//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef _SSE_H
#define _SSE_H

float _SSE_Sqrt(float x);
float _SSE_RSqrtAccurate(float a);
float _SSE_RSqrtFast(float x);
float FASTCALL _SSE_VectorNormalize(Vector& vec);
void FASTCALL _SSE_VectorNormalizeFast(Vector& vec);
float _SSE_InvRSquared(const float* v);
void _SSE_SinCos(float x, float* s, float* c);
float _SSE_cos( float x);
void _SSE2_SinCos(float x, float* s, float* c);
float _SSE2_cos(float x); 
void VectorTransformSSE(const float *in1, const matrix3x4_t& in2, float *out1);
void VectorRotateSSE( const float *in1, const matrix3x4_t& in2, float *out1 );

#endif // _SSE_H
