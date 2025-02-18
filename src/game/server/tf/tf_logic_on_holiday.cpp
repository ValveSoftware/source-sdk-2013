//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CLogicOnHoliday : public CLogicalEntity
{
	DECLARE_CLASS( CLogicOnHoliday, CLogicalEntity );
	DECLARE_DATADESC();

	COutputEvent m_IsAprilFools;
	COutputEvent m_IsFullMoon;
	COutputEvent m_IsHalloween;
	COutputEvent m_IsNothing;
	COutputEvent m_IsSmissmas;
	COutputEvent m_IsTFBirthday;
	COutputEvent m_IsValentines;

	void InputFire( inputdata_t & )
	{
		bool isAprilFools = TF_IsHolidayActive( kHoliday_AprilFools );
		bool isFullMoon = TF_IsHolidayActive( kHoliday_FullMoon );
		bool isHalloween = TF_IsHolidayActive( kHoliday_Halloween );
		bool isSmissmas = TF_IsHolidayActive( kHoliday_Christmas );
		bool isTFBirthday = TF_IsHolidayActive( kHoliday_TFBirthday );
		bool isValentines = TF_IsHolidayActive( kHoliday_Valentines );
		bool isNothing = !(isTFBirthday || isHalloween || isSmissmas || isValentines || isFullMoon || isAprilFools);

		if ( isNothing )
		{ 
			m_IsNothing.FireOutput( this, this );
			return;
		}

		if ( isAprilFools )		m_IsAprilFools.FireOutput( this, this );
		if ( isFullMoon )		m_IsFullMoon.FireOutput( this, this );
		if ( isHalloween )		m_IsHalloween.FireOutput( this, this );
		if ( isSmissmas )		m_IsSmissmas.FireOutput( this, this );
		if ( isTFBirthday )		m_IsTFBirthday.FireOutput( this, this );
		if ( isValentines )		m_IsValentines.FireOutput( this, this );

	}
};

LINK_ENTITY_TO_CLASS( tf_logic_on_holiday, CLogicOnHoliday );

BEGIN_DATADESC( CLogicOnHoliday )
	DEFINE_INPUTFUNC( FIELD_VOID, "Fire", InputFire ),
	DEFINE_OUTPUT( m_IsAprilFools, "IsAprilFools" ),
	DEFINE_OUTPUT( m_IsFullMoon, "IsFullMoon" ),
	DEFINE_OUTPUT( m_IsHalloween, "IsHalloween" ),
	DEFINE_OUTPUT( m_IsSmissmas, "IsSmissmas" ),
	DEFINE_OUTPUT( m_IsTFBirthday, "IsTFBirthday" ),
	DEFINE_OUTPUT( m_IsValentines, "IsValentines" ),
	DEFINE_OUTPUT( m_IsNothing, "IsNothing" ),
END_DATADESC()
