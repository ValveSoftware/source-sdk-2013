//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IVGUIMATINFOVAR_H
#define IVGUIMATINFOVAR_H


// wrapper for IMaterialVar
class IVguiMatInfoVar
{
public:
	// Add a virtual destructor to silence the clang warning.
	// This is harmless but not important since the only derived class
	// doesn't have a destructor.
	virtual ~IVguiMatInfoVar() {}

	virtual int GetIntValue ( void ) const = 0;
	virtual void SetIntValue ( int val ) = 0;

	// todo: if you need to add more IMaterialVar functions add them here
};

#endif //IVGUIMATINFOVAR_H
