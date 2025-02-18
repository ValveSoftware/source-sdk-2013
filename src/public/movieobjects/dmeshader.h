//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing a shader
//
//=============================================================================

#ifndef DMESHADER_H
#define DMESHADER_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IShader;


//-----------------------------------------------------------------------------
// A class representing a material
//-----------------------------------------------------------------------------
class CDmeShader : public CDmElement
{
	DEFINE_ELEMENT( CDmeShader, CDmElement );

public:
	void SetShaderName( const char *pShaderName );
	const char *GetShaderName() const;

	// Resolve
	virtual void Resolve();

private:
	// Finds a shader
	IShader *FindShader();

	// Remove all shader parameters that don't exist in the new shader
	void RemoveUnusedShaderParams( IShader *pShader );

	// Add all shader parameters that don't currently exist
	void AddNewShaderParams( IShader *pShader );

	// Add attribute for shader parameter
	CDmAttribute* AddAttributeForShaderParameter( IShader *pShader, int nIndex );

	IShader *m_pShader;
	CDmAttributeVarString m_ShaderName;
};

#endif // DMESHADER_H
