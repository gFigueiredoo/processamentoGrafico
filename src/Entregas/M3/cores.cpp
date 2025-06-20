#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <algorithm>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

#include <cmath>
#include <ctime>

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);

GLuint createQuad();
int setupShader();
void eliminarSimilares(float tolerancia);
void reiniciarJogo();

const GLuint WIDTH = 800, HEIGHT = 600;
const GLuint ROWS = 6, COLS = 8;
const GLuint QUAD_WIDTH = 100, QUAD_HEIGHT = 100;
const float dMax = sqrt(3.0);

const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
uniform mat4 projection;
uniform mat4 model;
void main()	
{
	gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
}
)";

const GLchar *fragmentShaderSource = R"(
#version 400
uniform vec4 inputColor;
out vec4 color;
void main()
{
	color = inputColor;
}
)";

struct Quad
{
	vec3 position;
	vec3 dimensions;
	vec3 color;
	bool eliminated;
};

Quad grid[ROWS][COLS];
int iSelected = -1;
int pontuacao = 0;
int tentativas = 0;
int pontosPorQuad = 10;

int main()
{
	srand(time(0));

	glfwInit();

	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "M3 - Jogo das cores - Conrado e Gabriel Figueiredo", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

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
	GLuint VAO = createQuad();

	reiniciarJogo();

	glUseProgram(shaderID);

	GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");
	mat4 projection = ortho(0.0, 800.0, 600.0, 0.0, -1.0, 1.0);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	cout << "=== JOGO DAS CORES ===" << endl;
	cout << "Clique nos quadrados para eliminar cores similares!" << endl;
	cout << "Pressione 'R' para reiniciar o jogo" << endl;
	cout << "Pressione 'ESC' para sair" << endl;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		glBindVertexArray(VAO);

		if (iSelected > -1)
		{
			eliminarSimilares(0.2);
		}

		for (int i = 0; i < ROWS; i++)
		{
			for (int j = 0; j < COLS; j++)
			{
				if (!grid[i][j].eliminated)
				{
					mat4 model = mat4(1);
					model = translate(model, grid[i][j].position);
					model = scale(model, grid[i][j].dimensions);
					glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
					glUniform4f(colorLoc, grid[i][j].color.r, grid[i][j].color.g, grid[i][j].color.b, 1.0f);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
				}
			}
		}

		glBindVertexArray(0);
		glfwSwapBuffers(window);
	}

	cout << "\\n=== JOGO FINALIZADO ===" << endl;
	cout << "Pontuacao final: " << pontuacao << endl;
	cout << "Tentativas: " << tentativas << endl;

	glfwTerminate();
	return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		reiniciarJogo();
		cout << "Jogo reiniciado!" << endl;
	}
}

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
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\\n"
				  << infoLog << std::endl;
	}
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\\n"
				  << infoLog << std::endl;
	}
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		int x = xpos / QUAD_WIDTH;
		int y = ypos / QUAD_HEIGHT;

		if (x >= 0 && x < COLS && y >= 0 && y < ROWS && !grid[y][x].eliminated)
		{
			iSelected = x + y * COLS;
		}
	}
}

GLuint createQuad()
{
	GLuint VAO;

	GLfloat vertices[] = {
		-0.5, 0.5, 0.0,
		-0.5, -0.5, 0.0,
		0.5, 0.5, 0.0,
		0.5, -0.5, 0.0
	};

	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}

void eliminarSimilares(float tolerancia)
{
	int x = iSelected % COLS;
	int y = iSelected / COLS;
	vec3 C = grid[y][x].color;

	int eliminados = 0;

	for (int i = 0; i < ROWS; i++)
	{
		for (int j = 0; j < COLS; j++)
		{
			if (!grid[i][j].eliminated)
			{
				vec3 O = grid[i][j].color;
				float d = sqrt(pow(C.r - O.r, 2) + pow(C.g - O.g, 2) + pow(C.b - O.b, 2));
				float dd = d / dMax;
				if (dd <= tolerancia)
				{
					grid[i][j].eliminated = true;
					eliminados++;
				}
			}
		}
	}

	tentativas++;
	int pontosGanhos = eliminados * pontosPorQuad;
	pontuacao += pontosGanhos;
	pontosPorQuad = std::max(1, 10 - tentativas * 2);

	cout << "Tentativa " << tentativas << ": " << eliminados << " quadrados eliminados. ";
	cout << "Pontos ganhos: " << pontosGanhos << ". Pontuacao total: " << pontuacao << endl;

	iSelected = -1;
}

void reiniciarJogo()
{
	for (int i = 0; i < ROWS; i++)
	{
		for (int j = 0; j < COLS; j++)
		{
			Quad quad;
			vec2 ini_pos = vec2(QUAD_WIDTH / 2, QUAD_HEIGHT / 2);
			quad.position = vec3(ini_pos.x + j * QUAD_WIDTH, ini_pos.y + i * QUAD_HEIGHT, 0.0);
			quad.dimensions = vec3(QUAD_WIDTH, QUAD_HEIGHT, 1.0);
			float r, g, b;
			r = rand() % 256 / 255.0;
			g = rand() % 256 / 255.0;
			b = rand() % 256 / 255.0;
			quad.color = vec3(r, g, b);
			quad.eliminated = false;
			grid[i][j] = quad;
		}
	}

	pontuacao = 0;
	tentativas = 0;
	pontosPorQuad = 10;
	iSelected = -1;
}