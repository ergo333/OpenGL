// win : CL /MD /Feapp /Iinclude e07.cpp /link /LIBPATH:lib\win /NODEFAULTLIB:MSVCRTD
// osx : c++ -std=c++11 -o app e07.cpp -I ./include -L ./lib/mac -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
// lin : c++ -std=c++0x -o app e07.cpp -I ./include -L./lib/lin -Wl,-rpath,./lib/lin/ -lglfw -lGL
// l32 : c++ -std=c++0x -o app e07.cpp -I ./include -L./lib/lin32 -Wl,-rpath,./lib/lin32/ -lglfw -lGL

#include <cstdlib>
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
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined(_MSC_VER)
#pragma comment(lib,"user32")
#pragma comment(lib,"gdi32")
#pragma comment(lib,"opengl32")
#pragma comment(lib,"glew32")
#pragma comment(lib,"glfw3")
#endif


glm::vec3 camera_position(0.0f, 0.0f, 5.0f);
glm::vec3 camera_direction(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up(0.0f, 1.0f, 0.0f);
enum action {
	translate_forward = 0,
	translate_backward,
	translate_right,
	translate_left,
	rotate_up,
	rotate_down,
	rotate_right,
	rotate_left,
	count
};
std::array<bool, action::count> actions;

static void error_callback(int error, const char* description)
{
	std::cerr << description << std::endl;
}
// alla pressione dei tasti salviamo l'azione che dovremmo compiere nell'aggiornare la camera
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if(key == GLFW_KEY_W)
	{
		if(action)
			actions[translate_forward] = true;
		else
			actions[translate_forward] = false;
	}
	if(key == GLFW_KEY_S)
	{
		if(action)
			actions[translate_backward] = true;
		else
			actions[translate_backward] = false;
	}
	if(key == GLFW_KEY_D)
	{
		if(action)
			actions[translate_right] = true;
		else
			actions[translate_right] = false;
	}
	if(key == GLFW_KEY_A)
	{
		if(action)
			actions[translate_left] = true;
		else
			actions[translate_left] = false;
	}
	if(key == GLFW_KEY_UP)
	{
		if(action)
			actions[rotate_up] = true;
		else
			actions[rotate_up] = false;
	}
	if(key == GLFW_KEY_DOWN)
	{
		if(action)
			actions[rotate_down] = true;
		else
			actions[rotate_down] = false;
	}
	if(key == GLFW_KEY_RIGHT)
	{
		if(action)
			actions[rotate_right] = true;
		else
			actions[rotate_right] = false;
	}
	if(key == GLFW_KEY_LEFT)
	{
		if(action)
			actions[rotate_left] = true;
		else
			actions[rotate_left] = false;
	}
}

// Shader sources
const GLchar* vertexSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec3 position;"
	"in vec3 normal;"
	"out vec3 Color;"
	"uniform mat4 model;"
	"uniform mat4 view;"
	"uniform mat4 projection;"
	"uniform float shininess;"
	"uniform vec3 material_ambient;"
	"uniform vec3 material_diffuse;"
	"uniform vec3 material_specular;"
	"uniform vec3 light_direction;"
	"uniform vec3 light_intensity;"
	"uniform vec3 view_position;"
	"void main() {"
	"	vec3 Normal = mat3(model) * normal;"
	"	gl_Position = projection * view * model * vec4(position, 1.0);"
	"	vec4 vertPos = model * vec4(position, 1.0);"
	"	vec3 Position = vec3(vertPos)/vertPos.w;"
	"	vec3 view_direction = normalize(view_position - Position);"
	"	vec3 R = reflect(-light_direction, Normal);"
	"	Color = light_intensity*material_ambient;"
	"	Color += light_intensity*material_diffuse * max(dot(light_direction, Normal), 0.0);"
	"	Color += light_intensity*material_specular * pow(max(dot(R, view_direction), 0.0), shininess);"
	"}";
const GLchar* fragmentSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec3 Color;"
	"out vec4 outColor;"
	"void main() {"
	"	outColor = vec4(Color, 1.0);"
	"}";

GLuint vao[2];
GLuint vbo[2];
GLuint ibo[2];

tinyobj::material_t material[2];
GLsizei element_count[2];

GLuint shaderProgram;
GLuint texture[2]; // 0 cubic.png
timer::time_point start_time, last_time;
float t = 0.0;
float dt = 0.0;

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

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}
void destroy_shader()
{
	glDeleteProgram(shaderProgram);
}

void initialize_vao_from_file(size_t i, const std::string& inputfile)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err = tinyobj::LoadObj(shapes, materials, inputfile.c_str(), "assets/sphere/"); // carica il modello definito da inputfile, l'ultimo parametro e' il path in cui cercare il material
	// shapes e' un array di shape con all'interno un nome e una mesh
	// ogni mesh ha un vettore di posizioni, uno di normali e uno di coordinate texture
	// ogni mesh inoltre ha un vettore di indici e l'id dei material utilizzati
	// ogni material ha un float[3] per ambient, diffuse, specular, transmittance e emission, un float per shininess, ior (index of refraction) e dissolve (0 trasparente, 1 opaco) e un int illum (illumination model)
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
		GLint posAttrib = glGetAttribLocation(shaderProgram, "position"); check(__LINE__);
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
		glBufferSubData(GL_ARRAY_BUFFER, offset, shapes[0].mesh.texcoords.size()*sizeof(float), shapes[0].mesh.texcoords.data()); check(__LINE__);
		// GLint cooAttrib = glGetAttribLocation(shaderProgram, "coord");
		// glEnableVertexAttribArray(cooAttrib);
		// glVertexAttribPointer(cooAttrib, 2, GL_FLOAT, GL_FALSE, 0, (void*)offset);
		offset += shapes[0].mesh.texcoords.size()*sizeof(float);
	}

	// salviamo il numero di indici e il material del modello per poterli utilizzare poi nella fase di draw della scena
	element_count[i] = shapes[0].mesh.indices.size();
	material[i] = materials[shapes[0].mesh.material_ids[0]];
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[i]); check(__LINE__);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*shapes[0].mesh.indices.size(), shapes[0].mesh.indices.data(), GL_STATIC_DRAW); check(__LINE__);
}

void initialize_vao()
{
	glGenVertexArrays(2, vao); check(__LINE__);
	glGenBuffers(2, vbo); check(__LINE__);
	glGenBuffers(2, ibo); check(__LINE__);
	initialize_vao_from_file(0, "assets/sphere/sphere.obj"); check(__LINE__);
	initialize_vao_from_file(1, "assets/sphere/sphere_hr.obj"); check(__LINE__);
}

void destroy_vao()
{
	glDeleteBuffers(2, ibo);
	glDeleteBuffers(2, vbo);

	glDeleteVertexArrays(2, vao);
}

void initialize_texture()
{
	glGenTextures(2, texture);
	std::vector<unsigned char> image;
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
}

void destroy_texture()
{
	glDeleteTextures(2, texture);
}

// aggiorniamo la posizione della camera in base ai tasti premuti e pesiamo le trasformazioni per il tempo trascorso dall'ultimo frame (dt) per renderle indipendenti dalla velocita' di disegno della scheda grafica
void update_camera()
{
	glm::vec3 right = glm::cross(camera_direction, camera_up);
	if(actions[translate_forward])
	{
		camera_position += camera_direction*dt;
	}
	if(actions[translate_backward])
	{
		camera_position -= camera_direction*dt;
	}
	if(actions[translate_right])
	{
		camera_position += right * dt;
	}
	if(actions[translate_left])
	{
		camera_position -= right * dt;
	}
	if(actions[rotate_up])
	{
		camera_direction = glm::rotate(camera_direction, dt, right);
		camera_up = glm::rotate(camera_up, dt, right);
	}
	if(actions[rotate_down])
	{
		camera_direction = glm::rotate(camera_direction, -dt, right);
		camera_up = glm::rotate(camera_up, -dt, right);
	}
	if(actions[rotate_right])
	{
		camera_direction = glm::rotate(camera_direction, -dt, camera_up);
	}
	if(actions[rotate_left])
	{
		camera_direction = glm::rotate(camera_direction, dt, camera_up);
	}
}
void draw(GLFWwindow* window)
{
	t = (timer::now()-start_time).count() * (float(timer::period::num)/float(timer::period::den));
	dt = (timer::now()-last_time).count() * (float(timer::period::num)/float(timer::period::den));
	update_camera();
	last_time = timer::now();
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram(shaderProgram);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(shaderProgram, "ambientSampler"), 0);

	glm::mat4 projection = glm::perspective(3.14f *(1.f/3.f), float(width)/float(height), 0.01f, 25.0f);
	glm::mat4 view = glm::lookAt(camera_position, camera_position + camera_direction, camera_up);
	glm::mat4 model(1.f);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);

	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glm::vec3 light_direction = glm::normalize(glm::vec3(1.f, 1.f, 3.f));
	glm::vec3 light_intensity = glm::vec3(1.f, 1.f, 1.f);
	glUniform3fv(glGetUniformLocation(shaderProgram, "light_direction"), 1, &light_direction[0]);
	glUniform3fv(glGetUniformLocation(shaderProgram, "light_intensity"), 1, &light_intensity[0]);
	glUniform3fv(glGetUniformLocation(shaderProgram, "view_position"), 1, &camera_position[0]);
	glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), material[0].shininess);
	glUniform3fv(glGetUniformLocation(shaderProgram, "material_ambient"), 1, material[0].ambient);
	glUniform3fv(glGetUniformLocation(shaderProgram, "material_diffuse"), 1, material[0].diffuse);
	glUniform3fv(glGetUniformLocation(shaderProgram, "material_specular"), 1, material[0].specular);
	glBindVertexArray(vao[0]);
	glDrawElements(GL_TRIANGLES, element_count[0], GL_UNSIGNED_INT, 0);

	model *= glm::translate(glm::mat4(1.f), glm::vec3(3.f, 0.f, 0.f));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
	glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), material[1].shininess);
	glUniform3fv(glGetUniformLocation(shaderProgram, "material_ambient"), 1, material[1].ambient);
	glUniform3fv(glGetUniformLocation(shaderProgram, "material_diffuse"), 1, material[1].diffuse);
	glUniform3fv(glGetUniformLocation(shaderProgram, "material_specular"), 1, material[1].specular);
	glBindVertexArray(vao[1]);
	glDrawElements(GL_TRIANGLES, element_count[1], GL_UNSIGNED_INT, 0);
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
