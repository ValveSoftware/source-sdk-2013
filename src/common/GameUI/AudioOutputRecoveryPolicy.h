#ifndef AUDIO_OUTPUT_RECOVERY_POLICY_H
#define AUDIO_OUTPUT_RECOVERY_POLICY_H

// Restoration never writes the preference. Established gameplay is allowed;
// connection setup, map transitions, loading and final shutdown are not.
inline bool AudioRecoveryAllowed( bool connected, bool inGame, bool loading,
 bool levelReady, bool transitioning, bool stopping )
{
 return !stopping && !loading && !transitioning &&
  ( ( !connected && !inGame ) || ( connected && inGame && levelReady ) );
}

// Pure main-thread transition policy. No engine, COM or wall-clock dependencies.
class CAudioOutputRecoveryPolicy
{
public:
 CAudioOutputRecoveryPolicy() { Reset(); }
 void Reset() { known=false; active=false; armed=false; since=0; suppressUntil=0; }
 // Only a confirmed absence arms recovery. Availability alone never does.
 bool Observe( bool available, double now, bool blocked )
 {
  if ( !known || available != active ) { known=true; active=available; since=now; }
  if ( now < suppressUntil ) { armed=false; since=now; return false; }
  if ( now-since < 1.0 ) return false;
  if ( !active ) { armed=true; return false; }
  if ( !armed || blocked ) return false;
  armed=false;
  suppressUntil=now+10.0; // Ignore device churn caused by our own restart.
  return true;
 }
 bool NeedsObservation( double now ) const
 {
  return known && ( now-since < 1.0 || ( active && armed ) || now < suppressUntil );
 }
private:
 bool known, active, armed;
 double since, suppressUntil;
};
#endif
