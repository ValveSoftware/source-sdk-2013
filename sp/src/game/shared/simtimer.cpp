//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "simtimer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CSimpleSimTimer )
	DEFINE_FIELD( m_next,			FIELD_TIME	),
END_DATADESC()

BEGIN_SIMPLE_DATADESC_( CSimTimer, CSimpleSimTimer )
	DEFINE_FIELD( m_interval,		FIELD_FLOAT	),
END_DATADESC()

BEGIN_SIMPLE_DATADESC_( CRandSimTimer, CSimpleSimTimer )
	DEFINE_FIELD( m_minInterval,		FIELD_FLOAT	),
	DEFINE_FIELD( m_maxInterval,		FIELD_FLOAT	),
END_DATADESC()

BEGIN_SIMPLE_DATADESC_( CStopwatchBase, CSimpleSimTimer )
	DEFINE_FIELD( m_fIsRunning,	FIELD_BOOLEAN	),
END_DATADESC()

BEGIN_SIMPLE_DATADESC_( CStopwatch, CStopwatchBase )
	DEFINE_FIELD( m_interval,		FIELD_FLOAT	),
END_DATADESC()

BEGIN_SIMPLE_DATADESC_( CRandStopwatch, CStopwatchBase )
	DEFINE_FIELD( m_minInterval,		FIELD_FLOAT	),
	DEFINE_FIELD( m_maxInterval,		FIELD_FLOAT	),
END_DATADESC()

//-----------------------------------------------------------------------------
