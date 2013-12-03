//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

// This defines things that allow particle effects to hook into the particle app.
#ifndef PARTICLE_PROTOTYPE_H
#define PARTICLE_PROTOTYPE_H


class CParticleMgr;
class RecvTable;


// Command-line args can be passed in to set state or options in the effects.
class IPrototypeArgAccess
{
public:
	virtual const char*	FindArg(const char *pName, const char *pDefault=0)=0;
};



// You must implement this interface for the prototype app to be able to run your effect.
class IPrototypeAppEffect
{
public:
	virtual				~IPrototypeAppEffect()	{}
	
	// Start the effect. You can get command-line args with pArgs.
	virtual void		Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)=0;
	
	// Return false if you don't allow properties to be edited in the prototype app.
	virtual bool		GetPropEditInfo(RecvTable **ppTable, void **ppObj) {return false;}
};



// Used internally.
typedef IPrototypeAppEffect* (*PrototypeEffectCreateFn)();
class PrototypeEffectLink
{
public:
							PrototypeEffectLink(PrototypeEffectCreateFn fn, const char *pName);								

	PrototypeEffectCreateFn	m_CreateFn;
	const char				*m_pEffectName;
	PrototypeEffectLink		*m_pNext;
};


#ifdef PARTICLEPROTOTYPE_APP
	extern PrototypeEffectLink *g_pPrototypeEffects;	// The list of prototype effects..


	// Expose your effect with this macro.
	#define EXPOSE_PROTOTYPE_EFFECT(effectName, className)								\
		static IPrototypeAppEffect* ___Create##effectName##() {return new className;}	\
		static PrototypeEffectLink ___effectlink_##effectName##(___Create##effectName##, #effectName);
#else
	#define EXPOSE_PROTOTYPE_EFFECT(effectName, className)
#endif


#endif


