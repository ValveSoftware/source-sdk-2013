//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MAT_STUB_H
#define MAT_STUB_H
#ifdef _WIN32
#pragma once
#endif


class IMaterialSystem;


// To stub out the material system in a block of code (if mat_stub is 1),
// make an instance of this class. You can unstub it by calling End() or 
// it will automatically unstub in its destructor.
class CMatStubHandler
{
public:
						CMatStubHandler();
						~CMatStubHandler();

	void				End();

public:

	IMaterialSystem		*m_pOldMaterialSystem;
};


// Returns true if mat_stub is 1.
bool IsMatStubEnabled();


#endif // MAT_STUB_H
