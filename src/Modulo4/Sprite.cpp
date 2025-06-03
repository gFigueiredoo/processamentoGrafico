#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <array>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Constants
namespace Config {
    constexpr GLint WINDOW_WIDTH = 800;
    constexpr GLint WINDOW_HEIGHT = 600;
    constexpr const char* WINDOW_TITLE = "Sprites com Textura";
}

// Shader sources
namespace Shaders {
    constexpr const char* VERTEX_SHADER = R"(
        #version 330 core
        layout (location = 0) in vec3 vPosition;
        layout (location = 1) in vec2 vTextureCoord;
        
        uniform mat4 projection;
        uniform mat4 model;
        
        out vec2 TexCoord;
        
        void main() {
            TexCoord = vTextureCoord;
            gl_Position = projection * model * vec4(vPosition, 1.0);
        }
    )";

    constexpr const char* FRAGMENT_SHADER = R"(
        #version 330 core
        in vec2 TexCoord;
        
        uniform sampler2D basic_texture;
        
        out vec4 FragColor;
        
        void main() {
            FragColor = texture(basic_texture, TexCoord);
        }
    )";
}

// Texture Manager
class TextureManager {
public:
    static bool loadTexture(const std::string& filename, GLuint& textureID) {
        int width, height, channels;
        constexpr int forceChannels = 4;
        
        unsigned char* imageData = stbi_load(filename.c_str(), &width, &height, &channels, forceChannels);
        if (!imageData) {
            std::cerr << "ERROR: Could not load texture: " << filename << std::endl;
            return false;
        }

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        // Anisotropic filtering if available
        GLfloat maxAniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(imageData);
        return true;
    }
};

// Shader Manager
class ShaderManager {
private:
    GLuint programID;

    GLuint compileShader(const char* source, GLenum type) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        int success;
        std::array<char, 512> infoLog;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, infoLog.size(), nullptr, infoLog.data());
            std::cerr << "ERROR: Shader compilation failed (type " << type << "): " 
                      << infoLog.data() << std::endl;
        }
        return shader;
    }

public:
    bool initialize() {
        GLuint vertexShader = compileShader(Shaders::VERTEX_SHADER, GL_VERTEX_SHADER);
        GLuint fragmentShader = compileShader(Shaders::FRAGMENT_SHADER, GL_FRAGMENT_SHADER);

        programID = glCreateProgram();
        glAttachShader(programID, vertexShader);
        glAttachShader(programID, fragmentShader);
        glLinkProgram(programID);

        int success;
        std::array<char, 512> infoLog;
        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(programID, infoLog.size(), nullptr, infoLog.data());
            std::cerr << "ERROR: Shader program linking failed: " << infoLog.data() << std::endl;
            return false;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return true;
    }

    void use() const { glUseProgram(programID); }
    GLuint getProgram() const { return programID; }
    
    void setMatrix4(const std::string& name, const glm::mat4& matrix) const {
        glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
    }
    
    void setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
    }

    ~ShaderManager() {
        if (programID) glDeleteProgram(programID);
    }
};

// Sprite class
class Sprite {
public:
    glm::vec2 position{0.0f};
    glm::vec2 scale{100.0f};
    float rotation{0.0f};
    GLuint textureID{0};

    Sprite(const glm::vec2& pos, const glm::vec2& scl, float rot, const std::string& texturePath)
        : position(pos), scale(scl), rotation(rot) {
        if (!TextureManager::loadTexture(texturePath, textureID)) {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
        }
    }

    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position, 0.0f));
        model = glm::translate(model, glm::vec3(0.5f * scale.x, 0.5f * scale.y, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-0.5f * scale.x, -0.5f * scale.y, 0.0f));
        model = glm::scale(model, glm::vec3(scale, 1.0f));
        return model;
    }

    ~Sprite() {
        if (textureID) glDeleteTextures(1, &textureID);
    }
};

// Renderer class
class SpriteRenderer {
private:
    GLuint VAO, VBO, EBO;
    ShaderManager shader;
    glm::mat4 projection;

    void setupQuad() {
        constexpr std::array<float, 20> quadVertices = {
            -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
             0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
             0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
            -0.5f, -0.5f, 0.0f,  0.0f, 0.0f
        };
        
        constexpr std::array<unsigned int, 6> quadIndices = {
            0, 1, 2,
            0, 2, 3
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, quadVertices.size() * sizeof(float), quadVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, quadIndices.size() * sizeof(unsigned int), quadIndices.data(), GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Texture coordinate attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

public:
    bool initialize() {
        if (!shader.initialize()) return false;
        
        setupQuad();
        projection = glm::ortho(0.0f, static_cast<float>(Config::WINDOW_WIDTH), 
                               static_cast<float>(Config::WINDOW_HEIGHT), 0.0f, -1.0f, 1.0f);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        return true;
    }

    void render(const std::vector<std::unique_ptr<Sprite>>& sprites) {
        shader.use();
        shader.setMatrix4("projection", projection);
        
        glBindVertexArray(VAO);
        
        for (const auto& sprite : sprites) {
            shader.setMatrix4("model", sprite->getModelMatrix());
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sprite->textureID);
            shader.setInt("basic_texture", 0);
            
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        
        glBindVertexArray(0);
    }

    ~SpriteRenderer() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};

// Application class
class Application {
private:
    GLFWwindow* window;
    SpriteRenderer renderer;
    std::vector<std::unique_ptr<Sprite>> sprites;

    bool initializeGLFW() {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
        glfwWindowHint(GLFW_SAMPLES, 4);

        window = glfwCreateWindow(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT, 
                                 Config::WINDOW_TITLE, nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window);
        return true;
    }

    bool initializeGLAD() {
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return false;
        }
        return true;
    }

    void createSprites() {
        sprites.emplace_back(std::make_unique<Sprite>(
            glm::vec2(100.0f, 100.0f), glm::vec2(100.0f, 100.0f), 45.0f, "../src/Entregas/m4/1.png"));
        
        sprites.emplace_back(std::make_unique<Sprite>(
            glm::vec2(400.0f, 300.0f), glm::vec2(150.0f, 150.0f), 0.0f, "../src/Entregas/m4/2.png"));
        
        sprites.emplace_back(std::make_unique<Sprite>(
            glm::vec2(600.0f, 50.0f), glm::vec2(200.0f, 100.0f), -30.0f, "../src/Entregas/m4/3.png"));
        
        sprites.emplace_back(std::make_unique<Sprite>(
            glm::vec2(300.0f, 450.0f), glm::vec2(120.0f, 80.0f), 15.0f, "../src/Entregas/m4/Cart.png"));
    }

    void processInput() {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

public:
    bool initialize() {
        return initializeGLFW() && initializeGLAD() && renderer.initialize();
    }

    void run() {
        createSprites();
        
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            processInput();

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            renderer.render(sprites);

            glfwSwapBuffers(window);
        }
    }

    ~Application() {
        glfwTerminate();
    }
};

// Main function
int main() {
    Application app;
    
    if (!app.initialize()) {
        return EXIT_FAILURE;
    }
    
    app.run();
    return EXIT_SUCCESS;
}