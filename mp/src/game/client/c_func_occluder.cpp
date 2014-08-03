//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_FuncOccluder : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_FuncOccluder, C_BaseEntity );

// Overrides.
public:
	virtual bool	ShouldDraw();
	virtual int		DrawModel( int flags );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

private:
	int m_nOccluderIndex;
	bool m_bActive;
};

IMPLEMENT_CLIENTCLASS_DT( C_FuncOccluder, DT_FuncOccluder, CFuncOccluder )
	RecvPropBool( RECVINFO( m_bActive ) ),
	RecvPropInt( RECVINFO(m_nOccluderIndex) ),
END_RECV_TABLE()


void C_FuncOccluder::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	engine->ActivateOccluder( m_nOccluderIndex, m_bActive );
}

bool C_FuncOccluder::ShouldDraw()
{
	return false;
}

int C_FuncOccluder::DrawModel( int flags )
{
	Assert(0);
	return 0;
}
