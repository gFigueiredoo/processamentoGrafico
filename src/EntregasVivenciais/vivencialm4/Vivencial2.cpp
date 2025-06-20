#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

glm::vec2 processInput(GLFWwindow* window, float deltaTime) {
    float speed = 200.0f * deltaTime;
    glm::vec2 deltaMovement(0.0f, 0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        deltaMovement.y += speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        deltaMovement.y -= speed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        deltaMovement.x -= speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        deltaMovement.x += speed;
    }
    return deltaMovement;
}

GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        std::cout << "Failed to load texture: " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

GLuint compileShader(const char* filePath, GLenum shaderType) {
    std::ifstream shaderFile(filePath);
    if (!shaderFile.is_open()) {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        return 0;
    }

    std::string shaderCode((std::istreambuf_iterator<char>(shaderFile)),
                           std::istreambuf_iterator<char>());
    const char* shaderSource = shaderCode.c_str();

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation error (" << filePath << "): " << infoLog << std::endl;
        return 0;
    }

    return shader;
}

GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    GLuint vertexShader = compileShader(vertexPath, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentPath, GL_FRAGMENT_SHADER);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking error: " << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

class Layer {
public:
    GLuint VAO, textureID;
    glm::vec2 position, scale;
    float parallaxFactor;
    bool isTiling;

    Layer(GLuint texture, glm::vec2 pos, glm::vec2 scl, float parallax)
        : textureID(texture), position(pos), scale(scl), parallaxFactor(parallax), isTiling(false) {
        setupGeometry();
    }

    Layer(GLuint texture, glm::vec2 pos, glm::vec2 scl, float parallax, bool tiling)
        : textureID(texture), position(pos), scale(scl), parallaxFactor(parallax), isTiling(tiling) {
        setupGeometry();
    }

    void update(float deltaX, float deltaY) {
        position.x += deltaX * parallaxFactor;
        position.y += deltaY * parallaxFactor;
    }

    void wrapAround(float screenWidth) {
        if (position.x + scale.x < 0) {
            position.x += scale.x * 2;
        }
        if (position.x > screenWidth) {
            position.x -= scale.x * 2;
        }
    }

    void draw(GLuint shaderProgram) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position, 0.0f));
        model = glm::scale(model, glm::vec3(scale, 1.0f));

        GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        if (isTiling) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(position.x + scale.x, position.y, 0.0f));
            model = glm::scale(model, glm::vec3(scale, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(position.x - scale.x, position.y, 0.0f));
            model = glm::scale(model, glm::vec3(scale, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }
    }

private:
    void setupGeometry() {
        float vertices[] = {
             1.0f,  1.0f,          1.0f, 1.0f,
             1.0f,  0.0f,          1.0f, 0.0f,
             0.0f,  0.0f,          0.0f, 0.0f,

             1.0f,  1.0f,          1.0f, 1.0f,
             0.0f,  0.0f,          0.0f, 0.0f,
             0.0f,  1.0f,          0.0f, 1.0f
        };

        GLuint VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Vivencial 2 - Conrado e Gabriel Figueiredo", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint shaderProgram = createShaderProgram(
        "../src/EntregasVivenciais/vivencialm4/vertex_shader.glsl",
        "../src/EntregasVivenciais/vivencialm4/fragment_shader.glsl"
    );

    glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT, -1.0f, 1.0f);
    glUseProgram(shaderProgram);
    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);

    GLuint textureLayerFar = loadTexture("../src/EntregasVivenciais/vivencialm4/game_background_1.png");
    GLuint textureLayerMid = loadTexture("../src/EntregasVivenciais/vivencialm4/game_background_4.png");
    GLuint textureLayerClose = loadTexture("../src/EntregasVivenciais/vivencialm4/game_background_3.png");
    GLuint characterTexture = loadTexture("../src/EntregasVivenciais/vivencialm4/character.png");
    Layer layerFar(textureLayerFar, glm::vec2(0, 0), glm::vec2(SCR_WIDTH, SCR_HEIGHT), 0.1f, true);
    Layer layerMid(textureLayerMid, glm::vec2(0, 0), glm::vec2(SCR_WIDTH, SCR_HEIGHT), 0.4f, true);
    Layer layerClose(textureLayerClose, glm::vec2(0, 0), glm::vec2(SCR_WIDTH, SCR_HEIGHT), 0.8f, true);

    glm::vec2 characterPosition(SCR_WIDTH / 2.0f - 32.0f, SCR_HEIGHT / 2.0f - 32.0f);
    Layer character(characterTexture, characterPosition, glm::vec2(64, 64), 0.0f, false);

    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glm::vec2 playerDelta = processInput(window, deltaTime);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        layerFar.update(-playerDelta.x, -playerDelta.y);
        layerFar.wrapAround(SCR_WIDTH);
        layerFar.draw(shaderProgram);

        layerMid.update(-playerDelta.x, -playerDelta.y);
        layerMid.wrapAround(SCR_WIDTH);
        layerMid.draw(shaderProgram);

        layerClose.update(-playerDelta.x, -playerDelta.y);
        layerClose.wrapAround(SCR_WIDTH);
        layerClose.draw(shaderProgram);

        character.position += playerDelta;
        character.draw(shaderProgram);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}