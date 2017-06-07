// win : CL /MD /Feapp /Iinclude e09.cpp /link /LIBPATH:lib\win /NODEFAULTLIB:MSVCRTD
// osx : c++ -std=c++11 -o app e09.cpp -I ./include -L ./lib/mac -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
// lin : c++ -std=c++0x -o app e09.cpp -I ./include -L./lib/lin -Wl,-rpath,./lib/lin/ -lglfw -lGL
// l32 : c++ -std=c++0x -o app e09.cpp -I ./include -L./lib/lin32 -Wl,-rpath,./lib/lin32/ -lglfw -lGL

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>
#include <array>
#include <chrono>

using timer = std::chrono::high_resolution_clock;

#ifdef _WIN32
#include <GL/glew.h>
#else
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLFW/glfw3.h>
#include <lodepng.hpp>
#include <tiny_obj_loader.hpp>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined(_MSC_VER)
#pragma comment(lib,"user32")
#pragma comment(lib,"gdi32")
#pragma comment(lib,"opengl32")
#pragma comment(lib,"glew32")
#pragma comment(lib,"glfw3")
#endif


glm::vec3 camera_position(0.0f, 0.0f, 6.0f);
glm::vec3 camera_direction(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up(0.0f, 1.0f, 0.0f);
glm::vec3 flagPosition(2.0f, 3.0f, -10.0f);
glm::vec3 scaleFlag(0.5f, 0.5f, 0.5f);


bool	translate_forward=false;
bool	translate_backward=false;
bool	translate_right=false;
bool	translate_left=false;
bool	rotate_up=false;
bool	rotate_down=false;
bool	rotate_right=false;
bool	rotate_left=false;
bool	rotate_z_left=false;
bool	rotate_z_right=false;
bool    sky_on=true;
bool    sun_visible=true;
bool    earth_visible=true;
bool    moon_visible=true;
bool 	gameEnded = false;		//true -> se il gioco termina

double x_mousepos, y_mousepos;
float button_size = 0.1; // dimensione in proporzione alla finestra
void gui_button_press();

static void error_callback(int error, const char* description)
{
	std::cerr << description << std::endl;
}

// la posizione del mouse viene riscalata nell'intervallo [0, 1] per supportare anche schermi dove la posizione a schermo e' diversa dalla posizione nel frame (retina e simili)
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	x_mousepos = xpos/width;
	y_mousepos = ypos/height;
}

// richiamiamo la funzione associata alla pressione solo se la posizione del mouse e' all'interno del quadrato che disegnamo
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		if ((0 <= x_mousepos && x_mousepos <= button_size) && (0 <= y_mousepos && y_mousepos <= button_size))
		{
			gui_button_press();
		}
	}
}
// alla pressione dei tasti salviamo l'azione che dovremmo compiere nell'aggiornare la camera
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (!gameEnded){

		if(key == GLFW_KEY_W)
		{
			if(action)
				translate_forward = true;
			else
				translate_forward = false;
		}
		if(key == GLFW_KEY_S)
		{
			if(action)
				translate_backward = true;
			else
				translate_backward = false;
		}
		if(key == GLFW_KEY_D)
		{
			if(action)
				translate_right = true;
			else
				translate_right = false;
		}
		if(key == GLFW_KEY_A)
		{
			if(action)
				translate_left = true;
			else
				translate_left = false;
		}
		if(key == GLFW_KEY_UP)
		{
			if(action)
				rotate_up = true;
			else
				rotate_up = false;
		}
		if(key == GLFW_KEY_DOWN)
		{
			if(action)
				rotate_down = true;
			else
				rotate_down = false;
		}
		if(key == GLFW_KEY_RIGHT)
		{
			if(action)
				rotate_right = true;
			else
				rotate_right = false;
		}
		if(key == GLFW_KEY_LEFT)
		{
			if(action)
				rotate_left = true;
			else
				rotate_left = false;
		}

	if(key == GLFW_KEY_Q)
		{
			if(action)
				rotate_z_left = true;
			else
				rotate_z_left = false;
		}
		if(key == GLFW_KEY_E)
		{
			if(action)
				rotate_z_right = true;
			else
				rotate_z_right = false;
		}
	}


}

// Shader sources
const GLchar* vertexShipSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec3 position;"
	"in vec3 normal;"
	"in vec2 coord;"
	"out vec3 Position;"
	"out vec3 Normal;"
	"out vec2 Coord;"
	"uniform mat4 model;"
	"uniform mat4 normal_matrix;"
	"uniform mat4 view;"
	"uniform mat4 projection;"
	"void main() {"
	"	Normal = vec3(normal_matrix * vec4(normal,0.0));"
	"	Coord = vec2(coord.x, 1.0-coord.y);"
	"	gl_Position = projection * view * model * vec4(position, 1.0);"
	"	vec4 vertPos = model * vec4(position, 1.0);"
	"	Position = vec3(vertPos)/vertPos.w;"
	"}";


const GLchar* fragmentShipSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec3 Position;"
	"in vec3 Normal;"
	"in vec2 Coord;"
	"out vec4 outColor;"
	"uniform mat4 normal_matrix;"
	"uniform float shininess;"
	"uniform vec3 material_ambient;"
	"uniform vec3 material_diffuse;"
	"uniform vec3 material_specular;"
	"uniform vec3 light_direction;"
	"uniform vec3 light_intensity;"
	"uniform vec3 view_position;"
	"uniform vec3 light_position;"

	"void main() {"
	"	vec3 view_direction = normalize(view_position - Position);"
	"	vec3 light_direction2 = normalize(light_position - Position);"
	"	vec3 R = normalize(reflect(-light_direction, Normal));"
	"	vec3 R2 = normalize(reflect(-light_direction2, Normal));"
	"	outColor = vec4(material_ambient, 1);"
	"	outColor += vec4(light_intensity*material_diffuse,1) * max(dot(light_direction, Normal), 0.0);"
	"	outColor +=  vec4(light_intensity*material_specular,1) * pow(max(dot(R, view_direction), 0.0), shininess);"
	"	outColor +=  vec4(light_intensity*material_diffuse,1)  * max(dot(light_direction2, Normal), 0.0);"
	"	outColor +=  vec4(light_intensity*material_specular,1) * pow(max(dot(R2, view_direction), 0.0), shininess);"
	"}";


const GLchar* vertexSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec3 position;"
	"in vec3 normal;"
	"in vec2 coord;"
	"out vec3 Position;"
	"out vec3 Normal;"
	"out vec2 Coord;"
	"uniform mat4 model;"
	"uniform mat4 normal_matrix;"
	"uniform mat4 view;"
	"uniform mat4 projection;"
	"void main() {"
	"	Normal = vec3(normal_matrix * vec4(normal,0.0));"
	"	Coord = vec2(coord.x, 1.0-coord.y);"
	"	gl_Position = projection * view * model * vec4(position, 1.0);"
	"	vec4 vertPos = model * vec4(position, 1.0);"
	"	Position = vec3(vertPos)/vertPos.w;"
	"}";

const GLchar* fragmentSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec3 Position;"
	"in vec3 Normal;"
	"in vec2 Coord;"
	"out vec4 outColor;"
	"uniform mat4 normal_matrix;"
	"uniform float shininess;"
	"uniform vec3 material_ambient;"
	"uniform vec3 material_diffuse;"
	"uniform vec3 material_specular;"
	"uniform vec3 light_direction;"
	"uniform vec3 light_intensity;"
	"uniform vec3 view_position;"
	"uniform vec3 light_position;"
	"uniform sampler2D textureSampler;"
	"void main() {"
	"	vec3 view_direction = normalize(view_position - Position);"
	"	vec3 light_direction2 = normalize(light_position - Position);"
	"	vec3 R = normalize(reflect(-light_direction, Normal));"
	"	vec3 R2 = normalize(reflect(-light_direction2, Normal));"
	"	outColor = vec4(material_ambient, 1);"
	"	outColor += vec4(light_intensity*material_diffuse,1) * texture(textureSampler, Coord) * max(dot(light_direction, Normal), 0.0);"
	"	outColor +=  vec4(light_intensity*material_specular,1) * pow(max(dot(R, view_direction), 0.0), shininess);"
	"	outColor +=  vec4(light_intensity*material_diffuse,1)  * texture(textureSampler, Coord)* max(dot(light_direction2, Normal), 0.0);"
	"	outColor +=  vec4(light_intensity*material_specular,1) * pow(max(dot(R2, view_direction), 0.0), shininess);"
	"}";

const GLchar* fragmentSource2 =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec3 Position;"
	"in vec3 Normal;"
	"in vec2 Coord;"
	"out vec4 outColor;"
	"uniform float shininess;"
	"uniform vec3 material_ambient;"
	"uniform vec3 material_diffuse;"
	"uniform vec3 material_specular;"
	"uniform vec3 light_direction;"
	"uniform vec3 light_intensity;"
	"uniform vec3 view_position;"	
	"uniform vec3 light_position;"

	"void main() {"

	"	vec3 light_direction2 = normalize(light_position - Position);"
		"	outColor = vec4(light_intensity*material_diffuse,1) *max(dot(light_direction, normalize(Normal)), 0.0);"
		"	outColor +=  vec4(light_intensity*material_diffuse,1)  * max(dot(light_direction2, normalize(Normal)), 0.0);"
	"}";

// le posizioni passate formano gia' un cubo compreso tra [-1,-1,-1] e [1,1,1], l'intervallo cioe' del clip-space
// non dobbiamo quindi applicare nessuna trasformazione
// moltiplicando i vertici per l'inversa della view_matrix otteniamo un versore che punta lungo proiezione sferica del cubo, ed e' esattamente
// il tipo di coordinata richiesta per le texture di tipo cubemap
const GLchar* vertexSkySource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"uniform mat3 view_rot;"
	"in vec3 position;"
	"out vec3 Coord;"
	"void main() {"
	"	Coord = view_rot * position;"
	"	gl_Position = vec4(position, 1.0);"
	"}";

const GLchar* fragmentSkySource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec3 Coord;"
	"out vec4 outColor;"
	"uniform samplerCube textureSampler;"
	"void main() {"
	"	outColor =  texture(textureSampler, Coord);"
	"}";


// visto che passiamo un quadrato con le coordinate gia' all'interno dell'intervallo -1,1 (clip-space) non dobbiamo applicare nessuna trasformazione
const GLchar* vertexGuiSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec2 position;"
	"in vec2 coord;"
	"out vec2 Coord;"
	"uniform float size;"
	"void main() {"
	"	Coord = vec2(coord.x, 1.0-coord.y);"
	"	gl_Position = vec4(position, 0.0, 1.0);"
	"}";

// shader per il rendering del quadrato con la texture per il pulsante
const GLchar* fragmentGuiSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec2 Coord;"
	"out vec4 outColor;"
	 "uniform sampler2D textureSampler;"
	"void main() {"
	"outColor =  texture(textureSampler, Coord);"
	"}";

const GLchar* vertexPlaneSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec2 coord;"
	"in vec3 position;"
	"out vec2 Coord;"
	"uniform mat4 model;"
	"uniform mat4 view;"
	"uniform mat4 projection;"
	"void main() {"
	"	Coord = coord;"
	"	gl_Position = projection * view * model * vec4(position, 1.0);"
	"}";
const GLchar* fragmentPlaneSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec2 Coord;"
	"out vec4 outColor;"
	"uniform sampler2D textureSampler;"
	"void main() {"
	" outColor = vec4(1.0)*texture(textureSampler, Coord);"	
	"}";

const GLchar* vertexFlagSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec3 position;"
	"uniform mat4 model;"
	"uniform mat4 view;"
	"uniform mat4 projection;"
	"void main() {"
	"	gl_Position = projection * view * model * vec4(position, 1.0);"
	"}";

const GLchar* fragmentFlagSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"out vec4 outColor;"
	"uniform float time;"
	"void main() {"
	"   outColor = vec4(abs(sin(time)), abs(3*sin(time)), abs(4*sin(time)), 1.0);"	
	"}";


const GLfloat vertices[] = {
//	Position			
	-1.0f, -1.0f,  1.0f, // 0
	 1.0f, -1.0f,  1.0f, // 1
	-1.0f,  1.0f,  1.0f, // 2
	 1.0f,  1.0f,  1.0f, // 3

	 1.0f, -1.0f, -1.0f, // 4
	-1.0f, -1.0f, -1.0f, // 5
	 1.0f,  1.0f, -1.0f, // 6
	-1.0f,  1.0f, -1.0f  // 7
};

const GLuint elements[] = {
	0, 2, 3,  0, 3, 1,
	1, 3, 6,  1, 6, 4,
	2, 7, 3,  2, 6, 3,
	5, 7, 2,  5, 2, 0,
	0, 1, 5,  5, 1, 4,
	4, 6, 5,  5, 6, 7
};

const GLfloat guiVertices[] = {
	-1.0f,                  1.0f-2.0f*button_size, 0.0f, 0.0f,
	-1.0f+2.0f*button_size, 1.0f-2.0f*button_size, 1.0f, 0.0f,
	-1.0f+2.0f*button_size, 1.0f,                  1.0f, 1.0f,
	-1.0f,                  1.0f,                  0.0f, 1.0f,
};

const GLuint guiElements[] = {
	0, 1, 3,
	1, 2, 3
};

// Aggiunta #3: piano
const GLfloat planeVertices[] = {
	-1.0f, 1.0f, 0.0f, 0.0f,   0.0f,
	1.0f,  1.0f, 0.0f, 10.0f,  0.0f,
	1.0f, -1.0f, 0.0f, 10.0f,  10.0f,
	-1.0f,-1.0f, 0.0f, 0.0f,   10.0f,
};

const GLuint planeElements[] = {
	0, 1, 3,
	1, 2, 3
};


// Ho cinque oggetti! sette
GLuint vao[7];
GLuint vbo[7];
GLuint ibo[7];

// ho tre modelli caricati da file
tinyobj::material_t material[7];
GLsizei element_count[7];

GLuint shaderProgram, shaderProgram2, shipProgram, planeProgram, flagProgram;
GLuint skyProgram, guiProgram;
GLuint texture[3]; // 0 cubic.png
GLuint textureCube;
timer::time_point start_time, last_time;
float t = 0.0;
float dt = 0.0;

float plane_height = -1.0f;

void check(int line)
{
	GLenum error = glGetError();
	while(error != GL_NO_ERROR)
	{
		switch(error)
		{ 
			case GL_INVALID_ENUM: std::cerr << "GL_INVALID_ENUM : " << line << std::endl; break;
			case GL_INVALID_VALUE: std::cerr << "GL_INVALID_VALUE : " << line << std::endl; break;
			case GL_INVALID_OPERATION: std::cerr << "GL_INVALID_OPERATION : " << line << std::endl; break;
			case GL_OUT_OF_MEMORY: std::cerr << "GL_OUT_OF_MEMORY : " << line << std::endl; break;
			default: std::cerr << "Unrecognized error : " << line << std::endl; break;
		}
		error = glGetError();
	}
}

void initialize_shader()
{
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	{
		GLchar log[512];
		GLsizei slen = 0;
		glGetShaderInfoLog(vertexShader, 512, &slen, log);
		if(slen)
			std::cerr << log << std::endl;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	{
		GLchar log[512];
		GLsizei slen = 0;
		glGetShaderInfoLog(fragmentShader, 512, &slen, log);
		if(slen)
			std::cerr << log << std::endl;
	}

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	GLint status = 0;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
	if(!status)
	{
		std::cout << "failed to link" << std::endl;
		GLchar log[256];
		GLsizei slen = 0;
		glGetProgramInfoLog(shaderProgram, 256, &slen, log);
		std::cout << log << std::endl;
	}

	glDeleteShader(fragmentShader);

	// Secondo shader	
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource2, NULL);
	glCompileShader(fragmentShader);
	
	shaderProgram2 = glCreateProgram();
	glAttachShader(shaderProgram2, vertexShader);
	glAttachShader(shaderProgram2, fragmentShader);
	glBindFragDataLocation(shaderProgram2, 0, "outColor");
	glLinkProgram(shaderProgram2);

	glDeleteShader(vertexShader);


	glDeleteShader(fragmentShader);

	// shader per la skybox
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSkySource, NULL);
	glCompileShader(vertexShader);
	{
		GLchar log[512];
		GLsizei slen = 0;
		glGetShaderInfoLog(vertexShader, 512, &slen, log);
		if(slen)
			std::cerr << log << std::endl;
	}

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSkySource, NULL);
	glCompileShader(fragmentShader);
	{
		GLchar log[512];
		GLsizei slen = 0;
		glGetShaderInfoLog(fragmentShader, 512, &slen, log);
		if(slen)
			std::cerr << log << std::endl;
	}

	skyProgram = glCreateProgram();
	glAttachShader(skyProgram, vertexShader);
	glAttachShader(skyProgram, fragmentShader);
	glBindFragDataLocation(skyProgram, 0, "outColor");
	glLinkProgram(skyProgram);
	status = 0;
	glGetProgramiv(skyProgram, GL_LINK_STATUS, &status);
	if(!status)
	{
		std::cout << "failed to link" << std::endl;
		GLchar log[256];
		GLsizei slen = 0;
		glGetProgramInfoLog(skyProgram, 256, &slen, log);
		std::cout << log << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);


	// shader per la nave
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShipSource, NULL);
	glCompileShader(vertexShader);
	{
		GLchar log[512];
		GLsizei slen = 0;
		glGetShaderInfoLog(vertexShader, 512, &slen, log);
		if(slen)
			std::cerr << log << std::endl;
	}

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShipSource, NULL);
	glCompileShader(fragmentShader);
	{
		GLchar log[512];
		GLsizei slen = 0;
		glGetShaderInfoLog(fragmentShader, 512, &slen, log);
		if(slen)
			std::cerr << log << std::endl;
	}

	shipProgram = glCreateProgram();
	glAttachShader(shipProgram, vertexShader);
	glAttachShader(shipProgram, fragmentShader);
	glBindFragDataLocation(shipProgram, 0, "outColor");
	glLinkProgram(shipProgram);
	status = 0;
	glGetProgramiv(shipProgram, GL_LINK_STATUS, &status);
	if(!status)
	{
		std::cout << "failed to link" << std::endl;
		GLchar log[256];
		GLsizei slen = 0;
		glGetProgramInfoLog(shipProgram, 256, &slen, log);
		std::cout << log << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// shader per la gui (pulsante)
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexGuiSource, NULL);
	glCompileShader(vertexShader);
	{
		GLchar log[512];
		GLsizei slen = 0;
		glGetShaderInfoLog(vertexShader, 512, &slen, log);
		if(slen)
			std::cerr << log << std::endl;
	}

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentGuiSource, NULL);
	glCompileShader(fragmentShader);
	{
		GLchar log[512];
		GLsizei slen = 0;
		glGetShaderInfoLog(fragmentShader, 512, &slen, log);
		if(slen)
			std::cerr << log << std::endl;
	}

	guiProgram = glCreateProgram();
	glAttachShader(guiProgram, vertexShader);
	glAttachShader(guiProgram, fragmentShader);
	glBindFragDataLocation(guiProgram, 0, "outColor");
	glLinkProgram(guiProgram);
	status = 0;
	glGetProgramiv(guiProgram, GL_LINK_STATUS, &status);
	if(!status)
	{
		std::cout << "failed to link" << std::endl;
		GLchar log[256];
		GLsizei slen = 0;
		glGetProgramInfoLog(guiProgram, 256, &slen, log);
		std::cout << log << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//Aggiornamento #3: Shader del piano orizzontale
	int vertexPlaneShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexPlaneShader, 1, &vertexPlaneSource, NULL);
	glCompileShader(vertexPlaneShader);
	{
		GLchar log[512];
		GLsizei slen = 0;
		glGetShaderInfoLog(vertexPlaneShader, 512, &slen, log);
		if(slen)
			std::cerr << log << std::endl;
	}
	int fragmentPlaneShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentPlaneShader, 1, &fragmentPlaneSource, NULL);
	glCompileShader(fragmentPlaneShader);
	{
		GLchar log[512];
		GLsizei slen = 0;
		glGetShaderInfoLog(fragmentPlaneShader, 512, &slen, log);
		if(slen)
			std::cerr << log << std::endl;
	}
	planeProgram = glCreateProgram();
	glAttachShader(planeProgram, vertexPlaneShader);
	glAttachShader(planeProgram, fragmentPlaneShader);
	glBindFragDataLocation(planeProgram, 0, "outColor");
	glLinkProgram(planeProgram);
	status = 0;
	glGetProgramiv(planeProgram, GL_LINK_STATUS, &status);
	if(!status)
	{
		std::cout << "failed to link" << std::endl;
		GLchar log[256];
		GLsizei slen = 0;
		glGetProgramInfoLog(planeProgram, 256, &slen, log);
		std::cout << log << std::endl;
	}
	glDeleteShader(vertexPlaneShader);
	glDeleteShader(fragmentPlaneShader);

	//Aggiunta #4 
	int vertexFlagShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexFlagShader, 1, &vertexFlagSource, NULL);
	glCompileShader(vertexFlagShader);
	{
		GLchar log[512];
		GLsizei slen = 0;
		glGetShaderInfoLog(vertexFlagShader, 512, &slen, log);
		if(slen)
			std::cerr << log << std::endl;
	}
	int fragmentFlagShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentFlagShader, 1, &fragmentFlagSource, NULL);
	glCompileShader(fragmentFlagShader);
	{
		GLchar log[512];
		GLsizei slen = 0;
		glGetShaderInfoLog(fragmentFlagShader, 512, &slen, log);
		if(slen)
			std::cerr << log << std::endl;
	}
	flagProgram = glCreateProgram();
	glAttachShader(flagProgram, vertexFlagShader);
	glAttachShader(flagProgram, fragmentFlagShader);
	glBindFragDataLocation(flagProgram, 0, "outColor");
	glLinkProgram(flagProgram);
	status = 0;
	glGetProgramiv(flagProgram, GL_LINK_STATUS, &status);
	if(!status)
	{
		std::cout << "failed to link" << std::endl;
		GLchar log[256];
		GLsizei slen = 0;
		glGetProgramInfoLog(flagProgram, 256, &slen, log);
		std::cout << log << std::endl;
	}
	glDeleteShader(vertexFlagShader);
	glDeleteShader(fragmentFlagShader);
}

void destroy_shader()
{
	glDeleteProgram(shaderProgram);
	glDeleteProgram(shaderProgram2);
	glDeleteProgram(skyProgram);
	glDeleteProgram(shipProgram);
	glDeleteProgram(guiProgram);
	glDeleteProgram(planeProgram);
}

void initialize_vao_from_file(size_t i, const std::string& inputfile, const std::string& materialPath)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err = tinyobj::LoadObj(shapes, materials, inputfile.c_str(), materialPath.c_str()); // carica il modello definito da inputfile, l'ultimo parametro e' il path in cui cercare il material
	// shapes e' un array di shape con all'interno un nome e una mesh
	// ogni mesh ha un vettore di posizioni, uno di normali e uno di coordinate texture
	// ogni mesh inoltre ha un vettore di indici e l'id dei material utilizzati
	// ogni material ha un float[3] per ambient, diffuse, specular, pmittance e emission, un float per shininess, ior (index of refraction) e dissolve (0 trasparente, 1 opaco) e un int illum (illumination model)
	// ogni material ha inoltre una std::string con il nome delle texture per ambient (ambient_texname), diffuse (diffuse_texname), specular (specular_texname) e normal (normal_texname)
	
	if (!err.empty()) {
		std::cerr << err << std::endl;
		exit(EXIT_FAILURE);
	}
	// il codice seguente presume che nel file ci sia un'unico oggetto che utilizza un solo material
	if(shapes.size() > 1 || materials.size() > 1)
	{
		std::cerr << "more than one object/material, currently not supported" << std::endl;
		exit(EXIT_FAILURE);
	}

	// ci assicuriamo che la mesh sia composta da triangoli
	assert((shapes[0].mesh.positions.size() % 3) == 0);
	assert((shapes[0].mesh.indices.size() % 3) == 0);

	glBindVertexArray(vao[i]); check(__LINE__);

	// allochiamo un buffer abbastanza grande da contenere tutti i dati della mesh e per il momento non carichiamo nessun dato (terzo parametro NULL)
	glBindBuffer(GL_ARRAY_BUFFER, vbo[i]); check(__LINE__);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(shapes[0].mesh.positions.size()+shapes[0].mesh.normals.size()+shapes[0].mesh.texcoords.size()), NULL, GL_STATIC_DRAW); check(__LINE__);
	
	// visto il modo in cui la libreria utilizzata (tiny_obj_loader) carica i dati il modo piu' semplice (anche se non ottimale) di caricare i dati dei vertici nel buffer
	// e' uno di seguito all'altro, avremo quindi prima tutte le posizioni, poi tutte le normali, poi tutte le coordinate delle texture a differenza dell'esercizio precedente
	// dove caricavamo posizione,normale e coordinate per ogni vertice
	// ppp..ppp/nnn...nnn/tt..tt vs ppp-nnn-tt/ppp-nnn-tt/.../ppp-nnn-tt
	// p = position n = normal t = texture coord
	// i parametri della glVertexAttribPointer verranno cambiati di conseguenza, il penultimo parametro sara' 0 (per ogni campo i dati sono contigui) e l'ultimo sara' l'offset dall'inizio del buffer stesso
	GLintptr offset = 0;
	if(shapes[0].mesh.positions.size())
	{
		// utilizziamo glBufferSubData per caricare i dati perche' il buffer e' gia' stato allocato (con la glBufferData precedente) e vogliamo passare solo il gruppo di dati che ci interessa
		// in questo caso vogliamo caricare i dati della posizione, offset sara' 0 e la dimensione e' data dal numero delle posizioni presenti nella mesh moltiplicata per la dimensione di un float
		glBufferSubData(GL_ARRAY_BUFFER, offset, shapes[0].mesh.positions.size()*sizeof(float), shapes[0].mesh.positions.data()); check(__LINE__);
		GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
		glEnableVertexAttribArray(posAttrib); check(__LINE__);
		glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset); check(__LINE__);
		// all'offset aggiungiamo la dimensione (in byte) di quanto abbiamo caricato nel buffer
		offset += shapes[0].mesh.positions.size()*sizeof(float);
	}
	if(shapes[0].mesh.normals.size())
	{
		// se sono presenti normali nel modello le carichiamo dopo tutte le posizioni
		glBufferSubData(GL_ARRAY_BUFFER, offset, shapes[0].mesh.normals.size()*sizeof(float), shapes[0].mesh.normals.data()); check(__LINE__);
		GLint norAttrib = glGetAttribLocation(shaderProgram, "normal"); check(__LINE__);
		glEnableVertexAttribArray(norAttrib); check(__LINE__);
		glVertexAttribPointer(norAttrib, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset); check(__LINE__);
		// e aggiorniamo di conseguenza l'offset dall'inizio del buffer
		offset += shapes[0].mesh.normals.size()*sizeof(float);
	}

	if(shapes[0].mesh.texcoords.size())
	{
		// se sono presenti anche le coordinate per le texture le carichiamo per ultime
		// offset sara' dato dalla dimensione delle posizioni e basta nel caso in cui non siano presenti le normali o dalla somma dei due nel caso in cui ci siano entrambi
		glBufferSubData(GL_ARRAY_BUFFER, offset, shapes[0].mesh.texcoords.size()*sizeof(float), shapes[0].mesh.texcoords.data()); check(__LINE__); check(__LINE__);
		GLint cooAttrib = glGetAttribLocation(shaderProgram, "coord"); check(__LINE__);
		glEnableVertexAttribArray(cooAttrib); check(__LINE__);
		glVertexAttribPointer(cooAttrib, 2, GL_FLOAT, GL_FALSE, 0, (void*)offset); check(__LINE__);
		offset += shapes[0].mesh.texcoords.size()*sizeof(float);
	}

	// salviamo il numero di indici e il material del modello per poterli utilizzare poi nella fase di draw della scena
	element_count[i] = shapes[0].mesh.indices.size();
	if(shapes[0].mesh.material_ids[0]!=-1)
		material[i] = materials[shapes[0].mesh.material_ids[0]];
	else  {
	material[i].name = "";
	for(int j=0;j<3;j++){
		material[i].ambient[j]=1;
		material[i].diffuse[j]=1;
		material[i].specular[j]=0;
		material[i].transmittance[j]=0;
		material[i].emission[j]=0;
		}
	material[i].shininess = 100;
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[i]); check(__LINE__);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*shapes[0].mesh.indices.size(), shapes[0].mesh.indices.data(), GL_STATIC_DRAW); check(__LINE__);
}

void initialize_vao()
{
	glGenVertexArrays(7, vao); check(__LINE__); // Aggiunta #3: da 5 oggetti diventano 6
	glGenBuffers(7, vbo); check(__LINE__);
	glGenBuffers(7, ibo); check(__LINE__);

	initialize_vao_from_file(0, "assets/sphere/sphere.obj", "assets/sphere/"); check(__LINE__);
//	
	initialize_vao_from_file(1, "assets/sphere/sphere_hr.obj", "assets/sphere/"); check(__LINE__);
	initialize_vao_from_file(2, "assets/ship/myShip.obj", "assets/ship/"); check(__LINE__);

	// skybox vao setup
	glBindVertexArray(vao[3]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	// shaderProgram must be already initialized
	GLint posAttrib = glGetAttribLocation(skyProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// gui vao setup
	glBindVertexArray(vao[4]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(guiVertices), guiVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[4]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(guiElements), guiElements, GL_STATIC_DRAW);

	// shaderProgram must be already initialized
	posAttrib = glGetAttribLocation(guiProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), 0);
	GLint cooAttrib = glGetAttribLocation(guiProgram, "coord");
	glEnableVertexAttribArray(cooAttrib);
	glVertexAttribPointer(cooAttrib, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (void*) GLintptr(2*sizeof(GLfloat)));

	// Aggiunta #3: piano sotto l'astronave
	glBindVertexArray(vao[5]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[5]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeElements), planeElements, GL_STATIC_DRAW);

	posAttrib = glGetAttribLocation(planeProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), 0);
	cooAttrib = glGetAttribLocation(planeProgram, "coord");
	glEnableVertexAttribArray(cooAttrib);
	glVertexAttribPointer(cooAttrib, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*) GLintptr(3*sizeof(GLfloat)));

	// Aggiunta #4:
	initialize_vao_from_file(6, "assets/bandiera/bandiera.obj", "assets/bandiera/"); check(__LINE__);
}

void destroy_vao()
{ 
	// Aggiunta #3: metto 6 oggetti invece che 5
	// Aggiunta #4: metto un altro oggetto e diventano 4
	glDeleteBuffers(7, ibo);
	glDeleteBuffers(7, vbo);

	glDeleteVertexArrays(7, vao);
}

void initialize_texture()
{
	glGenTextures(3, texture); // Aggiunta #3: agiunta texture groud
	std::vector<unsigned char> image, image2, imagePiano;
	unsigned width, height;

	// il nome della texture da caricare e' presente nel material del modello che abbiamo caricato, dobbiamo aggiungerci il path in cui andare a trovarla
	unsigned error = lodepng::decode(image, width, height, "assets/sphere/" + material[0].diffuse_texname);
	if(error) std::cout << "decode error " << error << ": " << lodepng_error_text(error) << std::endl;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Aggiunta #3: texture del piano
	error = lodepng::decode(imagePiano, width, height, "assets/ground.png");
	if(error) std::cout << "decode error " << error << ": " << lodepng_error_text(error) << std::endl;
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagePiano.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// texture del pulsante
	error = lodepng::decode(image2, width, height, "assets/button.png");
	
	if(error) std::cout << "decode error " << error << ": " << lodepng_error_text(error) << std::endl;

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image2.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenTextures(1, &textureCube); check(__LINE__);
	glActiveTexture(GL_TEXTURE3); check(__LINE__);
	std::array<GLenum, 6> targets = {GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube); check(__LINE__);
	for(int i = 0; i < 6; ++i)
	{
		std::vector<unsigned char> skyImage;
		unsigned skyWidth, skyHeight;
		std::string filename = "assets/skyBox/";
		filename += std::to_string(i);
		filename += ".png";
		error = lodepng::decode(skyImage, skyWidth, skyHeight, filename.c_str());
		if(error) std::cout << "decode error " << error << ": " << lodepng_error_text(error) << std::endl;
		glTexImage2D(targets[i], 0, GL_RGBA, skyWidth, skyHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, skyImage.data()); check(__LINE__);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

}

void destroy_texture()
{
	glDeleteTextures(3, texture);
	glDeleteTextures(1, &textureCube);
}

glm::vec3 flagBox;
// aggiorniamo la posizione della camera in base ai tasti premuti e pesiamo le trasformazioni per il tempo trascorso dall'ultimo frame (dt) per renderle indipendenti dalla velocita' di disegno della scheda grafica
void update_camera()
{

	if (gameEnded)
		return;
	
	glm::vec3 right = glm::cross(camera_direction, camera_up);
	glm::vec3 left = glm::cross(right, camera_up);
	if(translate_forward)
	{
		camera_position += camera_direction*dt;
	}
	if(translate_backward)
	{
		camera_position -= camera_direction*dt;
	}
	if(translate_right)
	{
		camera_position += right * dt;
	}
	if(translate_left)
	{
		camera_position -= right * dt;
	}
	if(rotate_up)
	{
		camera_direction = glm::rotate(camera_direction, dt, right);
		camera_up = glm::rotate(camera_up, dt, right);
	}
	if(rotate_down)
	{
		camera_direction = glm::rotate(camera_direction, -dt, right);
		camera_up = glm::rotate(camera_up, -dt, right);
	}
	if(rotate_right)
	{
		camera_direction = glm::rotate(camera_direction, -dt, camera_up);
	}
	if(rotate_left)
	{
		camera_direction = glm::rotate(camera_direction, dt, camera_up);
	}
	
	// Rotazione asse z
	if(rotate_z_left)
	{
		camera_direction = glm::rotate(camera_direction, dt, left);
		camera_up = glm::rotate(camera_up, dt, left);
	}
	if(rotate_z_right)
	{
		camera_direction = glm::rotate(camera_direction, -dt, left);
		camera_up = glm::rotate(camera_up, -dt, left);		
	}
}

// 0 sole, 1 pianeta, 2 luna
std::array<glm::vec3, 3> planetPosition; // posizione nel mondo
std::array<float, 3> planetSize; // meta' raggio

glm::vec3 traslazione_sistema = glm::vec3(-2.0f, 5.0f, -15.0f);

void check_collision()
{
	// std::cout << "planetPosition 0 " << planetPosition[0][0] << " " << planetPosition[0][1] << " " << planetPosition[0][2] << " size " << planetSize[0] << std::endl;
	// std::cout << "planetPosition 1 " << planetPosition[1][0] << " " << planetPosition[1][1] << " " << planetPosition[1][2] << " size " << planetSize[1] << std::endl;
	// std::cout << "planetPosition 2 " << planetPosition[2][0] << " " << planetPosition[2][1] << " " << planetPosition[2][2] << " size " << planetSize[2] << std::endl;
	// abbiamo collisione quando la distanza (al quadrato) tra camera e relativo pianeta e' minore della somma del raggio del pianeta e del raggio della camera (per semplicita' impostato a 0.1) al quadrato
	// solitamente si utilizza la distanza al quadrato e non la semplice distanza perche' la radice e' un'operazione lenta e complessa
	if(glm::distance2(camera_position, planetPosition[0]) <= std::pow((planetSize[0] + 0.1f),2.f)) {
		std::cout << "collision between camera and sun" << std::endl;
		sun_visible=false;
	}
	if(glm::distance2(camera_position, planetPosition[1]) <= std::pow((planetSize[1] + 0.1f),2.f))
		std::cout << "collision between camera and planet" << std::endl;
	if(glm::distance2(camera_position, planetPosition[2]) <= std::pow((planetSize[2] + 0.1f),2.f))
		std::cout << "collision between camera and moon" << std::endl;

	//Aggiornamento #3
	if(abs(camera_position.y - plane_height) < 0.001f){
		std::cout << "collision between camera and plane" << std::endl;
		gameEnded = true;
	}

	//Aggiornamento #4 -> collisione con bandierina
 
	float raggioCirconferenza = 5.0f;
	
	if(glm::distance2(camera_position, flagPosition) <= raggioCirconferenza){
		std::cout<<"Gioco terminato: collisione con traguardo" << std::endl;
		gameEnded = true; 
	}

	
	
}

void gui_button_press()
{
	sky_on=!sky_on;
	std::cout << "bottone premuto" << std::endl;
}

// sfruttiamo le relative matrici model per aggiornare la posizione dei vari oggetti con cui calcoleremo la collisione
void draw(GLFWwindow* window)
{
	t = (timer::now()-start_time).count() * (float(timer::period::num)/float(timer::period::den));
	dt = (timer::now()-last_time).count() * (float(timer::period::num)/float(timer::period::den));
	update_camera();
	check_collision();
	last_time = timer::now();
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	

	glm::mat4 projection = glm::perspective(3.14f *(1.f/3.f), float(width)/float(height), 0.01f, 25.0f);
	glm::mat4 view = glm::lookAt(camera_position, camera_position + camera_direction, camera_up);
	glm::mat4 model(1.f);
	model = glm::translate(model, traslazione_sistema); // Aggiunta #2: sposto il sistema solare
	
	if(sky_on) {
		// quando disegnamo la skybox disattiviamo la scrittura nel depth buffer per fare in modo che nonostante sia un quadrato da [-1,-1,-1] a [1,1,1] nella rasterizzazione venga posizionate dietro
		// a tutti gli altri oggetti disegnati successivamente
		glDepthMask(GL_FALSE);
		glUseProgram(skyProgram);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube);
		glUniform1i(glGetUniformLocation(skyProgram, "textureSampler"), 3);
		glUniformMatrix3fv(glGetUniformLocation(skyProgram, "view_rot"), 1, GL_FALSE, &(glm::mat3(glm::inverse(view)))[0][0]);
		glBindVertexArray(vao[3]);
		glDrawElements(GL_TRIANGLES, sizeof(elements)/sizeof(GLuint), GL_UNSIGNED_INT, 0);
   		glDepthMask(GL_TRUE);
	}

	
		
	glm::vec3 light_direction = glm::normalize(glm::vec3(1.f, 1.f, 3.f));
	glm::vec3 light_position =  glm::vec3(0.f, 0.f, 0.f) + traslazione_sistema;
	glm::vec3 light_intensity = glm::vec3(1.f, 1.f, 1.f);
	glm::mat4 normal_matrix = glm::transpose(glm::inverse(model));
	glm::mat4 inv_proj = glm::inverse(projection);
	
	if(sun_visible){
		glUseProgram(shaderProgram2);
		planetPosition[0] = glm::vec3(0.0f, 0.0f, 0.0f) + traslazione_sistema;
		planetSize[0] = 1.f;

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "model"), 1, GL_FALSE, &model[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "normal_matrix"), 1, GL_FALSE, &normal_matrix[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "inv_proj"), 1, GL_FALSE, &inv_proj[0][0]);
		glUniform3fv(glGetUniformLocation(shaderProgram2, "light_direction"), 1, &light_direction[0]);
		glUniform3fv(glGetUniformLocation(shaderProgram2, "light_intensity"), 1, &light_intensity[0]);
		glUniform3fv(glGetUniformLocation(shaderProgram2, "view_position"), 1, &camera_position[0]);
		glUniform1f(glGetUniformLocation(shaderProgram2, "shininess"), material[1].shininess);
		glUniform3fv(glGetUniformLocation(shaderProgram2, "material_ambient"), 1, material[1].ambient);
		glUniform3fv(glGetUniformLocation(shaderProgram2, "material_diffuse"), 1, material[1].diffuse);
		glUniform3fv(glGetUniformLocation(shaderProgram2, "material_specular"), 1, material[1].specular);
		glBindVertexArray(vao[1]);
		glDrawElements(GL_TRIANGLES, element_count[1], GL_UNSIGNED_INT, 0);
 	}

// terra

	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "textureSampler"), 0);

	model  = glm::translate(model, glm::vec3(sin(t)*2.0f, 0.f, cos(t)*2.0f));
	model  = glm::scale(model, glm::vec3(1/2.f, 1/2.f, 1/2.f));
	glm::mat4 model2 = glm::rotate(model, 5*t, glm::vec3(.0f, 1.0f, .0f));
	normal_matrix = glm::transpose(glm::inverse(model2));
	planetPosition[1] = glm::vec3(model2*glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	planetSize[1] = 1/2.f;
	
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model2[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "normal_matrix"), 1, GL_FALSE, &normal_matrix[0][0]);
	glUniform3fv(glGetUniformLocation(shaderProgram, "light_direction"), 1, &light_direction[0]);	
	glUniform3fv(glGetUniformLocation(shaderProgram, "light_intensity"), 1, &light_intensity[0]);
	glUniform3fv(glGetUniformLocation(shaderProgram, "light_position"), 1, &light_position[0]);
	glUniform3fv(glGetUniformLocation(shaderProgram, "view_position"), 1, &camera_position[0]);
	glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), material[0].shininess);
	glUniform3fv(glGetUniformLocation(shaderProgram, "material_ambient"), 1, material[0].ambient);
	glUniform3fv(glGetUniformLocation(shaderProgram, "material_diffuse"), 1, material[0].diffuse);
	glUniform3fv(glGetUniformLocation(shaderProgram, "material_specular"), 1, material[0].specular);
	glBindVertexArray(vao[0]);
	glDrawElements(GL_TRIANGLES, element_count[0], GL_UNSIGNED_INT, 0);


// luna 

glUseProgram(shaderProgram2);

	model  = glm::translate(model, glm::vec3(sin(2*t)*2.0f,  cos(2*t)*2.0f,.0f));
	model  = glm::scale(model, glm::vec3(1/4.f, 1/4.f, 1/4.f));
	normal_matrix = glm::transpose(glm::inverse(model));
	planetPosition[2] = glm::vec3(model*glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	planetSize[2] = 1/4.f;

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "projection"), 1, GL_FALSE, &projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "view"), 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "model"), 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "normal_matrix"), 1, GL_FALSE, &normal_matrix[0][0]);
	glUniform3fv(glGetUniformLocation(shaderProgram2, "light_direction"), 1, &light_direction[0]);	
	glUniform3fv(glGetUniformLocation(shaderProgram2, "light_intensity"), 1, &light_intensity[0]);
	glUniform3fv(glGetUniformLocation(shaderProgram2, "light_position"), 1, &light_position[0]);
	glUniform3fv(glGetUniformLocation(shaderProgram2, "view_position"), 1, &camera_position[0]);
	glUniform1f(glGetUniformLocation(shaderProgram2, "shininess"), material[0].shininess);
	glUniform3fv(glGetUniformLocation(shaderProgram2, "material_ambient"), 1, material[0].ambient);
	glUniform3fv(glGetUniformLocation(shaderProgram2, "material_diffuse"), 1, material[0].diffuse);
	glUniform3fv(glGetUniformLocation(shaderProgram2, "material_specular"), 1, material[0].specular);
	glBindVertexArray(vao[0]);
	glDrawElements(GL_TRIANGLES, element_count[0], GL_UNSIGNED_INT, 0);

// astronave

	glUseProgram(shipProgram);
	

//ATTENZIONE: qui c'è il trucco per rendere il disegno solidale alla camera!
//applico l'inversa della lookat e sposto relativamente

	model = glm::inverse(glm::lookAt(camera_position, camera_position + camera_direction, camera_up));
	model  = glm::translate(model, glm::vec3(0,0,-2));
	
	normal_matrix = glm::transpose(glm::inverse(model));
	model = glm::scale(model, glm::vec3(0.02f, 0.02f, 0.02f));
	

	glUniformMatrix4fv(glGetUniformLocation(shipProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shipProgram, "view"), 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shipProgram, "model"), 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shipProgram, "normal_matrix"), 1, GL_FALSE, &normal_matrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shipProgram, "inv_proj"), 1, GL_FALSE, &inv_proj[0][0]);
	glUniform3fv(glGetUniformLocation(shipProgram, "light_direction"), 1, &light_direction[0]);
	glUniform3fv(glGetUniformLocation(shipProgram, "light_intensity"), 1, &light_intensity[0]);
	glUniform3fv(glGetUniformLocation(shipProgram, "view_position"), 1, &camera_position[0]);
	glUniform1f(glGetUniformLocation(shipProgram, "shininess"), material[2].shininess);
	glUniform3fv(glGetUniformLocation(shipProgram, "material_ambient"), 1, material[2].ambient);
	glUniform3fv(glGetUniformLocation(shipProgram, "material_diffuse"), 1, material[2].diffuse);
	glUniform3fv(glGetUniformLocation(shipProgram, "material_specular"), 1, material[2].specular);
	glBindVertexArray(vao[2]);
	glDrawElements(GL_TRIANGLES, element_count[2], GL_UNSIGNED_INT, 0);

	// Aggiunta #3: piano
	glm::mat4 planeModel(1.0f);
	planeModel = glm::scale(planeModel, glm::vec3(100.0f, 1.0f, 100.0f));
	planeModel = glm::translate(planeModel, glm::vec3(0.0f, plane_height, 0.0f));
	planeModel = glm::rotate(planeModel, 3.14f/2, glm::vec3(1, 0, 0));

	glUseProgram(planeProgram);
	glUniformMatrix4fv(glGetUniformLocation(planeProgram, "model"), 1, GL_FALSE, &planeModel[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(planeProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(planeProgram, "view"), 1, GL_FALSE, &view[0][0]);
	
	glUniform1i(glGetUniformLocation(planeProgram, "textureSampler"), 2); // immagine ground
	glBindVertexArray(vao[5]);
	glDrawElements(GL_TRIANGLES, sizeof(planeElements)/sizeof(GLuint), GL_UNSIGNED_INT, 0);

	// Aggiunta #4: la bandiera -> con rotazione e scale
	glUseProgram(flagProgram);

	glm::mat4 flagModel(1.0f);
	flagModel = glm::translate(flagModel, flagPosition);
	flagModel = glm::rotate(flagModel, -3.14f/2, glm::vec3(1.0f, 0.0f, 0.0f));
	flagModel = glm::scale(flagModel, scaleFlag);

	glUseProgram(flagProgram);
	glUniformMatrix4fv(glGetUniformLocation(flagProgram, "model"), 1, GL_FALSE, &flagModel[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(flagProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(flagProgram, "view"), 1, GL_FALSE, &view[0][0]);
	glUniform1f(glGetUniformLocation(flagProgram, "time"), t);
	glBindVertexArray(vao[6]);
	glDrawElements(GL_TRIANGLES, element_count[6], GL_UNSIGNED_INT, 0);

	// pulsante
    glDepthMask(GL_FALSE);
	glUseProgram(guiProgram);
	glUniform1i(glGetUniformLocation(guiProgram, "textureSampler"), 1);
	glUniform1f(glGetUniformLocation(guiProgram, "size"), button_size);
	glBindVertexArray(vao[4]);
	glDrawElements(GL_TRIANGLES, sizeof(guiElements)/sizeof(GLuint), GL_UNSIGNED_INT, 0);
    glDepthMask(GL_TRUE);

}

int main(int argc, char const *argv[])
{
	GLFWwindow* window;

	glfwSetErrorCallback(error_callback);

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
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	initialize_shader(); check(__LINE__);
	initialize_vao(); check(__LINE__);
	initialize_texture(); check(__LINE__);

	start_time = timer::now();
	last_time = start_time;
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
