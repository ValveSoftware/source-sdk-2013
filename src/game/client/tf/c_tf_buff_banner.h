//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef C_TF_BUFF_BANNER_H
#define C_TF_BUFF_BANNER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_buff_item.h"

#define CTFBuffBanner C_TFBuffBanner

class C_TFBuffItem;

//-----------------------------------------------------------------------------
// Purpose: These need some base class derived from base animating that handles stuff like clientthink.
//-----------------------------------------------------------------------------
class C_TFBuffBanner : public CBaseAnimating
{
	DECLARE_CLASS( C_TFBuffBanner, CBaseAnimating );

public:

	DECLARE_NETWORKCLASS();

	C_TFBuffBanner();
	~C_TFBuffBanner() {}
	
	void SetBuffItem( C_TFBuffItem* newBuffItem ) { m_hBuffItem = newBuffItem; }

	virtual void NotifyBoneAttached( C_BaseAnimating* attachTarget );

	virtual void ClientThink( void );

	void SetBuffType( int iBuffType ) { m_iBuffType = iBuffType; }
private:

	float	m_flDetachTime;
	int		m_iBuffType;
	CHandle<C_TFBuffItem>	m_hBuffItem;
};

#endif // C_TF_BUFF_BANNER_H
