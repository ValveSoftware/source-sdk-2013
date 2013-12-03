//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef BONELIST_H
#define BONELIST_H
#ifdef _WIN32
#pragma once
#endif

#include "studio.h"

class CBoneList
{
public:

	CBoneList();

	void Release();

	static CBoneList *Alloc();

public:

	int			m_nBones;
	Vector		m_vecPos[ MAXSTUDIOBONES ];
	Quaternion	m_quatRot[ MAXSTUDIOBONES ];

private:
	bool		m_bShouldDelete;
};

class CFlexList
{
public:

	CFlexList();

	void Release();

	static CFlexList *Alloc();

public:

	int			m_nNumFlexes;
	float		m_flexWeights[ MAXSTUDIOFLEXCTRL ];

private:
	bool		m_bShouldDelete;
};

#endif // BONELIST_H
