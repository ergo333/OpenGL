// win : CL /MD /Feapp /Iinclude e04.cpp /link /LIBPATH:lib\win /NODEFAULTLIB:MSVCRTD
// osx : c++ -std=c++11 -o app e04.cpp -I ./include -L ./lib/mac -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
// lin : c++ -std=c++0x -o app e04.cpp -I ./include -L./lib/lin -Wl,-rpath,./lib/lin/ -lglfw -lGL
// l32 : c++ -std=c++0x -o app e04.cpp -I ./include -L./lib/lin32 -Wl,-rpath,./lib/lin32/ -lglfw -lGL


/**

	SOLUZIONE ESERCIZIO CON MIX DI DUE TEXTURE IN UNO SHADER

**/
#include <cstdlib>
#include <iostream>
#include <vector>

//using timer = std::chrono::high_resolution_clock;

#ifdef _WIN32
#include <GL/glew.h>
#else
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#endif
#define PI 3.1415926535897932384626433832795f
#include <GLFW/glfw3.h>
#include <lodepng.hpp>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined(_MSC_VER)
#pragma comment(lib,"user32")
#pragma comment(lib,"gdi32")
#pragma comment(lib,"opengl32")
#pragma comment(lib,"glew32")
#pragma comment(lib,"glfw3")
#endif

glm::vec3 camera_position = glm::vec3(2.5f, 2.5f, 2.5f);
bool translate_up = false;
bool mix_tex = false;
GLuint tex_flag;

static void error_callback(int error, const char* description)
{
	std::cerr << description << std::endl;
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) tex_flag=1;
	if (key == GLFW_KEY_0 && action == GLFW_PRESS) tex_flag=0;
	if (key == GLFW_KEY_2 && action == GLFW_PRESS) mix_tex = not mix_tex;
	if (key == GLFW_KEY_3 && action == GLFW_PRESS) translate_up=!translate_up 	;
}

// Shader sources
const GLchar* vertexSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec3 position;"
	"in vec3 color;"
	"in vec2 coord;"
	"out vec3 Color;"
	"out vec2 Coord;"
	"uniform mat4 model;"
	"uniform mat4 view;"
	"uniform mat4 projection;"
	"void main() {"
	"	Color = color;"
	"	Coord = coord;"
	"	gl_Position = projection * view * model * vec4(position, 1.0);"
	"}";
const GLchar* fragmentSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec3 Color;"
	"in vec2 Coord;"
	"out vec4 outColor;"
	"uniform sampler2D textureSampler;"
	"void main() {"
	"	outColor = vec4(Color, 1.0)*texture(textureSampler, Coord);"
	"}";

const GLchar* fragmentSource2 =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec3 Color;"
	"in vec2 Coord;"
	"out vec4 outColor;"
	"uniform sampler2D textureSampler;"
    	"uniform sampler2D textureSampler2;"
	"void main() {"
	" vec4 t1 = texture(textureSampler, Coord);"
	" vec4 t2 = texture(textureSampler2, Coord);"
	" outColor = vec4(Color, 1.0)*mix(t1,t2,0.5);"
	"}";


const GLfloat vertices[] = {
	//	Position				Color				Texcoords
	-0.5f,  0.5f, -0.5f,	1.0f, 1.0f, 1.0f,	.25f, 0.0f, // 0
	 0.5f,  0.5f, -0.5f,	1.0f, 1.0f, 1.0f,	0.5f, 0.0f, // 1

	-0.5f,  0.5f, -0.5f,	1.0f, 1.0f, 1.0f,	0.0f, 1.f/3.f, // 2
	-0.5f,  0.5f,  0.5f,	1.0f, 1.0f, 1.0f,	.25f, 1.f/3.f, // 3
	 0.5f,  0.5f,  0.5f,	1.0f, 1.0f, 1.0f,	0.5f, 1.f/3.f, // 4
	 0.5f,  0.5f, -0.5f,	1.0f, 1.0f, 1.0f,	.75f, 1.f/3.f, // 5
	-0.5f,  0.5f, -0.5f,	1.0f, 1.0f, 1.0f,	1.0f, 1.f/3.f, // 6

	-0.5f, -0.5f, -0.5f,	1.0f, 1.0f, 1.0f,	0.0f, 2.f/3.f, // 7
	-0.5f, -0.5f,  0.5f,	1.0f, 1.0f, 1.0f,	.25f, 2.f/3.f, // 8
	 0.5f, -0.5f,  0.5f,	1.0f, 1.0f, 1.0f,	0.5f, 2.f/3.f, // 9
	 0.5f, -0.5f, -0.5f,	1.0f, 1.0f, 1.0f,	.75f, 2.f/3.f, // 10
	-0.5f, -0.5f, -0.5f,	1.0f, 1.0f, 1.0f,	1.0f, 2.f/3.f, // 11

	-0.5f, -0.5f, -0.5f,	1.0f, 1.0f, 1.0f,	.25f, 1.0f, // 12
	 0.5f, -0.5f, -0.5f,	1.0f, 1.0f, 1.0f,	0.5f, 1.0f, // 13
};
/* 0--1
   |  |
2--3--4--5--6
|  |  |  |  |
7--8--9-10-11
   |  |
  12--13    */
const GLuint elements[] = {
	0, 3, 4,  0, 4, 1,
	2, 7, 8,  2, 8, 3,
	3, 8, 9,  3, 9, 4,
	4, 9,10,  4,10, 5,
	5,10,11,  5,11, 6,
	8,12,13,  8,13, 9
};

GLuint vao;
GLuint vbo;
GLuint ibo;
GLuint shaderProgram;
GLuint shaderProgram2;
GLuint textures[2];

float t = 0;

void check(int line)
{
	GLenum error = glGetError();
	while(error != GL_NO_ERROR)
	{
		switch(error)
		{
			case GL_INVALID_ENUM: std::cout << "GL_INVALID_ENUM : " << line << std::endl; break;
			case GL_INVALID_VALUE: std::cout << "GL_INVALID_VALUE : " << line << std::endl; break;
			case GL_INVALID_OPERATION: std::cout << "GL_INVALID_OPERATION : " << line << std::endl; break;
			case GL_OUT_OF_MEMORY: std::cout << "GL_OUT_OF_MEMORY : " << line << std::endl; break;
			default: std::cout << "Unrecognized error : " << line << std::endl; break;
		}
		error = glGetError();
	}
}

void initialize_shader(){
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	GLuint fragmentShader2 = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader2, 1, &fragmentSource2, NULL);
	glCompileShader(fragmentShader2);

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);

	shaderProgram2 = glCreateProgram();
	glAttachShader(shaderProgram2, vertexShader);
	glAttachShader(shaderProgram2, fragmentShader2);
	glBindFragDataLocation(shaderProgram2, 0, "outColor");
	glLinkProgram(shaderProgram2);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteShader(fragmentShader2);
}

void destroy_shader(){
	glDeleteProgram(shaderProgram);
}

void initialize_vao(){

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	//shaderProgram must be already initialized

	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);

	GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*) (3 * sizeof(GLfloat)));

	GLint cooAttrib = glGetAttribLocation(shaderProgram, "coord");
	glEnableVertexAttribArray(cooAttrib);
	glVertexAttribPointer(cooAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*) (6 * sizeof(GLfloat)));
}

void destroy_vao(){
	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

void initialize_texture(){

	glGenTextures(2, &textures[0]);

	std::vector<unsigned char> image;
	std::vector<unsigned char> image2;
	unsigned width, height;

	unsigned error = lodepng::decode(image, width, height, "cube.png");
	if(error) 
		std::cout << "decode error " << error << ": " << lodepng_error_text(error) << std::endl;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	unsigned error2 = lodepng::decode(image2, width, height, "cube2.png");

	if(error) 
		std::cout << "decode error " << error << ": " << lodepng_error_text(error) << std::endl;

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image2.data());

	// shaderProgram must be already initialized
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void destroy_texture(){
	glDeleteTextures(2, &textures[0]);
}

void update_camera(){
	if (translate_up)
		camera_position += glm::vec3(0.0f, 0.05f, 0.0f);
}

void draw(GLFWwindow* window){

	update_camera();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (mix_tex){
		glUseProgram(shaderProgram2);
		glUniform1i(glGetUniformLocation(shaderProgram2, "textureSampler"), 0);
		glUniform1i(glGetUniformLocation(shaderProgram2, "textureSampler2"), 1);
	}else{
		glUseProgram(shaderProgram);
		glUniform1i(glGetUniformLocation(shaderProgram, "textureSampler"), tex_flag);
	}


	glm::mat4 view = glm::lookAt(camera_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(.0f, 1.0f, .0f));

	glm::mat4 projection = glm::perspective(PI/4, 1.f/1.f, 1.0f, 30.0f);

	glm::mat4 model = glm::rotate(glm::mat4(1.f), PI, glm::vec3(0.0f, 1.0f, 0.0f));

	if (!mix_tex){
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1 ,GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
	}else{
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "view"), 1 ,GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "model"), 1, GL_FALSE, &model[0][0]);
	}
	

	glBindVertexArray(vao);

	glDrawElements(GL_TRIANGLES, 6 * 2 * 3, GL_UNSIGNED_INT, 0);   //facce * triangoli per faccia * vertici per triangolo

}

int main(int argc, char const *argv[])
{
	GLFWwindow* window;

	glfwSetErrorCallback(error_callback);

	tex_flag=0;

	if(!glfwInit())
		return EXIT_FAILURE;

#if defined(__APPLE_CC__)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#endif
	window = glfwCreateWindow(800, 800, "cg-lab", NULL, NULL);
	if(!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);

#if defined(_MSC_VER)
	glewExperimental = true;
	if (glewInit() != GL_NO_ERROR)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
#endif

	glfwSetKeyCallback(window, key_callback);

	initialize_shader(); check(__LINE__);
	initialize_vao(); check(__LINE__);
	initialize_texture(); check(__LINE__);

	//start = timer::now();
	glEnable(GL_DEPTH_TEST); check(__LINE__);
	while(!glfwWindowShouldClose(window))
	{
		draw(window); check(__LINE__);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	destroy_vao(); check(__LINE__);
	destroy_shader(); check(__LINE__);
	destroy_texture(); check(__LINE__);

	glfwDestroyWindow(window);

	glfwTerminate();
	return EXIT_SUCCESS;
}