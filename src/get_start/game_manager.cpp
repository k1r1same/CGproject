#include "game_manager.h"
#include <iostream>
#include <algorithm>

GameManager::GameManager() {
    reset();
}

void GameManager::update(float deltaTime) {
    if (_gameOver) return;
    
    _waveTimer += deltaTime;
    
    if (_waveTimer >= _waveTime) {
        nextWave();
    }
}

void GameManager::reset() {
    _gameOver = false;
    _currentWave = 1;
    _waveTimer = 0.0f;
    _waveTime = _config.waveTime;
    _playerHealth = _config.playerMaxHealth;
}

void GameManager::takeDamage() {
    _playerHealth--;
    if (_playerHealth <= 0) {
        _gameOver = true;
    }
}

void GameManager::nextWave() {
    _currentWave++;
    _waveTimer = 0.0f;
    std::cout << "Wave " << _currentWave << " started!" << std::endl;
}

UIRenderer::UIRenderer() {
}

void UIRenderer::renderGameUI(const GameManager& gameManager, int windowWidth, int windowHeight) {
    const float margin = 20.0f;
    const float topY = windowHeight - margin - 30.0f;
    
    renderText("Wave: " + std::to_string(gameManager.getCurrentWave()), 
               margin, topY, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    
    renderText("Health: " + std::to_string(gameManager.getPlayerHealth()), 
               margin, topY - 40.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    
    float timeRemaining = gameManager.getWaveTimeRemaining();
    glm::vec3 timerColor = timeRemaining < 5.0f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(1.0f, 1.0f, 1.0f);
    renderText("Time: " + std::to_string(static_cast<int>(timeRemaining)), 
               margin, topY - 80.0f, 1.0f, timerColor);
    
    renderHealthBar(gameManager.getPlayerHealth(), gameManager.getConfig().playerMaxHealth,
                    windowWidth - 220.0f, topY, 200.0f, 20.0f);
}

void UIRenderer::renderGameOverUI(int windowWidth, int windowHeight) {
    float centerX = windowWidth / 2.0f;
    float centerY = windowHeight / 2.0f;
    
    renderText("GAME OVER", centerX - 100.0f, centerY, 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    renderText("Press R to restart", centerX - 80.0f, centerY - 50.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
}

void UIRenderer::renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color) {
    // Placeholder for text rendering implementation
    // In a full implementation, this would use a text rendering library
    std::cout << "Render text: " << text << " at (" << x << ", " << y << ")" << std::endl;
}

void UIRenderer::renderHealthBar(int currentHealth, int maxHealth, float x, float y, float width, float height) {
    // Placeholder for health bar rendering
    // This would render a visual health bar using OpenGL primitives
    float healthRatio = static_cast<float>(currentHealth) / maxHealth;
    std::cout << "Render health bar: " << healthRatio * 100.0f << "% at (" << x << ", " << y << ")" << std::endl;
}

void UIRenderer::renderWaveTimer(float timeRemaining, float x, float y) {
    // Placeholder for timer rendering
    std::cout << "Render timer: " << timeRemaining << "s at (" << x << ", " << y << ")" << std::endl;
} 