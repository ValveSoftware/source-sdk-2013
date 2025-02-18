//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Describes the way to compile a MDL file (eventual replacement for qc)
//
//===========================================================================//

#ifndef DMEMDLMAKEFILE_H
#define DMEMDLMAKEFILE_H

#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmemakefile.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeMDL;


//-----------------------------------------------------------------------------
// Describes a skin source for MDL makefiles
//-----------------------------------------------------------------------------
class CDmeSourceSkin : public CDmeSource
{
	DEFINE_ELEMENT( CDmeSourceSkin, CDmeSource );

public:
	// These can be built from DCC makefiles
	virtual const char **GetSourceMakefileTypes();

	CDmaString m_SkinName;
	CDmaVar<bool> m_bFlipTriangles;
	CDmaVar<float> m_flScale;
};


//-----------------------------------------------------------------------------
// Describes a skin source for MDL makefiles
//-----------------------------------------------------------------------------
class CDmeSourceCollisionModel : public CDmeSource
{
	DEFINE_ELEMENT( CDmeSourceCollisionModel, CDmeSource );

public:
	// These can be built from DCC makefiles
	virtual const char **GetSourceMakefileTypes();

private:
};


//-----------------------------------------------------------------------------
// Describes an animation source for MDL makefiles
//-----------------------------------------------------------------------------
class CDmeSourceAnimation : public CDmeSource
{
	DEFINE_ELEMENT( CDmeSourceAnimation, CDmeSource );

public:
	// These can be built from DCC makefiles
	virtual const char **GetSourceMakefileTypes();

	CDmaString m_AnimationName;
	CDmaString m_SourceAnimationName;	// Name in the source file

private:
};


//-----------------------------------------------------------------------------
// Describes a MDL asset: something that is compiled from sources 
//-----------------------------------------------------------------------------
class CDmeMDLMakefile : public CDmeMakefile
{
	DEFINE_ELEMENT( CDmeMDLMakefile, CDmeMakefile );

public:
	void SetSkin( const char *pFullPath );
	void AddAnimation( const char *pFullPath );
	void RemoveAnimation( const char *pFullPath );
	void RemoveAllAnimations( );

	virtual DmeMakefileType_t *GetMakefileType();
	virtual DmeMakefileType_t* GetSourceTypes();
	virtual void GetOutputs( CUtlVector<CUtlString> &fullPaths );

private:
	// Inherited classes should re-implement these methods
	virtual CDmElement *CreateOutputElement( );
	virtual void DestroyOutputElement( CDmElement *pOutput );
	virtual const char *GetOutputDirectoryID() { return "makefilegamedir:.."; }

	CDmeHandle< CDmeMDL > m_hMDL;
	bool m_bFlushMDL;
};


#endif // DMEMDLMAKEFILE_H
