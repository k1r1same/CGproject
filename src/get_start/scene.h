#pragma once

#include <memory>
#include <vector>
#include <chrono>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/glsl_program.h"
#include "../base/model.h"
#include "../base/skybox.h"

enum class GameState {
    Playing,
    GameOver
};

struct Player {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    float moveRange = 5.0f;
    int health = 3;
    float radius = 0.5f;
    std::unique_ptr<Model> model;
};

struct Bullet {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float radius = 0.2f;
    bool active = true;
    std::unique_ptr<Model> model;
};

struct Launcher {
    glm::vec3 position;
    float fireInterval = 1.0f;
    float lastFireTime = 0.0f;
    glm::vec3 targetPosition;
};

class Scene : public Application {
public:
    Scene(const Options& options);
    ~Scene();

    void handleInput() override;
    void renderFrame() override;

private:
    // Game state
    GameState _gameState = GameState::Playing;
    float _gameTime = 0.0f;
    int _currentWave = 1;
    float _waveTime = 30.0f;
    float _waveTimer = 0.0f;
    
    // Camera
    std::unique_ptr<PerspectiveCamera> _camera;
    float _cameraDistance = 15.0f;
    float _cameraAngle = 0.0f;
    
    // Game objects
    Player _player;
    std::vector<Bullet> _bullets;
    std::vector<Launcher> _launchers;
    
    // Rendering
    std::unique_ptr<GLSLProgram> _shader;
    std::unique_ptr<Model> _sphereModel;
    std::unique_ptr<Model> _cylinderModel;
    std::unique_ptr<Model> _turretModel;
    std::unique_ptr<SkyBox> _skybox;
    
    // Game parameters
    float _bulletSpeed = 5.0f;
    int _initialLaunchers = 2;
    int _launchersPerWave = 2;
    float _launcherRadius = 8.0f;
    
    // Methods
    void initShader();
    void initGameObjects();
    void updateGame();
    void updatePlayer();
    void updateBullets();
    void updateLaunchers();
    void updateCamera();
    void checkCollisions();
    void handleWaveTransition();
    void spawnBullet(const Launcher& launcher);
    void destroyBullet(size_t index);
    void renderPlayer();
    void renderBullets();
    void renderLaunchers();
    void renderUI();
    void setupLaunchers(int count);
    void resetGame();
    bool isPlayerHit(const Bullet& bullet) const;
    void takeDamage();
    void saveScreenshot();
    
    // Camera controls
    void handleCameraInput();
    void orbitCamera(float deltaAngle);
    void zoomCamera(float deltaZoom);

    /// <summary>
    /// imgui
    /// </summary>
    void initImGui();
    void clearImGui();
    void renderInspectorPanel();
};