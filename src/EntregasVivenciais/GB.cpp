#define STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

int TILE_WIDTH = 64;
int TILE_HEIGHT = 32;
int TILESET_ROWS = 0;
int TILESET_COLS = 0;
std::string TILESET_PATH;

std::vector<std::vector<int>> game_map;
std::vector<std::vector<bool>> walkable_map;
std::vector<std::vector<char>> object_map;

int MAP_ROWS = 0;
int MAP_COLS = 0;

int cursor_row = 0;
int cursor_col = 0;

int items_collected = 0;
bool game_over = false;
bool game_won = false;

const int TILE_MOEDA = 5;
const int TILE_LAVA = 4;
const int TILE_VITORIA = 3;
const int CURSOR_TILE_ID = 6;

unsigned int texture;
unsigned int shaderProgram;

unsigned int VAO, VBO;

float vertices[] = {
    0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
    1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
    0.0f, 0.0f, 0.0f,  0.0f, 0.0f,

    0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
    1.0f, 1.0f, 0.0f,  1.0f, 1.0f,
    1.0f, 0.0f, 0.0f,  1.0f, 0.0f
};

const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "uniform mat4 projection;\n"
    "uniform mat4 model;\n"
    "uniform vec4 tileUVs;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * model * vec4(aPos, 1.0);\n"
    "   TexCoord = mix(tileUVs.xy, tileUVs.zw, aTexCoord);\n"
    "}\0";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D ourTexture;\n"
    "void main()\n"
    "{\n"
    "   FragColor = texture(ourTexture, TexCoord);\n"
    "}\n\0";

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void setupOpenGL();
void createShaders();
void loadTexture(const char* path);
bool loadMapConfig(const std::string& filename);
bool loadWalkability(const std::string& filename);
glm::vec2 gridToIsometric(int col, int row);
void renderMap();

int main() {
    std::cout << "---- Jogo Isometrico Iniciado ----" << std::endl;

    if (!loadMapConfig("map.txt")) return -1;
    if (!loadWalkability("walkable.txt")) return -1;

    object_map.resize(MAP_ROWS, std::vector<char>(MAP_COLS, '.'));
    object_map[1][1] = 'M';
    object_map[7][7] = 'L';
    object_map[13][13] = 'V';

    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Jogo Isometrico", NULL, NULL);
    if (!window) {
        std::cerr << "Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }

    setupOpenGL();
    createShaders();
    loadTexture(TILESET_PATH.c_str());

    std::cout << "Controles: W/S/A/D para mover, Q/E/Z/C para diagonais, ESC para sair." << std::endl;

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

bool loadMapConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "tileset_info:") {
            iss >> TILESET_PATH >> TILESET_COLS >> TILESET_ROWS >> TILE_WIDTH >> TILE_HEIGHT;
        } else if (key == "map_dimensions:") {
            iss >> MAP_ROWS >> MAP_COLS;
            game_map.resize(MAP_ROWS, std::vector<int>(MAP_COLS));
        } else if (key == "map_data:") {
            for (int r = 0; r < MAP_ROWS; ++r) {
                std::string row_str;
                if (!std::getline(file, row_str)) {
                    std::cerr << "Dados do mapa incompletos." << std::endl;
                    return false;
                }
                if ((int)row_str.length() != MAP_COLS) {
                    std::cerr << "Largura da linha do mapa incorreta." << std::endl;
                    return false;
                }
                for (int c = 0; c < MAP_COLS; ++c) {
                    game_map[r][c] = row_str[c] - '0';
                }
            }
        }
    }
    file.close();
    std::cout << "Configuração do mapa carregada." << std::endl;
    return true;
}

bool loadWalkability(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string key;
        iss >> key;
        if (key == "walkable_data:") break;
    }

    walkable_map.resize(MAP_ROWS, std::vector<bool>(MAP_COLS));
    for (int r = 0; r < MAP_ROWS; ++r) {
        if (!std::getline(file, line)) {
            std::cerr << "Dados de caminhabilidade incompletos." << std::endl;
            return false;
        }
        if ((int)line.length() != MAP_COLS) {
            std::cerr << "Largura da linha de caminhabilidade incorreta." << std::endl;
            return false;
        }
        for (int c = 0; c < MAP_COLS; ++c) {
            walkable_map[r][c] = (line[c] == 'W');
        }
    }
    file.close();
    std::cout << "Dados de caminhabilidade carregados." << std::endl;
    return true;
}

glm::vec2 gridToIsometric(int col, int row) {
    float isoX = (col - row) * (TILE_WIDTH / 2.0f);
    float isoY = (col + row) * (TILE_HEIGHT / 2.0f);

    float offsetX = SCR_WIDTH / 2.0f;
    float offsetY = TILE_HEIGHT / 2.0f;

    return glm::vec2(isoX + offsetX, isoY + offsetY);
}

void renderMap() {
    glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

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

    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            int tileId = game_map[r][c];
            glm::vec2 pos = gridToIsometric(c, r);

            float u_min = (float)(tileId % TILESET_COLS) / TILESET_COLS;
            float v_min = (float)(tileId / TILESET_COLS) / TILESET_ROWS;
            float u_max = (float)(tileId % TILESET_COLS + 1) / TILESET_COLS;
            float v_max = (float)(tileId / TILESET_COLS + 1) / TILESET_ROWS;

            glUniform4f(tileUVsLoc, u_min, v_min, u_max, v_max);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(pos.x - TILE_WIDTH / 2.0f, pos.y - TILE_HEIGHT, 0.0f));
            model = glm::scale(model, glm::vec3((float)TILE_WIDTH, (float)TILE_HEIGHT, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 6);

            char obj = object_map[r][c];
            int objTileId = -1;
            if (obj == 'M') objTileId = TILE_MOEDA;
            else if (obj == 'L') objTileId = TILE_LAVA;
            else if (obj == 'V') objTileId = TILE_VITORIA;

            if (objTileId != -1) {
                float ou_min = (float)(objTileId % TILESET_COLS) / TILESET_COLS;
                float ov_min = (float)(objTileId / TILESET_COLS) / TILESET_ROWS;
                float ou_max = (float)(objTileId % TILESET_COLS + 1) / TILESET_COLS;
                float ov_max = (float)(objTileId / TILESET_COLS + 1) / TILESET_ROWS;

                glUniform4f(tileUVsLoc, ou_min, ov_min, ou_max, ov_max);

                glm::mat4 objModel = glm::mat4(1.0f);
                objModel = glm::translate(objModel, glm::vec3(pos.x - TILE_WIDTH / 2.0f, pos.y - TILE_HEIGHT, 0.01f));
                objModel = glm::scale(objModel, glm::vec3((float)TILE_WIDTH, (float)TILE_HEIGHT, 1.0f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(objModel));
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            if (r == cursor_row && c == cursor_col) {
                float cu_min = (float)(CURSOR_TILE_ID % TILESET_COLS) / TILESET_COLS;
                float cv_min = (float)(CURSOR_TILE_ID / TILESET_COLS) / TILESET_ROWS;
                float cu_max = (float)(CURSOR_TILE_ID % TILESET_COLS + 1) / TILESET_COLS;
                float cv_max = (float)(CURSOR_TILE_ID / TILESET_COLS + 1) / TILESET_ROWS;

                glUniform4f(tileUVsLoc, cu_min, cv_min, cu_max, cv_max);

                glm::mat4 cursorModel = glm::mat4(1.0f);
                cursorModel = glm::translate(cursorModel, glm::vec3(pos.x - TILE_WIDTH / 2.0f, pos.y - TILE_HEIGHT, 0.02f));
                cursorModel = glm::scale(cursorModel, glm::vec3((float)TILE_WIDTH, (float)TILE_HEIGHT, 1.0f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cursorModel));
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }
}

void setupOpenGL() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
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
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Textura carregada: " << path << std::endl;
    } else {
        std::cerr << "Falha ao carregar textura: " << path << std::endl;
    }
    stbi_image_free(data);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    if (game_over || game_won) return;

    int new_row = cursor_row;
    int new_col = cursor_col;

    if (key == GLFW_KEY_W) new_row--;
    else if (key == GLFW_KEY_S) new_row++;
    else if (key == GLFW_KEY_A) new_col--;
    else if (key == GLFW_KEY_D) new_col++;
    else if (key == GLFW_KEY_Q) { new_row--; new_col--; }
    else if (key == GLFW_KEY_E) { new_row--; new_col++; }
    else if (key == GLFW_KEY_Z) { new_row++; new_col--; }
    else if (key == GLFW_KEY_C) { new_row++; new_col++; }

    if (new_row >= 0 && new_row < MAP_ROWS && new_col >= 0 && new_col < MAP_COLS) {
        if (walkable_map[new_row][new_col]) {
            cursor_row = new_row;
            cursor_col = new_col;
            std::cout << "Cursor movido para (" << cursor_col << ", " << cursor_row << ")" << std::endl;

            char obj = object_map[cursor_row][cursor_col];
            if (obj == 'M') {
                items_collected++;
                object_map[cursor_row][cursor_col] = '.';
                std::cout << "Moeda coletada! Total: " << items_collected << std::endl;
            } else if (obj == 'L') {
                game_over = true;
                std::cout << "Voce morreu na lava! Fim de jogo." << std::endl;
            } else if (obj == 'V') {
                game_won = true;
                std::cout << "Parabens! Voce venceu o jogo!" << std::endl;
            }
        } else {
            std::cout << "Tile nao caminhavel." << std::endl;
        }
    }
}