//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEOBJECT_SHARED_H
#define BASEOBJECT_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"

#if defined( CLIENT_DLL )
#define CBaseObject C_BaseObject
#endif

class CBaseObject;
typedef CHandle<CBaseObject>	ObjectHandle;
struct BuildPoint_t
{
	// If this is true, then objects are parented to the attachment point instead of 
	// parented to the entity's abs origin + angles. That way, they'll move if the 
	// attachment point animates.
	bool			m_bPutInAttachmentSpace;	

	int				m_iAttachmentNum;
	ObjectHandle	m_hObject;
	bool			m_bValidObjects[ OBJ_LAST ];
};

#define TF_OBJ_GROUND_CLEARANCE	32

// Shared header file for players
#if defined( CLIENT_DLL )
#include "c_baseobject.h"
#else
#include "tf_obj.h"
#endif

#endif // BASEOBJECT_SHARED_H
