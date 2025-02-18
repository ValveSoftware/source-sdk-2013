//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A particle system definition
//
//=============================================================================

#ifndef DMEPARTICLESYSTEMDEFINITION_H
#define DMEPARTICLESYSTEMDEFINITION_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"
#include "datamodel/dmehandle.h"
#include "particles/particles.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeEditorTypeDictionary;
class CDmeParticleSystemDefinition;


//-----------------------------------------------------------------------------
// Base class for particle functions inside a particle system definition
//-----------------------------------------------------------------------------
class CDmeParticleFunction : public CDmElement
{
	DEFINE_ELEMENT( CDmeParticleFunction, CDmElement );

public:
	virtual const char *GetFunctionType() const { return NULL; }
	virtual void Resolve();
	virtual void OnElementUnserialized();

	// Used for backward compat
	void AddMissingFields( const DmxElementUnpackStructure_t *pUnpack );

	// Returns the editor type dictionary
	CDmeEditorTypeDictionary* GetEditorTypeDictionary();

	// Marks a particle system as a new instance
	// This is basically a workaround to prevent newly-copied particle functions
	// from recompiling themselves a zillion times
	void MarkNewInstance();

protected:
	void UpdateAttributes( const DmxElementUnpackStructure_t *pUnpack );

private:
	// Defines widgets to edit this bad boy
	CDmeHandle< CDmeEditorTypeDictionary > m_hTypeDictionary;
	bool m_bSkipNextResolve;
};


//-----------------------------------------------------------------------------
// Something that updates particles
//-----------------------------------------------------------------------------
class CDmeParticleOperator : public CDmeParticleFunction
{
	DEFINE_ELEMENT( CDmeParticleOperator, CDmeParticleFunction );

public:
	// Sets the particle operator
	void SetFunction( IParticleOperatorDefinition *pDefinition );

	// Returns the function type
	virtual const char *GetFunctionType() const;

private:
	CDmaString m_FunctionName;
};


//-----------------------------------------------------------------------------
// A child of a particle system
//-----------------------------------------------------------------------------
class CDmeParticleChild : public CDmeParticleFunction
{
	DEFINE_ELEMENT( CDmeParticleChild, CDmeParticleFunction );

public:
	// Sets the particle operator
	void SetChildParticleSystem( CDmeParticleSystemDefinition *pDef, IParticleOperatorDefinition *pDefinition );

	// Returns the function type
	virtual const char *GetFunctionType() const;

	CDmaElement< CDmeParticleSystemDefinition > m_Child;
};



//-----------------------------------------------------------------------------
// Represents an editable entity; draws its helpers
//-----------------------------------------------------------------------------
class CDmeParticleSystemDefinition : public CDmElement
{
	DEFINE_ELEMENT( CDmeParticleSystemDefinition, CDmElement );

public:
	virtual void OnElementUnserialized();
	virtual void Resolve();

	// Add, remove
	CDmeParticleFunction* AddOperator( ParticleFunctionType_t type, const char *pFunctionName );
	CDmeParticleFunction* AddChild( CDmeParticleSystemDefinition *pChild );
	void RemoveFunction( ParticleFunctionType_t type, CDmeParticleFunction *pParticleFunction );
	void RemoveFunction( ParticleFunctionType_t type, int nIndex );

	// Find
	int FindFunction( ParticleFunctionType_t type, CDmeParticleFunction *pParticleFunction );
	int FindFunction( ParticleFunctionType_t type, const char *pFunctionName );

	// Iteration
	int GetParticleFunctionCount( ParticleFunctionType_t type ) const;
	CDmeParticleFunction *GetParticleFunction( ParticleFunctionType_t type, int nIndex );

	// Reordering
	void MoveFunctionUp( ParticleFunctionType_t type, CDmeParticleFunction *pElement );
	void MoveFunctionDown( ParticleFunctionType_t type, CDmeParticleFunction *pElement );

	// Returns the editor type dictionary
	CDmeEditorTypeDictionary* GetEditorTypeDictionary();

	// Recompiles the particle system when a change occurs
	void RecompileParticleSystem();

	// Marks a particle system as a new instance
	// This is basically a workaround to prevent newly-copied particle functions
	// from recompiling themselves a zillion times
	void MarkNewInstance();

	// Should we use name-based lookup?
	bool UseNameBasedLookup() const;

private:
	CDmaElementArray< CDmeParticleFunction > m_ParticleFunction[PARTICLE_FUNCTION_COUNT];
	CDmaVar< bool > m_bPreventNameBasedLookup;

	// Defines widgets to edit this bad boy
	CDmeHandle< CDmeEditorTypeDictionary > m_hTypeDictionary;
};


//-----------------------------------------------------------------------------
// Should we use name-based lookup?
//-----------------------------------------------------------------------------
inline bool CDmeParticleSystemDefinition::UseNameBasedLookup() const
{
	return !m_bPreventNameBasedLookup;
}


//-----------------------------------------------------------------------------
// Human readable string for the particle functions
//-----------------------------------------------------------------------------
const char *GetParticleFunctionTypeName( ParticleFunctionType_t type );


#endif // DMEPARTICLESYSTEMDEFINITION_H
