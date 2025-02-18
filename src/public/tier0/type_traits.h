//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: functionality that is provided in C++11 type_traits, which 
// currently isn't supported by the OSX compiler. Sadness.
//
//=============================================================================
#pragma once

template <class T>
struct V_remove_const
{
	typedef T type;
};

template <class T>
struct V_remove_const<const T>
{
	typedef T type;
};

template <class T>
struct V_remove_const<const T[]>
{
	typedef T type;
};

template <class T, unsigned int N>
struct V_remove_const<const T[N]>
{
	typedef T type;
};

