//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VPHYSICS_SOUND_H
#define VPHYSICS_SOUND_H
#ifdef _WIN32
#pragma once
#endif

#include "SoundEmitterSystem/isoundemittersystembase.h"

namespace physicssound
{
	struct impactsound_t
	{
		void			*pGameData;
		int				entityIndex;
		int				soundChannel;
		float			volume;
		float			impactSpeed;
		unsigned short	surfaceProps;
		unsigned short	surfacePropsHit;
		Vector			origin;
	};

	// UNDONE: Use a sorted container and sort by volume/distance?
	struct soundlist_t
	{
		CUtlVector<impactsound_t>	elements;
		impactsound_t	&GetElement(int index) { return elements[index]; }
		impactsound_t	&AddElement() { return elements[elements.AddToTail()]; }
		int Count() { return elements.Count(); }
		void RemoveAll() { elements.RemoveAll(); }
	};

	void PlayImpactSounds( soundlist_t &list )
	{
		for ( int i = list.Count()-1; i >= 0; --i )
		{
			impactsound_t &sound = list.GetElement(i);
			const surfacedata_t *psurf = physprops->GetSurfaceData( sound.surfaceProps );
			if ( psurf->sounds.impactHard )
			{
				const surfacedata_t *pHit = physprops->GetSurfaceData( sound.surfacePropsHit );
				unsigned short soundName = psurf->sounds.impactHard;
				if ( pHit && psurf->sounds.impactSoft )
				{
					if ( pHit->audio.hardnessFactor < psurf->audio.hardThreshold || 
						(psurf->audio.hardVelocityThreshold > 0 && psurf->audio.hardVelocityThreshold > sound.impactSpeed) )
					{
						soundName = psurf->sounds.impactSoft;
					}
				}
				const char *pSound = physprops->GetString( soundName );

				CSoundParameters params;
				if ( !CBaseEntity::GetParametersForSound( pSound, params, NULL ) )
					break;

				if ( sound.volume > 1 )
					sound.volume = 1;
				CPASAttenuationFilter filter( sound.origin, params.soundlevel );
				// JAY: If this entity gets deleted, the sound comes out at the world origin
				// this sounds bad!  Play on ent 0 for now.
				EmitSound_t ep;
				ep.m_nChannel = sound.soundChannel;
				ep.m_pSoundName = params.soundname;
				ep.m_flVolume = params.volume * sound.volume;
				ep.m_SoundLevel = params.soundlevel;
				ep.m_nPitch = params.pitch;
				ep.m_pOrigin = &sound.origin;

				CBaseEntity::EmitSound( filter, 0 /*sound.entityIndex*/, ep );
			}
		}
		list.RemoveAll();
	}
	void AddImpactSound( soundlist_t &list, void *pGameData, int entityIndex, int soundChannel, IPhysicsObject *pObject, int surfaceProps, int surfacePropsHit, float volume, float impactSpeed )
	{
		impactSpeed += 1e-4;
		for ( int i = list.Count()-1; i >= 0; --i )
		{
			impactsound_t &sound = list.GetElement(i);
			// UNDONE: Compare entity or channel somehow?
			// UNDONE: Doing one slot per entity is too noisy.  So now we use one slot per material

			// heuristic - after 4 impacts sounds in one frame, start merging everything
			if ( surfaceProps == sound.surfaceProps || list.Count() > 4 )
			{
				// UNDONE: Store instance volume separate from aggregate volume and compare that?
				if ( volume > sound.volume )
				{
					pObject->GetPosition( &sound.origin, NULL );
					sound.pGameData = pGameData;
					sound.entityIndex = entityIndex;
					sound.soundChannel = soundChannel;
					sound.surfacePropsHit = surfacePropsHit;
				}
				sound.volume += volume;
				sound.impactSpeed = MAX(impactSpeed,sound.impactSpeed);
				return;
			}
		}

		impactsound_t &sound = list.AddElement();
		sound.pGameData = pGameData;
		sound.entityIndex = entityIndex;
		sound.soundChannel = soundChannel;
		pObject->GetPosition( &sound.origin, NULL );
		sound.surfaceProps = surfaceProps;
		sound.surfacePropsHit = surfacePropsHit;
		sound.volume = volume;
		sound.impactSpeed = impactSpeed;
	}

	struct breaksound_t
	{
		Vector			origin;
		int				surfacePropsBreak;
	};

	void AddBreakSound( CUtlVector<breaksound_t> &list, const Vector &origin, unsigned short surfaceProps )
	{
		const surfacedata_t *psurf = physprops->GetSurfaceData( surfaceProps );
		if ( !psurf->sounds.breakSound )
			return;

		for ( int i = list.Count()-1; i >= 0; --i )
		{
			breaksound_t &sound = list.Element(i);
			// Allow 3 break sounds before you start merging anything.
			if ( list.Count() > 2 && surfaceProps == sound.surfacePropsBreak )
			{
				sound.origin = (sound.origin + origin) * 0.5f;
				return;
			}
		}
		breaksound_t sound;
		sound.origin = origin;
		sound.surfacePropsBreak = surfaceProps;
		list.AddToTail(sound);

	}

	void PlayBreakSounds( CUtlVector<breaksound_t> &list )
	{
		for ( int i = list.Count()-1; i >= 0; --i )
		{
			breaksound_t &sound = list.Element(i);

			const surfacedata_t *psurf = physprops->GetSurfaceData( sound.surfacePropsBreak );
			const char *pSound = physprops->GetString( psurf->sounds.breakSound );
			CSoundParameters params;
			if ( !CBaseEntity::GetParametersForSound( pSound, params, NULL ) )
				return;

			// Play from the world, because the entity is breaking, so it'll be destroyed soon
			CPASAttenuationFilter filter( sound.origin, params.soundlevel );
			EmitSound_t ep;
			ep.m_nChannel = CHAN_STATIC;
			ep.m_pSoundName = params.soundname;
			ep.m_flVolume = params.volume;
			ep.m_SoundLevel = params.soundlevel;
			ep.m_nPitch = params.pitch;
			ep.m_pOrigin = &sound.origin;
			CBaseEntity::EmitSound( filter, 0 /*sound.entityIndex*/, ep );
		}
		list.RemoveAll();
	}
};


#endif // VPHYSICS_SOUND_H
