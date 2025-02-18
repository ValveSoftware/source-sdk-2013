//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Interface for makefiles to build differently depending on where they are run from
//
//===========================================================================//

#ifndef DMEMAKEFILEUTILS_H
#define DMEMAKEFILEUTILS_H

#ifdef _WIN32
#pragma once
#endif


#include "movieobjects/idmemakefileutils.h"
#include "datamodel/dmehandle.h"
#include "tier1/utlsymbol.h"
#include "tier3/tier3.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeMakefileUtils;
class CDmeMDLMakefile;
class CDmeMayaMakefile;
class CDmeSourceMayaFile;
class CDmeMakefile;


//-----------------------------------------------------------------------------
//
// This glue code here is to make it easy to create methods using various DmElement types
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Compilation steps
//-----------------------------------------------------------------------------
enum CompilationStep_t
{
	BUILDING_STANDARD_DEPENDENCIES = 0,
	BUILDING_ALL_DEPENDENCIES,
	BEFORE_COMPILATION,
	PERFORMING_COMPILATION,
	AFTER_COMPILATION_FAILED,
	AFTER_COMPILATION_SUCCEEDED,
	NOT_COMPILING,
};


//-----------------------------------------------------------------------------
// Utility adapter class to hook compile funcs into the map
//-----------------------------------------------------------------------------
class CCompileFuncAdapterBase
{
public:
	virtual void InitializeAdapter( ) = 0;
	virtual bool PerformCompilationStep( CDmElement *pElement, CompilationStep_t step ) = 0;

protected:
	// Constructor, protected because these should never be instanced directly
	CCompileFuncAdapterBase( ) {}

public:
	CUtlSymbol m_ElementType;

	CCompileFuncAdapterBase *m_pNext;
};


template< class U, class T >
class CCompileFuncAdapter : public CCompileFuncAdapterBase
{
	typedef CCompileFuncAdapterBase BaseClass;

public:
	CCompileFuncAdapter( )
	{
		// Hook into the list
		m_pNext = U::m_CompileFuncTree.m_pFirstAdapter;
		U::m_CompileFuncTree.m_pFirstAdapter = this;
	}

	virtual void InitializeAdapter( )
	{
		m_ElementType = T::GetStaticTypeSymbol();
		if ( m_pNext )
		{
			m_pNext->InitializeAdapter();
		}
	}

	virtual bool PerformCompilationStep( CDmElement *pElement, CompilationStep_t step )
	{
		T *pConverted = CastElement< T >( pElement );
		if ( pConverted )
			return U::m_pSingleton->PerformCompilationStep( pConverted, step );
		return false;
	}
};

//-----------------------------------------------------------------------------
// Utility adapter class to hook editor opening funcs into the map
//-----------------------------------------------------------------------------
class COpenEditorFuncAdapterBase
{
public:
	virtual void InitializeAdapter( ) = 0;
	virtual void OpenEditor( CDmElement *pElement ) = 0;

protected:
	// Constructor, protected because these should never be instanced directly
	COpenEditorFuncAdapterBase( ) {}

public:
	CUtlSymbol m_ElementType;
	COpenEditorFuncAdapterBase *m_pNext;
};


template< class U, class T >
class COpenEditorFuncAdapter : public COpenEditorFuncAdapterBase
{
	typedef COpenEditorFuncAdapterBase BaseClass;

public:
	COpenEditorFuncAdapter( )
	{
		// Hook into the list
		m_pNext = U::m_OpenEditorFuncTree.m_pFirstAdapter;
		U::m_OpenEditorFuncTree.m_pFirstAdapter = this;
	}

	virtual void InitializeAdapter( )
	{
		m_ElementType = T::GetStaticTypeSymbol();
		if ( m_pNext )
		{
			m_pNext->InitializeAdapter();
		}
	}

	virtual void OpenEditor( CDmElement *pElement )
	{
		T *pConverted = CastElement< T >( pElement );
		if ( pConverted )
		{
			U::m_pSingleton->OpenEditor( pConverted );
		}
	}
};


#define DECLARE_DMEMAKEFILE_UTIL_CLASS_BASE( _className )				\
	protected:															\
		typedef _className ThisClass;									\
		static CompileFuncTree_t m_CompileFuncTree;						\
		static OpenEditorFuncTree_t m_OpenEditorFuncTree;				\
		static _className *m_pSingleton;								\
		template< typename U, typename T > friend class CCompileFuncAdapter; \
		template< typename U, typename T > friend class COpenEditorFuncAdapter; \
		virtual CompileFuncTree_t* GetCompileTree()						\
		{																\
			return &m_CompileFuncTree;									\
		}																\
		virtual OpenEditorFuncTree_t* GetOpenEditorTree()				\
		{																\
			return &m_OpenEditorFuncTree;								\
		}																\


#define DECLARE_DMEMAKEFILE_UTIL_CLASS( _className, _baseClass )		\
	DECLARE_DMEMAKEFILE_UTIL_CLASS_BASE( _className )					\
	typedef _baseClass BaseClass;										\
	protected:															\
		virtual void InitializeFuncMaps()								\
		{																\
			m_pSingleton = this;										\
			m_CompileFuncTree.m_pBaseAdapterTree = &BaseClass::m_CompileFuncTree; \
			m_CompileFuncTree.m_pFirstAdapter->InitializeAdapter( );	\
			m_OpenEditorFuncTree.m_pBaseAdapterTree = &BaseClass::m_OpenEditorFuncTree; \
			m_OpenEditorFuncTree.m_pFirstAdapter->InitializeAdapter( );	\
			BaseClass::InitializeFuncMaps();							\
		}																\

#define DECLARE_DMEMAKEFILE_UTIL_CLASS_ROOT( _className )				\
	DECLARE_DMEMAKEFILE_UTIL_CLASS_BASE( _className )					\
	protected:															\
		virtual void InitializeFuncMaps()								\
		{																\
			m_pSingleton = this;										\
			m_CompileFuncTree.m_pBaseAdapterTree = NULL;				\
			m_CompileFuncTree.m_pFirstAdapter->InitializeAdapter( );	\
			m_OpenEditorFuncTree.m_pBaseAdapterTree = NULL;				\
			m_OpenEditorFuncTree.m_pFirstAdapter->InitializeAdapter( );	\
		}																\

#define IMPLEMENT_DMEMAKEFILE_UTIL_CLASS( _className )					\
	CDmeMakefileUtils::CompileFuncTree_t _className::m_CompileFuncTree;	\
	CDmeMakefileUtils::OpenEditorFuncTree_t _className::m_OpenEditorFuncTree;	\
	_className *_className::m_pSingleton;								\

#define DECLARE_COMPILEFUNC( _className )						\
	bool PerformCompilationStep( _className *pClassName, CompilationStep_t step );								\
	CCompileFuncAdapter< ThisClass, _className > m_##_className##CompileAdapter

#define DECLARE_OPENEDITORFUNC( _className )	\
	void OpenEditor( _className *pClassName );	\
	COpenEditorFuncAdapter< ThisClass, _className > m_##_className##OpenEditorAdapter

	
//-----------------------------------------------------------------------------
// Interface for makefiles to build differently depending on where they are run from
//-----------------------------------------------------------------------------
class CDmeMakefileUtils : public CTier3AppSystem<IDmeMakefileUtils>
{
protected:
	struct CompileFuncTree_t										
	{																
		CCompileFuncAdapterBase *m_pFirstAdapter;					
		CompileFuncTree_t *m_pBaseAdapterTree;						
	};

	struct OpenEditorFuncTree_t										
	{																
		COpenEditorFuncAdapterBase *m_pFirstAdapter;				
		OpenEditorFuncTree_t *m_pBaseAdapterTree;					
	};																

	typedef CTier3AppSystem< IDmeMakefileUtils > BaseClass;

	DECLARE_DMEMAKEFILE_UTIL_CLASS_ROOT( CDmeMakefileUtils );

public:
	// Constructor, destructor
	CDmeMakefileUtils();
	virtual ~CDmeMakefileUtils();

	// Inherited from IAppSystem
	virtual void *QueryInterface( const char *pInterfaceName );
	virtual InitReturnVal_t Init();

	// Inherited from IDmeMakefileUtils
	virtual void PerformCompile( CDmElement *pElement, bool bBuildAllDependencies );
	virtual bool IsCurrentlyCompiling( );
	virtual int GetCompileOutputSize();
	virtual CompilationState_t UpdateCompilation( char *pOutputBuf, int nBufLen );
	virtual void AbortCurrentCompilation();
	virtual void PerformOpenEditor( CDmElement *pElement );
	virtual int GetExitCode();

protected:
	// Compile functions + editor functions
	DECLARE_COMPILEFUNC( CDmElement );
	DECLARE_COMPILEFUNC( CDmeMakefile );
	DECLARE_COMPILEFUNC( CDmeMDLMakefile );
	DECLARE_COMPILEFUNC( CDmeMayaMakefile );
	DECLARE_OPENEDITORFUNC( CDmeSourceMayaFile );

	// Queues up a compilation task
	// ( Call only in BUILDING_STANDARD_DEPENDENCIES or BUILDING_ALL_DEPENDENCIES )
	void AddCompilationTask( CDmElement* pElement );

	// Sets the compilation process handle
	// ( Call only in PERFORMING_COMPILATION )
	void SetCompileProcess( ProcessHandle_t hProcess );

private:
	struct CompileInfo_t
	{
		CDmeHandle< CDmElement > m_hElement;
		CCompileFuncAdapterBase *m_pAdapter;
	};

	// Finds the adapter class associated with a particular element type
	CCompileFuncAdapterBase *DetermineCompileAdapter( CDmElement *pElement );
	COpenEditorFuncAdapterBase *DetermineOpenEditorAdapter( CDmElement *pElement );

	// Dequeue the first compile task and start it up
	void StartNextCompileTask();

	// Performs the compilation step on all elements
	bool PerformCompilationStep( CompilationStep_t step );

	// Queues up a compilation task
	void AddCompilationTask( CDmElement* pElement, CCompileFuncAdapterBase *pAdapter );

	// Default implementatations for compile dependencies
	bool AddCompileDependencies( CDmeMakefile *pMakefile, bool bBuildAllDependencies );

	CUtlVector< CompileInfo_t > m_CompileTasks;
	ProcessHandle_t m_hCompileProcess;
	int m_nCurrentCompileTask;
	int m_nExitCode;
	CompilationStep_t m_CompilationStep;
};


#endif // DMEMAKEFILEUTILS_H
