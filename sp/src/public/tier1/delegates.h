//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple macros to generate delegation code
//
//=============================================================================

#ifndef DELEGATES_H
#define DELEGATES_H

#if defined( _WIN32 )
#pragma once
#endif

#define DELEGATE_TO_OBJECT_0( RetType, FuncName, pDelegated ) RetType FuncName()	{ return (pDelegated)->FuncName(); }
#define DELEGATE_TO_OBJECT_0V( FuncName, pDelegated ) void FuncName()	{ (pDelegated)->FuncName(); }
#define DELEGATE_TO_OBJECT_1( RetType, FuncName, ArgType1, pDelegated ) RetType FuncName( ArgType1 a1 )	{ return (pDelegated)->FuncName( a1 ); }
#define DELEGATE_TO_OBJECT_1V( FuncName, ArgType1, pDelegated ) void FuncName( ArgType1 a1 )	{ (pDelegated)->FuncName( a1 ); }
#define DELEGATE_TO_OBJECT_2( RetType, FuncName, ArgType1, ArgType2, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2 )	{ return (pDelegated)->FuncName( a1, a2 ); }
#define DELEGATE_TO_OBJECT_2V( FuncName, ArgType1, ArgType2, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2 )	{ (pDelegated)->FuncName( a1, a2 ); }
#define DELEGATE_TO_OBJECT_3( RetType, FuncName, ArgType1, ArgType2, ArgType3, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3 )	{ return (pDelegated)->FuncName( a1, a2, a3 ); }
#define DELEGATE_TO_OBJECT_3V( FuncName, ArgType1, ArgType2, ArgType3, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3 )	{ (pDelegated)->FuncName( a1, a2, a3 ); }
#define DELEGATE_TO_OBJECT_4( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4 )	{ return (pDelegated)->FuncName( a1, a2, a3, a4 ); }
#define DELEGATE_TO_OBJECT_4V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4 )	{ (pDelegated)->FuncName( a1, a2, a3, a4 ); }
#define DELEGATE_TO_OBJECT_5( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5 )	{ return (pDelegated)->FuncName( a1, a2, a3, a4, a5 ); }
#define DELEGATE_TO_OBJECT_5V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5 )	{ (pDelegated)->FuncName( a1, a2, a3, a4, a5 ); }
#define DELEGATE_TO_OBJECT_6( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6 )	{ return (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6 ); }
#define DELEGATE_TO_OBJECT_6V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6 )	{ (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6 ); }
#define DELEGATE_TO_OBJECT_7( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7 )	{ return (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7 ); }
#define DELEGATE_TO_OBJECT_7V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7 )	{ (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7 ); }
#define DELEGATE_TO_OBJECT_8( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8 )	{ return (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7, a8 ); }
#define DELEGATE_TO_OBJECT_8V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8 )	{ (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7, a8 ); }
#define DELEGATE_TO_OBJECT_9( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, ArgType9, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8, ArgType9 a9 )	{ return (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7, a8, a9 ); }
#define DELEGATE_TO_OBJECT_9V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, ArgType9, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8, ArgType9 a9 )	{ (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7, a8, a9 ); }
#define DELEGATE_TO_OBJECT_11V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, ArgType9, ArgType10, ArgType11, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8, ArgType9 a9, ArgType10 a10, ArgType11 a11 )	{ (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11 ); }

#define DELEGATE_TO_OBJECT_0C( RetType, FuncName, pDelegated ) RetType FuncName()	const { return (pDelegated)->FuncName(); }
#define DELEGATE_TO_OBJECT_0VC( FuncName, pDelegated ) void FuncName()	const { (pDelegated)->FuncName(); }
#define DELEGATE_TO_OBJECT_1C( RetType, FuncName, ArgType1, pDelegated ) RetType FuncName( ArgType1 a1 )	const { return (pDelegated)->FuncName( a1 ); }
#define DELEGATE_TO_OBJECT_1VC( FuncName, ArgType1, pDelegated ) void FuncName( ArgType1 a1 )	const { (pDelegated)->FuncName( a1 ); }
#define DELEGATE_TO_OBJECT_2C( RetType, FuncName, ArgType1, ArgType2, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2 )	const { return (pDelegated)->FuncName( a1, a2 ); }
#define DELEGATE_TO_OBJECT_2VC( FuncName, ArgType1, ArgType2, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2 )	const { (pDelegated)->FuncName( a1, a2 ); }
#define DELEGATE_TO_OBJECT_3C( RetType, FuncName, ArgType1, ArgType2, ArgType3, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3 )	const { return (pDelegated)->FuncName( a1, a2, a3 ); }
#define DELEGATE_TO_OBJECT_3VC( FuncName, ArgType1, ArgType2, ArgType3, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3 )	const { (pDelegated)->FuncName( a1, a2, a3 ); }
#define DELEGATE_TO_OBJECT_4C( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4 )	const { return (pDelegated)->FuncName( a1, a2, a3, a4 ); }
#define DELEGATE_TO_OBJECT_4VC( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4 )	const { (pDelegated)->FuncName( a1, a2, a3, a4 ); }
#define DELEGATE_TO_OBJECT_5C( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5 )	const { return (pDelegated)->FuncName( a1, a2, a3, a4, a5 ); }
#define DELEGATE_TO_OBJECT_5VC( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5 )	const { (pDelegated)->FuncName( a1, a2, a3, a4, a5 ); }
#define DELEGATE_TO_OBJECT_6C( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6 )	const { return (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6 ); }
#define DELEGATE_TO_OBJECT_6VC( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6 )	const { (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6 ); }
#define DELEGATE_TO_OBJECT_7C( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7 )	const { return (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7 ); }
#define DELEGATE_TO_OBJECT_7VC( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7 )	const { (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7 ); }
#define DELEGATE_TO_OBJECT_8C( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8 )	const { return (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7, a8 ); }
#define DELEGATE_TO_OBJECT_8VC( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8 )	const { (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7, a8 ); }
#define DELEGATE_TO_OBJECT_9C( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, ArgType9, pDelegated ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8, ArgType9 a9 )	const { return (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7, a8, a9 ); }
#define DELEGATE_TO_OBJECT_9VC( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, ArgType9, pDelegated ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8, ArgType9 a9 )	const { (pDelegated)->FuncName( a1, a2, a3, a4, a5, a6, a7, a8, a9 ); }

#define DELEGATE_TO_BASE_0( RetType, FuncName, BaseClass ) RetType FuncName()	{ return BaseClass::FuncName(); }
#define DELEGATE_TO_BASE_0V( FuncName, BaseClass ) void FuncName()	{ BaseClass::FuncName(); }
#define DELEGATE_TO_BASE_1( RetType, FuncName, ArgType1, BaseClass ) RetType FuncName( ArgType1 a1 )	{ return BaseClass::FuncName( a1 ); }
#define DELEGATE_TO_BASE_1V( FuncName, ArgType1, BaseClass ) void FuncName( ArgType1 a1 )	{ BaseClass::FuncName( a1 ); }
#define DELEGATE_TO_BASE_2( RetType, FuncName, ArgType1, ArgType2, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2 )	{ return BaseClass::FuncName( a1, a2 ); }
#define DELEGATE_TO_BASE_2V( FuncName, ArgType1, ArgType2, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2 )	{ BaseClass::FuncName( a1, a2 ); }
#define DELEGATE_TO_BASE_3( RetType, FuncName, ArgType1, ArgType2, ArgType3, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3 )	{ return BaseClass::FuncName( a1, a2, a3 ); }
#define DELEGATE_TO_BASE_3V( FuncName, ArgType1, ArgType2, ArgType3, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3 )	{ BaseClass::FuncName( a1, a2, a3 ); }
#define DELEGATE_TO_BASE_4( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4 )	{ return BaseClass::FuncName( a1, a2, a3, a4 ); }
#define DELEGATE_TO_BASE_4V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4 )	{ BaseClass::FuncName( a1, a2, a3, a4 ); }
#define DELEGATE_TO_BASE_5( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5 )	{ return BaseClass::FuncName( a1, a2, a3, a4, a5 ); }
#define DELEGATE_TO_BASE_5V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5 )	{ BaseClass::FuncName( a1, a2, a3, a4, a5 ); }
#define DELEGATE_TO_BASE_6( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6 )	{ return BaseClass::FuncName( a1, a2, a3, a4, a5, a6 ); }
#define DELEGATE_TO_BASE_6V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6 )	{ BaseClass::FuncName( a1, a2, a3, a4, a5, a6 ); }
#define DELEGATE_TO_BASE_7( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7 )	{ return BaseClass::FuncName( a1, a2, a3, a4, a5, a6, a7 ); }
#define DELEGATE_TO_BASE_7V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7 )	{ BaseClass::FuncName( a1, a2, a3, a4, a5, a6, a7 ); }
#define DELEGATE_TO_BASE_8( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8 )	{ return BaseClass::FuncName( a1, a2, a3, a4, a5, a6, a7, a8 ); }
#define DELEGATE_TO_BASE_8V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8 )	{ BaseClass::FuncName( a1, a2, a3, a4, a5, a6, a7, a8 ); }
#define DELEGATE_TO_BASE_9( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, ArgType9, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8, ArgType9 a9 )	{ return BaseClass::FuncName( a1, a2, a3, a4, a5, a6, a7, a8, a9 ); }
#define DELEGATE_TO_BASE_9V( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, ArgType9, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8, ArgType9 a9 )	{ BaseClass::FuncName( a1, a2, a3, a4, a5, a6, a7, a8, a9 ); }

#define DELEGATE_TO_BASE_0C( RetType, FuncName, BaseClass ) RetType FuncName()	const { return BaseClass::FuncName(); }
#define DELEGATE_TO_BASE_0VC( FuncName, BaseClass ) void FuncName()	const { BaseClass::FuncName(); }
#define DELEGATE_TO_BASE_1C( RetType, FuncName, ArgType1, BaseClass ) RetType FuncName( ArgType1 a1 )	const { return BaseClass::FuncName( a1 ); }
#define DELEGATE_TO_BASE_1VC( FuncName, ArgType1, BaseClass ) void FuncName( ArgType1 a1 )	const { BaseClass::FuncName( a1 ); }
#define DELEGATE_TO_BASE_2C( RetType, FuncName, ArgType1, ArgType2, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2 )	const { return BaseClass::FuncName( a1, a2 ); }
#define DELEGATE_TO_BASE_2VC( FuncName, ArgType1, ArgType2, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2 )	const { BaseClass::FuncName( a1, a2 ); }
#define DELEGATE_TO_BASE_3C( RetType, FuncName, ArgType1, ArgType2, ArgType3, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3 )	const { return BaseClass::FuncName( a1, a2, a3 ); }
#define DELEGATE_TO_BASE_3VC( FuncName, ArgType1, ArgType2, ArgType3, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3 )	const { BaseClass::FuncName( a1, a2, a3 ); }
#define DELEGATE_TO_BASE_4C( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4 )	const { return BaseClass::FuncName( a1, a2, a3, a4 ); }
#define DELEGATE_TO_BASE_4VC( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4 )	const { BaseClass::FuncName( a1, a2, a3, a4 ); }
#define DELEGATE_TO_BASE_5C( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5 )	const { return BaseClass::FuncName( a1, a2, a3, a4, a5 ); }
#define DELEGATE_TO_BASE_5VC( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5 )	const { BaseClass::FuncName( a1, a2, a3, a4, a5 ); }
#define DELEGATE_TO_BASE_6C( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6 )	const { return BaseClass::FuncName( a1, a2, a3, a4, a5, a6 ); }
#define DELEGATE_TO_BASE_6VC( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6 )	const { BaseClass::FuncName( a1, a2, a3, a4, a5, a6 ); }
#define DELEGATE_TO_BASE_7C( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7 )	const { return BaseClass::FuncName( a1, a2, a3, a4, a5, a6, a7 ); }
#define DELEGATE_TO_BASE_7VC( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7 )	const { BaseClass::FuncName( a1, a2, a3, a4, a5, a6, a7 ); }
#define DELEGATE_TO_BASE_8C( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8 )	const { return BaseClass::FuncName( a1, a2, a3, a4, a5, a6, a7, a8 ); }
#define DELEGATE_TO_BASE_8VC( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8 )	const { BaseClass::FuncName( a1, a2, a3, a4, a5, a6, a7, a8 ); }
#define DELEGATE_TO_BASE_9C( RetType, FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, ArgType9, BaseClass ) RetType FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8, ArgType9 a9 )	const { return BaseClass::FuncName( a1, a2, a3, a4, a5, a6, a7, a8, a9 ); }
#define DELEGATE_TO_BASE_9VC( FuncName, ArgType1, ArgType2, ArgType3, ArgType4, ArgType5, ArgType6, ArgType7, ArgType8, ArgType9, BaseClass ) void FuncName( ArgType1 a1, ArgType2 a2, ArgType3 a3, ArgType4 a4, ArgType5 a5, ArgType6 a6, ArgType7 a7, ArgType8 a8, ArgType9 a9 )	const { BaseClass::FuncName( a1, a2, a3, a4, a5, a6, a7, a8, a9 ); }

#endif // DELEGATES_H
