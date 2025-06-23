#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const GLint WIDTH = 800, HEIGHT = 600;

const char* vertex_shader_src =
    "#version 330 core\n"
    "layout (location = 0) in vec3 vPosition;\n"
    "layout (location = 1) in vec2 vTextureCoord;\n"
    "uniform mat4 projection;\n"
    "uniform mat4 model;\n"
    "uniform vec4 spriteUVs;\n"
    "out vec2 TexCoord;\n"
    "void main() {\n"
    "   TexCoord.x = mix(spriteUVs.x, spriteUVs.z, vTextureCoord.x);\n"
    "   TexCoord.y = mix(spriteUVs.y, spriteUVs.w, vTextureCoord.y);\n"
    "   gl_Position = projection * model * vec4(vPosition, 1.0);\n"
    "}\n";

const char* fragment_shader_src =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D basic_texture;\n"
    "out vec4 FragColor;\n"
    "void main(){\n"
    "   FragColor = texture(basic_texture, TexCoord);\n"
    "}\n";

GLuint compile_shader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED of type " << type << "\n" << infoLog << std::endl;
    }
    return shader;
}

enum class AnimationType {
    IDLE_FRONT = 0,
    IDLE_LEFT,
    IDLE_RIGHT,
    IDLE_BACK
};

class GameCharacter {
public:
    GameCharacter(
        unsigned int shaderProgram,
        const std::string& texturePath,
        float spriteDisplayWidth,
        float spriteDisplayHeight,
        int totalRows,
        int totalCols
    ) :
        shaderProgram(shaderProgram),
        position(0.0f, 0.0f),
        scale(spriteDisplayWidth, spriteDisplayHeight),
        rotation(0.0f),
        totalAnimationRows(totalRows),
        totalAnimationCols(totalCols),
        currentFrame(0),
        animationFPS(10.0f),
        movementSpeed(150.0f),
        currentAnimationType(AnimationType::IDLE_FRONT)
    {
        loadTexture(texturePath.c_str(), &textureID);
        setupMesh();
        calculateCurrentFrameUVs();
        lastFrameTime = glfwGetTime();
    }

    ~GameCharacter() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteTextures(1, &textureID);
    }

    void update(float deltaTime) {
        double currentTime = glfwGetTime();
        if (currentTime - lastFrameTime >= 1.0 / animationFPS) {
            currentFrame = (currentFrame + 1) % totalAnimationCols;
            calculateCurrentFrameUVs();
            lastFrameTime = currentTime;
        }
    }

    void processInput(GLFWwindow* window, float deltaTime) {
        glm::vec2 oldPosition = position;
        
        AnimationType previousAnimationType = currentAnimationType;

        bool moved = false;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            position.y -= movementSpeed * deltaTime;
            currentAnimationType = AnimationType::IDLE_BACK;
            moved = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            position.y += movementSpeed * deltaTime;
            currentAnimationType = AnimationType::IDLE_FRONT;
            moved = true;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            position.x -= movementSpeed * deltaTime;
            currentAnimationType = AnimationType::IDLE_LEFT;
            moved = true;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            position.x += movementSpeed * deltaTime;
            currentAnimationType = AnimationType::IDLE_RIGHT;
            moved = true;
        }

        if (!moved) {
            if (previousAnimationType != currentAnimationType) {
                currentFrame = 0;
                lastFrameTime = glfwGetTime();
            }
        }
        
        if (currentAnimationType != previousAnimationType) {
            currentFrame = 0;
            lastFrameTime = glfwGetTime();
        }

        if (position.x < 0) position.x = 0;
        if (position.x + scale.x > WIDTH) position.x = WIDTH - scale.x;
        if (position.y < 0) position.y = 0;
        if (position.y + scale.y > HEIGHT) position.y = HEIGHT - scale.y;
    }

    void draw(const glm::mat4& projection) {
        glUseProgram(shaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shaderProgram, "basic_texture"), 0);

        glUniform4f(glGetUniformLocation(shaderProgram, "spriteUVs"), 
                    currentFrameUVs.x, currentFrameUVs.y, currentFrameUVs.z, currentFrameUVs.w);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position, 0.0f));
        model = glm::translate(model, glm::vec3(0.5f * scale.x, 0.5f * scale.y, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-0.5f * scale.x, -0.5f * scale.y, 0.0f));
        model = glm::scale(model, glm::vec3(scale, 1.0f));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void setPosition(float x, float y) {
        position.x = x;
        position.y = y;
    }

    void setMovementSpeed(float speed) { movementSpeed = speed; }

    void setAnimationFPS(float fps) { animationFPS = fps; }

private:
    GLuint VAO, VBO, EBO;
    GLuint textureID;
    unsigned int shaderProgram;

    glm::vec2 position;
    glm::vec2 scale;
    float rotation;

    int totalAnimationRows;
    int totalAnimationCols;
    int currentFrame;
    double lastFrameTime;
    float animationFPS;
    glm::vec4 currentFrameUVs;

    float movementSpeed;

    AnimationType currentAnimationType;

    bool loadTexture(const char* file_name, GLuint* tex_id) {
        int x, y, n;
        int force_channels = 4;
        stbi_set_flip_vertically_on_load(true); 
        unsigned char* image_data = stbi_load(file_name, &x, &y, &n, force_channels);
        if (!image_data) {
            fprintf(stderr, "ERROR: could not load %s\n", file_name);
            return false;
        }

        glGenTextures(1, tex_id);
        glBindTexture(GL_TEXTURE_2D, *tex_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        GLfloat max_aniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image_data);
        std::cout << "Textura carregada com sucesso: " << file_name << std::endl;
        return true;
    }

    void setupMesh() {
        float quad_vertices[] = {
            -0.5f,  0.5f, 0.0f,   0.0f, 1.0f,
             0.5f,  0.5f, 0.0f,   1.0f, 1.0f,
             0.5f, -0.5f, 0.0f,   1.0f, 0.0f,
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f
        };
        unsigned int quad_indices[] = {
            0, 1, 2,
            0, 2, 3
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void calculateCurrentFrameUVs() {
        float frameWidthNormalized = 1.0f / totalAnimationCols;
        float frameHeightNormalized = 1.0f / totalAnimationRows;

        float u_min = currentFrame * frameWidthNormalized;
        float u_max = u_min + frameWidthNormalized;

        float v_min = static_cast<int>(currentAnimationType) * frameHeightNormalized;
        float v_max = v_min + frameHeightNormalized;
        
        currentFrameUVs = glm::vec4(u_min, v_min, u_max, v_max);
    }
};

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Tarefa M5 - Gabriel", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW Window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint vertex_shader = compile_shader(vertex_shader_src, GL_VERTEX_SHADER);
    GLuint fragment_shader = compile_shader(fragment_shader_src, GL_FRAGMENT_SHADER);

    GLuint shader_programme = glCreateProgram();
    glAttachShader(shader_programme, fragment_shader);
    glAttachShader(shader_programme, vertex_shader);
    glLinkProgram(shader_programme);

    int success;
    char infoLog[512];
    glGetProgramiv(shader_programme, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_programme, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);

    GameCharacter player(shader_programme,
                         "../assets/sprites/Slime1_Idle_full.png", 
                         64.0f, 64.0f,
                         4, 6);
    player.setPosition(WIDTH / 2.0f - 32.0f, HEIGHT / 2.0f - 32.0f);
    player.setMovementSpeed(200.0f);
    player.setAnimationFPS(10.0f);

    double lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentFrameTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;

        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        player.processInput(window, deltaTime);
        player.update(deltaTime);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        player.draw(projection);

        glfwSwapBuffers(window);
    }

    glDeleteProgram(shader_programme);
    glfwTerminate();
    return EXIT_SUCCESS;
}
