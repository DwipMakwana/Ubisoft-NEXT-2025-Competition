#include "GameState.h"
#include "../UI/UIManager.h"
#include "../ContestAPI/App.h"
#include "../Utilities/Logger.h"
#include "../Rendering/Camera3D.h"
#include <string>

GameState currentState = GameState::MAIN_MENU;
bool gamePaused = true;

static float menuSelectTimer = 0.0f;
static int menuSelection = 0;  // 0=Play, 1=Quit

void GameState_Init() {
    // Load menu sprites if needed (assume "button.png" exists or use text-only)
    // UIManager::LoadSprite("button", "assets/button.png", 1, 1);
    //Logger::LogInfo("GameState initialized - Main Menu");
}

void GameState_Update(float deltaTime) {
    if (currentState == GameState::QUIT) {
        
        return;
    }

    // Clear UI elements each frame for menu
    UIManager::RemoveText("menu_title");
    UIManager::RemoveText("menu_play");
    UIManager::RemoveText("menu_quit");

    if (currentState == GameState::MAIN_MENU) {
        gamePaused = true;  // Pause game logic

        // Title
        UIManager::AddText("menu_title", 100, 600, "Space Wars : Salvage", 0.7f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_18);

        // Play option
        std::string playText = (menuSelection == 0) ? "PLAY <" : "PLAY";
        UIManager::AddText("menu_play", 100, 450, playText, 0.0f, 1.0f, 0.0f, GLUT_BITMAP_HELVETICA_12);

        // Quit option
        std::string quitText = (menuSelection == 1) ? "QUIT <" : "QUIT ";
        UIManager::AddText("menu_quit", 100, 400, quitText, 1.0f, 0.0f, 0.0f, GLUT_BITMAP_HELVETICA_12);

		// Description
        UIManager::AddText("menu_desc", 100, 200, "Try to save your planet and maintian the resources, have fun!", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);

    }
    else if (currentState == GameState::PLAYING) {
        gamePaused = false;
        // Add in-game UI like FPS, score if needed
    }
}

void GameState_Render(const Camera3D& camera) {
    if (currentState == GameState::MAIN_MENU) {
        // Game renders in background (paused), menu overlays via UI
    }
    UIManager::Render();
}

void GameState_HandleInput() {
    if (currentState == GameState::MAIN_MENU) {
        if (App::IsKeyPressed(App::KEY_UP) || App::IsKeyPressed(App::KEY_W)) {
            menuSelection = 0;
        }
        else if (App::IsKeyPressed(App::KEY_DOWN) || App::IsKeyPressed(App::KEY_S)) {
            menuSelection = 1;
        }
        else if (App::IsKeyPressed(App::KEY_ENTER) || App::IsKeyPressed(App::KEY_SPACE)) {
            if (menuSelection == 0) {
                currentState = GameState::PLAYING;
                //Logger::LogInfo("Starting game!");
            }
            else {
                currentState = GameState::QUIT;
            }
        }
    }
    else if (currentState == GameState::PLAYING) {
        if (App::IsKeyPressed(App::KEY_ESC)) {
            currentState = GameState::PAUSED;
            gamePaused = true;
            //Logger::LogInfo("Returning to main menu");
        }
    }
    else if (currentState == GameState::PAUSED) {
        if (App::IsKeyPressed(App::KEY_ESC)) {
            currentState = GameState::PLAYING;
            gamePaused = false;
            //Logger::LogInfo("Returning to main menu");
        }
    }
}
