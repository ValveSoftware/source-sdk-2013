//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTrainingAnnotation Entity.  This entity is used to place
//          annotations on maps.
//=============================================================================//

#include "cbase.h"

//=============================================================================
// CTrainingAnnotation Entity.
//=============================================================================
class CTrainingAnnotation : public CPointEntity 
{
	DECLARE_CLASS( CTrainingAnnotation, CPointEntity );

public:
	DECLARE_DATADESC();

	CTrainingAnnotation();
	~CTrainingAnnotation(){}

	// Input.
	void InputShow( inputdata_t &inputdata ){ Show(); }
	void InputHide( inputdata_t &inputdata ) {Hide(); }

	void Show();
	void Hide();

	string_t	m_displayText;
	float       m_flLifetime;
	float		m_flVerticalOffset;
};
