//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_BOOSTPYTHON_H
#define SRCPY_BOOSTPYTHON_H
#ifdef _WIN32
#pragma once
#endif

#include <boost/python.hpp>
#ifdef _DEBUG
	// boost redefines _DEBUG
	#undef _DEBUG
	#define _DEBUG 1
#endif // _DEBUG

#endif // SRCPY_BOOSTPYTHON_H