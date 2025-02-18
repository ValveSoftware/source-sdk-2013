//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ANIMATIONCONTROLLER_H
#define ANIMATIONCONTROLLER_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>

#include "tier1/utlsymbol.h"
#include "tier1/utlvector.h"

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Handles controlling panel animation
//			It is never visible, but needs to be a panel so that can receive messages
//-----------------------------------------------------------------------------
class AnimationController : public Panel
{
	DECLARE_CLASS_SIMPLE( AnimationController, Panel );

public:
	AnimationController(Panel *parent);
	~AnimationController();

	// sets which script file to use
	bool SetScriptFile( VPANEL sizingPanel, const char *fileName, bool wipeAll = false );

	// reloads the currently set script file
	void ReloadScriptFile();

	// runs a frame of animation (time is passed in so slow motion, etc. works)
	void UpdateAnimations( float curtime );
	
	int	 GetNumActiveAnimations( void ) { return m_ActiveAnimations.Count(); }

	// plays all animations to completion instantly
	void RunAllAnimationsToCompletion();

	// stops all animations
	void CancelAllAnimations();

	// starts an animation sequence script
	bool StartAnimationSequence(const char *sequenceName, bool bCanBeCancelled = true );
	bool StartAnimationSequence(Panel *pWithinParent, const char *sequenceName, bool bCanBeCancelled = true );

	bool StopAnimationSequence( Panel *pWithinParent, const char *sequenceName );
	void CancelAnimationsForPanel( Panel *pWithinParent );

	// gets the length of an animation sequence, in seconds
	float GetAnimationSequenceLength(const char *sequenceName);

	// sets that the script file should be reloaded each time a script is ran
	// used for development
	void SetAutoReloadScript(bool state);

	enum Interpolators_e
	{
		INTERPOLATOR_LINEAR,
		INTERPOLATOR_ACCEL,
		INTERPOLATOR_DEACCEL,
		INTERPOLATOR_PULSE,
		INTERPOLATOR_FLICKER,
		INTERPOLATOR_SIMPLESPLINE, // ease in / out
		INTERPOLATOR_BOUNCE,	   // gravitational bounce
		INTERPOLATOR_BIAS,
		INTERPOLATOR_GAIN,
	};

	// runs the specific animation command (doesn't use script file at all)
	void RunAnimationCommand(vgui::Panel *panel, const char *variable, float targetValue, float startDelaySeconds, float durationSeconds, Interpolators_e interpolator, float animParameter = 0, bool bClearValueQueue = true, bool bCanBeCancelled = true );
	void RunAnimationCommand(vgui::Panel *panel, const char *variable, Color targetValue, float startDelaySeconds, float durationSeconds, Interpolators_e interpolator, float animParameter = 0, bool bClearValueQueue = true, bool bCanBeCancelled = true );

private:
	bool UpdateScreenSize();
	
	bool LoadScriptFile(const char *fileName);
	bool ParseScriptFile(char *pMem, int length);

	void UpdatePostedMessages(bool bRunToCompletion);
	void UpdateActiveAnimations(bool bRunToCompletion);

	bool m_bAutoReloadScript;
	float m_flCurrentTime;

	enum AnimCommandType_e
	{
		CMD_ANIMATE,
		CMD_RUNEVENT,
		CMD_STOPEVENT,
		CMD_STOPANIMATION,
		CMD_STOPPANELANIMATIONS,
		CMD_SETFONT,
		CMD_SETTEXTURE,
		CMD_SETSTRING,
		CMD_RUNEVENTCHILD,
		CMD_FIRECOMMAND,
		CMD_PLAYSOUND,
		CMD_SETVISIBLE,
		CMD_SETINPUTENABLED,
	};

	enum RelativeAlignment
	{
		a_northwest = 0,
		a_north,
		a_northeast,
		a_west,
		a_center,
		a_east,
		a_southwest,
		a_south,
		a_southeast,
	};

	struct RelativeAlignmentLookup
	{
		RelativeAlignment align;
		char const *name;
	};

	// a single animatable value
	// some var types use 1, 2, 3 or all 4 of the values
	struct Value_t
	{
		float a, b, c, d;
	};

	struct AnimAlign_t
	{
		// For Position, Xpos, YPos
		bool				relativePosition;
		UtlSymId_t			alignPanel;
		RelativeAlignment	alignment;
	};

	// info for the animate command
	struct AnimCmdAnimate_t
	{
		UtlSymId_t panel;
		UtlSymId_t variable;
		Value_t target;
		int interpolationFunction;
		float	interpolationParameter;
		float startTime;
		float duration;

		AnimAlign_t align;

	};

	// info for the run event command
	struct AnimCmdEvent_t
	{
		UtlSymId_t event;
		UtlSymId_t variable;
		UtlSymId_t variable2;
		float timeDelay;
	};

	// holds a single command from an animation sequence
	struct AnimCommand_t
	{
		AnimCommandType_e commandType;
		union
		{
			AnimCmdAnimate_t animate;
			AnimCmdEvent_t runEvent;
		} cmdData;
	};

	// holds a full sequence
	struct AnimSequence_t
	{
		UtlSymId_t name;
		float duration;
		CUtlVector<AnimCommand_t> cmdList;
	};

	// holds the list of sequences
	CUtlVector<AnimSequence_t> m_Sequences;

	// list of active animations
	struct ActiveAnimation_t
	{
		PHandle panel;
		UtlSymId_t seqName;		// the sequence this belongs to
		UtlSymId_t variable;
		bool started;
		Value_t startValue;
		Value_t endValue;
		int interpolator;
		float interpolatorParam;
		float startTime;
		float endTime;
		bool canBeCancelled;

		AnimAlign_t align;
	};
	CUtlVector<ActiveAnimation_t> m_ActiveAnimations;

	// posted messages
	struct PostedMessage_t
	{
		AnimCommandType_e commandType; 
		UtlSymId_t seqName;
		UtlSymId_t event;
		UtlSymId_t variable;
		UtlSymId_t variable2;
		float startTime;
		PHandle parent;
		bool canBeCancelled;
	};
	CUtlVector<PostedMessage_t> m_PostedMessages;

	struct RanEvent_t
	{
		UtlSymId_t event;
		Panel *pParent;
	
		bool operator==( const RanEvent_t &other ) const
		{
			return ( event == other.event && pParent == other.pParent );
		}
	};

	// variable names
	UtlSymId_t m_sPosition, m_sSize, m_sFgColor, m_sBgColor;
	UtlSymId_t m_sXPos, m_sYPos, m_sWide, m_sTall;
	UtlSymId_t m_sModelPos;

	// file name
	CUtlVector<UtlSymId_t>	m_ScriptFileNames;

	// runs a single line of the script
	void ExecAnimationCommand(UtlSymId_t seqName, AnimCommand_t &animCommand, Panel *pWithinParent, bool bCanBeCancelled);
	// removes all commands belonging to a script
	void RemoveQueuedAnimationCommands(UtlSymId_t seqName, vgui::Panel *panel = NULL);
	// removes an existing instance of a command
	void RemoveQueuedAnimationByType(vgui::Panel *panel, UtlSymId_t variable, UtlSymId_t sequenceToIgnore);

	// handlers
	void StartCmd_Animate(UtlSymId_t seqName, AnimCmdAnimate_t &cmd, Panel *pWithinParent, bool bCanBeCancelled);
	void StartCmd_Animate(Panel *panel, UtlSymId_t seqName, AnimCmdAnimate_t &cmd, bool bCanBeCancelled);
	void RunCmd_RunEvent(PostedMessage_t &msg);
	void RunCmd_StopEvent(PostedMessage_t &msg);
	void RunCmd_StopPanelAnimations(PostedMessage_t &msg);
	void RunCmd_StopAnimation(PostedMessage_t &msg);
	void RunCmd_SetFont(PostedMessage_t &msg);
	void RunCmd_SetTexture(PostedMessage_t &msg);
	void RunCmd_SetString(PostedMessage_t &msg);

	// value access
	Value_t GetValue(ActiveAnimation_t& anim, Panel *panel, UtlSymId_t var);
	void SetValue(ActiveAnimation_t& anim, Panel *panel, UtlSymId_t var, Value_t &value);

	// interpolation
	Value_t GetInterpolatedValue(int interpolator, float interpolatorParam, float currentTime, float startTime, float endTime, Value_t &startValue, Value_t &endValue);

	void	SetupPosition( AnimCmdAnimate_t& cmd, float *output, char const *psz, int screendimension );
	static RelativeAlignment LookupAlignment( char const *token );
	static RelativeAlignmentLookup g_AlignmentLookup[];

	int		GetRelativeOffset( AnimAlign_t& cmd, bool xcoord );

	VPANEL			m_hSizePanel;
	int				m_nScreenBounds[ 4 ];
};

// singleton accessor for use only by other vgui_controls
extern AnimationController *GetAnimationController();

} // namespace vgui

#endif // ANIMATIONCONTROLLER_H
