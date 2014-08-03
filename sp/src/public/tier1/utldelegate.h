//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A header describing use of the delegate system. It's hiding
// the highly complex implementation details of the delegate system
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef UTLDELEGATE_H
#define UTLDELEGATE_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// The delegate system: A method of invoking methods, whether they are
// member methods of classes, static methods of classes, or free functions,
// dealing with all the nastiness in differences between how the calls have
// to happen yet works in a highly optimal fashion. For details, see
//
//			http://www.codeproject.com/cpp/FastDelegate.asp
//
// The delegate design pattern is described here
//
//			http://en.wikipedia.org/wiki/Delegation_(programming)
//-----------------------------------------------------------------------------

#ifdef UTLDELEGATE_USAGE_DEMONSTRATION

//-----------------------------------------------------------------------------
// Here, we show how to use this system (the ifdef UTLDELEGATE_USAGE_DEMONSTRATION is used to get syntax coloring).
//-----------------------------------------------------------------------------

// First, define the functions you wish to call.
int Test1( char *pString, float x );
class CTestClass
{
public:
	void Test2();
	static float Test3( int x );
};

void Test()
{
	CTestClass testClass;

	// CUtlDelegate is a class that can be used to invoke methods of classes
	// or static functions in a highly efficient manner.

	// There are a couple ways to hook up a delegate. One is in a constructor
	// Note that the template parameter of CUtlFastDelegate looks like the
	// function type: first, you have the return type, then ( parameter list )
	CUtlDelegate< int ( char *, float ) > delegate1( &Test1 );

	// Another way is to use the UtlMakeDelegate method, allowing you to
	// define the delegate later. Note that UtlMakeDelegate does *not* do a heap allocation
	CUtlDelegate< void () > delegate2;
	delegate2 = UtlMakeDelegate( &testClass, &CTestClass::Test2 );

	// A third method is to use the Bind() method of CUtlFastDelegate
	// Note that you do not pass in the class pointer for static functions
	CUtlDelegate< float ( int ) > delegate3;
	delegate3.Bind( &CTestClass::Test3 );

	// Use the () operator to invoke the function calls.
	int x = delegate1( "hello", 1.0f );
	delegate2();
	float y = delegate3( 5 );

	// Use the Clear() method to unbind a delegate.
	delegate1.Clear();

	// You can use operator! or IsEmpty() to see if a delegate is bound
	if ( !delegate1.IsEmpty() )
	{
		delegate1( "hello2" );
	}

	// Delegates maintain an internal non-templatized representation of the
	// functions they are bound to called CUtlAbstractDelegate. These are
	// useful when keeping a list of untyped delegates or when passing 
	// delegates across interface boundaries.
	const CUtlAbstractDelegate &abstractDelegate3 = delegate3.GetAbstractDelegate();
	CUtlDelegate< float ( int ) > delegate4;
	delegate4.SetAbstractDelegate( abstractDelegate3 );
	delegate4( 10 );
}

#endif // UTLDELEGATE_USAGE_DEMONSTRATION

// Looking in this file may cause blindness.
#include "tier1/utldelegateimpl.h"

#endif // UTLDELEGATE_H
