#pragma once

#include <memory>
#include <vector>
#include <chrono>
#include "text.h"

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/glsl_program.h"
#include "../base/model.h"
#include "../base/skybox.h"
#include "../base/texture2d.h"


enum class GameState {
    WaitingToStart,
    Playing,
    WaveBreak,
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
    bool destroying = false;
    float destroyTimer = 0.0f;
    float destroyDuration = 0.5f;
    std::unique_ptr<Model> model;
};

struct Launcher {
    glm::vec3 position;
    float fireInterval = 1.0f;
    float lastFireTime = 0.0f;
    glm::vec3 targetPosition;
};

struct Gun {
    glm::vec3 position{0.5f, -0.35f, -0.75f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
};

struct MuzzleFlash {
    glm::vec3 position{0.5f, -0.1f, -1.2f};
};

class Scene : public Application {
public:
    Scene(const Options& options);
    ~Scene();

    void handleInput() override;
    void renderFrame() override;

private:
    // Game state
    GameState _gameState = GameState::WaitingToStart;
    float _gameTime = 0.0f;
    int _currentWave = 1;
    float _waveTime = 30.0f;
    float _waveTimer = 0.0f;
    float _breakTime = 5.0f;
    float _breakTimer = 0.0f;

    int _currentFlashtex = 0;
    
    // UI effects
    float _blinkTimer = 0.0f;
    bool _showStartText = true;
    
    // Camera
    std::unique_ptr<PerspectiveCamera> _camera;
    float _cameraDistance = 15.0f;
    float _cameraAngle = 0.0f;
    
    // Mouse camera control
    float _mouseSensitivity = 0.1f;
    float _yaw = 0.0f;
    float _pitch = 0.0f;
    bool _firstMouse = true;
    float _lastMouseX = 0.0f;
    float _lastMouseY = 0.0f;
    
    // Free camera movement
    float _cameraMoveSpeed = 10.0f;
    glm::vec3 _freeCameraPos = glm::vec3(0.0f, 5.0f, 15.0f);
    
    // Mouse click state tracking
    bool _prevMouseLeftPressed = false;
    bool _isFlashing = false;
    bool _isRecoiling = false;
    
    // Mouse mode for UI/Camera control
    bool _cameraControlMode = true;
    bool _prevTabPressed = false;
    
    // Game objects
    Player _player;
    std::vector<Bullet> _bullets;
    std::vector<Launcher> _launchers;
    Gun _gun;
    MuzzleFlash _muzzleFlash;
    
    // Rendering
    std::unique_ptr<GLSLProgram> _shader;
    std::unique_ptr<GLSLProgram> _texshader;
    std::unique_ptr<GLSLProgram> _litTexShader;  // 带光照的纹理着色器
    std::unique_ptr<Model> _sphereModel;
    std::unique_ptr<Model> _cylinderModel;
    std::unique_ptr<Model> _turretModel[2];
    std::unique_ptr<Model> _gunModel;
    std::unique_ptr<Model> _flashModel;
    std::unique_ptr<SkyBox> _skybox;
    
    // Texture
    std::shared_ptr<Texture2D> _turrettex;
    std::shared_ptr<Texture2D> _guntexbase;
    std::vector<std::shared_ptr<Texture2D>> _flashtexs;

    // Text
    std::unique_ptr<TextRenderer> _textrenderer;

    // Game parameters
    float _bulletSpeed = 2.0f;
    int _initialLaunchers = 2;
    int _launchersPerWave = 2;
    float _launcherRadius = 8.0f;
    float _fireInterval = 1.0f;
    float _waveBreakTime = 5.0f;
    
    // Lighting parameters
    glm::vec3 _lightPosition = glm::vec3(5.0f, 10.0f, 5.0f);
    glm::vec3 _lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float _lightIntensity = 1.0f;
    float _ambientStrength = 0.3f;
    float _specularStrength = 0.5f;
    float _shininess = 32.0f;
    
    // Methods
    void initShader();
    void initTexShader();
    void initLitTexShader();  // 初始化带光照的纹理着色器，用于模型的光照
    void initGameObjects();
    void initTex();
    void updateGame();
    void updatePlayer();
    void updateBullets();
    void updateLaunchers();
    void updateGun();
    void updateCamera();
    void checkCollisions();
    void handleWaveTransition();
    void spawnBullet(const Launcher& launcher);
    void destroyBullet(size_t index);
    void renderPlayer();
    void renderBullets();
    void renderLaunchers();
    void renderGun();
    void renderMuzzleFlash();
    void renderLightIndicator();
    void renderUI();
    void renderGameUI();
    void renderCrosshair();
    void renderWaveBreakUI();
    void setupLaunchers(int count);
    void resetGame();
    void startGame();
    void updateWaitingState();
    void renderStartScreen();
    bool isPlayerHit(const Bullet& bullet) const;
    void takeDamage();
    void saveScreenshot();
    
    // Camera controls
    void handleCameraInput();
    void orbitCamera(float deltaAngle);
    void zoomCamera(float deltaZoom);
    void handleMouseCamera();
    void handleFreeCameraMovement();
    void setupCameraForGameState();
    
    // Ray casting for mouse clicks
    glm::vec3 screenToWorldRay(float mouseX, float mouseY);
    bool rayIntersectsSphere(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, 
                           const glm::vec3& sphereCenter, float sphereRadius, float& distance);
    void handleMouseClick();
    void startBulletDestroy(size_t bulletIndex);
    void toggleMouseMode();

    /// <summary>
    /// imgui
    /// </summary>
    void initImGui();
    void clearImGui();
    void renderInspectorPanel();
};