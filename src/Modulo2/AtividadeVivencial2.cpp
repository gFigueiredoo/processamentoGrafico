/*
 * Hello Triangle - Versão Interativa
 * Clique para criar vértices. A cada 3, um triângulo colorido aparece!
 */

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

const GLuint WIDTH = 800, HEIGHT = 600;

// Vertex Shader
const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
void main()
{
    gl_Position = vec4(position, 1.0);
}
)";

// Fragment Shader
const GLchar *fragmentShaderSource = R"(
#version 400
uniform vec4 inputColor;
out vec4 color;
void main()
{
    color = inputColor;
}
)";

// Estrutura para triângulo
struct Triangulo {
    GLfloat vertices[9]; // 3 vértices (x, y, z)
    GLfloat cor[4];      // RGBA
};

vector<Triangulo> triangulos;
vector<GLfloat> vertices_temp; // Armazena até 3 vértices

GLuint shaderID, VAO, VBO;
GLint colorLoc;

// Funções
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
int setupShader();
void setupGeometry();

int main()
{
    srand((unsigned int)time(0)); // Semente para cores aleatórias

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Triângulos Interativos", nullptr, nullptr);
    if (!window)
    {
        cerr << "Falha ao criar a janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    shaderID = setupShader();
    setupGeometry();

    colorLoc = glGetUniformLocation(shaderID, "inputColor");
    glUseProgram(shaderID);

    // Loop principal
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);

        // Desenha todos os triângulos
        for (const auto& t : triangulos) {
            // Atualiza o VBO com os vértices do triângulo atual
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(t.vertices), t.vertices, GL_STATIC_DRAW);

            // Envia a cor para o shader
            glUniform4fv(colorLoc, 1, t.cor);

            // Desenha o triângulo
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

// Callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

// Callback de mouse
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Converte para coordenadas do mundo (janela = mundo)
        float x = (float)xpos;
        float y = (float)(HEIGHT - ypos); // Inverte eixo Y

        // Converte para NDC [-1, 1]
        float ndc_x = (x / WIDTH) * 2.0f - 1.0f;
        float ndc_y = (y / HEIGHT) * 2.0f - 1.0f;

        vertices_temp.push_back(ndc_x);
        vertices_temp.push_back(ndc_y);
        vertices_temp.push_back(0.0f);

        if (vertices_temp.size() == 9) {
            Triangulo t;
            for (int i = 0; i < 9; ++i)
                t.vertices[i] = vertices_temp[i];

            // Gera cor aleatória
            t.cor[0] = static_cast<float>(rand()) / RAND_MAX;
            t.cor[1] = static_cast<float>(rand()) / RAND_MAX;
            t.cor[2] = static_cast<float>(rand()) / RAND_MAX;
            t.cor[3] = 1.0f;

            triangulos.push_back(t);
            vertices_temp.clear();
        }
    }
}

// Compila e linka os shaders
int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Cria VAO e VBO (usados para todos os triângulos)
void setupGeometry()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // O layout do vertex shader espera 3 floats por vértice
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}