#include "primitive_renderer.h"
#include <iostream>
#include <cmath>
#include <glad/glad.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PrimitiveRenderer::PrimitiveRenderer() {
}

PrimitiveRenderer::~PrimitiveRenderer() {
    cleanup();
}

void PrimitiveRenderer::init() {
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
        "void main() {\n"
        "    vec3 ambient = 0.3 * lightColor;\n"
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

    createSphereMesh();
    createCylinderMesh();
    createCubeMesh();
}

void PrimitiveRenderer::cleanup() {
    if (_sphereMesh.VAO) {
        glDeleteVertexArrays(1, &_sphereMesh.VAO);
        glDeleteBuffers(1, &_sphereMesh.VBO);
        glDeleteBuffers(1, &_sphereMesh.EBO);
    }
    if (_cylinderMesh.VAO) {
        glDeleteVertexArrays(1, &_cylinderMesh.VAO);
        glDeleteBuffers(1, &_cylinderMesh.VBO);
        glDeleteBuffers(1, &_cylinderMesh.EBO);
    }
    if (_cubeMesh.VAO) {
        glDeleteVertexArrays(1, &_cubeMesh.VAO);
        glDeleteBuffers(1, &_cubeMesh.VBO);
        glDeleteBuffers(1, &_cubeMesh.EBO);
    }
}

void PrimitiveRenderer::createSphereMesh() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    const int latitudes = 20;
    const int longitudes = 20;
    
    for (int lat = 0; lat <= latitudes; ++lat) {
        float theta = lat * M_PI / latitudes;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= longitudes; ++lon) {
            float phi = lon * 2 * M_PI / longitudes;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }
    
    for (int lat = 0; lat < latitudes; ++lat) {
        for (int lon = 0; lon < longitudes; ++lon) {
            int first = lat * (longitudes + 1) + lon;
            int second = first + longitudes + 1;
            
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    
    createMesh(vertices, indices, _sphereMesh);
}

void PrimitiveRenderer::createCylinderMesh() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    const int segments = 20;
    const float height = 2.0f;
    
    for (int i = 0; i <= segments; ++i) {
        float angle = i * 2.0f * M_PI / segments;
        float x = cos(angle);
        float z = sin(angle);
        
        vertices.push_back(x);
        vertices.push_back(height * 0.5f);
        vertices.push_back(z);
        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(z);
        
        vertices.push_back(x);
        vertices.push_back(-height * 0.5f);
        vertices.push_back(z);
        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(z);
    }
    
    for (int i = 0; i < segments; ++i) {
        int base = i * 2;
        indices.push_back(base);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }
    
    createMesh(vertices, indices, _cylinderMesh);
}

void PrimitiveRenderer::createCubeMesh() {
    std::vector<float> vertices = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f
    };
    
    std::vector<unsigned int> indices = {
        0,  1,  2,   2,  3,  0,
        4,  5,  6,   6,  7,  4,
        8,  9, 10,  10, 11,  8,
       12, 13, 14,  14, 15, 12,
       16, 17, 18,  18, 19, 16,
       20, 21, 22,  22, 23, 20
    };
    
    createMesh(vertices, indices, _cubeMesh);
}

void PrimitiveRenderer::createMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices, Mesh& mesh) {
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);
    
    glBindVertexArray(mesh.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    mesh.indexCount = indices.size();
}

void PrimitiveRenderer::renderSphere(const glm::mat4& modelMatrix, const glm::vec3& color, 
                                    const glm::mat4& view, const glm::mat4& projection) {
    renderMesh(_sphereMesh, modelMatrix, color, view, projection);
}

void PrimitiveRenderer::renderCylinder(const glm::mat4& modelMatrix, const glm::vec3& color,
                                      const glm::mat4& view, const glm::mat4& projection) {
    renderMesh(_cylinderMesh, modelMatrix, color, view, projection);
}

void PrimitiveRenderer::renderCube(const glm::mat4& modelMatrix, const glm::vec3& color,
                                  const glm::mat4& view, const glm::mat4& projection) {
    renderMesh(_cubeMesh, modelMatrix, color, view, projection);
}

void PrimitiveRenderer::renderMesh(const Mesh& mesh, const glm::mat4& modelMatrix, const glm::vec3& color,
                                  const glm::mat4& view, const glm::mat4& projection) {
    if (!_shader || mesh.VAO == 0) return;
    
    _shader->use();
    _shader->setUniformMat4("model", modelMatrix);
    _shader->setUniformMat4("view", view);
    _shader->setUniformMat4("projection", projection);
    _shader->setUniformVec3("objectColor", color);
    _shader->setUniformVec3("lightPos", glm::vec3(0.0f, 10.0f, 0.0f));
    _shader->setUniformVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    
    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

bool MathUtils::sphereIntersection(const glm::vec3& center1, float radius1,
                                  const glm::vec3& center2, float radius2) {
    float distance = glm::length(center1 - center2);
    return distance < (radius1 + radius2);
}

glm::vec3 MathUtils::rayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                         const glm::vec3& planePoint, const glm::vec3& planeNormal) {
    float denom = glm::dot(planeNormal, rayDirection);
    if (abs(denom) < 1e-6) {
        return rayOrigin;
    }
    
    float t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
    return rayOrigin + t * rayDirection;
}

bool MathUtils::rayTriangleIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                       const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                                       float& t) {
    const float EPSILON = 0.0000001f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(rayDirection, edge2);
    float a = glm::dot(edge1, h);
    
    if (a > -EPSILON && a < EPSILON) return false;
    
    float f = 1.0f / a;
    glm::vec3 s = rayOrigin - v0;
    float u = f * glm::dot(s, h);
    
    if (u < 0.0f || u > 1.0f) return false;
    
    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(rayDirection, q);
    
    if (v < 0.0f || u + v > 1.0f) return false;
    
    t = f * glm::dot(edge2, q);
    return t > EPSILON;
}

glm::vec3 MathUtils::screenToWorld(const glm::vec2& screenPos, const glm::mat4& view, 
                                  const glm::mat4& projection, const glm::vec4& viewport) {
    glm::vec3 worldPos = glm::unProject(
        glm::vec3(screenPos.x, viewport.w - screenPos.y, 0.5f),
        view, projection, viewport
    );
    return worldPos;
}

float MathUtils::smoothstep(float edge0, float edge1, float x) {
    float t = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
    return t * t * (3.0f - 2.0f * t);
}

float MathUtils::lerp(float a, float b, float t) {
    return a + t * (b - a);
}

glm::vec3 MathUtils::lerp(const glm::vec3& a, const glm::vec3& b, float t) {
    return a + t * (b - a);
} 