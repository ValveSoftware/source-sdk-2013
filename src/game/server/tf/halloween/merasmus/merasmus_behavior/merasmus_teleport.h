//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MERASMUS_TELEPORT_H
#define MERASMUS_TELEPORT_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CMerasmusTeleport : public Action< CMerasmus >
{
public:
	CMerasmusTeleport( bool bShouldAOE, bool bGoToCap );

	virtual ActionResult< CMerasmus >	OnStart( CMerasmus *me, Action< CMerasmus > *priorAction );
	virtual ActionResult< CMerasmus >	Update( CMerasmus *me, float interval );

	virtual const char *GetName( void ) const	{ return "Teleport"; }		// return name of this action

private:
	enum TeleportState
	{
		TELEPORTING_OUT,
		TELEPORTING_IN,
		DONE
	};
	TeleportState m_state;

	bool m_bShouldAOE;
	bool m_bShouldGoToCap;

	Vector GetTeleportPosition( CMerasmus *me ) const;
};


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CMerasmusEscape : public Action< CMerasmus >
{
public:
	virtual ActionResult< CMerasmus >	OnStart( CMerasmus *me, Action< CMerasmus > *priorAction );
	virtual ActionResult< CMerasmus >	Update( CMerasmus *me, float interval );

	virtual const char *GetName( void ) const	{ return "Escape"; }		// return name of this action
};


#endif // MERASMUS_TELEPORT_H
