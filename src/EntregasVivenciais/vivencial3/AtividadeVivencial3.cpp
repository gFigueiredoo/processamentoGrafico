#define STB_IMAGE_IMPLEMENTATION


#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <gl_utils.h>
#include <stb_image.h>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const int TILE_WIDTH = 96;
const int TILE_HEIGHT = 48;
const int TILESET_ROWS = 1;
const int TILESET_COLS = 7;

int map[3][3] = {
    {1, 1, 4},
    {4, 1, 4},
    {4, 4, 1}
};

const int MAP_ROWS = 3;
const int MAP_COLS = 3;

int cursor_row = 0;
int cursor_col = 0;

const int CURSOR_TILE_ID = 6;

unsigned int texture;

const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "uniform mat4 projection;\n"
    "uniform mat4 model;\n"
    "uniform vec4 tileUVs; \n"
    "void main()\n"
    "{\n"
    "gl_Position = projection * model * vec4(aPos, 1.0);\n"
    "TexCoord = mix(tileUVs.xy, tileUVs.zw, aTexCoord);\n"
    "}\0";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D ourTexture;\n"
    "void main()\n"
    "{\n"
    "FragColor = texture(ourTexture, TexCoord);\n"
    "}\n\0";

unsigned int shaderProgram;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void renderMap();
void setupOpenGL();
void loadTexture(const char* path);
void createShaders();

glm::vec2 gridToIsometric(int map_col, int map_row) {
    float isoX = (float)(map_col - map_row) * (TILE_WIDTH / 2.0f);
    float isoY = (float)(map_col + map_row) * (TILE_HEIGHT / 2.0f);
    
    float map_center_x = (float)(MAP_COLS - MAP_ROWS) * (TILE_WIDTH / 4.0f);
    float map_center_y = (float)(MAP_COLS + MAP_ROWS) * (TILE_HEIGHT / 4.0f);

    float screen_center_x = SCR_WIDTH / 2.0f;
    float screen_center_y = SCR_HEIGHT / 2.0f;

    float offsetX = screen_center_x - map_center_x;
    float offsetY = screen_center_y - map_center_y;

    return glm::vec2(isoX + offsetX, isoY + offsetY);
}

int main() {
    if (!glfwInit()) {
        std::cout << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Vivencial 3 - Conrado e Gabriel Figueiredo", NULL, NULL);
    if (window == NULL) {
        std::cout << "Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }

    setupOpenGL();
    createShaders();
    loadTexture("../assets/tilesets/tilesetIso.png");
    
    std::cout << "Posicao inicial do cursor: (" << cursor_col << ", " << cursor_row << ") - Tile ID: " << map[cursor_row][cursor_col] << std::endl;
    std::cout << "Controles:" << std::endl;
    std::cout << "Q/E - Diagonais superiores" << std::endl;
    std::cout << "Z/C - Diagonais inferiores" << std::endl;
    std::cout << "ESC - Sair" << std::endl;
    
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);

        renderMap();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        int new_row = cursor_row;
        int new_col = cursor_col;
        
        if (key == GLFW_KEY_W) {
            new_row--;
        } else if (key == GLFW_KEY_S) {
            new_row++;
        } else if (key == GLFW_KEY_A) {
            new_col--;
        } else if (key == GLFW_KEY_D) {
            new_col++;
        } else if (key == GLFW_KEY_Q) {
            new_row--;
            new_col--;
        } else if (key == GLFW_KEY_E) {
            new_row--;
            new_col++;
        } else if (key == GLFW_KEY_Z) {
            new_row++;
            new_col--;
        } else if (key == GLFW_KEY_C) {
            new_row++;
            new_col++;
        }

        if (new_row >= 0 && new_row < MAP_ROWS && new_col >= 0 && new_col < MAP_COLS) {
            cursor_row = new_row;
            cursor_col = new_col;
            std::cout << "Cursor movido para: (" << cursor_col << ", " << cursor_row << ") - Tile ID: " << map[cursor_row][cursor_col] << std::endl;
        }
    }
}

void setupOpenGL() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); 
    
    std::cout << "OpenGL configurado com blend e depth test habilitados." << std::endl;
}

void createShaders() {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void loadTexture(const char* path) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        std::cout << "Textura carregada com sucesso: " << path << std::endl;
        std::cout << "Dimensoes: " << width << "x" << height << ", Canais: " << nrChannels << std::endl;
    } else {
        std::cout << "Falha ao carregar textura: " << path << std::endl;
        std::cout << "Erro STB_Image: " << stbi_failure_reason() << std::endl;
    }
    stbi_image_free(data);
}

unsigned int VAO, VBO;
float vertices[] = {
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

    0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 0.0f
};

void renderMap() {
    glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, 0.0f, -1.0f, 1.0f);
    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);       
    }
    glBindVertexArray(VAO);

    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int tileUVsLoc = glGetUniformLocation(shaderProgram, "tileUVs");

    static bool teste_feito = false;
    if (!teste_feito) {
        float u_min = 0.0f;
        float v_min = 0.0f;
        float u_max = 1.0f / (float)TILESET_COLS;
        float v_max = 1.0f;
        glUniform4f(tileUVsLoc, u_min, v_min, u_max, v_max);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-TILE_WIDTH/2.0f, -TILE_HEIGHT/2.0f, 0.0f));
        model = glm::scale(model, glm::vec3((float)TILE_WIDTH, (float)TILE_HEIGHT, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glDrawArrays(GL_TRIANGLES, 0, 6);
        teste_feito = true;
    }
    
    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            int tileId = map[r][c];

            glm::vec2 isoPos = gridToIsometric(c, r);

            float u_min = (float)(tileId % TILESET_COLS) / (float)TILESET_COLS;
            float v_min = 0.0f;
            float u_max = (float)(tileId % TILESET_COLS + 1) / (float)TILESET_COLS;
            float v_max = 1.0f;
            glUniform4f(tileUVsLoc, u_min, v_min, u_max, v_max);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(isoPos.x - (TILE_WIDTH / 2.0f), isoPos.y - TILE_HEIGHT, 0.0f)); 
            model = glm::scale(model, glm::vec3((float)TILE_WIDTH, (float)TILE_HEIGHT, 1.0f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 6);

            if (r == cursor_row && c == cursor_col) {
                int cursorTileId = CURSOR_TILE_ID;

                float cursor_u_min = (float)(cursorTileId % TILESET_COLS) / (float)TILESET_COLS;
                float cursor_v_min = 0.0f;
                float cursor_u_max = (float)(cursorTileId % TILESET_COLS + 1) / (float)TILESET_COLS;
                float cursor_v_max = 1.0f;
                glUniform4f(tileUVsLoc, cursor_u_min, cursor_v_min, cursor_u_max, cursor_v_max);

                glm::mat4 cursor_model = glm::mat4(1.0f);
                cursor_model = glm::translate(cursor_model, glm::vec3(isoPos.x - (TILE_WIDTH / 2.0f), isoPos.y - TILE_HEIGHT, 0.01f)); 
                cursor_model = glm::scale(cursor_model, glm::vec3((float)TILE_WIDTH, (float)TILE_HEIGHT, 1.0f));

                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cursor_model));
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }
}