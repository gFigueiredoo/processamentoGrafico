#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const GLint WIDTH = 800, HEIGHT = 600;

class Sprite {
public:
    GLuint VAO;
    GLuint textureID;
    glm::vec2 position;
    glm::vec2 scale;
    float rotation;
};

bool load_texture(const char* file_name, GLuint* tex_id) {
    int x, y, n;
    int force_channels = 4;
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
    return true;
}

const char* vertex_shader_src =
    "#version 330 core\n"
    "layout ( location = 0 ) in vec3 vPosition;"
    "layout ( location = 1 ) in vec2 vTextureCoord;"
    "uniform mat4 projection;"
    "uniform mat4 model;" 
    "out vec2 TexCoord;"
    "void main() {"
    "   TexCoord = vTextureCoord;"
    "   gl_Position = projection * model * vec4 ( vPosition, 1.0);"
    "}";

const char* fragment_shader_src =
    "#version 330 core\n"
    "in vec2 TexCoord;"
    "uniform sampler2D basic_texture;"
    "out vec4 FragColor;"
    "void main(){"
    "   FragColor = texture(basic_texture, TexCoord);"
    "}";

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

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Sprites com Textura", nullptr, nullptr);
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

    float quad_vertices[] = {
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f
    };
    unsigned int quad_indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    GLuint quad_VBO, quad_VAO, quad_EBO;
    glGenVertexArrays(1, &quad_VAO);
    glGenBuffers(1, &quad_VBO);
    glGenBuffers(1, &quad_EBO);

    glBindVertexArray(quad_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, quad_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    std::vector<Sprite> sprites;

    Sprite sprite1;
    sprite1.position = glm::vec2(100.0f, 100.0f);
    sprite1.scale = glm::vec2(100.0f, 100.0f);
    sprite1.rotation = 45.0f;
    load_texture("../src/Entregas/m4/1.png", &sprite1.textureID);
    sprites.push_back(sprite1);

    Sprite sprite2;
    sprite2.position = glm::vec2(400.0f, 300.0f);
    sprite2.scale = glm::vec2(150.0f, 150.0f);
    sprite2.rotation = 0.0f;
    load_texture("../src/Entregas/m4/2.png", &sprite2.textureID);
    sprites.push_back(sprite2);

    Sprite sprite3;
    sprite3.position = glm::vec2(600.0f, 50.0f);
    sprite3.scale = glm::vec2(200.0f, 100.0f);
    sprite3.rotation = -30.0f;
    load_texture("../src/Entregas/m4/3.png", &sprite3.textureID);
    sprites.push_back(sprite3);

    Sprite spriteCart;
    spriteCart.position = glm::vec2(300.0f, 450.0f);
    spriteCart.scale = glm::vec2(120.0f, 80.0f);
    spriteCart.rotation = 15.0f;
    load_texture("../src/Entregas/m4/Cart.png", &spriteCart.textureID);
    sprites.push_back(spriteCart);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_programme);
        glUniformMatrix4fv(glGetUniformLocation(shader_programme, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(quad_VAO);

        for (const auto& sprite : sprites) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(sprite.position, 0.0f));
            model = glm::translate(model, glm::vec3(0.5f * sprite.scale.x, 0.5f * sprite.scale.y, 0.0f));
            model = glm::rotate(model, glm::radians(sprite.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::translate(model, glm::vec3(-0.5f * sprite.scale.x, -0.5f * sprite.scale.y, 0.0f));
            model = glm::scale(model, glm::vec3(sprite.scale, 1.0f));

            glUniformMatrix4fv(glGetUniformLocation(shader_programme, "model"), 1, GL_FALSE, glm::value_ptr(model));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sprite.textureID);
            glUniform1i(glGetUniformLocation(shader_programme, "basic_texture"), 0);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &quad_VAO);
    glDeleteBuffers(1, &quad_VBO);
    glDeleteBuffers(1, &quad_EBO);

    for (const auto& sprite : sprites) {
        glDeleteTextures(1, &sprite.textureID);
    }

    glDeleteProgram(shader_programme);
    glfwTerminate();
    return EXIT_SUCCESS;
}