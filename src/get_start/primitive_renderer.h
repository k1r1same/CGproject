#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../base/glsl_program.h"

class PrimitiveRenderer {
public:
    PrimitiveRenderer();
    ~PrimitiveRenderer();

    void init();
    void cleanup();
    
    void renderSphere(const glm::mat4& modelMatrix, const glm::vec3& color, 
                     const glm::mat4& view, const glm::mat4& projection);
    void renderCylinder(const glm::mat4& modelMatrix, const glm::vec3& color,
                       const glm::mat4& view, const glm::mat4& projection);
    void renderCube(const glm::mat4& modelMatrix, const glm::vec3& color,
                   const glm::mat4& view, const glm::mat4& projection);

private:
    struct Mesh {
        unsigned int VAO = 0;
        unsigned int VBO = 0;
        unsigned int EBO = 0;
        int indexCount = 0;
    };
    
    Mesh _sphereMesh;
    Mesh _cylinderMesh;
    Mesh _cubeMesh;
    
    std::unique_ptr<GLSLProgram> _shader;
    
    void createSphereMesh();
    void createCylinderMesh();
    void createCubeMesh();
    void createMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices, Mesh& mesh);
    void renderMesh(const Mesh& mesh, const glm::mat4& modelMatrix, const glm::vec3& color,
                   const glm::mat4& view, const glm::mat4& projection);
};

class MathUtils {
public:
    static bool sphereIntersection(const glm::vec3& center1, float radius1,
                                  const glm::vec3& center2, float radius2);
    
    static glm::vec3 rayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                         const glm::vec3& planePoint, const glm::vec3& planeNormal);
    
    static bool rayTriangleIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                       const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                                       float& t);
    
    static glm::vec3 screenToWorld(const glm::vec2& screenPos, const glm::mat4& view, 
                                  const glm::mat4& projection, const glm::vec4& viewport);
    
    static float smoothstep(float edge0, float edge1, float x);
    static float lerp(float a, float b, float t);
    static glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t);
}; 