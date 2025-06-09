#include "scene.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <cmath>
#include <random>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Scene::Scene(const Options& options) : Application(options) {
	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	const float aspect = 1.0f * _windowWidth / _windowHeight;
	_camera.reset(new PerspectiveCamera(glm::radians(60.0f), aspect, 0.1f, 1000.0f));
	_camera->transform.position = glm::vec3(0.0f, 5.0f, _cameraDistance);
	_camera->transform.lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	initShader();
	initGameObjects();

	//skybox
	const std::vector<std::string> skyboxTextureRelPaths = {
		"texture/skybox/Right_Tex.jpg", "texture/skybox/Left_Tex.jpg",  "texture/skybox/Up_Tex.jpg",
		"texture/skybox/Down_Tex.jpg",  "texture/skybox/Front_Tex.jpg", "texture/skybox/Back_Tex.jpg" };
	std::vector<std::string> skyboxTextureFullPaths;
	for (size_t i = 0; i < skyboxTextureRelPaths.size(); i++) {
		skyboxTextureFullPaths.push_back(getAssetFullPath(skyboxTextureRelPaths[i]));
	}
	_skybox.reset(new SkyBox(skyboxTextureFullPaths));

	// init imGUI
	initImGui();
}

void Scene::initImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(_window, true);
	ImGui_ImplOpenGL3_Init();

	// ÉèÖÃ³õÊ¼Î»ÖÃ
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	/*ImGui::SetNextWindowPos(ImVec2(0, 0));
	renderScenePanel();*/
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	renderInspectorPanel();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Scene::clearImGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Scene::renderInspectorPanel() {
	const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

	if (!ImGui::Begin("Scene", nullptr, flags)) {
		ImGui::End();
		return;
	}
	if (ImGui::CollapsingHeader("Params", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("BulletSpeed", &_bulletSpeed, 0.5f, 10.0f);
		ImGui::SliderFloat("LauncherRadius", &_launcherRadius, 4.0f, 12.0f);
		ImGui::InputInt("InitialLaunchers", &_initialLaunchers);
		ImGui::InputInt("LaunchersPerWave", &_launchersPerWave);
		ImGui::SliderFloat("WaveTime", &_waveTime, 10.0f, 100.0f);
	}
	if (ImGui::CollapsingHeader("States", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::TextColored(ImVec4(1, 1, 0, 1), std::string("GameState:").append(_gameState == GameState::Playing ? "Playing" : "GameOver").c_str());
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "GameTime: %.2f", _gameTime);
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "CurrentWave: %d", _currentWave);
		ImGui::TextColored(ImVec4(0, 0, 1, 1), "WaveTimer: %.2f", _waveTimer);
	}

	ImGui::End();
}

void Scene::renderUI() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	renderInspectorPanel();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


Scene::~Scene() {
	clearImGui();
}

void Scene::initShader() {
	const char* vsCode =
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPosition;\n"
		"layout(location = 1) in vec3 aNormal;\n"

		"out vec3 worldPosition;\n"
		"out vec3 normal;\n"

		"uniform mat4 model;\n"
		"uniform mat4 view;\n"
		"uniform mat4 projection;\n"

		"void main() {\n"
		"    normal = mat3(transpose(inverse(model))) * aNormal;\n"
		"    worldPosition = vec3(model * vec4(aPosition, 1.0f));\n"
		"    gl_Position = projection * view * vec4(worldPosition, 1.0f);\n"
		"}\n";

	const char* fsCode =
		"#version 330 core\n"
		"in vec3 worldPosition;\n"
		"in vec3 normal;\n"
		"out vec4 fragColor;\n"

		"uniform vec3 objectColor;\n"
		"uniform vec3 lightPos;\n"
		"uniform vec3 lightColor;\n"
		"uniform float ambientStrength;\n"

		"void main() {\n"
		"    vec3 ambient = ambientStrength * lightColor;\n"
		"    vec3 lightDir = normalize(lightPos - worldPosition);\n"
		"    float diff = max(dot(normalize(normal), lightDir), 0.0);\n"
		"    vec3 diffuse = diff * lightColor;\n"
		"    vec3 result = (ambient + diffuse) * objectColor;\n"
		"    fragColor = vec4(result, 1.0);\n"
		"}\n";

	_shader.reset(new GLSLProgram);
	_shader->attachVertexShader(vsCode);
	_shader->attachFragmentShader(fsCode);
	_shader->link();
}

void Scene::initGameObjects() {
	try {
		_sphereModel.reset(new Model(getAssetFullPath("obj/sphere.obj")));
	}
	catch (...) {
		std::cout << "Warning: sphere.obj not found, using basic rendering" << std::endl;
	}

	try {
		_cylinderModel.reset(new Model(getAssetFullPath("obj/cylinder.obj")));
	}
	catch (...) {
		std::cout << "Warning: cylinder.obj not found, using basic rendering" << std::endl;
	}

	try {
		_turretModel.reset(new Model(getAssetFullPath("obj/turret.obj")));
	}
	catch (...) {
		std::cout << "Warning: turret.obj not found, using basic rendering" << std::endl;
	}

	_player.position = glm::vec3(0.0f, 0.0f, 0.0f);
	_player.health = 3;

	setupLaunchers(_initialLaunchers);
}

void Scene::setupLaunchers(int count) {
	_launchers.clear();
	for (int i = 0; i < count; ++i) {
		Launcher launcher;
		float angle = (2.0f * M_PI * i) / count;
		launcher.position = glm::vec3(
			_launcherRadius * cos(angle),
			0.0f,
			_launcherRadius * sin(angle)
		);
		launcher.targetPosition = _player.position;
		_launchers.push_back(launcher);
	}
}

void Scene::handleInput() {
	if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
		glfwSetWindowShouldClose(_window, true);
		return;
	}

	if (_gameState == GameState::Playing) {
		updatePlayer();
		handleCameraInput();

		if (_input.mouse.press.left) {
			// Handle bullet destruction on mouse click
			// Ray casting implementation would go here
		}

		if (_input.keyboard.keyStates[GLFW_KEY_F12] == GLFW_PRESS) {
			saveScreenshot();
		}

		if (_input.keyboard.keyStates[GLFW_KEY_R] == GLFW_PRESS) {
			resetGame();
		}
	}
	else if (_gameState == GameState::GameOver) {
		if (_input.keyboard.keyStates[GLFW_KEY_R] == GLFW_PRESS) {
			resetGame();
		}
	}

	_input.forwardState();
	updateGame();
}

void Scene::updatePlayer() {
	float moveSpeed = 5.0f;

	if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE ||
		_input.keyboard.keyStates[GLFW_KEY_UP] != GLFW_RELEASE) {
		_player.position.y += moveSpeed * _deltaTime;
		_player.position.y = std::min(_player.position.y, _player.moveRange);
		updateCamera();
	}

	if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE ||
		_input.keyboard.keyStates[GLFW_KEY_DOWN] != GLFW_RELEASE) {
		_player.position.y -= moveSpeed * _deltaTime;
		_player.position.y = std::max(_player.position.y, -_player.moveRange);
		updateCamera();
	}
}

void Scene::handleCameraInput() {
	const float orbitSpeed = 1.0f;
	const float zoomSpeed = 5.0f;

	if (_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
		orbitCamera(-orbitSpeed * _deltaTime);
	}
	if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
		orbitCamera(orbitSpeed * _deltaTime);
	}
	if (_input.keyboard.keyStates[GLFW_KEY_Q] != GLFW_RELEASE) {
		zoomCamera(-zoomSpeed * _deltaTime);
	}
	if (_input.keyboard.keyStates[GLFW_KEY_E] != GLFW_RELEASE) {
		zoomCamera(zoomSpeed * _deltaTime);
	}
}

void Scene::orbitCamera(float deltaAngle) {
	_cameraAngle += deltaAngle;
	updateCamera();
}

void Scene::zoomCamera(float deltaZoom) {
	_cameraDistance += deltaZoom;
	_cameraDistance = std::max(5.0f, std::min(_cameraDistance, 50.0f));
	updateCamera();
}

void Scene::updateCamera() {
	float x = _cameraDistance * cos(_cameraAngle);
	float z = _cameraDistance * sin(_cameraAngle);
	_camera->transform.position = glm::vec3(x, 5.0f, z);
	_camera->transform.lookAt(_player.position, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Scene::renderFrame() {
	showFpsInWindowTitle();

	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glm::mat4 projection = _camera->getProjectionMatrix();
	glm::mat4 view = _camera->getViewMatrix();

	_shader->use();
	_shader->setUniformMat4("projection", projection);
	_shader->setUniformMat4("view", view);
	_shader->setUniformVec3("lightPos", glm::vec3(0.0f, 10.0f, 0.0f));
	_shader->setUniformVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
	_shader->setUniformFloat("ambientStrength", 0.3f);

	renderPlayer();
	renderBullets();
	renderLaunchers();

	_skybox->draw(projection, view);

	renderUI();
}

void Scene::updateGame() {
	_gameTime += _deltaTime;
	_waveTimer += _deltaTime;

	updateBullets();
	updateLaunchers();
	checkCollisions();
	handleWaveTransition();
}

void Scene::updateBullets() {
	for (auto& bullet : _bullets) {
		if (!bullet.active) continue;

		bullet.position += bullet.velocity * _deltaTime;

		float distanceFromCenter = glm::length(bullet.position);
		if (distanceFromCenter > 20.0f) {
			bullet.active = false;
		}
	}

	_bullets.erase(std::remove_if(_bullets.begin(), _bullets.end(),
		[](const Bullet& b) { return !b.active; }), _bullets.end());
}

void Scene::updateLaunchers() {
	for (auto& launcher : _launchers) {
		if (_gameTime - launcher.lastFireTime >= launcher.fireInterval) {
			launcher.targetPosition = _player.position;
			spawnBullet(launcher);
			launcher.lastFireTime = _gameTime;
		}
		launcher.position = glm::vec3(launcher.position.x, _player.position.y, launcher.position.z);
	}
}

void Scene::spawnBullet(const Launcher& launcher) {
	Bullet bullet;
	bullet.position = launcher.position;

	glm::vec3 direction = glm::normalize(launcher.targetPosition - launcher.position);
	bullet.velocity = direction * _bulletSpeed;
	bullet.color = glm::vec3(1.0f, 0.8f, 0.2f);
	bullet.active = true;

	_bullets.push_back(std::move(bullet));
}

void Scene::checkCollisions() {
	for (const auto& bullet : _bullets) {
		if (!bullet.active) continue;

		if (isPlayerHit(bullet)) {
			takeDamage();
			break;
		}
	}
}

bool Scene::isPlayerHit(const Bullet& bullet) const {
	if (abs(bullet.position.y - _player.position.y) > (_player.radius + bullet.radius)) {
		return false;
	}

	float distance = glm::length(bullet.position - _player.position);
	return distance < (_player.radius + bullet.radius);
}

void Scene::takeDamage() {
	_player.health--;
	if (_player.health <= 0) {
		_gameState = GameState::GameOver;
	}

	_bullets.clear();
}

void Scene::handleWaveTransition() {
	if (_waveTimer >= _waveTime) {
		_currentWave++;
		_waveTimer = 0.0f;
		_bullets.clear();

		int newLauncherCount = _initialLaunchers + (_currentWave - 1) * _launchersPerWave;
		setupLaunchers(newLauncherCount);
	}
}

void Scene::renderPlayer() {
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, _player.position);
	model = glm::scale(model, glm::vec3(_player.radius));

	_shader->setUniformMat4("model", model);

	if (_gameState == GameState::Playing) {
		_shader->setUniformVec3("objectColor", glm::vec3(0.2f, 0.8f, 0.2f));
	}
	else {
		_shader->setUniformVec3("objectColor", glm::vec3(0.8f, 0.2f, 0.2f));
	}

	if (_sphereModel) {
		_sphereModel->draw();
	}

}

void Scene::renderBullets() {
	for (const auto& bullet : _bullets) {
		if (!bullet.active) continue;

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, bullet.position);
		model = glm::scale(model, glm::vec3(bullet.radius));

		_shader->setUniformMat4("model", model);
		_shader->setUniformVec3("objectColor", bullet.color);

		if (_sphereModel) {
			_sphereModel->draw();
		}
	}
}

void Scene::renderLaunchers() {
	for (const auto& launcher : _launchers) {
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, launcher.position);
		model = glm::scale(model, glm::vec3(0.5f, 2.0f, 0.5f));

		_shader->setUniformMat4("model", model);
		_shader->setUniformVec3("objectColor", glm::vec3(0.6f, 0.3f, 0.8f));

		if (_turretModel) {
			_turretModel->draw();
		}
	}
}

void Scene::resetGame() {
	_gameState = GameState::Playing;
	_gameTime = 0.0f;
	_currentWave = 1;
	_waveTimer = 0.0f;
	_player.health = 3;
	_player.position = glm::vec3(0.0f, 0.0f, 0.0f);
	_bullets.clear();
	setupLaunchers(_initialLaunchers);
}

void Scene::saveScreenshot() {
	std::cout << "Screenshot saved (feature to be implemented)" << std::endl;
}