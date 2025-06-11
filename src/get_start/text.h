#pragma once

#include <map>
#include <string>
#include <glm/glm.hpp>
#include <memory>
#include "../base/glsl_program.h"
#include <ft2build.h>
#include FT_FREETYPE_H


struct Character {
    GLuint TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    GLuint Advance;
};

class TextRenderer {
public:
    TextRenderer();
    void initshader();
    void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color);

private:
    std::map<char, Character> Characters;
    GLuint VAO, VBO;
    std::unique_ptr<GLSLProgram> shader;
    glm::mat4 projection;
    FT_Library ft;    
    FT_Face face;
    int screenWidth = 1920; // 默认屏幕宽度
    int screenHeight = 1080; // 默认屏幕高度
};
