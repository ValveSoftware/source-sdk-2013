//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_SPEECHFILTER_H
#define AI_SPEECHFILTER_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAI_SpeechFilter : public CBaseEntity, public IEntityListener
{
	DECLARE_CLASS( CAI_SpeechFilter, CBaseEntity );
public:
	DECLARE_DATADESC();

	void	Spawn( void );
	void	Activate( void );
	void	UpdateOnRemove( void );

	void	Enable( bool bEnable );
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	void	InputSetIdleModifier( inputdata_t &inputdata );

	void	PopulateSubjectList( bool purge = false );


	// Accessors for our NPC
	float	GetIdleModifier( void ) { return m_flIdleModifier; }
	bool	NeverSayHello( void ) { return m_bNeverSayHello; }

	void	OnEntityCreated( CBaseEntity *pEntity );
	void	OnEntityDeleted( CBaseEntity *pEntity );

protected:
	string_t	m_iszSubject;
	float		m_flIdleModifier;	// Multiplier to the percentage chance that our NPC will idle speak
	bool		m_bNeverSayHello;	// If set, the NPC never says hello to the player
	bool		m_bDisabled;
};

#endif // AI_SPEECHFILTER_H
