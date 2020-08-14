//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class C_FuncClientClip : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_FuncClientClip, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	void OnDataChanged( DataUpdateType_t type );
	void ClientThink();
	bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );

	bool m_bDisabled;
};

IMPLEMENT_CLIENTCLASS_DT( C_FuncClientClip, DT_FuncClientClip, CFuncClientClip )
	RecvPropBool( RECVINFO( m_bDisabled ) ),
END_RECV_TABLE()

void C_FuncClientClip::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	//if ( type == DATA_UPDATE_CREATED )
	//{
		SetSolid(GetMoveParent() ? SOLID_VPHYSICS :  SOLID_BSP); // SOLID_VPHYSICS
	//}
	
	if ( !m_bDisabled )
	{
		VPhysicsDestroyObject();
		VPhysicsInitShadow( true, true );

		// Think constantly updates the shadow
		if (GetMoveParent())
			SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
	else
	{
		// Disabling
		VPhysicsDestroyObject();
		SetNextClientThink( CLIENT_THINK_NEVER );
	}
}

void C_FuncClientClip::ClientThink()
{
	// We shouldn't be thinking if we're disabled
	Assert(!m_bDisabled);

	if (VPhysicsGetObject())
	{
		// Constantly updates the shadow.
		// This think function should really only be active when we're parented.
		VPhysicsGetObject()->UpdateShadow( GetAbsOrigin(), GetAbsAngles(), false, TICK_INTERVAL );
	}
	else
	{
		// This should never happen...
		VPhysicsInitShadow( true, true );
	}

	BaseClass::ClientThink();
}

bool C_FuncClientClip::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	if ( m_bDisabled )
		return false;

	if ( !VPhysicsGetObject() )
		return false;

	physcollision->TraceBox( ray, VPhysicsGetObject()->GetCollide(), GetAbsOrigin(), GetAbsAngles(), &trace );

	if ( trace.DidHit() )
	{
		trace.surface.surfaceProps = VPhysicsGetObject()->GetMaterialIndex();
		return true;
	}

	return false;
}
