//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Place where we can't build things.
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_FUNC_NO_BUILD_H
#define TF_FUNC_NO_BUILD_H

#ifdef _WIN32
#pragma once
#endif

class CBaseObject;
class Vector;

//-----------------------------------------------------------------------------
// Is a given point contained within any construction yard?
//-----------------------------------------------------------------------------
bool PointInNoBuild( const Vector &vecBuildOrigin, const CBaseObject* pObj );

#endif // TF_FUNC_NO_BUILD_H
