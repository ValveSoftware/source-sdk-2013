//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Data types used inside constraints for the purpose of playing sounds
//			during movement.
//
//=============================================================================//

#ifndef PHYSCONSTRAINT_SOUNDS_H
#define PHYSCONSTRAINT_SOUNDS_H
#ifdef _WIN32
#pragma once
#endif


#include <mathlib/ssemath.h>
#include "soundenvelope.h"


/** \brief Class to store a sampled history of velocity for an object -- used for certain sound calculations

Although this contains only one sample for now, it exists as an interface 
so as to make simpler the possibility of moving to a ring buffer 
implementation in the future. 

The "sample rate" variable is not nominal: it should be used to specify 
the ClientThink() interval. 

Be sure to use the beginSampling() function for the first sample, and 
addSample() thereafter: this will be relevant and necessary for a ring
buffer implementation (which will have to perform certain initialization).
*/
class VelocitySampler
{	
public:
	/*
	enum
	{
	HISTORY_DEPTH_LOG	= 3, // < log-base-2 of the sampler's array depth
	HISTORY_DEPTH		= (1 << VELOCITY_SAMPLER_HISTORY_DEPTH_LOG),
	};
	*/

	/// Return the internally stored sample rate.
	inline float getSampleRate() 
	{
		return m_fIdealSampleRate;
	}


	/// Store off the first recorded sample for the given object.
	inline void BeginSampling(const Vector &relativeVelocity);

	/// Record a sample. Do this LAST, after calling hasReversed() et al.
	inline void AddSample(const Vector &relativeVelocity);

	/// Using the sample history, determine if the object has reversed direction
	/// with at least the given acceleration (in units/sec^2).
	int HasReversed(const Vector &relativeVelocity, const float thresholdAcceleration[], const unsigned short numThresholds);

	/// Call this in spawn(). (Not a constructor because those are difficult to use in entities.)
	void Initialize(float samplerate);


	/// A convenience function for extracting the linear velocity of one object relative to another.
	inline static Vector GetRelativeVelocity(IPhysicsObject *pObj,	IPhysicsObject *pReferenceFrame);

	/// A convenience function for extracting the angular velocity of one object relative to another.
	inline static Vector GetRelativeAngularVelocity(IPhysicsObject *pObj,	IPhysicsObject *pReferenceFrame);


protected:
	Vector m_prevSample;
	float m_fPrevSampleTime;

	float m_fIdealSampleRate;

};

struct SimpleConstraintSoundProfile
{
	// define the indices of the sound points:
	enum
	{
		kMIN_THRESHOLD, ///< below this no sound is played
		kMIN_FULL,      ///< at this velocity sound is at its loudest

		kHIGHWATER,	///< high water mark for this enum
	} eKeypoints;

	float m_keyPoints[kHIGHWATER];

	/// Number of entries in the reversal sound array 
	enum { kREVERSAL_SOUND_ARRAY_SIZE = 3 };

	/// Acceleration threshold for playing the hard-reverse sound. Divided into sections.
	/// Below the 0th threshold no sound will play.
	float m_reversalSoundThresholds[kREVERSAL_SOUND_ARRAY_SIZE]; 

	/// Get volume for given velocity [0..1]
	float GetVolume(float inVel);
};

float SimpleConstraintSoundProfile::GetVolume(float inVel)
{
	// clamped lerp on 0-1
	if (inVel <= m_keyPoints[kMIN_THRESHOLD])
	{
		return 0;
	}
	else if (inVel >= m_keyPoints[kMIN_FULL])
	{
		return 1;
	}
	else	// lerp...
	{
		return (inVel - m_keyPoints[kMIN_THRESHOLD])/(m_keyPoints[kMIN_FULL] - m_keyPoints[kMIN_THRESHOLD]);
	}
}

class CPhysConstraint;
/** This class encapsulates the data and behavior necessary for a constraint to play sounds.

	For the moment I have no easy means of populating this from an entity's datadesc.
	You should explicitly fill out the fields with eg

	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_keyPoints[SimpleConstraintSoundProfile::kMIN_THRESHOLD] , FIELD_FLOAT, "minSoundThreshold" ),
	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_keyPoints[SimpleConstraintSoundProfile::kMIN_FULL] , FIELD_FLOAT, "maxSoundThreshold" ),
	DEFINE_KEYFIELD( m_soundInfo.m_iszTravelSoundFwd, FIELD_SOUNDNAME, "slidesoundfwd" ),
	DEFINE_KEYFIELD( m_soundInfo.m_iszTravelSoundBack, FIELD_SOUNDNAME, "slidesoundback" ),
	DEFINE_KEYFIELD( m_soundInfo.m_iszReversalSound, FIELD_SOUNDNAME, "reversalsound" ),
	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_reversalSoundThreshold , FIELD_FLOAT, "reversalsoundthreshold" ),

 */
class ConstraintSoundInfo
{
public:
	// no ctor.
	// dtor
	~ConstraintSoundInfo();

	/// Call from the constraint's Activate()
	void OnActivate( CPhysConstraint *pOuter );

	/// Constraint should have a think function that calls this. It should pass in relative velocity
	/// between child and parent. (This need not be linear velocity; it may be angular.)
	void OnThink( CPhysConstraint *pOuter, const Vector &relativeVelocity );

	/// This is how often the think function should be run:
	inline float getThinkRate() const { return 0.09f; }

	/// Call this before the first call to OnThink()
	void StartThinking( CPhysConstraint *pOuter, const Vector &relativeVelocity, const Vector &forwardVector );

	/// Call this if you intend to stop calling OnThink():
	void StopThinking( CPhysConstraint *pOuter );

	/// Call from owner's Precache().
	void OnPrecache( CPhysConstraint *pOuter );


	VelocitySampler m_vSampler;
	SimpleConstraintSoundProfile m_soundProfile;

	Vector m_forwardAxis; ///< velocity in this direction is forward. The opposite direction is backward.

	string_t m_iszTravelSoundFwd,m_iszTravelSoundBack;			// Path/filename of WAV file to play.
	CSoundPatch		*m_pTravelSound;
	bool			m_bPlayTravelSound;

	string_t m_iszReversalSounds[SimpleConstraintSoundProfile::kREVERSAL_SOUND_ARRAY_SIZE];			// Path/filename of WAV files to play -- one per entry in threshold.
	// CSoundPatch		*m_pReversalSound;
	bool			m_bPlayReversalSound;

protected:
	/// Maintain consistency of internal datastructures on start
	void ValidateInternals( CPhysConstraint *pOuter );

	/// Stop playing any active sounds.
	void DeleteAllSounds();
};


/////////////// INLINE FUNCTIONS
	

/// compute the relative velocity between an object and its parent. Just a convenience.
Vector VelocitySampler::GetRelativeVelocity( IPhysicsObject *pObj, IPhysicsObject *pReferenceFrame )
{
	Vector childVelocity, parentVelocity;
	pObj->GetImplicitVelocity( &childVelocity, NULL );
	pReferenceFrame->GetImplicitVelocity(&parentVelocity, NULL);

	return (childVelocity - parentVelocity);
}


Vector VelocitySampler::GetRelativeAngularVelocity( IPhysicsObject *pObj, IPhysicsObject *pReferenceFrame )
{
	Assert(pObj);

	if ( pReferenceFrame )
	{
		Vector childVelocityLocal, parentVelocityLocal, childVelocityWorld, parentVelocityWorld;
		pObj->GetImplicitVelocity( NULL, &childVelocityLocal );
		pObj->LocalToWorldVector( &childVelocityWorld, childVelocityLocal );
		pReferenceFrame->GetImplicitVelocity( NULL, &parentVelocityLocal );
		pObj->LocalToWorldVector( &parentVelocityWorld, parentVelocityLocal );

		return (childVelocityWorld - parentVelocityWorld);
	}
	else
	{
		Vector childVelocityLocal, childVelocityWorld;
		pObj->GetImplicitVelocity( NULL, &childVelocityLocal );
		pObj->LocalToWorldVector( &childVelocityWorld, childVelocityLocal );

		return (childVelocityWorld);
	}
}

/************************************************************************/
// This function is nominal -- it's here as an interface because in the 
// future there will need to be special initialization for the first entry
// in a ring buffer. (I made a test implementation of this, then reverted it
// later; this is not an arbitrary assumption.)
/************************************************************************/
/// Store off the first recorded sample for the given object.
void VelocitySampler::BeginSampling(const Vector &relativeVelocity)
{
	return AddSample(relativeVelocity);
}

// Record a sample for the given object
void VelocitySampler::AddSample(const Vector &relativeVelocity)
{
	m_prevSample = relativeVelocity;
	m_fPrevSampleTime = gpGlobals->curtime;
}



/* // abandoned -- too complicated, no way to set from keyfields
#pragma warning(push)
#pragma warning( disable:4201 ) // C4201: nonstandard extension used: nameless struct/union
/// Stores information used for playing sounds based on 
/// constraint movement
class ConstraintSoundProfile
{
public:
/// Defines a point in the sound profile: volume and pitch for the sound to play.
/// Implicit crossfading between two sounds. Used to map velocity to a sound profile.
struct SoundInfoTuple
{
float minVelocity;
union {
struct{
float volume1,pitch1; //< volume and pitch of sound 1
float volume2,pitch2; //< volume and pitch of sound 2
};
fltx4 m_as4;
};

inline SoundInfoTuple(float _minVelocity, float _volume1, float _pitch1, float _volume2, float _pitch2) :
minVelocity(_minVelocity), volume1(_volume1), pitch1(_pitch1), volume2(_volume2), pitch2(_pitch2) 
{}
};

ConstraintSoundProfile(const SoundInfoTuple *soundTable, unsigned int tableSize) 
: m_pSoundInfos(soundTable), m_numSoundInfos(tableSize) 
{}


protected:

/// A table of sound info structs
const SoundInfoTuple * const m_pSoundInfos;
/// Size of the table
const unsigned int m_numSoundInfos;
};

static ConstraintSoundProfile::SoundInfoTuple CSDebugProfileTable[] =
{
ConstraintSoundProfile::SoundInfoTuple(12,0,0,0,0),
ConstraintSoundProfile::SoundInfoTuple(24,0,0,0,0),

};
#pragma warning(pop)
*/


#endif
