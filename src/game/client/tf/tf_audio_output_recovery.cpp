#include "cbase.h"
#ifdef _WIN64
#include "igamesystem.h"
#include "GameUI/AudioOutputWatchWin.h"
#include "GameUI/AudioOutputRecoveryPolicy.h"

// Runs independently of the options dialog, including the disconnected menu.
class CTFAudioOutputRecovery : public CAutoGameSystemPerFrame
{
public:
 CTFAudioOutputRecovery() : CAutoGameSystemPerFrame("TFAudioOutputRecovery"),
  m_Started(false), m_Attempted(false), m_Stopping(false), m_LevelReady(false), m_LevelTransition(false), m_NextSample(0) {}
 void Shutdown() OVERRIDE { m_Stopping=true; m_Watch.Stop(); m_Started=false; }
 void LevelInitPreEntity() OVERRIDE { m_LevelReady=false; m_LevelTransition=true; }
 void LevelInitPostEntity() OVERRIDE { m_LevelReady=true; m_LevelTransition=false; }
 void LevelShutdownPreEntity() OVERRIDE { m_LevelReady=false; m_LevelTransition=true; }
 void LevelShutdownPostEntity() OVERRIDE { m_LevelTransition=false; }
 void Update(float) OVERRIDE
 {
  if(m_Stopping || !engine || !cvar) return;
  ConVar *preference=cvar->FindVar("sound_device_override");
  ConCommand *restart=cvar->FindCommand("snd_restart");
  if(!preference || !restart || !preference->IsFlagSet(FCVAR_ARCHIVE) ||
   !preference->IsFlagSet(FCVAR_NOT_CONNECTED) ||
   preference->IsFlagSet(FCVAR_NEVER_AS_STRING | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY |
    FCVAR_HIDDEN | FCVAR_REPLICATED | FCVAR_MATERIAL_THREAD_MASK) ||
   restart->IsFlagSet(FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY)) return;
  const char *id=preference->GetString();
  bool changed=V_strcmp(id,m_Preference.Get())!=0;
  if(changed) { m_Preference=id; m_Policy.Reset(); }
  if(!*id) { m_Policy.Reset(); return; } // No recovery for System Default.
  if(!m_Attempted) { m_Attempted=true; m_Started=m_Watch.Start(); changed=true; }
  if(!m_Started) return;
  double now=Plat_FloatTime();
  bool event=m_Watch.Poll();
  if(!event && !changed && (!m_Policy.NeedsObservation(now) || now<m_NextSample)) return;
  m_NextSample=now+0.25;
  bool active=false;
  if(!m_Watch.Query(m_Preference.Get(),active)) { m_Policy.Reset(); return; }
  bool blocked=!RecoveryAllowed();
  if(!m_Policy.Observe(active,now,blocked)) return;
  // Consume the transition before execution; no delayed command survives shutdown
  // or a later preference/connection change. This is a fixed trusted command.
  if(V_strcmp(preference->GetString(),m_Preference.Get()) || !RecoveryAllowed()) return;
  DevMsg("Audio output: requesting recovery of the reconnected preferred endpoint.\n");
  engine->ExecuteClientCmd("snd_restart\n");
 }
private:
 bool RecoveryAllowed() const
 {
  return AudioRecoveryAllowed(engine->IsConnected(),engine->IsInGame(),
   engine->IsDrawingLoadingImage(),m_LevelReady,m_LevelTransition,m_Stopping);
 }
 CAudioOutputWatchWin m_Watch;
 CAudioOutputRecoveryPolicy m_Policy;
 CUtlString m_Preference;
 bool m_Started,m_Attempted,m_Stopping,m_LevelReady,m_LevelTransition;
 double m_NextSample;
};
static CTFAudioOutputRecovery g_TFAudioOutputRecovery;
#endif
