//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Describes the way to compile data in DCC files (describes an export step)
//
//===========================================================================//

#ifndef DMEDCCMAKEFILE_H
#define DMEDCCMAKEFILE_H

#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmemakefile.h"


//-----------------------------------------------------------------------------
// Describes a source for DCC makefiles
//-----------------------------------------------------------------------------
class CDmeSourceDCCFile : public CDmeSource
{
	DEFINE_ELEMENT( CDmeSourceDCCFile, CDmeSource );

public:
	CDmaStringArray m_RootDCCObjects;
	CDmaVar< int > m_ExportType;	// 0 == model, 1 == skeletal animation
	CDmaVar< float > m_FrameStart;
	CDmaVar< float > m_FrameEnd;
	CDmaVar< float > m_FrameIncrement;
};


//-----------------------------------------------------------------------------
// Strictly here to customize OpenEditor
//-----------------------------------------------------------------------------
class CDmeSourceMayaFile : public CDmeSourceDCCFile
{
	DEFINE_ELEMENT( CDmeSourceMayaFile, CDmeSourceDCCFile );
};

class CDmeSourceMayaModelFile : public CDmeSourceMayaFile
{
	DEFINE_ELEMENT( CDmeSourceMayaModelFile, CDmeSourceMayaFile );
};

class CDmeSourceMayaAnimationFile : public CDmeSourceMayaFile
{
	DEFINE_ELEMENT( CDmeSourceMayaAnimationFile, CDmeSourceMayaFile );
};

class CDmeSourceXSIFile : public CDmeSourceDCCFile
{
	DEFINE_ELEMENT( CDmeSourceXSIFile, CDmeSourceDCCFile );
};


//-----------------------------------------------------------------------------
// Describes a DCC asset
//-----------------------------------------------------------------------------
class CDmeDCCMakefile : public CDmeMakefile
{
	DEFINE_ELEMENT( CDmeDCCMakefile, CDmeMakefile );

public:
	virtual void GetOutputs( CUtlVector<CUtlString> &fullPaths );

private:
	virtual CDmElement *CreateOutputElement( );
	virtual void DestroyOutputElement( CDmElement *pOutput );
	virtual const char *GetOutputDirectoryID() { return "makefiledir:..\\dmx"; }
	bool m_bFlushFile;
};


//-----------------------------------------------------------------------------
// Describes a Maya asset
//-----------------------------------------------------------------------------
class CDmeMayaMakefile : public CDmeDCCMakefile
{
	DEFINE_ELEMENT( CDmeMayaMakefile, CDmeDCCMakefile );
};


//-----------------------------------------------------------------------------
// Describes a XSI asset
//-----------------------------------------------------------------------------
class CDmeXSIMakefile : public CDmeDCCMakefile
{
	DEFINE_ELEMENT( CDmeXSIMakefile, CDmeDCCMakefile );

public:
	// Compiling is just exporting the data in the file
	virtual DmeMakefileType_t *GetMakefileType();
	virtual DmeMakefileType_t* GetSourceTypes();
};


//-----------------------------------------------------------------------------
// Describes a Maya model/animation asset
//-----------------------------------------------------------------------------
class CDmeMayaModelMakefile : public CDmeMayaMakefile
{
	DEFINE_ELEMENT( CDmeMayaModelMakefile, CDmeMayaMakefile );

public:
	// Compiling is just exporting the data in the file
	virtual DmeMakefileType_t *GetMakefileType();
	virtual DmeMakefileType_t* GetSourceTypes();
};

class CDmeMayaAnimationMakefile : public CDmeMayaMakefile
{
	DEFINE_ELEMENT( CDmeMayaAnimationMakefile, CDmeMayaMakefile );

public:
	// Compiling is just exporting the data in the file
	virtual DmeMakefileType_t *GetMakefileType();
	virtual DmeMakefileType_t* GetSourceTypes();
};


//-----------------------------------------------------------------------------
// Describes a XSI animation asset
//-----------------------------------------------------------------------------
class CDmeXSIAnimationMakefile : public CDmeXSIMakefile
{
	DEFINE_ELEMENT( CDmeXSIAnimationMakefile, CDmeXSIMakefile );
};


#endif // DMEDCCMAKEFILE_H
