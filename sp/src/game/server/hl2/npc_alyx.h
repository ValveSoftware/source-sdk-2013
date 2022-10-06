//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "ai_baseactor.h"
#include "npc_playercompanion.h"
#include "ai_behavior_holster.h"

class CNPC_Alyx : public CNPC_PlayerCompanion
{
public:
	DECLARE_CLASS( CNPC_Alyx, CNPC_PlayerCompanion );

	bool	CreateBehaviors();
	void	Spawn( void );
	void	SelectModel();
	void	Precache( void );
	void	SetupAlyxWithoutParent( void );
	void	CreateEmpTool( void );
	void	PrescheduleThink( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	Activity NPC_TranslateActivity ( Activity activity );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	bool	ShouldLookForBetterWeapon() { return false; }
	bool	IsReadinessCapable() { return false; }
	void	DeathSound( const CTakeDamageInfo &info );

#ifdef MAPBASE
	// Alyx was never meant to automatically unholster her weapon in non-episodic Half-Life 2.
	// Now that all allies can holster/unholster, this is a precaution in case it breaks anything.
	// Try OnFoundEnemy > UnholsterWeapon if you want Alyx to automatically unholster in non-episodic HL2 maps.
	bool	CanUnholsterWeapon() { return false; }

	// Use Alyx's default subtitle color (255,212,255)
	bool	GetGameTextSpeechParams( hudtextparms_t &params ) { params.r1 = 255; params.g1 = 212; params.b1 = 255; return BaseClass::GetGameTextSpeechParams( params ); }
#endif

	EHANDLE	m_hEmpTool;

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};
