//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basecombatweapon.h"
#include "explode.h"
#include "eventqueue.h"
#include "gamerules.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "game.h"
#ifdef MAPBASE
#include <vgui_controls/Controls.h> 
#include <vgui/ILocalize.h>
#include "utlbuffer.h"
#include "saverestore_utlvector.h"
#endif

#include "player.h"
#include "entitylist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Spawnflags
#define SF_MESSAGE_DISABLED		1

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMessageEntity : public CPointEntity
{
	DECLARE_CLASS( CMessageEntity, CPointEntity );

public:
	void	Spawn( void );
	void	Activate( void );
	void	Think( void );
#ifdef MAPBASE
	virtual
#endif
	void	DrawOverlays(void);

	virtual void UpdateOnRemove();

	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
#ifdef MAPBASE
	virtual void	InputSetMessage( inputdata_t &inputdata );
#endif

	DECLARE_DATADESC();

protected:
	int				m_radius;
	string_t		m_messageText;
	bool			m_drawText;
	bool			m_bDeveloperOnly;
	bool			m_bEnabled;
};

LINK_ENTITY_TO_CLASS( point_message, CMessageEntity );

BEGIN_DATADESC( CMessageEntity )

	DEFINE_KEYFIELD( m_radius, FIELD_INTEGER, "radius" ),
	DEFINE_KEYFIELD( m_messageText, FIELD_STRING, "message" ),
	DEFINE_KEYFIELD( m_bDeveloperOnly, FIELD_BOOLEAN, "developeronly" ),
	DEFINE_FIELD( m_drawText, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,	 "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID,	 "Disable", InputDisable ),
#ifdef MAPBASE
	DEFINE_INPUTFUNC( FIELD_STRING,	 "SetMessage", InputSetMessage ),
#endif

END_DATADESC()

static CUtlVector< CHandle< CMessageEntity > >	g_MessageEntities;

//-----------------------------------------
// Spawn
//-----------------------------------------
void CMessageEntity::Spawn( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );
	m_drawText = false;
	m_bDeveloperOnly = false;
	m_bEnabled = !HasSpawnFlags( SF_MESSAGE_DISABLED );
	//m_debugOverlays |= OVERLAY_TEXT_BIT;		// make sure we always show the text
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageEntity::Activate( void )
{
	BaseClass::Activate();

	CHandle< CMessageEntity > h;
	h = this;
	g_MessageEntities.AddToTail( h );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageEntity::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	CHandle< CMessageEntity > h;
	h = this;
	g_MessageEntities.FindAndRemove( h );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------
// Think
//-----------------------------------------
void CMessageEntity::Think( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	// check for player distance
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	if ( !pPlayer || ( pPlayer->GetFlags() & FL_NOTARGET ) )
		return;

	Vector worldTargetPosition = pPlayer->EyePosition();

	// bail if player is too far away
	if ( (worldTargetPosition - GetAbsOrigin()).Length() > m_radius )
	{
		m_drawText = false;
		return;
	}

	// turn on text
	m_drawText = true;
}
	
//-------------------------------------------
//-------------------------------------------
void CMessageEntity::DrawOverlays(void) 
{
	if ( !m_drawText )
		return;

	if ( m_bDeveloperOnly && !g_pDeveloper->GetInt() )
		return;

	if ( !m_bEnabled )
		return;

	// display text if they are within range
	char tempstr[512];
	Q_snprintf( tempstr, sizeof(tempstr), "%s", STRING(m_messageText) );
	EntityText( 0, tempstr, 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageEntity::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageEntity::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageEntity::InputSetMessage( inputdata_t &inputdata )
{
	char newmessage[256];
	Q_strncpy(newmessage, inputdata.value.String(), sizeof(newmessage));

	m_messageText = AllocPooledString(newmessage);
}
#endif

// This is a hack to make point_message stuff appear in developer 0 release builds
//  for now
void DrawMessageEntities()
{
	int c = g_MessageEntities.Count();
	for ( int i = c - 1; i >= 0; i-- )
	{
		CMessageEntity *me = g_MessageEntities[ i ];
		if ( !me )
		{
			g_MessageEntities.Remove( i );
			continue;
		}

		me->DrawOverlays();
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMessageEntityLocalized : public CMessageEntity
{
	DECLARE_CLASS( CMessageEntityLocalized, CMessageEntity );

public:
	bool	KeyValue(const char *szKeyName, const char *szValue);
	void	SetMessage(const char *szValue);
	void	DrawOverlays(void);
	void	InputSetMessage( inputdata_t &inputdata );

	CUtlVector<string_t> m_messageLines;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( point_message_localized, CMessageEntityLocalized );

BEGIN_DATADESC( CMessageEntityLocalized )

	DEFINE_UTLVECTOR( m_messageLines, FIELD_STRING ),

	//DEFINE_INPUTFUNC( FIELD_STRING,	 "SetMessage", InputSetMessage ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Handles key values from the BSP before spawn is called.
//-----------------------------------------------------------------------------
bool CMessageEntityLocalized::KeyValue(const char *szKeyName, const char *szValue)
{
	if (FStrEq(szKeyName, "message"))
	{
		SetMessage(szValue);
		return true;
	}

	return BaseClass::KeyValue(szKeyName, szValue);
}

// I would use "\\n", but Hammer doesn't let you use back slashes.
#define CONVERSION_CHAR "/n"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageEntityLocalized::SetMessage(const char *szValue)
{
	// Find a localization token matching this string
	wchar_t *pszMessage = g_pVGuiLocalize->Find(szValue);

	// If this is a localized string, convert it back to char.
	// If it isn't, just copy it right into this.
	char szBackToChar[256];
	if (pszMessage)
		g_pVGuiLocalize->ConvertUnicodeToANSI(pszMessage, szBackToChar, sizeof(szBackToChar));
	else
		Q_strncpy(szBackToChar, szValue, sizeof(szBackToChar));

	// szTemp is used to turn \n from localized strings into /n.
	char szTemp[256];
	if (Q_strstr(szBackToChar, "\n"))
	{
		char *token = strtok(szBackToChar, "\n");
		while (token)
		{
			Q_snprintf(szTemp, sizeof(szTemp), "%s%s%s", szTemp, token, CONVERSION_CHAR);
			token = strtok(NULL, "\n");
		}
	}
	else
	{
		Q_strncpy(szTemp, szBackToChar, sizeof(szTemp));
	}

	m_messageLines.RemoveAll();

	CUtlStringList vecLines;
	Q_SplitString(szTemp, CONVERSION_CHAR, vecLines);
	FOR_EACH_VEC( vecLines, i )
	{
		m_messageLines.AddToTail( AllocPooledString(vecLines[i]) );
	}

	vecLines.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageEntityLocalized::InputSetMessage( inputdata_t &inputdata )
{
	SetMessage(inputdata.value.String());
}

//-------------------------------------------
//-------------------------------------------
void CMessageEntityLocalized::DrawOverlays(void) 
{
	if ( !m_drawText )
		return;

	if ( m_bDeveloperOnly && !g_pDeveloper->GetInt() )
		return;

	if ( !m_bEnabled )
		return;

	// display text if they are within range
	int offset = 0;
	FOR_EACH_VEC( m_messageLines, i )
	{
		EntityText( offset, STRING(m_messageLines[i]), 0 );
		offset++;
	}
}
#endif
