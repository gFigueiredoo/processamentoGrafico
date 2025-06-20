#include <iostream>
#include <string>
#include <vector>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM (ainda incluído, mas não usaremos as matrizes na Parte 1)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;
using namespace std;

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipo da função createTriangle
GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2);

// Protótipos das funções de shader (ajustadas para serem mais simples para a Parte 1)
GLuint setupShader();

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Código fonte do Vertex Shader (sem matrizes para a Parte 1)
const GLchar *vertexShaderSource = "#version 330 core\n" // Use 330 core
                                   "layout (location = 0) in vec2 aPos;\n" // Use vec2 para 2D
                                   "void main()\n"
                                   "{\n"
                                   "gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
                                   "}\0";

// Código fonte do Fragment Shader (sem uniforms de cor, cor fixa)
const GLchar *fragmentShaderSource = "#version 330 core\n" // Use 330 core
                                     "out vec4 FragColor;\n"
                                     "void main()\n"
                                     "{\n"
                                     "FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n" // Cor fixa para todos os triângulos
                                     "}\n\0";

// Função MAIN
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Exercicios Parte 1 - Triangulos - Gabriel", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();

    // EXERCÍCIO 2: Instancie 5 triângulos na tela
    vector<GLuint> VAOs;
    VAOs.push_back(createTriangle(-0.9f, -0.9f, -0.7f, -0.9f, -0.8f, -0.7f)); // Triângulo 1
    VAOs.push_back(createTriangle(0.1f, -0.5f, 0.3f, -0.5f, 0.2f, -0.3f));    // Triângulo 2
    VAOs.push_back(createTriangle(-0.4f, 0.2f, -0.2f, 0.2f, -0.3f, 0.4f));    // Triângulo 3
    VAOs.push_back(createTriangle(0.6f, 0.6f, 0.8f, 0.6f, 0.7f, 0.8f));      // Triângulo 4
    VAOs.push_back(createTriangle(-0.1f, 0.0f, 0.1f, 0.0f, 0.0f, 0.2f));      // Triângulo 5
    
    glUseProgram(shaderID);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Nova cor de fundo
        glClear(GL_COLOR_BUFFER_BIT);

        glLineWidth(10);
        glPointSize(20);

        // Desenhar cada triângulo do vetor
        for (GLuint vao : VAOs)
        {
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    // Liberar os VAOs
    for (GLuint vao : VAOs) {
        glDeleteVertexArrays(1, &vao);
    }
    glDeleteProgram(shaderID);

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

// Adaptação da função setupShader para usar os novos códigos de shader
GLuint setupShader()
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
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Implementação da função createTriangle (ajustada para vec2 no VBO)
GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2)
{
    GLuint VAO;
    GLfloat vertices[] = {
        x0, y0,
        x1, y1,
        x2, y2
    };

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0); // 2 componentes (x, y)
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}