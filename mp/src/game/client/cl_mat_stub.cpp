//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "bitmap/imageformat.h"
#include "cl_mat_stub.h"
#include "materialsystem/imaterialsystemstub.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Hook the engine's mat_stub cvar.
ConVar mat_stub( "mat_stub", "0", FCVAR_CHEAT );
extern ConVar gl_clear;


IMaterialSystemStub* GetStubMaterialSystem()
{
	return materials_stub;
}

// ---------------------------------------------------------------------------------------- //
// CMatStubHandler implementation.
// ---------------------------------------------------------------------------------------- //

CMatStubHandler::CMatStubHandler()
{
	if ( mat_stub.GetInt() )
	{
		m_pOldMaterialSystem = materials;

		// Replace all material system pointers with the stub.
		GetStubMaterialSystem()->SetRealMaterialSystem( materials );
		materials->SetInStubMode( true );
		materials = GetStubMaterialSystem();
		engine->Mat_Stub( materials );
	}
	else
	{
		m_pOldMaterialSystem = 0;
	}
}


CMatStubHandler::~CMatStubHandler()
{
	End();
}


void CMatStubHandler::End()
{
	// Put back the original material system pointer.
	if ( m_pOldMaterialSystem )
	{
		materials = m_pOldMaterialSystem;
		materials->SetInStubMode( false );
		engine->Mat_Stub( materials );
		m_pOldMaterialSystem = 0;
//		if( gl_clear.GetBool() )
		{
			materials->ClearBuffers( true, true );
		}
	}
}


bool IsMatStubEnabled()
{
	return mat_stub.GetBool();
}
