//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MERASMUS_DISGUISE_H
#define MERASMUS_DISGUISE_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CMerasmusDisguise : public Action< CMerasmus >
{
public:
	virtual ActionResult< CMerasmus >	OnStart( CMerasmus *me, Action< CMerasmus > *priorAction );
	virtual ActionResult< CMerasmus >	Update( CMerasmus *me, float interval );
	virtual void OnEnd( CMerasmus *me, Action< CMerasmus > *nextAction );
	virtual const char *GetName( void ) const	{ return "Disguise"; }		// return name of this action

private:
	void TryToDisguiseSpawn( CMerasmus *me );
	CountdownTimer m_findPropsFailTimer;
	CountdownTimer m_findSpawnPositionTime;
	bool m_bSpawnedProps;

	void RandomDisguiseTauntTimer();
	CountdownTimer m_disguiseTauntTimer;
	
	float m_flStartRegenTime;
	int m_nStartRegenHealth;
};


#endif // MERASMUS_DISGUISE_H
