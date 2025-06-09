#pragma once

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>

struct GameConfig {
    float waveTime = 30.0f;
    int initialLaunchers = 4;
    int launchersPerWave = 2;
    float bulletSpeed = 5.0f;
    float launcherRadius = 8.0f;
    float playerMoveRange = 5.0f;
    float playerRadius = 0.5f;
    float bulletRadius = 0.2f;
    int playerMaxHealth = 3;
};

class GameManager {
public:
    GameManager();
    ~GameManager() = default;

    void update(float deltaTime);
    void reset();
    
    GameConfig& getConfig() { return _config; }
    const GameConfig& getConfig() const { return _config; }
    
    bool isGameOver() const { return _gameOver; }
    int getCurrentWave() const { return _currentWave; }
    float getWaveTimeRemaining() const { return _waveTime - _waveTimer; }
    int getPlayerHealth() const { return _playerHealth; }
    
    void setGameOver(bool gameOver) { _gameOver = gameOver; }
    void setPlayerHealth(int health) { _playerHealth = health; }
    void takeDamage();
    void nextWave();

private:
    GameConfig _config;
    bool _gameOver = false;
    int _currentWave = 1;
    float _waveTimer = 0.0f;
    float _waveTime = 30.0f;
    int _playerHealth = 3;
};

class UIRenderer {
public:
    UIRenderer();
    ~UIRenderer() = default;
    
    void renderGameUI(const GameManager& gameManager, int windowWidth, int windowHeight);
    void renderGameOverUI(int windowWidth, int windowHeight);

private:
    void renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color);
    void renderHealthBar(int currentHealth, int maxHealth, float x, float y, float width, float height);
    void renderWaveTimer(float timeRemaining, float x, float y);
};

class ObjectPool {
public:
    template<typename T>
    class Pool {
    public:
        Pool(size_t initialSize = 100) {
            _objects.reserve(initialSize);
            for (size_t i = 0; i < initialSize; ++i) {
                _objects.emplace_back();
                _available.push_back(&_objects.back());
            }
        }
        
        T* acquire() {
            if (_available.empty()) {
                _objects.emplace_back();
                return &_objects.back();
            }
            
            T* obj = _available.back();
            _available.pop_back();
            return obj;
        }
        
        void release(T* obj) {
            if (obj) {
                _available.push_back(obj);
            }
        }
        
        void clear() {
            _available.clear();
            for (auto& obj : _objects) {
                _available.push_back(&obj);
            }
        }
        
    private:
        std::vector<T> _objects;
        std::vector<T*> _available;
    };
}; 