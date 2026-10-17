#ifndef AUDIO_OUTPUT_WATCH_WIN_H
#define AUDIO_OUTPUT_WATCH_WIN_H
#ifdef _WIN64
// Start/Poll/Query/Stop must all run on the same game thread.
class CAudioOutputWatchWin
{
public:
 CAudioOutputWatchWin();
 ~CAudioOutputWatchWin();
 bool Start();
 void Stop();
 bool Poll();
 // false means unknown/error; never infer device absence from a COM error.
 bool Query( const char *utf8ID, bool &active );
private:
 struct State;
 State *m_State;
 CAudioOutputWatchWin( const CAudioOutputWatchWin & );
 CAudioOutputWatchWin &operator=( const CAudioOutputWatchWin & );
};
#endif
#endif
