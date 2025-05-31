/* Atividade Vivencial 1 - Processamento Gráfico
 * Nomes: Conrado Maia e Gabriel Figueiredo
*/

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <ctime>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções de callback do mouse
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// Protótipos das funções
int setupShader();
int setupGeometry();
void createTriangle();

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = R"(
    #version 450 core
    layout (location = 0) in vec3 position;
    
    uniform mat4 model;
    uniform mat4 projection;
    
    void main()
    {
        gl_Position = projection * model * vec4(position, 1.0);
    }
)";

// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = R"(
    #version 450 core
    uniform vec4 inputColor;
    out vec4 color;
    
    void main()
    {
        color = inputColor;
    }
)";

// Estrutura para armazenar os vértices de um triângulo
struct Triangle {
    std::vector<glm::vec3> vertices;
    glm::vec3 color;
    GLuint VAO;
};

// Variáveis globais
std::vector<glm::vec3> vertices;  // Armazena os vértices temporários
std::vector<Triangle> triangles;  // Armazena os triângulos criados
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0.0, 1.0);

// Função MAIN
int main()
{
    // Inicialização da GLFW
    glfwInit();

    // Criação da janela GLFW
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Atividade Vivencial 1 - Triângulos", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Fazendo o registro da função de callback para a janela GLFW
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // GLAD: carrega todos os ponteiros de funções da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Obtendo as informações de versão
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    // Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Compilando e buildando o programa de shader
    GLuint shaderID = setupShader();

    // Inicializar o gerador de números aleatórios
    srand(time(0));

    // Matriz de projeção paralela ortográfica
    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);

    // Enviando a matriz de projeção para o shader
    glUseProgram(shaderID);
    GLint projLoc = glGetUniformLocation(shaderID, "projection");
    glUniformMatrix4fv(projLoc, 1, false, glm::value_ptr(projection));

    // Instruções para o usuário
    cout << "Clique na tela para adicionar vértices. A cada 3 vértices, um triângulo será criado." << endl;
    cout << "Pressione ESC para sair." << endl;

    // Loop da aplicação - "game loop"
    while (!glfwWindowShouldClose(window))
    {
        // Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
        glfwPollEvents();

        // Limpa o buffer de cor
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
        glClear(GL_COLOR_BUFFER_BIT);

        // Desenha os triângulos
        for (const auto& triangle : triangles)
        {
            glUseProgram(shaderID);
            
            // Matriz de modelo: transformações no objeto
            glm::mat4 model = glm::mat4(1.0f); // Matriz identidade
            GLint modelLoc = glGetUniformLocation(shaderID, "model");
            glUniformMatrix4fv(modelLoc, 1, false, glm::value_ptr(model));
            
            // Enviando a cor para o fragment shader
            GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");
            glUniform4f(colorLoc, triangle.color.r, triangle.color.g, triangle.color.b, 1.0f);
            
            // Conecta o VAO do triângulo
            glBindVertexArray(triangle.VAO);
            
            // Desenha o triângulo
            glDrawArrays(GL_TRIANGLES, 0, 3);
            
            // Desconecta o VAO
            glBindVertexArray(0);
        }

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }

    // Limpa os VAOs e VBOs
    for (auto& triangle : triangles)
    {
        glDeleteVertexArrays(1, &triangle.VAO);
    }

    // Finaliza a execução da GLFW, limpando os recursos alocados por ela
    glfwTerminate();
    return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

// Função de callback do mouse - é chamada sempre que um botão do mouse for pressionado
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Obtém a posição do cursor
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        // Adiciona o vértice à lista
        vertices.push_back(glm::vec3(xpos, ypos, 0.0f));
        
        cout << "Vértice adicionado: (" << xpos << ", " << ypos << ")" << endl;
        
        // Se tivermos 3 vértices, cria um triângulo
        if (vertices.size() == 3)
        {
            createTriangle();
            vertices.clear(); // Limpa os vértices para o próximo triângulo
        }
    }
}

// Esta função está bastante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no início deste arquivo
// A função retorna o identificador do programa de shader
int setupShader()
{
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Checando erros de compilação (exibição via log no terminal)
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Checando erros de compilação (exibição via log no terminal)
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Linkando os shaders e criando o identificador do programa de shader
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Checando por erros de linkagem
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    // Deletando os shaders, pois eles já foram incorporados ao programa
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

// Função para criar um triângulo a partir dos vértices atuais
void createTriangle()
{
    Triangle triangle;
    
    // Copia os vértices
    triangle.vertices = vertices;
    
    // Gera uma cor aleatória
    triangle.color = glm::vec3(dis(gen), dis(gen), dis(gen));
    
    // Cria o VAO e VBO para o triângulo
    GLuint VBO;
    glGenVertexArrays(1, &triangle.VAO);
    glGenBuffers(1, &VBO);
    
    // Vincula o VAO
    glBindVertexArray(triangle.VAO);
    
    // Vincula o VBO e envia os dados
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(glm::vec3), triangle.vertices.data(), GL_STATIC_DRAW);
    
    // Configura os atributos do vértice
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Desvincula o VBO e VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Adiciona o triângulo à lista
    triangles.push_back(triangle);
    
    cout << "Triângulo criado com cor: (" 
         << triangle.color.r << ", " 
         << triangle.color.g << ", " 
         << triangle.color.b << ")" << endl;
}