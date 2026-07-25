//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side view model implementation. Responsible for drawing
//			the view model.
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_BASEVIEWMODEL_H
#define C_BASEVIEWMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseanimating.h"
#include "utlvector.h"
#include "baseviewmodel_shared.h"

#ifdef TF_CLIENT_DLL
bool TeamFortress_ShouldFlipClientViewModel( void );
#endif //TF_CLIENT_DLL

#endif // C_BASEVIEWMODEL_H
