//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_roster.h
// entity that dictates what classes a bot can choose when spawning
// Tom Bui, April 2010

#ifndef TF_BOT_ROSTER_H
#define TF_BOT_ROSTER_H

class CTFBotRoster : public CPointEntity
{
	DECLARE_CLASS( CTFBotRoster, CPointEntity );
public:
	DECLARE_DATADESC();

	CTFBotRoster( void );
	virtual ~CTFBotRoster() {}

	// input
	void InputSetAllowScout( inputdata_t &inputdata );
	void InputSetAllowSniper( inputdata_t &inputdata );
	void InputSetAllowSoldier( inputdata_t &inputdata );
	void InputSetAllowDemoman( inputdata_t &inputdata );
	void InputSetAllowMedic( inputdata_t &inputdata );
	void InputSetAllowHeavy( inputdata_t &inputdata );
	void InputSetAllowPyro( inputdata_t &inputdata );
	void InputSetAllowSpy( inputdata_t &inputdata );
	void InputSetAllowEngineer( inputdata_t &inputdata );

	// misc.
	bool IsClassAllowed( int iBotClass ) const;
	bool IsClassChangeAllowed() const;

public:
	string_t m_teamName;
	bool m_bAllowClassChanges;
	bool m_bAllowedClasses[TF_LAST_NORMAL_CLASS];
};

#endif // TF_BOT_ROSTER_H
