#define STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cmath>
#include <map>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>

GLuint compile_shader(const char* source, GLenum type);
void setupOpenGL();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

enum class AnimationType {
    IDLE_FRONT = 0,
    IDLE_LEFT,
    IDLE_RIGHT,
    IDLE_BACK
};

class GameCharacter;
class MovementCommand;

class GameManager {
private:
    static GameManager* instance;

    GLFWwindow* glfwWindow;
    unsigned int shaderProgram;
    unsigned int texture;
    unsigned int VAO, VBO;
    GameCharacter* player_char;
    class InputHandler* inputHandler;

    const unsigned int SCR_WIDTH = 1920;
    const unsigned int SCR_HEIGHT = 1080;

    int m_baseTileWidth = 0;
    int m_baseTileHeight = 0;

    int TILE_WIDTH_SCALED = 64;
    int TILE_HEIGHT_SCALED = 32;

    int TILESET_ROWS = 0;
    int TILESET_COLS = 0;
    std::string TILESET_PATH;

    float GAME_SCALE = 2.0f;

    std::vector<std::vector<int>> game_map;
    std::vector<std::vector<int>> initial_game_map;

    int MAP_ROWS = 0;
    int MAP_COLS = 0;

    int items_collected = 0;
    int total_coins_on_map = 0;
    bool game_over = false;
    bool game_won = false;
    bool game_ended_by_lava = false;
    bool effect_applied = false;

    const int TILE_MOEDA = 0;
    const int TILE_CHAO = 1;
    const int TILE_PAREDE = 2;
    const int TILE_LAVA = 3;
    const int TILE_INICIO = 4;
    const int TILE_AGUA = 5;
    const int TILE_VICTORY_EFFECT_TILE_ID = 6;

    float vertices[30] = {
        0.0f, 1.0f, 0.0f,     0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,     1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,     0.0f, 0.0f,

        0.0f, 1.0f, 0.0f,     0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,     1.0f, 1.0f,
        1.0f, 0.0f, 0.0f,     1.0f, 0.0f
    };

    const char* vertexShaderSource =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 model;\n"
        "uniform vec4 spriteUVs;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "   TexCoord.x = mix(spriteUVs.x, spriteUVs.z, aTexCoord.x);\n"
        "   TexCoord.y = mix(spriteUVs.y, spriteUVs.w, aTexCoord.y);\n"
        "   gl_Position = projection * model * vec4(aPos, 1.0);\n"
        "}\n";

    const char* fragmentShaderSource =
        "#version 330 core\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D basic_texture;\n"
        "out vec4 FragColor;\n"
        "void main(){\n"
        "   FragColor = texture(basic_texture, TexCoord);\n"
        "}\n";

    GameManager();

    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    bool loadTexture(const char* path);
    bool loadMapConfig(const std::string& filename);
    void renderMap();

public:
    static GameManager* getInstance();

    ~GameManager();

    void initialize(GLFWwindow* window);
    void update(float deltaTime);
    void render();
    void resetGame();

    void handleKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void processPlayerMovement(int new_row, int new_col);

    static glm::vec2 gridToIsometric(int col, int row);

    int getTileWidth() const { return TILE_WIDTH_SCALED; } 
    int getTileHeight() const { return TILE_HEIGHT_SCALED; }
    int getMapRows() const { return MAP_ROWS; }
    int getMapCols() const { return MAP_COLS; }
    int getTileId(int r, int c) const;
    void setPlayerAnimation(AnimationType type);
    bool isGameOver() const { return game_over; }
    bool hasGameWon() const { return game_won; }
};

class GameCharacter {
public:
    int row;
    int col;

    GameCharacter(
        unsigned int sharedShaderProgram,
        const std::string& texturePath,
        float spriteDisplayWidth,
        float spriteDisplayHeight,
        int totalRows,
        int totalCols
    );

    ~GameCharacter();

    void update(float deltaTime);
    void setGridPosition(int r, int c);
    void draw(const glm::mat4& projection, glm::vec2 (*gridToIsometricFunc)(int, int));
    void setAnimationFPS(float fps);
    void setAnimationType(AnimationType type);
    AnimationType getAnimationType() const;

private:
    GLuint VAO, VBO, EBO;
    GLuint textureID;
    unsigned int shaderProgram;

    glm::vec2 displayScale;
    float rotation;

    int totalAnimationRows;
    int totalAnimationCols;
    int currentFrame;
    double lastFrameTime;
    float animationFPS;
    glm::vec4 currentFrameUVs;

    AnimationType currentAnimationType;

    bool loadTexture(const char* file_name, GLuint* tex_id);
    void setupMesh();
    void calculateCurrentFrameUVs();
};

class MovementCommand {
public:
    virtual void execute(GameCharacter* character) = 0;
    virtual ~MovementCommand() {}
};

class MoveUpCommand : public MovementCommand {
public:
    void execute(GameCharacter* character) override;
};

class MoveDownCommand : public MovementCommand {
public:
    void execute(GameCharacter* character) override;
};

class MoveLeftCommand : public MovementCommand {
public:
    void execute(GameCharacter* character) override;
};

class MoveRightCommand : public MovementCommand {
public:
    void execute(GameCharacter* character) override;
};

class MoveUpLeftCommand : public MovementCommand {
public:
    void execute(GameCharacter* character) override;
};

class MoveUpRightCommand : public MovementCommand {
public:
    void execute(GameCharacter* character) override;
};

class MoveDownLeftCommand : public MovementCommand {
public:
    void execute(GameCharacter* character) override;
};

class MoveDownRightCommand : public MovementCommand {
public:
    void execute(GameCharacter* character) override;
};

class ResetGameCommand : public MovementCommand {
public:
    void execute(GameCharacter* character) override;
};

class InputHandler {
public:
    InputHandler();
    ~InputHandler();
    void handleInput(GLFWwindow* window, int key, int scancode, int action, int mods, GameCharacter* player_char);

private:
    std::map<int, MovementCommand*> commandMap;
};

GameManager* GameManager::instance = nullptr;

GameManager::GameManager() : glfwWindow(nullptr), shaderProgram(0), texture(0), VAO(0), VBO(0), player_char(nullptr), inputHandler(nullptr) {}

GameManager::~GameManager() {
    delete player_char;
    player_char = nullptr;
    delete inputHandler;
    inputHandler = nullptr;
    glDeleteProgram(shaderProgram);
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
    if (texture != 0) glDeleteTextures(1, &texture);
}

GameManager* GameManager::getInstance() {
    if (instance == nullptr) {
        instance = new GameManager();
    }
    return instance;
}

void GameManager::initialize(GLFWwindow* window) {
    glfwWindow = window;

    if (!loadMapConfig("map.txt")) {
        std::cerr << "Erro ao carregar mapa inicial!" << std::endl;
        return;
    }

    TILE_WIDTH_SCALED = static_cast<int>(m_baseTileWidth * GAME_SCALE);
    TILE_HEIGHT_SCALED = static_cast<int>(m_baseTileHeight * GAME_SCALE);

    total_coins_on_map = 0;
    int initial_player_row = -1;
    int initial_player_col = -1;

    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            if (game_map[r][c] == TILE_MOEDA) {
                total_coins_on_map++;
            }
            if (game_map[r][c] == TILE_INICIO) {
                if (initial_player_row == -1) {
                    initial_player_row = r;
                    initial_player_col = c;
                }
            }
        }
    }
    std::cout << "Total de moedas no mapa: " << total_coins_on_map << std::endl;

    setupOpenGL();

    GLuint vertShader = compile_shader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragShader = compile_shader(fragmentShaderSource, GL_FRAGMENT_SHADER);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, fragShader);
    glAttachShader(shaderProgram, vertShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    if (!loadTexture(TILESET_PATH.c_str())) {
        std::cerr << "Falha ao carregar textura do tileset!" << std::endl;
        return;
    }

    player_char = new GameCharacter(shaderProgram,
                                    "../assets/sprites/Slime1_Idle_full.png",
                                    (float)TILE_WIDTH_SCALED, (float)TILE_HEIGHT_SCALED * 2.0f,
                                    4, 6);
    if (initial_player_row != -1 && initial_player_col != -1) {
        player_char->setGridPosition(initial_player_row, initial_player_col);
    } else {
        std::cerr << "Nenhum tile de inicio (ID " << TILE_INICIO << ") encontrado no mapa. Personagem iniciado em (0,0)." << std::endl;
        player_char->setGridPosition(0, 0);
    }
    player_char->setAnimationFPS(10.0f);

    inputHandler = new InputHandler();

    std::cout << "Controles: W/S/A/D para mover, Q/E/Z/C para diagonais, ESC para sair. R para resetar." << std::endl;
}

void GameManager::update(float deltaTime) {
    if (player_char) {
        player_char->update(deltaTime);
    }
}

void GameManager::render() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    if (game_won && !effect_applied) {
        for (int r = 0; r < MAP_ROWS; ++r) {
            for (int c = 0; c < MAP_COLS; ++c) {
                game_map[r][c] = TILE_VICTORY_EFFECT_TILE_ID;
            }
        }
        effect_applied = true;
    } else if (game_over && game_ended_by_lava && !effect_applied) {
        for (int r = 0; r < MAP_ROWS; ++r) {
            for (int c = 0; c < MAP_COLS; ++c) {
                game_map[r][c] = TILE_LAVA;
            }
        }
        effect_applied = true;
    }

    renderMap();

    if (player_char) {
        glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, 0.0f, -1.0f, 1.0f);
        player_char->draw(projection, &GameManager::gridToIsometric);
    }

    glfwSwapBuffers(glfwWindow);
}

void GameManager::resetGame() {
    items_collected = 0;
    game_over = false;
    game_won = false;
    game_ended_by_lava = false;
    effect_applied = false;

    if (!loadMapConfig("map.txt")) {
        std::cerr << "Erro ao recarregar o mapa durante o reset!" << std::endl;
        return;
    }

    TILE_WIDTH_SCALED = static_cast<int>(m_baseTileWidth * GAME_SCALE);
    TILE_HEIGHT_SCALED = static_cast<int>(m_baseTileHeight * GAME_SCALE);

    total_coins_on_map = 0;
    int initial_player_row = -1;
    int initial_player_col = -1;

    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            if (game_map[r][c] == TILE_MOEDA) {
                total_coins_on_map++;
            }
            if (game_map[r][c] == TILE_INICIO) {
                if (initial_player_row == -1) {
                    initial_player_row = r;
                    initial_player_col = c;
                }
            }
        }
    }
    std::cout << "Total de moedas no mapa (reset): " << total_coins_on_map << std::endl;

    if (player_char) {
        if (initial_player_row != -1 && initial_player_col != -1) {
            player_char->setGridPosition(initial_player_row, initial_player_col);
        } else {
            player_char->setGridPosition(0, 0);
        }
        player_char->setAnimationType(AnimationType::IDLE_FRONT);
    }
    std::cout << "Jogo resetado!" << std::endl;
}

void GameManager::handleKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (inputHandler && player_char) {
        inputHandler->handleInput(window, key, scancode, action, mods, player_char);
    }
}

void GameManager::processPlayerMovement(int new_row, int new_col) {
    if (player_char) {
        if (new_row >= 0 && new_row < MAP_ROWS && new_col >= 0 && new_col < MAP_COLS) {
            int target_tile_id = game_map[new_row][new_col];

            if (target_tile_id == TILE_PAREDE) {
                std::cout << "Tile (" << new_col << ", " << new_row << ") não é caminhável (Parede)." << std::endl;
                return;
            }
            if (target_tile_id == TILE_AGUA) {
                std::cout << "Tile (" << new_col << ", " << new_row << ") não é caminhável (Água)." << std::endl;
                return;
            }
            if (target_tile_id == TILE_LAVA) {
                game_over = true;
                game_ended_by_lava = true;
                std::cout << "Você morreu na lava! Fim de jogo." << std::endl;
                return;
            }

            player_char->setGridPosition(new_row, new_col);
            std::cout << "Player movido para (" << player_char->col << ", " << player_char->row << ")" << std::endl;

            if (target_tile_id == TILE_MOEDA) {
                items_collected++;
                game_map[player_char->row][player_char->col] = TILE_CHAO;
                std::cout << "Moeda coletada! Total: " << items_collected << std::endl;

                if (items_collected == total_coins_on_map) {
                    game_won = true;
                    std::cout << "Parabens! Voce coletou todas as moedas e venceu o jogo!" << std::endl;
                }
            }
        }
    }
}

int GameManager::getTileId(int r, int c) const {
    if (r >= 0 && r < MAP_ROWS && c >= 0 && c < MAP_COLS) {
        return game_map[r][c];
    }
    return -1;
}

void GameManager::setPlayerAnimation(AnimationType type) {
    if (player_char) {
        player_char->setAnimationType(type);
    }
}

bool GameManager::loadTexture(const char* path) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 4) format = GL_RGBA;
        else if (nrChannels == 3) format = GL_RGB;
        else {
            std::cerr << "Formato de imagem nao suportado para textura: " << nrChannels << " canais." << std::endl;
            stbi_image_free(data);
            return false;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        std::cout << "Textura carregada: " << path << " (Width: " << width << ", Height: " << height << ", Channels: " << nrChannels << ")" << std::endl;
        stbi_image_free(data);
        return true;
    } else {
        std::cerr << "Falha ao carregar textura: " << path << std::endl;
        return false;
    }
}

bool GameManager::loadMapConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << filename << std::endl;
        return false;
    }

    std::string line;
    std::string temp_tileset_path;
    int temp_tileset_cols, temp_tileset_rows, temp_tile_width, temp_tile_height;
    int temp_map_rows, temp_map_cols;

    std::getline(file, line);
    std::istringstream iss_tileset(line);
    std::string key_tileset;
    iss_tileset >> key_tileset >> temp_tileset_path >> temp_tileset_cols >> temp_tileset_rows >> temp_tile_width >> temp_tile_height;

    std::getline(file, line);
    std::istringstream iss_dim(line);
    std::string key_dim;
    iss_dim >> key_dim >> temp_map_rows >> temp_map_cols;

    m_baseTileWidth = temp_tile_width;
    m_baseTileHeight = temp_tile_height;

    TILESET_COLS = temp_tileset_cols;
    TILESET_ROWS = temp_tileset_rows;
    TILESET_PATH = temp_tileset_path;
    MAP_ROWS = temp_map_rows;
    MAP_COLS = temp_map_cols;

    std::getline(file, line);

    game_map.clear();
    game_map.resize(MAP_ROWS, std::vector<int>(MAP_COLS));

    for (int r = 0; r < MAP_ROWS; ++r) {
        std::string row_str;
        if (!std::getline(file, row_str)) {
            std::cerr << "Dados do mapa incompletos durante recarregamento." << std::endl;
            file.close();
            return false;
        }
        size_t first = row_str.find_first_not_of(" \t\n\r");
        size_t last = row_str.find_last_not_of(" \t\n\r");
        if (std::string::npos != first) {
            row_str = row_str.substr(first, (last - first + 1));
        } else {
            row_str = "";
        }

        if ((int)row_str.length() != MAP_COLS) {
            std::cerr << "Largura da linha do mapa incorreta durante recarregamento. Esperado " << MAP_COLS << ", obtido " << row_str.length() << " na linha: \"" << row_str << "\"" << std::endl;
            file.close();
            return false;
        }
        for (int c = 0; c < MAP_COLS; ++c) {
            game_map[r][c] = row_str[c] - '0';
        }
    }
    file.close();
    std::cout << "Configuração do mapa carregada." << std::endl;
    return true;
}

glm::vec2 GameManager::gridToIsometric(int col, int row) {
    float isoX_raw = (col - row) * (GameManager::getInstance()->getTileWidth() / 2.0f);
    float isoY_raw = (col + row) * (GameManager::getInstance()->getTileHeight() / 2.0f);

    float min_iso_x_map = -(GameManager::getInstance()->getMapRows() - 1) * (GameManager::getInstance()->getTileWidth() / 2.0f);
    float max_iso_x_map = (GameManager::getInstance()->getMapCols() - 1) * (GameManager::getInstance()->getTileWidth() / 2.0f);
    float min_iso_y_map = 0.0f;
    float max_iso_y_map = (GameManager::getInstance()->getMapCols() + GameManager::getInstance()->getMapRows() - 2) * (GameManager::getInstance()->getTileHeight() / 2.0f) + GameManager::getInstance()->getTileHeight();

    float map_visual_width = max_iso_x_map - min_iso_x_map;
    float map_visual_height = max_iso_y_map - min_iso_y_map;

    float map_center_x = min_iso_x_map + map_visual_width / 2.0f;
    float map_center_y = min_iso_y_map + map_visual_height / 2.0f;

    float globalOffsetX = (GameManager::getInstance()->SCR_WIDTH / 2.0f) - map_center_x;
    float globalOffsetY = (GameManager::getInstance()->SCR_HEIGHT / 2.0f) - map_center_y;

    float isoX = isoX_raw + globalOffsetX;
    float isoY = isoY_raw + globalOffsetY;

    return glm::vec2(isoX, isoY);
}

void GameManager::renderMap() {
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
    int spriteUVsLoc = glGetUniformLocation(shaderProgram, "spriteUVs");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "basic_texture"), 0);

    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            int tileId = game_map[r][c];
            glm::vec2 pos = gridToIsometric(c, r);
            float u_min = (float)(tileId % TILESET_COLS) / TILESET_COLS;
            float v_min = (float)(tileId / TILESET_COLS) / TILESET_ROWS;
            float u_max = (float)(tileId % TILESET_COLS + 1) / TILESET_COLS;
            float v_max = (float)(tileId / TILESET_COLS + 1) / TILESET_ROWS;

            glUniform4f(spriteUVsLoc, u_min, v_min, u_max, v_max);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(pos.x - TILE_WIDTH_SCALED / 2.0f, pos.y - TILE_HEIGHT_SCALED, 0.0f));
            model = glm::scale(model, glm::vec3((float)TILE_WIDTH_SCALED, (float)TILE_HEIGHT_SCALED, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
}


GameCharacter::GameCharacter(unsigned int sharedShaderProgram, const std::string& texturePath, float spriteDisplayWidth, float spriteDisplayHeight, int totalRows, int totalCols) :
    shaderProgram(sharedShaderProgram), displayScale(spriteDisplayWidth, spriteDisplayHeight), rotation(0.0f), totalAnimationRows(totalRows), totalAnimationCols(totalCols), currentFrame(0), animationFPS(10.0f), currentAnimationType(AnimationType::IDLE_FRONT), row(0), col(0) {
    loadTexture(texturePath.c_str(), &textureID);
    setupMesh();
    calculateCurrentFrameUVs();
    lastFrameTime = glfwGetTime();
}

GameCharacter::~GameCharacter() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &textureID);
}

void GameCharacter::update(float deltaTime) {
    double currentTime = glfwGetTime();
    if (currentTime - lastFrameTime >= 1.0 / animationFPS) {
        currentFrame = (currentFrame + 1) % totalAnimationCols;
        calculateCurrentFrameUVs();
        lastFrameTime = currentTime;
    }
}

void GameCharacter::setGridPosition(int r, int c) {
    row = r;
    col = c;
}

void GameCharacter::draw(const glm::mat4& projection, glm::vec2 (*gridToIsometricFunc)(int, int)) {
    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(shaderProgram, "basic_texture"), 0);

    glUniform4f(glGetUniformLocation(shaderProgram, "spriteUVs"),
                currentFrameUVs.x, currentFrameUVs.y, currentFrameUVs.z, currentFrameUVs.w);

    glm::vec2 screen_pos = gridToIsometricFunc(col, row);

    glm::mat4 model = glm::mat4(1.0f);
    float adjustedX = screen_pos.x - (displayScale.x / 2.0f) + (GameManager::getInstance()->getTileWidth() / 2.0f);
    float adjustedY = screen_pos.y - displayScale.y + (GameManager::getInstance()->getTileHeight() / 2.0f) + GameManager::getInstance()->getTileHeight();


    model = glm::translate(model, glm::vec3(adjustedX, adjustedY, 0.02f));
    model = glm::translate(model, glm::vec3(0.5f * displayScale.x, 0.5f * displayScale.y, 0.0f));
    model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(-0.5f * displayScale.x, -0.5f * displayScale.y, 0.0f));
    model = glm::scale(model, glm::vec3(displayScale, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void GameCharacter::setAnimationFPS(float fps) {
    animationFPS = fps;
}

void GameCharacter::setAnimationType(AnimationType type) {
    if (currentAnimationType != type) {
        currentAnimationType = type;
        currentFrame = 0;
        lastFrameTime = glfwGetTime();
    }
}

AnimationType GameCharacter::getAnimationType() const {
    return currentAnimationType;
}

bool GameCharacter::loadTexture(const char* file_name, GLuint* tex_id) {
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
    if (max_aniso > 0.0f) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image_data);
    std::cout << "Textura carregada com sucesso: " << file_name << std::endl;
    return true;
}

void GameCharacter::setupMesh() {
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

void GameCharacter::calculateCurrentFrameUVs() {
    float frameWidthNormalized = 1.0f / totalAnimationCols;
    float frameHeightNormalized = 1.0f / totalAnimationRows;

    float u_min = currentFrame * frameWidthNormalized;
    float u_max = u_min + frameWidthNormalized;

    float v_min = static_cast<int>(currentAnimationType) * frameHeightNormalized;
    float v_max = v_min + frameHeightNormalized;

    currentFrameUVs = glm::vec4(u_min, v_min, u_max, v_max);
}


void MoveUpCommand::execute(GameCharacter* character) {
    GameManager::getInstance()->processPlayerMovement(character->row - 1, character->col);
    GameManager::getInstance()->setPlayerAnimation(AnimationType::IDLE_BACK);
}

void MoveDownCommand::execute(GameCharacter* character) {
    GameManager::getInstance()->processPlayerMovement(character->row + 1, character->col);
    GameManager::getInstance()->setPlayerAnimation(AnimationType::IDLE_FRONT);
}

void MoveLeftCommand::execute(GameCharacter* character) {
    GameManager::getInstance()->processPlayerMovement(character->row, character->col - 1);
    GameManager::getInstance()->setPlayerAnimation(AnimationType::IDLE_LEFT);
}

void MoveRightCommand::execute(GameCharacter* character) {
    GameManager::getInstance()->processPlayerMovement(character->row, character->col + 1);
    GameManager::getInstance()->setPlayerAnimation(AnimationType::IDLE_RIGHT);
}

void MoveUpLeftCommand::execute(GameCharacter* character) {
    GameManager::getInstance()->processPlayerMovement(character->row - 1, character->col - 1);
    GameManager::getInstance()->setPlayerAnimation(AnimationType::IDLE_LEFT);
}

void MoveUpRightCommand::execute(GameCharacter* character) {
    GameManager::getInstance()->processPlayerMovement(character->row - 1, character->col + 1);
    GameManager::getInstance()->setPlayerAnimation(AnimationType::IDLE_RIGHT);
}

void MoveDownLeftCommand::execute(GameCharacter* character) {
    GameManager::getInstance()->processPlayerMovement(character->row + 1, character->col - 1);
    GameManager::getInstance()->setPlayerAnimation(AnimationType::IDLE_LEFT);
}

void MoveDownRightCommand::execute(GameCharacter* character) {
    GameManager::getInstance()->processPlayerMovement(character->row + 1, character->col + 1);
    GameManager::getInstance()->setPlayerAnimation(AnimationType::IDLE_RIGHT);
}

void ResetGameCommand::execute(GameCharacter* character) {
    GameManager::getInstance()->resetGame();
}

InputHandler::InputHandler() {
    commandMap[GLFW_KEY_W] = new MoveUpCommand();
    commandMap[GLFW_KEY_S] = new MoveDownCommand();
    commandMap[GLFW_KEY_A] = new MoveLeftCommand();
    commandMap[GLFW_KEY_D] = new MoveRightCommand();
    commandMap[GLFW_KEY_Q] = new MoveUpLeftCommand();
    commandMap[GLFW_KEY_E] = new MoveUpRightCommand();
    commandMap[GLFW_KEY_Z] = new MoveDownLeftCommand();
    commandMap[GLFW_KEY_C] = new MoveDownRightCommand();
    commandMap[GLFW_KEY_R] = new ResetGameCommand();
}

InputHandler::~InputHandler() {
    for (auto const& [key, val] : commandMap) {
        delete val;
    }
}

void InputHandler::handleInput(GLFWwindow* window, int key, int scancode, int action, int mods, GameCharacter* player_char) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
        return;
    }

    if (GameManager::getInstance()->isGameOver() || GameManager::getInstance()->hasGameWon()) {
        if (key == GLFW_KEY_R) { 
             if (commandMap.count(key)) {
                commandMap[key]->execute(player_char);
            }
        }
        return;
    }

    if (commandMap.count(key)) {
        commandMap[key]->execute(player_char);
    }
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    GameManager::getInstance()->handleKeyCallback(window, key, scancode, action, mods);
}

void setupOpenGL() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

GLuint compile_shader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

int main() {
    std::cout << "---- Jogo Iniciado ----" << std::endl;

    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Trabalho GB - Conrado Maia e Gabriel Figueiredo", NULL, NULL);
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

    GameManager::getInstance()->initialize(window);

    double lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentFrameTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;

        glfwPollEvents();

        GameManager::getInstance()->update(deltaTime);
        GameManager::getInstance()->render();
    }

    delete GameManager::getInstance();
    glfwTerminate();

    return 0;
}