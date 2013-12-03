//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef ICHOREOEVENTCALLBACK_H
#define ICHOREOEVENTCALLBACK_H
#ifdef _WIN32
#pragma once
#endif

class CChoreoEvent;
class CChoreoChannel;
class CChoreoActor;
class CChoreoScene;

//-----------------------------------------------------------------------------
// Purpose: During choreo playback, events are triggered by calling back from
//  the scene through this interface.
//-----------------------------------------------------------------------------
abstract_class IChoreoEventCallback
{
public:
	virtual void StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event ) = 0;
	// Only called for events with HasEndTime() == true
	virtual void EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event ) = 0;
	// Called for events which have been started but aren't done yet
	virtual void ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event ) = 0;
	// Called for events that are part of a pause condition
	virtual bool CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event ) = 0;
};

#endif // ICHOREOEVENTCALLBACK_H
