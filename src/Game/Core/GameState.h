#ifndef GAMESTATE_H
#define GAMESTATE_H

class Camera3D;

enum class GameState {
    MAIN_MENU,
    PLAYING,
    PAUSED,
    QUIT
};

extern GameState currentState;
extern bool gamePaused;  // For game logic pause (dt=0 when true)

void GameState_Init();
void GameState_Update(float deltaTime);
void GameState_Render(const Camera3D& camera);
void GameState_HandleInput();

#endif
