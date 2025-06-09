#pragma once

#include <memory>
#include <vector>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/glsl_program.h"
#include "../base/model.h"

class Scene : public Application {
public:
    Scene(const Options& options);
    ~Scene() = default;

    void handleInput() override;

    void renderFrame() override;

private:
    std::unique_ptr<Camera> _camera;

    std::unique_ptr<Model> _turret;

    std::unique_ptr<GLSLProgram> _shader;

    void initShader();
};