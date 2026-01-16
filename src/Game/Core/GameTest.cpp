#include "App.h"
#include "AppSettings.h"
#include "../Rendering/Camera3D.h"
#include "../Rendering/Renderer3D.h"
#include "../Rendering/Mesh3D.h"
#include "../Systems/LightingSystem.h"
#include "../UI/UIManager.h"
#include "../Physics/PhysicsSystem.h"
#include "ctime"
#include "algorithm"
#include <Utilities/Logger.h>
#include <Managers/AudioManager.h>

Camera3D camera3D;
Mesh3D planeMesh, sphereMesh, cubeMesh, suzzaneMesh;
PhysicsSystem physicsSystem;
AudioManager audioManager;
LightingSystem* lighting = nullptr;

CSimpleSprite* testSprite = nullptr;

bool renderWireframe = false;
bool showColliders = false;

void Init() {
    srand((unsigned int)time(nullptr));

    // Camera setup
    camera3D.SetPosition(Vec3(8, 6, 10));
    camera3D.SetTarget(Vec3(0, 1, 0));
    camera3D.SetUp(Vec3(0, 1, 0));

    // Create meshes
    float planeWidth = 20.0f;
    float planeDepth = 20.0f;
    planeMesh = Mesh3D::CreatePlane(planeWidth, planeDepth);
    sphereMesh = Mesh3D::CreateSphere(1.0f, 16);
    cubeMesh = Mesh3D::CreateCube(1.0f);
    bool suzanneLoaded = suzzaneMesh.LoadFromOBJ("../../../data/TestData/Suzanne.obj");

    if (suzanneLoaded) {
        printf("✓ Suzanne loaded successfully!\n");
        physicsSystem.SetSuzanneMesh(&suzzaneMesh);
    }
    else {
        printf("✗ Warning: Could not load Suzanne from Assets/suzanne.obj\n");
        printf("  Spawning type 2 will fallback to sphere.\n");
        physicsSystem.SetSuzanneMesh(nullptr);
    }

    // Setup lighting
    LightingSystem::Init();
    LightingSystem::ambientColor = Vec3(0.3f, 0.3f, 0.4f);
    LightingSystem::lightColor = Vec3(1.0f, 0.95f, 0.8f);
    LightingSystem::lightIntensity = 0.8f;

    // Setup Audio
    audioManager.Initialize();
    audioManager.SetCategoryVolume(AudioCategory::SFX, 0.8f);

    // UI
    UIManager::Init();

    UIManager::LoadSprite("character", "../../../data/TestData/Test.bmp", 8, 4);  // 8x4 sprite
    // Add sprite instances
    UIManager::AddSprite("player", "character", 100, 100, 64, 64);
    UIManager::SetSpriteLayer("player", 2);      // Top

    // Setup walking animation for character (frames 8-15)
    UIManager::PlaySpriteAnimation("player", 0, 31, 0.1f);

    UIManager::AddText("controls", 20, 70,
        "WASD/E/Q: Move Camera | SPACE: Spawn Shape | F: Wireframe | C: Show Colliders",
        1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_12);
    UIManager::AddText("bodycount", 20, 20, "Bodies: 0", 1.0f, 1.0f, 0.0f, 
        GLUT_BITMAP_HELVETICA_12);
}

void Update(float deltaTime) {
    float dt = deltaTime / 1000.0f;

    // Camera controls
    float forward = 0, right = 0, up = 0;
    if (App::IsKeyPressed(App::KEY_W)) forward += 1;
    if (App::IsKeyPressed(App::KEY_S)) forward -= 1;
    if (App::IsKeyPressed(App::KEY_D)) right += 1;
    if (App::IsKeyPressed(App::KEY_A)) right -= 1;
    if (App::IsKeyPressed(App::KEY_E)) up += 1;
    if (App::IsKeyPressed(App::KEY_Q)) up -= 1;
    camera3D.Flythrough(forward, right, up, 0, 0);

    // Toggle wireframe
    static bool fWas = false;
    bool fNow = App::IsKeyPressed(App::KEY_F);
    if (fNow && !fWas) {
        renderWireframe = !renderWireframe;
    }
    fWas = fNow;

    // Toggle collider visualization
    static bool cWas = false;
    bool cNow = App::IsKeyPressed(App::KEY_C);
    if (cNow && !cWas) {
        showColliders = !showColliders;
    }
    cWas = cNow;

    // Spawn random shape on SPACE
    static bool spaceWas = false;
    bool spaceNow = App::IsKeyPressed(App::KEY_SPACE);
    if (spaceNow && !spaceWas) {
        int type = rand() % 3;
        physicsSystem.SpawnBody(type);
        printf("Spawned %s\n", type == 0 ? "sphere" : "cube");

        audioManager.PlaySound("../../../data/TestData/Test.wav", false, AudioCategory::SFX);
    }
    spaceWas = spaceNow;

    // Update physics
    physicsSystem.Update(dt);

    // Update UI
    UIManager::Update(dt);
}

void Render() {
    // Draw ground plane - FIXED SCALE
    Renderer3D::DrawMesh(
        planeMesh,
        Vec3(0, 0, 0),           // Position at origin
        Vec3(0, 0, 0),           // No rotation
        Vec3(1, 1, 1),           // FIXED: Normal scale (was 10,1,10)
        camera3D,
        0.5f, 0.5f, 0.5f,       // Gray color
        renderWireframe
    );

    // Render all physics bodies
    physicsSystem.Render(camera3D, sphereMesh, cubeMesh, renderWireframe);

    // Optional: Draw collider visualization
    if (showColliders) {
        // Draw ground plane collider in green wireframe
        Renderer3D::DrawMesh(
            planeMesh,
            Vec3(0, 0, 0),           // Position at ground level
            Vec3(0, 0, 0),           // No rotation
            Vec3(1, 1, 1),           // Normal scale
            camera3D,
            0.0f, 1.0f, 0.0f,       // GREEN color
            true                     // Force wireframe
        );

        for (const auto& body : physicsSystem.bodies) {
            Vec3 rotDegrees = body.rotation * (180.0f / 3.14159265359f);

            if (body.colliderType == ColliderType::SPHERE) {
                Renderer3D::DrawMesh(
                    sphereMesh,
                    body.position,
                    Vec3(0, 0, 0),
                    Vec3(body.radius, body.radius, body.radius),
                    camera3D,
                    0.0f, 1.0f, 0.0f,  // Green
                    true  // Force wireframe
                );
            }
            else if (body.colliderType == ColliderType::BOX) {
                Renderer3D::DrawMesh(
                    cubeMesh,
                    body.position,
                    rotDegrees,
                    body.halfExtents * 2.0f,
                    camera3D,
                    0.0f, 1.0f, 0.0f,  // Green
                    true  // Force wireframe
                );
            }
            else if (body.colliderType == ColliderType::CONVEX_HULL) {
                if (body.renderMeshRef != nullptr) {
                    Renderer3D::DrawMesh(
                        *body.renderMeshRef,
                        body.position,
                        rotDegrees,
                        Vec3(1.0f, 1.0f, 1.0f),  // Normal scale
                        camera3D,
                        0.0f, 1.0f, 0.0f,  // Green wireframe
                        true  // Force wireframe
                    );
                }
                else {
                    // Fallback: draw bounding sphere if mesh not available
                    Renderer3D::DrawMesh(
                        sphereMesh,
                        body.position,
                        Vec3(0, 0, 0),
                        Vec3(body.radius, body.radius, body.radius),
                        camera3D,
                        0.0f, 1.0f, 0.0f,
                        true
                    );
                }
            }
        }
    }

    UIManager::Render();
}

void Shutdown() {
    UIManager::Shutdown();
    audioManager.Shutdown();
}
