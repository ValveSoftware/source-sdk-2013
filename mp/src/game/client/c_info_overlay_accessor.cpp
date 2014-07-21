//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "materialsystem/imesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------------------------------------------------------------------- //
// An entity used to access overlays (and change their texture)
// -------------------------------------------------------------------------------- //
class C_InfoOverlayAccessor : public C_BaseEntity
{
public:

	DECLARE_CLASS( C_InfoOverlayAccessor, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_InfoOverlayAccessor();

	virtual void OnDataChanged( DataUpdateType_t updateType );

private:

	int		m_iOverlayID;
};

// Expose it to the engine.
IMPLEMENT_CLIENTCLASS(C_InfoOverlayAccessor, DT_InfoOverlayAccessor, CInfoOverlayAccessor);

BEGIN_RECV_TABLE_NOBASE(C_InfoOverlayAccessor, DT_InfoOverlayAccessor)
	RecvPropInt(RECVINFO(m_iTextureFrameIndex)),
	RecvPropInt(RECVINFO(m_iOverlayID)),
END_RECV_TABLE()


// -------------------------------------------------------------------------------- //
// Functions.
// -------------------------------------------------------------------------------- //

C_InfoOverlayAccessor::C_InfoOverlayAccessor()
{
}

void C_InfoOverlayAccessor::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Update overlay's bind proxy
		engine->SetOverlayBindProxy( m_iOverlayID, GetClientRenderable() );
	}
}
