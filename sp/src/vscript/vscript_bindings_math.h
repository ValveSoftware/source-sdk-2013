//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 =================
//
// Purpose: Shared VScript math functions.
//
// $NoKeywords: $
//=============================================================================

#ifndef VSCRIPT_BINDINGS_MATH
#define VSCRIPT_BINDINGS_MATH
#ifdef _WIN32
#pragma once
#endif

void RegisterMathBaseBindings( IScriptVM *pVM );

// Some base bindings require VM functions
extern IScriptVM *g_pScriptVM;

//-----------------------------------------------------------------------------
// Exposes matrix3x4_t to VScript
//-----------------------------------------------------------------------------
inline matrix3x4_t *ToMatrix3x4( HSCRIPT hMat ) { return HScriptToClass<matrix3x4_t>( hMat ); }

static void ScriptFreeMatrixInstance( HSCRIPT hMat )
{
	matrix3x4_t *smatrix = HScriptToClass<matrix3x4_t>( hMat );
	if (smatrix)
	{
		g_pScriptVM->RemoveInstance( hMat );
		delete smatrix;
	}
}

//-----------------------------------------------------------------------------
// Exposes Quaternion to VScript
//-----------------------------------------------------------------------------
class CScriptQuaternionInstanceHelper : public IScriptInstanceHelper
{
	bool ToString( void *p, char *pBuf, int bufSize );

	bool Get( void *p, const char *pszKey, ScriptVariant_t &variant );
	bool Set( void *p, const char *pszKey, ScriptVariant_t &variant );

	ScriptVariant_t *Add( void *p, ScriptVariant_t &variant );
	//ScriptVariant_t *Subtract( void *p, ScriptVariant_t &variant );
	//ScriptVariant_t *Multiply( void *p, ScriptVariant_t &variant );
	//ScriptVariant_t *Divide( void *p, ScriptVariant_t &variant );
};

inline Quaternion *ToQuaternion( HSCRIPT hQuat ) { return HScriptToClass<Quaternion>( hQuat ); }

static void ScriptFreeQuaternionInstance( HSCRIPT hQuat )
{
	Quaternion *squat = HScriptToClass<Quaternion>( hQuat );
	if (squat)
	{
		g_pScriptVM->RemoveInstance( hQuat );
		delete squat;
	}
}

#endif
