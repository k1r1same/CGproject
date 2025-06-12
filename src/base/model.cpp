#include <algorithm>
#include <iostream>
#include <limits>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "model.h"

Model::Model(const std::string& filepath) {
    Assimp::Importer importer;
    
    // 设置后处理选项
    const aiScene* scene = importer.ReadFile(filepath, 
        aiProcess_Triangulate | 
        aiProcess_FlipUVs | 
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("Failed to load model: " + std::string(importer.GetErrorString()));
    }
    
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<Vertex, uint32_t> uniqueVertices;
    
    // 处理所有网格
    processNode(scene->mRootNode, scene, vertices, indices, uniqueVertices);
    
    _vertices = vertices;
    _indices = indices;

    computeBoundingBox();

    initGLResources();

    initBoxGLResources();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        cleanup();
        throw std::runtime_error("OpenGL Error: " + std::to_string(error));
    }
}

void Model::processNode(aiNode* node, const aiScene* scene, 
                       std::vector<Vertex>& vertices, 
                       std::vector<uint32_t>& indices,
                       std::unordered_map<Vertex, uint32_t>& uniqueVertices) {
    // 处理当前节点的所有网格
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene, vertices, indices, uniqueVertices);
    }
    
    // 递归处理子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, vertices, indices, uniqueVertices);
    }
}

void Model::processMesh(aiMesh* mesh, const aiScene* scene,
                       std::vector<Vertex>& vertices, 
                       std::vector<uint32_t>& indices,
                       std::unordered_map<Vertex, uint32_t>& uniqueVertices) {
    // 处理顶点
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{};
        
        // 位置
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;
        
        // 法线
        if (mesh->HasNormals()) {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
        }
        
        // 纹理坐标
        if (mesh->mTextureCoords[0]) {
            vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        } else {
            vertex.texCoord = glm::vec2(0.0f, 0.0f);
        }
        
        // 检查顶点是否已存在以减少冗余数据
        if (uniqueVertices.count(vertex) == 0) {
            uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
            vertices.push_back(vertex);
        }
    }
    
    // 处理索引
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            Vertex vertex{};
            
            // 获取顶点数据
            vertex.position.x = mesh->mVertices[face.mIndices[j]].x;
            vertex.position.y = mesh->mVertices[face.mIndices[j]].y;
            vertex.position.z = mesh->mVertices[face.mIndices[j]].z;
            
            if (mesh->HasNormals()) {
                vertex.normal.x = mesh->mNormals[face.mIndices[j]].x;
                vertex.normal.y = mesh->mNormals[face.mIndices[j]].y;
                vertex.normal.z = mesh->mNormals[face.mIndices[j]].z;
            }
            
            if (mesh->mTextureCoords[0]) {
                vertex.texCoord.x = mesh->mTextureCoords[0][face.mIndices[j]].x;
                vertex.texCoord.y = mesh->mTextureCoords[0][face.mIndices[j]].y;
            } else {
                vertex.texCoord = glm::vec2(0.0f, 0.0f);
            }
            
            indices.push_back(uniqueVertices[vertex]);
        }
    }
}

Model::Model(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : _vertices(vertices), _indices(indices) {

    computeBoundingBox();

    initGLResources();

    initBoxGLResources();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        cleanup();
        throw std::runtime_error("OpenGL Error: " + std::to_string(error));
    }
}

Model::Model(Model&& rhs) noexcept
    : _vertices(std::move(rhs._vertices)), _indices(std::move(rhs._indices)),
      _boundingBox(std::move(rhs._boundingBox)), _vao(rhs._vao), _vbo(rhs._vbo), _ebo(rhs._ebo),
      _boxVao(rhs._boxVao), _boxVbo(rhs._boxVbo), _boxEbo(rhs._boxEbo) {
    _vao = 0;
    _vbo = 0;
    _ebo = 0;
    _boxVao = 0;
    _boxVbo = 0;
    _boxEbo = 0;
}

Model::~Model() {
    cleanup();
}

BoundingBox Model::getBoundingBox() const {
    return _boundingBox;
}

void Model::draw() const {
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Model::drawBoundingBox() const {
    glBindVertexArray(_boxVao);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

GLuint Model::getVao() const {
    return _vao;
}

GLuint Model::getBoundingBoxVao() const {
    return _boxVao;
}

size_t Model::getVertexCount() const {
    return _vertices.size();
}

size_t Model::getFaceCount() const {
    return _indices.size() / 3;
}

void Model::initGLResources() {
    // create a vertex array object
    glGenVertexArrays(1, &_vao);
    // create a vertex buffer object
    glGenBuffers(1, &_vbo);
    // create a element array buffer
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(Vertex) * _vertices.size(), _vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(uint32_t), _indices.data(),
        GL_STATIC_DRAW);

    // specify layout, size of a vertex, data type, normalize, sizeof vertex array, offset of the
    // attribute
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Model::computeBoundingBox() {
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    float maxZ = -std::numeric_limits<float>::max();

    for (const auto& v : _vertices) {
        minX = std::min(v.position.x, minX);
        minY = std::min(v.position.y, minY);
        minZ = std::min(v.position.z, minZ);
        maxX = std::max(v.position.x, maxX);
        maxY = std::max(v.position.y, maxY);
        maxZ = std::max(v.position.z, maxZ);
    }

    _boundingBox.min = glm::vec3(minX, minY, minZ);
    _boundingBox.max = glm::vec3(maxX, maxY, maxZ);
}

void Model::initBoxGLResources() {
    std::vector<glm::vec3> boxVertices = {
        glm::vec3(_boundingBox.min.x, _boundingBox.min.y, _boundingBox.min.z),
        glm::vec3(_boundingBox.max.x, _boundingBox.min.y, _boundingBox.min.z),
        glm::vec3(_boundingBox.min.x, _boundingBox.max.y, _boundingBox.min.z),
        glm::vec3(_boundingBox.max.x, _boundingBox.max.y, _boundingBox.min.z),
        glm::vec3(_boundingBox.min.x, _boundingBox.min.y, _boundingBox.max.z),
        glm::vec3(_boundingBox.max.x, _boundingBox.min.y, _boundingBox.max.z),
        glm::vec3(_boundingBox.min.x, _boundingBox.max.y, _boundingBox.max.z),
        glm::vec3(_boundingBox.max.x, _boundingBox.max.y, _boundingBox.max.z),
    };

    std::vector<uint32_t> boxIndices = {0, 1, 0, 2, 0, 4, 3, 1, 3, 2, 3, 7,
                                        5, 4, 5, 1, 5, 7, 6, 4, 6, 7, 6, 2};

    glGenVertexArrays(1, &_boxVao);
    glGenBuffers(1, &_boxVbo);
    glGenBuffers(1, &_boxEbo);

    glBindVertexArray(_boxVao);
    glBindBuffer(GL_ARRAY_BUFFER, _boxVbo);
    glBufferData(
        GL_ARRAY_BUFFER, boxVertices.size() * sizeof(glm::vec3), boxVertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _boxEbo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, boxIndices.size() * sizeof(uint32_t), boxIndices.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Model::cleanup() {
    if (_boxEbo) {
        glDeleteBuffers(1, &_boxEbo);
        _boxEbo = 0;
    }

    if (_boxVbo) {
        glDeleteBuffers(1, &_boxVbo);
        _boxVbo = 0;
    }

    if (_boxVao) {
        glDeleteVertexArrays(1, &_boxVao);
        _boxVao = 0;
    }

    if (_ebo != 0) {
        glDeleteBuffers(1, &_ebo);
        _ebo = 0;
    }

    if (_vbo != 0) {
        glDeleteBuffers(1, &_vbo);
        _vbo = 0;
    }

    if (_vao != 0) {
        glDeleteVertexArrays(1, &_vao);
        _vao = 0;
    }
}
Model Model::interpolateModel(const Model& m1, const Model& m2, float t) {
    const auto& v1 = m1.getVertices();
    const auto& v2 = m2.getVertices();
    std::vector<Vertex> interpolatedVertices;

    if (v1.size() != v2.size()) {
        throw std::runtime_error("Model vertex count mismatch!");
    }

    for (size_t i = 0; i < v1.size(); ++i) {
        Vertex v;
        v.position = glm::mix(v1[i].position, v2[i].position, t);
        v.normal = glm::normalize(glm::mix(v1[i].normal, v2[i].normal, t));
        v.texCoord = glm::mix(v1[i].texCoord, v2[i].texCoord, t);
        interpolatedVertices.push_back(v);
    }

    return Model(interpolatedVertices, m1.getIndices());
}
