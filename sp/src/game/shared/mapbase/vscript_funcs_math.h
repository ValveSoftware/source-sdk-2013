//========= Copyright Valve Corporation, All rights reserved. =================
//
// Purpose: Shared VScript math functions.
//
// $NoKeywords: $
//=============================================================================

#ifndef VSCRIPT_FUNCS_MATH
#define VSCRIPT_FUNCS_MATH
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Exposes matrix3x4_t to VScript
//-----------------------------------------------------------------------------
class CScriptMatrix3x4
{
public:
	CScriptMatrix3x4()
	{
		matrix = new matrix3x4_t();
		m_bFree = true;
	}

	~CScriptMatrix3x4()
	{
		if (m_bFree == true)
			delete matrix;
	}

	CScriptMatrix3x4( matrix3x4_t &inmatrix ) { matrix = &inmatrix; }

	matrix3x4_t *GetMatrix()					{ return matrix; }
	void SetMatrix( matrix3x4_t &inmatrix )		{ matrix = &inmatrix; }

	void Init( const Vector& xAxis, const Vector& yAxis, const Vector& zAxis, const Vector &vecOrigin )
	{
		matrix->Init( xAxis, yAxis, zAxis, vecOrigin );
	}

private:
	matrix3x4_t *matrix;
	bool m_bFree = false;
};

inline matrix3x4_t *ToMatrix3x4( HSCRIPT hMat ) { return HScriptToClass<CScriptMatrix3x4>( hMat )->GetMatrix(); }

static HSCRIPT ScriptCreateMatrixInstance( matrix3x4_t &matrix )
{
	CScriptMatrix3x4 *smatrix = new CScriptMatrix3x4( matrix );

	return g_pScriptVM->RegisterInstance( smatrix );
}

static void ScriptFreeMatrixInstance( HSCRIPT hMat )
{
	CScriptMatrix3x4 *smatrix = HScriptToClass<CScriptMatrix3x4>( hMat );
	if (smatrix)
	{
		g_pScriptVM->RemoveInstance( hMat );
		delete smatrix;
	}
}

#endif
