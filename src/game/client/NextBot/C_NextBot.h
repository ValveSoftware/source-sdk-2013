// C_NextBot.h
// Next generation bot system
// Author: Michael Booth, April 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _C_NEXT_BOT_H_
#define _C_NEXT_BOT_H_

#include "c_ai_basenpc.h"

//----------------------------------------------------------------------------------------------------------------
/**
* The interface holding IBody information
*/
class IBodyClient
{
public:
	enum ActivityType 
	{ 
		MOTION_CONTROLLED_XY	= 0x0001,	// XY position and orientation of the bot is driven by the animation.
		MOTION_CONTROLLED_Z		= 0x0002,	// Z position of the bot is driven by the animation.
		ACTIVITY_UNINTERRUPTIBLE= 0x0004,	// activity can't be changed until animation finishes
		ACTIVITY_TRANSITORY		= 0x0008,	// a short animation that takes over from the underlying animation momentarily, resuming it upon completion
		ENTINDEX_PLAYBACK_RATE	= 0x0010,	// played back at different rates based on entindex
	};
};


//--------------------------------------------------------------------------------------------------------
/**
 * The client-side implementation of the NextBot
 */
class C_NextBotCombatCharacter : public C_BaseCombatCharacter
{
public:
	DECLARE_CLASS( C_NextBotCombatCharacter, C_BaseCombatCharacter );
	DECLARE_CLIENTCLASS();

	C_NextBotCombatCharacter();
	virtual ~C_NextBotCombatCharacter();

public:	
	virtual void Spawn( void );
	virtual void UpdateClientSideAnimation( void );
	virtual ShadowType_t ShadowCastType( void );
	virtual bool IsNextBot() { return true; }
	void ForceShadowCastType( bool bForce, ShadowType_t forcedShadowType = SHADOWS_NONE ) { m_bForceShadowType = bForce; m_forcedShadowType = forcedShadowType; }
	bool GetForcedShadowCastType( ShadowType_t* pForcedShadowType ) const;

	// Local In View Data.
	void InitFrustumData( void )						{ m_bInFrustum = false; m_flFrustumDistanceSqr = FLT_MAX; m_nInFrustumFrame = gpGlobals->framecount; }
	bool IsInFrustumValid( void )						{ return ( m_nInFrustumFrame == gpGlobals->framecount ); }
	void SetInFrustum( bool bInFrustum )				{ m_bInFrustum = bInFrustum; }
	bool IsInFrustum( void )							{ return m_bInFrustum; }
	void SetInFrustumDistanceSqr( float flDistance )	{ m_flFrustumDistanceSqr = flDistance; }
	float GetInFrustumDistanceSqr( void )				{ return m_flFrustumDistanceSqr; }

private:
	ShadowType_t	m_shadowType;			// Are we LOD'd to simple shadows?
	CountdownTimer m_shadowTimer;	// Timer to throttle checks for shadow LOD
	ShadowType_t	m_forcedShadowType;
	bool			m_bForceShadowType;
	void UpdateShadowLOD( void );

	// Local In View Data.
	int			m_nInFrustumFrame;
	bool		m_bInFrustum;
	float		m_flFrustumDistanceSqr;

private:
	C_NextBotCombatCharacter( const C_NextBotCombatCharacter & );				// not defined, not accessible
};

//--------------------------------------------------------------------------------------------------------
/**
 * The C_NextBotManager manager 
 */
class C_NextBotManager
{
public:
	C_NextBotManager( void );
	~C_NextBotManager();

	/**
	 * Execute functor for each NextBot in the system.
	 * If a functor returns false, stop iteration early
	 * and return false.
	 */	
	template < typename Functor >
	bool ForEachCombatCharacter( Functor &func )
	{
		for( int i=0; i < m_botList.Count(); ++i )
		{
			C_NextBotCombatCharacter *character = m_botList[i];
			if ( character->IsPlayer() )
			{
				continue;
			}

			if ( character->IsDormant() )
			{
				continue;
			}

			if ( !func( character ) )
			{
				return false;
			}
		}

		return true;
	}

	int	 GetActiveCount()						    { return m_botList.Count(); }

	bool SetupInFrustumData( void );
	bool IsInFrustumDataValid( void )				{ return ( m_nInFrustumFrame == gpGlobals->framecount ); }

private:
	friend class C_NextBotCombatCharacter;

	void Register( C_NextBotCombatCharacter *bot );
	void UnRegister( C_NextBotCombatCharacter *bot );

	CUtlVector< C_NextBotCombatCharacter * > m_botList;				///< list of all active NextBots

	int	m_nInFrustumFrame;
};

// singleton accessor
extern C_NextBotManager &TheClientNextBots( void );


#endif // _C_NEXT_BOT_H_
