//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_GET_AMMO_H
#define HL2MP_BOT_GET_AMMO_H

class CHL2MPBotGetAmmo : public Action< CHL2MPBot >
{
public:
	CHL2MPBotGetAmmo( void );

	static bool IsPossible( CHL2MPBot *me );			// return true if this Action has what it needs to perform right now

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual EventDesiredResult< CHL2MPBot > OnContact( CHL2MPBot *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual EventDesiredResult< CHL2MPBot > OnStuck( CHL2MPBot *me );
	virtual EventDesiredResult< CHL2MPBot > OnMoveToSuccess( CHL2MPBot *me, const Path *path );
	virtual EventDesiredResult< CHL2MPBot > OnMoveToFailure( CHL2MPBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual const char *GetName( void ) const	{ return "GetAmmo"; };

private:
	PathFollower m_path;
	CHandle< CBaseEntity > m_ammo;
};


#endif // HL2MP_BOT_GET_AMMO_H
