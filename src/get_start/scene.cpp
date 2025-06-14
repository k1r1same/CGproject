#include "scene.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <cmath>
#include <random>
#include <algorithm>
#include <limits>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Scene::Scene(const Options& options) : Application(options) {
	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	const float aspect = 1.0f * _windowWidth / _windowHeight;
	_camera.reset(new PerspectiveCamera(glm::radians(60.0f), aspect, 0.1f, 1000.0f));
	_camera->transform.position = _freeCameraPos;
	_camera->transform.lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	_lastMouseX = _windowWidth / 2.0f;
	_lastMouseY = _windowHeight / 2.0f;

  _textrenderer.reset(new TextRenderer());

	initShader();
  initTexShader();
	initLitTexShader();
  _textrenderer->initshader();
	initGameObjects();
  initTex();

	//skybox
	const std::vector<std::string> skyboxTextureRelPaths = {
		"texture/skybox/Right_Tex.jpg", "texture/skybox/Left_Tex.jpg",  "texture/skybox/Down_Tex.jpg","texture/skybox/Up_Tex.jpg",
		"texture/skybox/Front_Tex.jpg", "texture/skybox/Back_Tex.jpg" };
	std::vector<std::string> skyboxTextureFullPaths;
	for (size_t i = 0; i < skyboxTextureRelPaths.size(); i++) {
		skyboxTextureFullPaths.push_back(getAssetFullPath(skyboxTextureRelPaths[i]));
	}
	_skybox.reset(new SkyBox(skyboxTextureFullPaths));

  // 初始化相机初始视角
  glm::vec3 dir = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - _freeCameraPos);
  _yaw = glm::degrees(atan2(dir.z, dir.x));
  _pitch = glm::degrees(asin(dir.y));

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
		ImGui::SliderFloat("FireInterval", &_fireInterval, 0.2f, 5.0f);
		ImGui::InputInt("InitialLaunchers", &_initialLaunchers);
		ImGui::InputInt("LaunchersPerWave", &_launchersPerWave);
		ImGui::SliderFloat("WaveTime", &_waveTime, 10.0f, 100.0f);
		ImGui::SliderFloat("WaveBreakTime", &_waveBreakTime, 1.0f, 15.0f);
	}
	if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat3("LightPosition", &_lightPosition.x, -20.0f, 20.0f);
		ImGui::ColorEdit3("LightColor", &_lightColor.x);
		ImGui::SliderFloat("LightIntensity", &_lightIntensity, 0.0f, 3.0f);
		ImGui::SliderFloat("AmbientStrength", &_ambientStrength, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularStrength", &_specularStrength, 0.0f, 2.0f);
		ImGui::SliderFloat("Shininess", &_shininess, 1.0f, 128.0f);
	}
	if (ImGui::CollapsingHeader("States", ImGuiTreeNodeFlags_DefaultOpen)) {
		std::string gameStateStr;
		switch (_gameState) {
			case GameState::WaitingToStart: gameStateStr = "WaitingToStart"; break;
			case GameState::Playing: gameStateStr = "Playing"; break;
			case GameState::WaveBreak: gameStateStr = "WaveBreak"; break;
			case GameState::GameOver: gameStateStr = "GameOver"; break;
		}
		ImGui::TextColored(ImVec4(1, 1, 0, 1), std::string("GameState:").append(gameStateStr).c_str());
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "GameTime: %.2f", _gameTime);
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "CurrentWave: %d", _currentWave);
		ImGui::TextColored(ImVec4(0, 0, 1, 1), "WaveTimer: %.2f", _waveTimer);
	}
	if (ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (_gameState == GameState::WaitingToStart) {
			if (_cameraControlMode) {
				ImGui::TextColored(ImVec4(0, 1, 1, 1), "Mode: Camera Control");
				ImGui::TextColored(ImVec4(1, 1, 1, 1), "Press TAB to switch to UI mode");
			} else {
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "Mode: UI Interaction");
				ImGui::TextColored(ImVec4(1, 1, 1, 1), "Press TAB to switch to camera mode");
			}
		}
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "WASD/Arrow Keys: Move player");
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "Mouse: Look around");
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "Left Click: Destroy bullets");
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "Enter: Start game");
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "R: Reset game");
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

void Scene::renderGameUI() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	
	ImGuiIO& io = ImGui::GetIO();
	
	// Health display in top-left corner
	ImGui::SetNextWindowPos(ImVec2(20, 20));
	ImGui::SetNextWindowBgAlpha(0.8f);
	ImGui::Begin("Health", nullptr, 
		ImGuiWindowFlags_NoTitleBar | 
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoScrollbar | 
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_AlwaysAutoResize);
	
	ImGui::SetWindowFontScale(1.5f);
	
	// Health display
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Health: ");
	ImGui::SameLine();
	
	for (int i = 0; i < 3; ++i) {
		if (i < _player.health) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[#]");
		} else {
			ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.3f, 1.0f), "[ ]");
		}
		if (i < 2) ImGui::SameLine();
	}
	ImGui::End();
	
	// Wave info in top-center
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - 120, 20));
	ImGui::SetNextWindowBgAlpha(0.8f);
	ImGui::Begin("Wave", nullptr, 
		ImGuiWindowFlags_NoTitleBar | 
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoScrollbar | 
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_AlwaysAutoResize);
	
	ImGui::SetWindowFontScale(1.8f);
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Wave %d", _currentWave);
	ImGui::End();
	
	// Countdown timer in top-right corner
	float timeRemaining = _waveTime - _waveTimer;
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 180, 20));
	ImGui::SetNextWindowBgAlpha(0.8f);
	ImGui::Begin("Timer", nullptr, 
		ImGuiWindowFlags_NoTitleBar | 
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoScrollbar | 
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_AlwaysAutoResize);
	
	ImGui::SetWindowFontScale(1.6f);
	if (timeRemaining <= 10.0f) {
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.1fs", timeRemaining);
	} else {
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%.1fs", timeRemaining);
	}
	ImGui::End();
	
	// Game Over screen
	if (_gameState == GameState::GameOver) {
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - 200, io.DisplaySize.y * 0.5f - 120));
		ImGui::SetNextWindowBgAlpha(0.9f);
		ImGui::Begin("GameOver", nullptr, 
			ImGuiWindowFlags_NoTitleBar | 
			ImGuiWindowFlags_NoResize | 
			ImGuiWindowFlags_NoMove | 
			ImGuiWindowFlags_NoScrollbar | 
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_AlwaysAutoResize);
		
		ImGui::SetWindowFontScale(2.0f);
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "GAME OVER");
    
		ImGui::SetWindowFontScale(1.4f);
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Survived %d waves", _currentWave - 1);
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Press R to restart");
    _textrenderer->renderText("GAME OVER", 800.0f, 540.0f, 1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		ImGui::End();
	}
	
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Scene::renderWaveBreakUI() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	
	ImGuiIO& io = ImGui::GetIO();
	
	// 休息提示
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - 200, io.DisplaySize.y * 0.5f - 100));
	ImGui::SetNextWindowBgAlpha(0.9f);
	ImGui::Begin("WaveBreak", nullptr, 
		ImGuiWindowFlags_NoTitleBar | 
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoScrollbar | 
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_AlwaysAutoResize);
	
	ImGui::SetWindowFontScale(2.5f);
	ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Wave %d Complete!", _currentWave);
	
	ImGui::SetWindowFontScale(1.8f);
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Preparing Wave %d...", _currentWave + 1);
	
	float timeRemaining = _breakTime - _breakTimer;
	ImGui::SetWindowFontScale(2.0f);
	if (timeRemaining <= 3.0f) {
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.1f", timeRemaining);
	} else {
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%.1f", timeRemaining);
	}
	
	ImGui::End();
	
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
		"uniform vec3 viewPos;\n"
		"uniform float lightIntensity;\n"
		"uniform float ambientStrength;\n"
		"uniform float specularStrength;\n"
		"uniform float shininess;\n"

		"void main() {\n"
		"    vec3 normalizedNormal = normalize(normal);\n"
		"    \n"
		"    // Ambient lighting\n"
		"    vec3 ambient = ambientStrength * lightColor;\n"
		"    \n"
		"    // Diffuse lighting\n"
		"    vec3 lightDir = normalize(lightPos - worldPosition);\n"
		"    float diff = max(dot(normalizedNormal, lightDir), 0.0);\n"
		"    vec3 diffuse = diff * lightColor;\n"
		"    \n"
		"    // Specular lighting\n"
		"    vec3 viewDir = normalize(viewPos - worldPosition);\n"
		"    vec3 reflectDir = reflect(-lightDir, normalizedNormal);\n"
		"    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);\n"
		"    vec3 specular = specularStrength * spec * lightColor;\n"
		"    \n"
		"    // Combine results\n"
		"    vec3 result = (ambient + diffuse + specular) * lightIntensity * objectColor;\n"
		"    fragColor = vec4(result, 1.0);\n"
		"}\n";

	_shader.reset(new GLSLProgram);
	_shader->attachVertexShader(vsCode);
	_shader->attachFragmentShader(fsCode);
	_shader->link();
}

void Scene::initTexShader(){
    const char* vsCode =
      "#version 330 core\n"
      "layout(location = 0) in vec3 aPosition;\n"
      "layout(location = 1) in vec3 aNormal;\n"
      "layout(location = 2) in vec2 aTexCoord;\n"
      "out vec2 fTexCoord;\n"
      "uniform mat4 projection;\n"
      "uniform mat4 view;\n"
      "uniform mat4 model;\n"

      "void main() {\n"
      "    fTexCoord = aTexCoord;\n"
      "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
      "}\n";

    const char* fsCode =
      "#version 330 core\n"
      "in vec2 fTexCoord;\n"
      "out vec4 color;\n"
      "uniform sampler2D mapKd;\n"
      "void main() {\n"
      "    color = texture(mapKd, fTexCoord);\n"
      "}\n";

    _texshader.reset(new GLSLProgram);
    _texshader->attachVertexShader(vsCode);
    _texshader->attachFragmentShader(fsCode);
    _texshader->link();
}

void Scene::initLitTexShader() {
    const char* vsCode =
      "#version 330 core\n"
      "layout(location = 0) in vec3 aPosition;\n"
      "layout(location = 1) in vec3 aNormal;\n"
      "layout(location = 2) in vec2 aTexCoord;\n"
      
      "out vec3 worldPosition;\n"
      "out vec3 normal;\n"
      "out vec2 fTexCoord;\n"
      
      "uniform mat4 model;\n"
      "uniform mat4 view;\n"
      "uniform mat4 projection;\n"

      "void main() {\n"
      "    normal = mat3(transpose(inverse(model))) * aNormal;\n"
      "    worldPosition = vec3(model * vec4(aPosition, 1.0f));\n"
      "    fTexCoord = aTexCoord;\n"
      "    gl_Position = projection * view * vec4(worldPosition, 1.0f);\n"
      "}\n";

    const char* fsCode =
      "#version 330 core\n"
      "in vec3 worldPosition;\n"
      "in vec3 normal;\n"
      "in vec2 fTexCoord;\n"
      "out vec4 fragColor;\n"
      
      "uniform sampler2D mapKd;\n"
      "uniform vec3 lightPos;\n"
      "uniform vec3 lightColor;\n"
      "uniform vec3 viewPos;\n"
      "uniform float lightIntensity;\n"
      "uniform float ambientStrength;\n"
      "uniform float specularStrength;\n"
      "uniform float shininess;\n"

      "void main() {\n"
      "    vec3 normalizedNormal = normalize(normal);\n"
      "    \n"
      "    // Sample texture color\n"
      "    vec3 textureColor = texture(mapKd, fTexCoord).rgb;\n"
      "    \n"
      "    // Ambient lighting\n"
      "    vec3 ambient = ambientStrength * lightColor;\n"
      "    \n"
      "    // Diffuse lighting\n"
      "    vec3 lightDir = normalize(lightPos - worldPosition);\n"
      "    float diff = max(dot(normalizedNormal, lightDir), 0.0);\n"
      "    vec3 diffuse = diff * lightColor;\n"
      "    \n"
      "    // Specular lighting\n"
      "    vec3 viewDir = normalize(viewPos - worldPosition);\n"
      "    vec3 reflectDir = reflect(-lightDir, normalizedNormal);\n"
      "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);\n"
      "    vec3 specular = specularStrength * spec * lightColor;\n"
      "    \n"
      "    // Combine results with texture\n"
      "    vec3 result = (ambient + diffuse + specular) * lightIntensity * textureColor;\n"
      "    fragColor = vec4(result, 1.0);\n"
      "}\n";

    _litTexShader.reset(new GLSLProgram);
    _litTexShader->attachVertexShader(vsCode);
    _litTexShader->attachFragmentShader(fsCode);
    _litTexShader->link();
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
		_turretModel[0].reset(new Model(getAssetFullPath("obj/turret01.obj")));
    _turretModel[0]->transform.scale = glm::vec3(6.0f, 1.5f, 1.5f);
	}
	catch (...) {
		std::cout << "Warning: turret.obj not found, using basic rendering" << std::endl;
	}
  try {
		_turretModel[1].reset(new Model(getAssetFullPath("obj/turret02.obj")));
    _turretModel[1]->transform.scale = glm::vec3(6.0f, 1.5f, 1.5f);
	}
	catch (...) {
		std::cout << "Warning: turret02.obj not found, using basic rendering" << std::endl;
	}
	try {
		std::cout << "loading: " + getAssetFullPath("obj/colt_SAA_(OBJ).obj") << std::endl;
		_gunModel.reset(new Model(getAssetFullPath("obj/colt_SAA_(OBJ).obj")));
    _gunModel->transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
	}
	catch (...) {
		std::cout << "Warning: colt_SAA_(OBJ).obj not found, using basic rendering" << std::endl;
	}

	try {
		_flashModel.reset(new Model(getAssetFullPath("obj/muzzle_flash.obj")));
	_flashModel->transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
	}
	catch (...) {
		std::cout << "Warning: muzzle_flash.obj not found, using basic rendering" << std::endl;
	}

	_player.position = glm::vec3(0.0f, 0.0f, 0.0f);
	_player.health = 3;

	setupLaunchers(_initialLaunchers);
}

void Scene::initTex() {
	const std::string turretTextureRelPath = "texture/turret/T_2K__albedo.png";
	const std::string gunTextureBaseRelPath = "texture/gun/colt_saa_BaseColor.png";
	const std::vector<std::string> flashTextureRelPaths = {
		"texture/flash/muzzle_flash_01.png",
		"texture/flash/muzzle_flash_02.png",
		"texture/flash/muzzle_flash_04.png",
		"texture/flash/muzzle_flash_05.png"
	};

	std::shared_ptr<Texture2D> gunTextureBase = std::make_shared<ImageTexture2D>(getAssetFullPath(gunTextureBaseRelPath));
	std::shared_ptr<Texture2D> turretTexture = std::make_shared<ImageTexture2D>(getAssetFullPath(turretTextureRelPath));
	_turrettex = turretTexture;
	_guntexbase = gunTextureBase;
	_flashtexs.clear();
	for (auto flashTextureRelPath : flashTextureRelPaths) {
		std::shared_ptr<Texture2D> flashTexture = std::make_shared<ImageTexture2D>(getAssetFullPath(flashTextureRelPath));
		_flashtexs.push_back(flashTexture);
	}
}

void Scene::setupLaunchers(int count) {
	//std::cout << "Setting up " << count << " launchers" << std::endl;
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
		launcher.fireInterval = _fireInterval; 
		
		// 错开发射，但保持相同的发射频率
		float timeOffset = (_fireInterval * i) / count;
		launcher.lastFireTime = _gameTime - _fireInterval + timeOffset;
		
		_launchers.push_back(launcher);
	}
}

void Scene::handleInput() {
	if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
		glfwSetWindowShouldClose(_window, true);
		return;
	}
  if (_input.keyboard.keyStates[GLFW_KEY_F2] == GLFW_PRESS) {
			saveScreenshot();
	}
	if (_gameState == GameState::WaitingToStart) {
		bool currentTabPressed = (_input.keyboard.keyStates[GLFW_KEY_TAB] == GLFW_PRESS);
		if (currentTabPressed && !_prevTabPressed) {
			toggleMouseMode();
		}
		_prevTabPressed = currentTabPressed;
		
		if (!_cameraControlMode) {
			return;
		}
		
		handleMouseCamera();
		handleFreeCameraMovement();
		
		if (_input.keyboard.keyStates[GLFW_KEY_ENTER] == GLFW_PRESS) {
			startGame();
		}
	}
	else if (_gameState == GameState::Playing) {
		updatePlayer();
		handleMouseCamera();

		bool currentMouseLeftPressed = _input.mouse.press.left;
		if (currentMouseLeftPressed && !_prevMouseLeftPressed) {
			handleMouseClick();
		}
		_prevMouseLeftPressed = currentMouseLeftPressed;

		if (_input.keyboard.keyStates[GLFW_KEY_R] == GLFW_PRESS) {
			resetGame();
		}
	}
	else if (_gameState == GameState::WaveBreak) {
		// 休息期间允许相机控制
		handleMouseCamera();
		
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
	if(_gameState == GameState::Playing) {
	if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE ||
		_input.keyboard.keyStates[GLFW_KEY_UP] != GLFW_RELEASE) {
		_player.position.y += moveSpeed * _deltaTime;
		_player.position.y = std::min(_player.position.y, _player.moveRange);
	}

	if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE ||
		_input.keyboard.keyStates[GLFW_KEY_DOWN] != GLFW_RELEASE) {
		_player.position.y -= moveSpeed * _deltaTime;
		_player.position.y = std::max(_player.position.y, -_player.moveRange);
	}
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

	_texshader->use();
	_texshader->setUniformMat4("projection", projection);
	_texshader->setUniformMat4("view", view);
	
	if (_gameState == GameState::WaitingToStart) {
		renderPlayer();
		renderLaunchers();
		renderLightIndicator();
		_skybox->draw(projection, view);

		if (_cameraControlMode) {
			renderStartScreen();
		} else {
			renderUI();
		}
	}
	else if (_gameState == GameState::Playing) {
		renderPlayer();
		renderBullets();
		renderLaunchers();
		renderLightIndicator();
		_skybox->draw(projection, view);
		renderGun();
		renderMuzzleFlash();
		renderGameUI();
		renderCrosshair();
	}
	else if (_gameState == GameState::WaveBreak) {
		renderPlayer();
		renderBullets(); // 渲染剩余子弹
		renderLaunchers();
		renderLightIndicator();
		_skybox->draw(projection, view);
		renderGun();
		renderMuzzleFlash();
		renderWaveBreakUI();
	}
	else if (_gameState == GameState::GameOver) {
		renderPlayer();
		renderBullets();
		renderLaunchers();
		renderLightIndicator();
		_skybox->draw(projection, view);
		renderGun();
		renderMuzzleFlash();
		renderGameUI();
	}
	
}

void Scene::updateGame() {
	if(_gameState == GameState::Playing) {
		_gameTime += _deltaTime;
		_waveTimer += _deltaTime;

		updateBullets();
		updateLaunchers();
		checkCollisions();
		updateGun();
		handleWaveTransition();
	}
	else if (_gameState == GameState::WaitingToStart) {
		updateWaitingState();
	}
	else if (_gameState == GameState::WaveBreak) {
		_breakTimer += _deltaTime;
		updateBullets(); 
		
		// 休息时间结束后开始下一波
		if (_breakTimer >= _breakTime) {
			_currentWave++;
			_waveTimer = 0.0f;
			_gameState = GameState::Playing;
			
			int newLauncherCount = _initialLaunchers + (_currentWave - 1) * _launchersPerWave;
			setupLaunchers(newLauncherCount);
		}
	}
}

void Scene::updateBullets() {
	for (auto& bullet : _bullets) {
		if (!bullet.active) continue;

		if (bullet.destroying) {
			bullet.destroyTimer += _deltaTime;
			if (bullet.destroyTimer >= bullet.destroyDuration) {
				bullet.active = false;
			}
		} else {
			bullet.position += bullet.velocity * _deltaTime;

			float distanceFromCenter = glm::length(bullet.position);
			if (distanceFromCenter > 20.0f) {
				bullet.active = false;
			}
		}
	}

	_bullets.erase(std::remove_if(_bullets.begin(), _bullets.end(),
		[](const Bullet& b) { return !b.active; }), _bullets.end());
}

void Scene::updateLaunchers() {
	for (auto& launcher : _launchers) {
		// 动态更新发射间隔
		launcher.fireInterval = _fireInterval;
		
		if (_gameTime - launcher.lastFireTime >= launcher.fireInterval) {
			launcher.targetPosition = _player.position;
			spawnBullet(launcher);
			launcher.lastFireTime = _gameTime;
		}
		launcher.position = glm::vec3(launcher.position.x, _player.position.y, launcher.position.z);
	}
}

void Scene::updateGun() {
    static float recoilTimer = 0.0f;
	static float flashTimer = 0.0f;
    static const float recoilDuration = 0.1f;
	static const float flashDuration = 0.02f;
    static const float recoilDistance = 0.2f;
	static const float maxPitchAngle = glm::radians(25.0f);

	if (_isFlashing) {
		flashTimer += _deltaTime;
		if (flashTimer >= flashDuration) {
			_isFlashing = false;
			flashTimer = 0.0f;
			_currentFlashtex = (_currentFlashtex + 1) % _flashtexs.size();
		}
	}

    if (_isRecoiling) {
        recoilTimer += _deltaTime;
        float t = recoilTimer / recoilDuration;

		float angle;
        if (t < 0.5f) {
            _gun.position.z = -0.75f + recoilDistance * (t / 0.5f);
			angle = maxPitchAngle * (t / 0.5f);
			 glm::mat4 pitchMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1, 0, 0));
			_gun.direction = glm::vec3(pitchMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
        } else if (t < 1.0f) {
			angle = maxPitchAngle * (1.0f - t) / 0.5f;
            _gun.position.z = -0.75f + recoilDistance * (1.0f - t) / 0.5f;
			 glm::mat4 pitchMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1, 0, 0));
			_gun.direction = glm::vec3(pitchMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
        } else {
			_gun.direction = glm::vec3(0.0f, 0.0f, -1.0f);
            _gun.position.z = -0.75f;
            _isRecoiling = false;
            recoilTimer = 0.0f;
        }
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

		if (!bullet.destroying && isPlayerHit(bullet)) {
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

	// 让所有活跃子弹开始销毁动画，而不是直接清空
	for (auto& bullet : _bullets) {
		if (bullet.active && !bullet.destroying) {
			bullet.destroying = true;
			bullet.destroyTimer = 0.0f;
		}
	}
}

void Scene::handleWaveTransition() {
	if (_waveTimer >= _waveTime) {
		_gameState = GameState::WaveBreak;
		_breakTimer = 0.0f;
		_breakTime = _waveBreakTime; 
		
		// 让所有活跃子弹开始销毁动画，而不是直接清空
		for (auto& bullet : _bullets) {
			if (bullet.active && !bullet.destroying) {
				bullet.destroying = true;
				bullet.destroyTimer = 0.0f;
			}
		}
	}
}

void Scene::renderPlayer() {
	_shader->use();
	_shader->setUniformMat4("projection", _camera->getProjectionMatrix());
	_shader->setUniformMat4("view", _camera->getViewMatrix());
	_shader->setUniformVec3("lightPos", _lightPosition);
	_shader->setUniformVec3("lightColor", _lightColor);
	_shader->setUniformVec3("viewPos", _camera->transform.position);
	_shader->setUniformFloat("lightIntensity", _lightIntensity);
	_shader->setUniformFloat("ambientStrength", _ambientStrength);
	
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, _player.position);
	model = glm::scale(model, glm::vec3(_player.radius));
	_shader->setUniformMat4("model", model);

	if (_gameState == GameState::Playing || _gameState == GameState::WaveBreak) {
		_shader->setUniformVec3("objectColor", glm::vec3(0.2f, 0.8f, 0.2f));
		// 玩家使用金属材质
		_shader->setUniformFloat("specularStrength", 0.8f);
		_shader->setUniformFloat("shininess", 64.0f);
	}
	else {
		_shader->setUniformVec3("objectColor", glm::vec3(0.8f, 0.2f, 0.2f));
		_shader->setUniformFloat("specularStrength", 0.3f);
		_shader->setUniformFloat("shininess", 16.0f);
	}

	if (_sphereModel) {
		_sphereModel->draw();
	}
}

void Scene::renderBullets() {
	_shader->use();
	_shader->setUniformMat4("projection", _camera->getProjectionMatrix());
	_shader->setUniformMat4("view", _camera->getViewMatrix());
	_shader->setUniformVec3("lightPos", _lightPosition);
	_shader->setUniformVec3("lightColor", _lightColor);
	_shader->setUniformVec3("viewPos", _camera->transform.position);
	_shader->setUniformFloat("lightIntensity", _lightIntensity);
	_shader->setUniformFloat("ambientStrength", _ambientStrength);
	
	for (const auto& bullet : _bullets) {
		if (!bullet.active) continue;

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, bullet.position);
		
		if (bullet.destroying) {
			float progress = bullet.destroyTimer / bullet.destroyDuration;
			float scale = bullet.radius * (1.0f + progress * 0.5f);
			model = glm::scale(model, glm::vec3(scale));
			
			glm::vec3 destroyColor = glm::mix(bullet.color, glm::vec3(1.0f, 0.0f, 0.0f), progress);
			_shader->setUniformVec3("objectColor", destroyColor);
			// 销毁时高亮发光
			_shader->setUniformFloat("specularStrength", 1.0f + progress);
			_shader->setUniformFloat("shininess", 128.0f);
		} else {
			model = glm::scale(model, glm::vec3(bullet.radius));
			_shader->setUniformVec3("objectColor", bullet.color);
			// 子弹有轻微的镜面反射
			_shader->setUniformFloat("specularStrength", 0.4f);
			_shader->setUniformFloat("shininess", 32.0f);
		}

		_shader->setUniformMat4("model", model);

		if (_sphereModel) {
			_sphereModel->draw();
		}
	}
}

void Scene::renderLaunchers() {
	// 使用带光照的纹理着色器
	_litTexShader->use();
	_litTexShader->setUniformMat4("projection", _camera->getProjectionMatrix());
	_litTexShader->setUniformMat4("view", _camera->getViewMatrix());
	_litTexShader->setUniformVec3("lightPos", _lightPosition);
	_litTexShader->setUniformVec3("lightColor", _lightColor);
	_litTexShader->setUniformVec3("viewPos", _camera->transform.position);
	_litTexShader->setUniformFloat("lightIntensity", _lightIntensity);
	_litTexShader->setUniformFloat("ambientStrength", _ambientStrength);
	_litTexShader->setUniformFloat("specularStrength", _specularStrength);
	_litTexShader->setUniformFloat("shininess", _shininess);
	
	for (const auto& launcher : _launchers) {
        glm::vec3 dir = glm::normalize(_player.position - launcher.position);
        glm::vec3 up = glm::vec3(0, 1, 0);
        glm::vec3 right = glm::normalize(glm::cross(up, dir));
        glm::vec3 realUp = glm::cross(dir, right);

        glm::mat4 rotation = glm::mat4(1.0f);
        rotation[0] = glm::vec4(right, 0.0f);
        rotation[1] = glm::vec4(realUp, 0.0f);
        rotation[2] = glm::vec4(dir, 0.0f);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, launcher.position-glm::vec3(0.0f, 1.2f, 0.0f));
		    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 0.8f));
		    model *= rotation;
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
		
        static float time = 0.0f;
        static bool forward = true;
        if (forward) time += _deltaTime*0.1f;
        else time -= _deltaTime*0.1f;
        if (time >= 1.0f) {
          std::cout << "time: " << time << std::endl;
          time = 1.0f;
          forward = false;
        }
        if (time <= 0.0f) {
          time = 0.0f;
          forward = true;
        }
        if (_turretModel[0]) {
            _litTexShader->setUniformMat4("model", model);
            _turrettex->bind();
            Model currentmodel = _turretModel[0]->interpolateModel(
                *_turretModel[0], *_turretModel[1], 
                time
            );
	        currentmodel.draw();
	    }
	}
}

void Scene::renderGun() {
	_litTexShader->use();
	_litTexShader->setUniformVec3("lightPos", _lightPosition);
	_litTexShader->setUniformVec3("lightColor", _lightColor);
	_litTexShader->setUniformVec3("viewPos", _camera->transform.position);
	_litTexShader->setUniformFloat("lightIntensity", _lightIntensity);
	_litTexShader->setUniformFloat("ambientStrength", _ambientStrength);
	_litTexShader->setUniformFloat("specularStrength", _specularStrength);
	_litTexShader->setUniformFloat("shininess", _shininess);
		
	glm::mat4 projection = _camera->getProjectionMatrix();
    glm::mat4 view = glm::mat4(1.0f);

	glm::vec3 defaultDir(0.0f, 0.0f, -1.0f);
	glm::vec3 gunDir = glm::normalize(_gun.direction);

    glm::vec3 rotationAxis = glm::cross(defaultDir, gunDir);
    float dot = glm::dot(defaultDir, gunDir);
    float rotationAngle = acos(dot);

    glm::mat4 rotation = glm::mat4(1.0f);
    if (glm::length(rotationAxis) > 0.0001f) {
        rotation = glm::rotate(glm::mat4(1.0f), rotationAngle, glm::normalize(rotationAxis));
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, _gun.position);
	model = model * rotation;
    model = glm::scale(model, glm::vec3(2.5f));

    if (_gunModel) {
		_litTexShader->setUniformMat4("projection", projection);
		_litTexShader->setUniformMat4("view", view);
		_litTexShader->setUniformMat4("model", model);
		_guntexbase->bind(0);
        _gunModel->draw();
    }
}

void Scene::renderMuzzleFlash() {
	if (!_isFlashing) { return; }
	_texshader->use();
    glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = _camera->getProjectionMatrix();
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, _muzzleFlash.position);
	model = glm::scale(model, glm::vec3(0.8f));

	if (_flashModel) {
	_texshader->setUniformMat4("projection", projection);
	_texshader->setUniformMat4("view", view); 
		_texshader->setUniformMat4("model", model);
		_flashtexs[_currentFlashtex]->bind();
		_flashModel->draw();
	}
}

void Scene::renderLightIndicator() {
    _shader->use();
    _shader->setUniformMat4("projection", _camera->getProjectionMatrix());
    _shader->setUniformMat4("view", _camera->getViewMatrix());
    _shader->setUniformVec3("lightPos", _lightPosition);
    _shader->setUniformVec3("lightColor", _lightColor);
    _shader->setUniformVec3("viewPos", _camera->transform.position);
    _shader->setUniformFloat("lightIntensity", _lightIntensity);
    _shader->setUniformFloat("ambientStrength", 1.0f); 
    _shader->setUniformFloat("specularStrength", 0.0f);
    _shader->setUniformFloat("shininess", 1.0f);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, _lightPosition);
    model = glm::scale(model, glm::vec3(0.3f)); // 小球形光源

    _shader->setUniformMat4("model", model);
    _shader->setUniformVec3("objectColor", _lightColor);

    if (_sphereModel) {
        _sphereModel->draw();
    }
}

void Scene::resetGame() {
	_gameState = GameState::WaitingToStart;
	_gameTime = 0.0f;
	_currentWave = 1;
	_waveTimer = 0.0f;
	_breakTimer = 0.0f;
	_player.health = 3;
	_player.position = glm::vec3(0.0f, 0.0f, 0.0f);
	_bullets.clear();
	_blinkTimer = 0.0f;
	_showStartText = true;
	
	// 重置光照参数为默认值
	_lightPosition = glm::vec3(5.0f, 10.0f, 5.0f);
	_lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	_lightIntensity = 1.0f;
	_ambientStrength = 0.3f;
	_specularStrength = 0.5f;
	_shininess = 32.0f;
	
	_freeCameraPos = glm::vec3(0.0f, 5.0f, 15.0f);
	_firstMouse = true;
	_prevMouseLeftPressed = false;
	_prevTabPressed = false;
	_cameraControlMode = true;
	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	setupCameraForGameState();
	setupLaunchers(_initialLaunchers);
}

void Scene::startGame() {
	_gameState = GameState::Playing;
	_gameTime = 0.0f;
	_waveTimer = 0.0f;
	
	_firstMouse = true;
	_cameraControlMode = true;
	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	setupCameraForGameState();
	setupLaunchers(_initialLaunchers);
}

void Scene::updateWaitingState() {
	_blinkTimer += _deltaTime;
	if (_blinkTimer >= 0.8f) {
		_showStartText = !_showStartText;
		_blinkTimer = 0.0f;
	}
}

void Scene::renderStartScreen() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	
	if (_showStartText) {
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 textSize = ImGui::CalcTextSize("Press 'Enter' to Start Game");
    _textrenderer->renderText("Press 'Enter' to Start Game", 680.0f, 540.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
		ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - textSize.x) * 0.5f, io.DisplaySize.y * 0.9f));
		ImGui::SetNextWindowBgAlpha(0.0f);
		
		ImGui::Begin("StartPrompt", nullptr, 
			ImGuiWindowFlags_NoTitleBar | 
			ImGuiWindowFlags_NoResize | 
			ImGuiWindowFlags_NoMove | 
			ImGuiWindowFlags_NoScrollbar | 
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_AlwaysAutoResize);
		
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Press 'Enter' to Start Game");
    
		ImGui::End();
	}
	
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Scene::saveScreenshot() {
	int width = _windowWidth;
  int height = _windowHeight;
  std::vector<unsigned char> pixels(width * height * 3);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
  // 垂直翻转图像
  for (int y = 0; y < height / 2; ++y) {
    for (int x = 0; x < width * 3; ++x) {
      std::swap(pixels[y * width * 3 + x], pixels[(height - 1 - y) * width * 3 + x]);
    }
  }
  // 生成文件名，带时间戳
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::tm* tm = std::localtime(&now_c);
  char filename[128];
  std::strftime(filename, sizeof(filename), "../../screenshots/screenshot_%Y%m%d_%H%M%S.png", tm);
  // 写入文件
  if (stbi_write_png(filename, width, height, 3, pixels.data(), width * 3)) {
    std::cout << "Screenshot saved to " << filename << std::endl;
  } 
  else {
    std::cerr << "Failed to save screenshot" << std::endl;
  }
}

void Scene::handleMouseCamera() {
	double xpos, ypos;
	glfwGetCursorPos(_window, &xpos, &ypos);
	
	if (_firstMouse) {
		_lastMouseX = xpos;
		_lastMouseY = ypos;
		_firstMouse = false;
	}
	
	float xoffset = xpos - _lastMouseX;
	float yoffset = _lastMouseY - ypos;
	_lastMouseX = xpos;
	_lastMouseY = ypos;
	
	xoffset *= _mouseSensitivity;
	yoffset *= _mouseSensitivity;
	
	_yaw += xoffset;
	_pitch += yoffset;
	
	if (_pitch > 89.0f) _pitch = 89.0f;
	if (_pitch < -89.0f) _pitch = -89.0f;
	
	glm::vec3 front;
	front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	front.y = sin(glm::radians(_pitch));
	front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	front = glm::normalize(front);
	
	if (_gameState == GameState::WaitingToStart) {
		_camera->transform.position = _freeCameraPos;
		_camera->transform.lookAt(_freeCameraPos + front, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	else if (_gameState == GameState::Playing || _gameState == GameState::WaveBreak) {
		_camera->transform.position = _player.position + glm::vec3(0.0f, 1.0f, 0.0f);
		_camera->transform.lookAt(_player.position + glm::vec3(0.0f, 1.0f, 0.0f) + front, glm::vec3(0.0f, 1.0f, 0.0f));
	}
}

void Scene::handleFreeCameraMovement() {
	float velocity = _cameraMoveSpeed * _deltaTime;
	
	glm::vec3 front;
	front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	front.y = sin(glm::radians(_pitch));
	front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	front = glm::normalize(front);
	
	glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) {
		_freeCameraPos += front * velocity;
	}
	if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) {
		_freeCameraPos -= front * velocity;
	}
	if (_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
		_freeCameraPos -= right * velocity;
	}
	if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
		_freeCameraPos += right * velocity;
	}
	if (_input.keyboard.keyStates[GLFW_KEY_Q] != GLFW_RELEASE) {
		_freeCameraPos -= up * velocity;
	}
	if (_input.keyboard.keyStates[GLFW_KEY_E] != GLFW_RELEASE) {
		_freeCameraPos += up * velocity;
	}
}

void Scene::setupCameraForGameState() {
	if (_gameState == GameState::WaitingToStart) {
		_camera->transform.position = _freeCameraPos;
		_camera->transform.lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		if (_yaw == 0.0f && _pitch == 0.0f) {
			_yaw = -90.0f;
			_pitch = 0.0f;
		}
	}
	else if (_gameState == GameState::Playing || _gameState == GameState::WaveBreak) {
		if (_yaw == 0.0f && _pitch == 0.0f) {
			_yaw = -90.0f;
			_pitch = 0.0f;
		}
		_camera->transform.position = _player.position + glm::vec3(0.0f, 1.0f, 0.0f);
		_camera->transform.lookAt(_player.position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}
}

void Scene::renderCrosshair() {
	if (!_cameraControlMode) return;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	
	ImGuiIO& io = ImGui::GetIO();
	float centerX = io.DisplaySize.x * 0.5f;
	float centerY = io.DisplaySize.y * 0.5f;
	float size = 15.0f;
	float thickness = 2.0f;
	
	ImDrawList* drawList = ImGui::GetForegroundDrawList();
	ImU32 color = IM_COL32(255, 255, 255, 200);
	
	// Draw crosshair
	drawList->AddLine(
		ImVec2(centerX - size, centerY), 
		ImVec2(centerX + size, centerY), 
		color, thickness
	);
	drawList->AddLine(
		ImVec2(centerX, centerY - size), 
		ImVec2(centerX, centerY + size), 
		color, thickness
	);
	
	// Add small center dot
	drawList->AddCircleFilled(ImVec2(centerX, centerY), 1.5f, color);
	
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

glm::vec3 Scene::screenToWorldRay(float mouseX, float mouseY) {
	glm::mat4 projection = _camera->getProjectionMatrix();
	glm::mat4 view = _camera->getViewMatrix();
	glm::vec4 viewport = glm::vec4(0, 0, _windowWidth, _windowHeight);
	
	glm::vec3 nearPoint = glm::unProject(
		glm::vec3(mouseX, _windowHeight - mouseY, 0.0f),
		view, projection, viewport
	);
	
	glm::vec3 farPoint = glm::unProject(
		glm::vec3(mouseX, _windowHeight - mouseY, 1.0f),
		view, projection, viewport
	);
	
	return glm::normalize(farPoint - nearPoint);
}

bool Scene::rayIntersectsSphere(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, 
                               const glm::vec3& sphereCenter, float sphereRadius, float& distance) {
	glm::vec3 oc = rayOrigin - sphereCenter;
	float a = glm::dot(rayDirection, rayDirection);
	float b = 2.0f * glm::dot(oc, rayDirection);
	float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
	
	float discriminant = b * b - 4 * a * c;
	if (discriminant < 0) {
		return false;
	}
	
	float t1 = (-b - sqrt(discriminant)) / (2.0f * a);
	float t2 = (-b + sqrt(discriminant)) / (2.0f * a);
	
	if (t1 > 0) {
		distance = t1;
		return true;
	} else if (t2 > 0) {
		distance = t2;
		return true;
	}
	
	return false;
}

void Scene::handleMouseClick() {
	glm::vec3 rayOrigin = _camera->transform.position;
	
	glm::vec3 rayDirection = screenToWorldRay(_windowWidth * 0.5f, _windowHeight * 0.5f);
	
	float closestDistance = std::numeric_limits<float>::max();
	int closestBulletIndex = -1;
	
	for (size_t i = 0; i < _bullets.size(); ++i) {
		const auto& bullet = _bullets[i];
		if (!bullet.active || bullet.destroying) continue;
		
		float effectiveRadius = bullet.radius * 2.0f;
		
		float distance;
		if (rayIntersectsSphere(rayOrigin, rayDirection, bullet.position, effectiveRadius, distance)) {
			
			if (distance < closestDistance) {
				closestDistance = distance;
				closestBulletIndex = static_cast<int>(i);
			}
		}
	}
	
	if (closestBulletIndex >= 0) {
		startBulletDestroy(closestBulletIndex);
	}

	_isRecoiling = true;
	_isFlashing = true;
}

void Scene::startBulletDestroy(size_t bulletIndex) {
	if (bulletIndex < _bullets.size()) {
		_bullets[bulletIndex].destroying = true;
		_bullets[bulletIndex].destroyTimer = 0.0f;
	}
}

void Scene::toggleMouseMode() {
	_cameraControlMode = !_cameraControlMode;
	
	if (_cameraControlMode) {
		glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		_firstMouse = true;
		std::cout << "Camera control mode enabled" << std::endl;
	} else {
		glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		std::cout << "UI interaction mode enabled" << std::endl;
	}
}