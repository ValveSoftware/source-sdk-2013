//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: An entity that can be used to control speech behaviour for a group
//			of NPCs.
//
//=============================================================================//

#include "cbase.h"
#include "ai_speechfilter.h"
#ifndef CSTRIKE_DLL
#include "ai_playerally.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( ai_speechfilter, CAI_SpeechFilter );

BEGIN_DATADESC( CAI_SpeechFilter )
	DEFINE_KEYFIELD( m_iszSubject,		FIELD_STRING, "subject" ),
	DEFINE_KEYFIELD( m_flIdleModifier,	FIELD_FLOAT, "IdleModifier" ),
	DEFINE_KEYFIELD( m_bNeverSayHello,	FIELD_BOOLEAN, "NeverSayHello" ),
	DEFINE_KEYFIELD( m_bDisabled,		FIELD_BOOLEAN, "StartDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetIdleModifier", InputSetIdleModifier ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_SpeechFilter::Spawn( void )
{
	BaseClass::Spawn();

	gEntList.AddListenerEntity( this );
}

//-----------------------------------------------------------------------------
// Purpose: Connect to all our NPCs
//-----------------------------------------------------------------------------
void CAI_SpeechFilter::Activate( void )
{
	BaseClass::Activate();

	PopulateSubjectList( m_bDisabled );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_SpeechFilter::UpdateOnRemove(void)
{
	gEntList.RemoveListenerEntity( this );
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_SpeechFilter::Enable( bool bEnable )
{
	m_bDisabled = !bEnable;

	PopulateSubjectList( m_bDisabled );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_SpeechFilter::InputEnable( inputdata_t &inputdata )
{
	Enable( true );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_SpeechFilter::InputDisable( inputdata_t &inputdata )
{
	Enable( false );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_SpeechFilter::InputSetIdleModifier( inputdata_t &inputdata )
{
	m_flIdleModifier = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_SpeechFilter::PopulateSubjectList( bool purge )
{
	// Populate the subject list. Try targetname first.
	CBaseEntity *pSearch = NULL;
	int iNumSubjects = 0;
	do
	{
		pSearch = gEntList.FindEntityByName( pSearch, m_iszSubject );
		if ( pSearch )
		{
#ifndef CSTRIKE_DLL
			CAI_PlayerAlly *pAlly = dynamic_cast<CAI_PlayerAlly *>(pSearch);
			if ( pAlly )
			{
				if( purge )
				{
					pAlly->SetSpeechFilter( NULL );
				}
				else
				{
					if( pAlly->GetSpeechFilter() != NULL )
					{
						DevWarning("ai_speechfilter %s is slamming NPC %s's current speech filter.\n", STRING(GetEntityName()), STRING(pSearch->GetEntityName()) );
					}

					pAlly->SetSpeechFilter( this );
				}
			}
			else if ( pSearch->IsNPC() )
			{
				DevWarning("ai_speechfilter %s tries to use %s as a subject, but it's not a talking NPC.\n", STRING(GetEntityName()), STRING(pSearch->GetEntityName()) );
			}
#endif
			iNumSubjects++;
		}
	} while( pSearch );

	if ( !iNumSubjects )
	{
		// No subjects found by targetname! Assume classname.
		do
		{
			pSearch = gEntList.FindEntityByClassname( pSearch, STRING(m_iszSubject) );
			if( pSearch )
			{
#ifndef CSTRIKE_DLL
				CAI_PlayerAlly *pAlly = dynamic_cast<CAI_PlayerAlly *>(pSearch);
				if ( pAlly )
				{
					if( purge )
					{
						pAlly->SetSpeechFilter( NULL );
					}
					else
					{
						if( pAlly->GetSpeechFilter() != NULL )
						{
							DevWarning("ai_speechfilter %s is slamming NPC %s's current speech filter.\n", STRING(GetEntityName()), STRING(pSearch->GetEntityName()) );
						}

						pAlly->SetSpeechFilter( this );
					}
				}
				else if ( pSearch->IsNPC() )
				{
					DevWarning("ai_speechfilter %s tries to use %s as a subject, but it's not a talking NPC.\n", STRING(GetEntityName()), STRING(pSearch->GetEntityName()) );
				}
#endif
				iNumSubjects++;
			}
		} while( pSearch );
	}

	// If the subject list is still empty, we have an error!
	if ( !iNumSubjects )
	{
		DevMsg( 2, "ai_speechfilter finds no subject(s) called: %s\n", STRING( m_iszSubject ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void CAI_SpeechFilter::OnEntityCreated( CBaseEntity *pEntity )
{
	if ( !m_bDisabled && ( pEntity->NameMatches( m_iszSubject ) || pEntity->ClassMatches( m_iszSubject ) ) )
	{
#ifndef CSTRIKE_DLL
		CAI_PlayerAlly *pAlly = dynamic_cast<CAI_PlayerAlly *>(pEntity);
		if ( pAlly )
		{
			pAlly->SetSpeechFilter( this );
		}
		else if ( pEntity->IsNPC() )
		{
			DevWarning("ai_speechfilter %s tries to use %s as a subject, but it's not a talking NPC.\n", STRING(GetEntityName()), STRING(pEntity->GetEntityName()) );
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void CAI_SpeechFilter::OnEntityDeleted( CBaseEntity *pEntity )
{
}
