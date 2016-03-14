#ifndef SERVER_EVENTS_H
#define SERVER_EVENTS_H
#ifdef _WIN32
#pragma once
#endif

namespace Momentum {

void OnServerDLLInit();
void OnMapStart(const char *pMapName);
void OnMapEnd(const char *pMapName);
void OnGameFrameStart();
void GameInit();
//void OnGameFrameEnd();

} // namespace Momentum

#endif // SERVER_EVENTS_H
