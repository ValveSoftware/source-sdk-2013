//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CTextureToggle : public CPointEntity
{
public:
	DECLARE_CLASS( CTextureToggle, CPointEntity );

	void	InputIncrementBrushTexIndex( inputdata_t &inputdata );
	void	InputSetBrushTexIndex( inputdata_t &inputdata );

private:
	
	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( env_texturetoggle, CTextureToggle );

BEGIN_DATADESC( CTextureToggle )

	DEFINE_INPUTFUNC( FIELD_VOID, "IncrementTextureIndex", InputIncrementBrushTexIndex ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetTextureIndex", InputSetBrushTexIndex ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CTextureToggle::InputIncrementBrushTexIndex( inputdata_t& inputdata )
{
	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, m_target );
		
	while( pEntity ) 
	{
		int iCurrentIndex =  pEntity->GetTextureFrameIndex() + 1;
		pEntity->SetTextureFrameIndex( iCurrentIndex );

		pEntity = gEntList.FindEntityByName( pEntity, m_target ); 
	}
}

void CTextureToggle::InputSetBrushTexIndex( inputdata_t& inputdata )
{
	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, m_target );
		
	while( pEntity ) 
	{
		int iData = inputdata.value.Int();

		pEntity->SetTextureFrameIndex( iData );
		pEntity = gEntList.FindEntityByName( pEntity, m_target ); 
	}
}

