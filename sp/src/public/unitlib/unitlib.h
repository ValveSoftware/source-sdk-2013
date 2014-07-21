//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
	   
#ifndef UNITLIB_H
#define UNITLIB_H

#ifdef _WIN32
#pragma once
#endif
 
#include "tier0/platform.h"
#include "tier1/interface.h"
#include "appframework/IAppSystem.h"


//-----------------------------------------------------------------------------
// Usage model for the UnitTest library
//
// The general methodology here is that clients are expected to create unit
// test DLLs that statically link to the unit test DLL and which implement
// tests of various libraries. The unit test application will dynamically
// load all DLLs in a directory, which causes them to install their test cases 
// into the unit test system. The application then runs through all tests and
// executes them all, displaying the results.
//
//	*** NOTE: The test suites are compiled in both debug and release builds,
//	even though it's expected to only be useful in debug builds. This is because
//  I couldn't come up with a good way of disabling the code in release builds.
//	(The only options I could come up with would still compile in the functions,
//	just not install them into the unit test library, or would make it so that
//	you couldn't step through the unit test code). 
//
//	Even though this is the case, there's no reason not to add test cases 
//	directly into your shipping DLLs, as long as you surround the code with 
//	#ifdef _DEBUG. To error check a project to make sure it's not compiling
//	in unit tests in Release build, just don't link in unitlib.lib in Release.
//	You can of course also put your test suites into separate DLLs. 
//
//	All tests inherit from the ITestCase interface. There are two major kinds
//	of tests; the first is a single test case meant to run a piece of 
//	code and check its results match expected values using the Assert macros.
//	The second kind is a test suite which is simply a list of other tests.
//
//	The following classes and macros are used to easily create unit test cases
//	and suites:
//
//  Use DEFINE_TESTSUITE to define a particular test suite, and DEFINE_TESTCASE
//  to add as many test cases as you like to that test suite, as follows:
//
//  DEFINE_TESTSUITE( VectorTestSuite )
//  
//  DEFINE_TESTCASE( VectorAdditionTest, VectorTestSuite )
//  {
//     .. test code here ..
//  }
//
//  Note that the definition of the test suite can occur in a different file
//  as the test case. A link error will occur if the test suite to which a
//  test case is added has not been defined.
//
//  To create a test case that is not part of a suite, use...
//
//  DEFINE_TESTCASE_NOSUITE( VectorAdditionTest )
//  {
//     .. test code here ..
//  }
//
//  You can also create a suite which is a child of another suite using
//
//	DEFINE_SUBSUITE( VectorTestSuite, MathTestSuite )
//
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// dll export stuff
//-----------------------------------------------------------------------------

#ifdef UNITLIB_DLL_EXPORT
#define UNITLIB_INTERFACE DLL_EXPORT
#define UNITLIB_CLASS_INTERFACE DLL_CLASS_EXPORT
#define UNITLIB_GLOBAL_INTERFACE DLL_GLOBAL_EXPORT
#else
#define UNITLIB_INTERFACE DLL_IMPORT
#define UNITLIB_CLASS_INTERFACE DLL_CLASS_IMPORT
#define UNITLIB_GLOBAL_INTERFACE DLL_GLOBAL_IMPORT
#endif


//-----------------------------------------------------------------------------
// All unit test libraries can be asked for a unit test 
// AppSystem to perform connection
//-----------------------------------------------------------------------------
#define UNITTEST_INTERFACE_VERSION		"UnitTestV001"


//-----------------------------------------------------------------------------
//
// NOTE: All classes and interfaces below you shouldn't use directly.
// Use the DEFINE_TESTSUITE and DEFINE_TESTCASE macros instead.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Test case + suite interface
//-----------------------------------------------------------------------------
class ITestCase
{
public:
	// This returns the	test name
	virtual char const* GetName() = 0;

	// This runs the test
	virtual void RunTest() = 0;
};

class ITestSuite : public ITestCase
{
public:
	// Add a test to the suite...
	virtual void AddTest( ITestCase* pTest ) = 0;
};



//-----------------------------------------------------------------------------
// This is the main function exported by the unit test library used by
// unit test DLLs to install their test cases into a list to be run
//-----------------------------------------------------------------------------
UNITLIB_INTERFACE	void UnitTestInstallTestCase( ITestCase* pTest );


//-----------------------------------------------------------------------------
// These are the methods used by the unit test running program to run all tests
//-----------------------------------------------------------------------------
UNITLIB_INTERFACE	int UnitTestCount();
UNITLIB_INTERFACE	ITestCase* GetUnitTest( int i );


//-----------------------------------------------------------------------------
// Helper for unit test DLLs to expose IAppSystems
//-----------------------------------------------------------------------------
#define USE_UNITTEST_APPSYSTEM( _className )	\
	static _className s_UnitTest ## _className;	\
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR( _className, IAppSystem, UNITTEST_INTERFACE_VERSION, s_UnitTest ## _className );
	

//-----------------------------------------------------------------------------
// Base class for test cases
//-----------------------------------------------------------------------------
class UNITLIB_CLASS_INTERFACE CTestCase : public ITestCase
{
public:
	CTestCase( char const* pName, ITestSuite* pParent = 0 );
	~CTestCase();

	// Returns the test name
	char const* GetName();

private:
	char* m_pName;
};


//-----------------------------------------------------------------------------
// Test suite class
//-----------------------------------------------------------------------------
class UNITLIB_CLASS_INTERFACE CTestSuite : public ITestSuite
{
public:
	CTestSuite( char const* pName, ITestSuite* pParent = 0 );
	~CTestSuite();

	// This runs the test
	void RunTest();
	
	// Add a test to the suite...
	void AddTest( ITestCase* pTest );

	// Returns the test name
	char const* GetName();

protected:
	int	m_TestCount;
	ITestCase** m_ppTestCases;
	char* m_pName;
};

#define TESTSUITE_CLASS( _suite )			\
	class CTS ## _suite : public CTestSuite \
	{										\
	public:									\
		CTS ## _suite();					\
	};
	
#define TESTSUITE_ACCESSOR( _suite ) 		\
	CTS ## _suite* GetTS ## _suite()		\
	{										\
		static CTS ## _suite s_TS ## _suite;	\
		return &s_TS ## _suite;				\
	}

#define FWD_DECLARE_TESTSUITE( _suite ) 	\
	class CTS ## _suite;					\
	CTS ## _suite* GetTS ## _suite();

#define DEFINE_TESTSUITE( _suite )			\
	TESTSUITE_CLASS( _suite )				\
	TESTSUITE_ACCESSOR( _suite )			\
	CTS ## _suite::CTS ## _suite() : CTestSuite( #_suite ) {}

#define DEFINE_SUBSUITE( _suite, _parent )	\
	TESTSUITE_CLASS( _suite )				\
	TESTSUITE_ACCESSOR( _suite )			\
	FWD_DECLARE_TESTSUITE( _parent )		\
	CTS ## _suite::CTS ## _suite() : CTestSuite( #_suite, GetTS ## _parent() ) {}

#define TESTCASE_CLASS( _case )				\
	class CTC ## _case  : public CTestCase	\
	{										\
	public:									\
		CTC ## _case ();					\
		void RunTest();						\
	};

#define DEFINE_TESTCASE_NOSUITE( _case )	\
	TESTCASE_CLASS( _case )					\
	CTC ## _case::CTC ## _case () : CTestCase( #_case ) {} \
											\
	CTC ## _case s_TC ## _case;				\
											\
	void CTC ## _case ::RunTest()

#define DEFINE_TESTCASE( _case, _suite )	\
	TESTCASE_CLASS( _case )					\
	FWD_DECLARE_TESTSUITE( _suite )			\
	CTC ## _case::CTC ## _case () : CTestCase( #_case, GetTS ## _suite() ) {}	\
											\
	CTC ## _case s_TC ## _case;				\
											\
	void CTC ## _case ::RunTest()


#define  _Shipping_AssertMsg( _exp, _msg, _executeExp, _bFatal )	\
	do {																\
		if (!(_exp)) 													\
		{ 																\
			_SpewInfo( SPEW_ASSERT, __TFILE__, __LINE__ );				\
			SpewRetval_t ret = _SpewMessage(_msg);						\
			_executeExp; 												\
			if ( ret == SPEW_DEBUGGER)									\
			{															\
				if ( !ShouldUseNewAssertDialog() || DoNewAssertDialog( __TFILE__, __LINE__, _msg ) ) \
					DebuggerBreak();									\
				if ( _bFatal )											\
					_ExitOnFatalAssert( __TFILE__, __LINE__ );			\
			}															\
		}																\
	} while (0)

#define  Shipping_Assert( _exp )           							_Shipping_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), ((void)0), false )


#endif	// UNITLIB_H
