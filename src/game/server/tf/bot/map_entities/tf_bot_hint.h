//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_hint.h
// Designer-placed hint for TFBots

#ifndef TF_BOT_HINT_H
#define TF_BOT_HINT_H

class CTFBot;

//-----------------------------------------------------------------------------------------------------
/**
 * An entity that specifies TFBot behavior hints.
 */
class CTFBotHint : public CBaseEntity
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTFBotHint, CBaseEntity );

	CTFBotHint( void );
	virtual ~CTFBotHint() { }

	enum HintType
	{
		HINT_SNIPER_SPOT = 0,
		HINT_SENTRY_SPOT = 1,
	};

	bool IsA( HintType type ) const;

	bool IsFor( CTFBot *who ) const;				// return true if this hint applies to the given entity

	virtual void Spawn( void );
	virtual void UpdateOnRemove( void );

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	bool IsEnabled( void ) const { return !m_isDisabled; }

protected:
	int m_team;
	int m_hint;
	bool m_isDisabled;

	void UpdateNavDecoration( void );
};

inline bool CTFBotHint::IsA( HintType type ) const
{
	return ( m_hint == type );
}


#endif // TF_BOT_HINT_H
