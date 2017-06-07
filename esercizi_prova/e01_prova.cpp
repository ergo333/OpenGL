// win : CL /MD /Feapp /Iinclude e01.cpp /link /LIBPATH:lib\win /NODEFAULTLIB:MSVCRTD
// osx : c++ -std=c++11 -o app e01.cpp -I ./include -L ./lib/mac -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
// lin : c++ -std=c++0x -o app e01.cpp -I ./include -L./lib/lin -Wl,-rpath,./lib/lin/ -lglfw -lGL
// l32 : c++ -std=c++0x -o app e01.cpp -I ./include -L./lib/lin32 -Wl,-rpath,./lib/lin32/ -lglfw -lGL

#include <cstdlib>
#include <iostream>

#ifdef _WIN32
#include <GL/glew.h>
#else
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLFW/glfw3.h>

#if defined(_MSC_VER)
#pragma comment(lib,"user32")
#pragma comment(lib,"gdi32")
#pragma comment(lib,"opengl32")
#pragma comment(lib,"glew32")
#pragma comment(lib,"glfw3")
#endif

static void error_callback(int error, const char* description)
{
	std::cerr << description << std::endl;
}
/*
	funzione richiamata da glfw ogni volta che viene rilevata la pressione o il rilascio di un tasto (della tastiera)
*/
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// alla pressione del tasto ESC settiamo la flag per segnalare che vogliamo terminare l'esecuzione dell'applicazione il piu' presto possibile
	// nel main glfwPollEvents viene richiamata al termine del while quindi l'uscita sara' immediata, diversamente se la richiamassimo prima di glfwSwapBuffers potrebbe esserci dell'attesa dovuta alla sincronizzazione di opengl stesso
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

// Shader sources
/*
	passiamo alla gpu la posizione in screen-space quindi non c'e' nessuna trasformazione da applicare
*/
const GLchar* vertexSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"in vec2 position;"
	"void main() {"
		"gl_Position = vec4(position, 0.0, 1.0);"
	"}";
	
/*
	applichiamo un colore uniforme (bianco) a tutti i pixel che andiamo a disegnare
*/
const GLchar* fragmentSource =
#if defined(__APPLE_CC__)
	"#version 150 core\n"
#else
	"#version 130\n"
#endif
	"out vec4 outColor;"
	"void main(){"
		"outColor = vec4(1.0, 1.0, 1.0, 1.0);"
	"}";
	

/*
	semplice quadrato che ricopre 1/4 del centro della finestra
	0--1
	|\ |
	| \|
	3--2
*/

const GLfloat vertices[] = {
	-0.5f,  0.5f,		//top-left
	 0.5f,  0.5f,		//top-right
	 0.5f, -0.5f, 		//bottom-right
	-0.5f, -0.5f		//botom-left 
};

const GLuint elements[] = {
	0, 1, 2, 
	2, 3, 0
};

GLuint vao;
GLuint vbo;
GLuint ibo;
GLuint shaderProgram;

/*
	(rudimentale) controllo degli errori per OpenGL, il while e' necessario nel caso in cui siano stati accumulati errori (restano "in coda" fino a quando non vengono letti)
	da utilizzare per scoprire dove puo' essere l'errore (nel caso di operazioni non valide)
	es: check(__LINE__) stampera' l'errore e la linea di codice in cui e' stato rilevato
*/
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

/*	compila i due shader precedentemente definiti e li linka nel program che utilizzeremo per disegnare i nostri oggetti
	
	per controllare lo stato di compilazione degli shader si puo' utilizzare la funzione glGetShaderiv(handle, GL_COMPILE_STATUS, &status)
	dove handle e' l'identificatore ritornato da glCreateShader e status e' un GLint che sara' 1 se lo shader e' valido
	per ottenere gli eventuali errori presenti si puo' utilizzare glGetShaderInfoLog(handle, slen, &slen, log)
	dove slen e' la dimensione disponibile in log e dove verra' salvato lo spazio effettivamente usato dall'output, log e' un puntatore ad un buff dove verra' copiato l'output

	analogamente per il program esistono le funzioni glGetProgramiv(program, GL_LINK_STATUS, &status) e glGetProgramInfoLog(program, slen, &slen, log)
*/

void initialize_shader()
{

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

}
/*
	rimuove le informazioni lato gpu degli shader
*/
void destroy_shader()
{
	glDeleteProgram(shaderProgram);
}

/*
	inizializza un VertexArrayObject che conterra' i dati di vertici e indici necessari a disegnare gli oggetti a schermo
	all'interno del VAO viene salvato lo stato di bind dell'array di indici
	differentemente per i vertici vengono salvati i puntatori relativi ai buffer da cui reperire i dati
	grazie a questo e' possibile avere piu' buffer da cui reperire le informazioni di un singolo vertice,
	ad esempio potremo avere un buffer con le posizioni che verra' aggiornato regolarmente (ogni frame) mentre colore e coordinate delle texture restano invariate in un secondo buffer
	nei nostri esempi utilizzeremo un unico buffer con le informazioni di ogni vertice compatte (es: posizione, colore, posizione, colore, ecc..)
*/
void initialize_vao()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

}

/*
	distrugge il VertexArrayObject utilizzato per disegnare e i buffer associati
	le eleminazione vengono fatte in ordine inverso rispetto alla loro creazione
*/
void destroy_vao()
{
	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &vbo);

	glDeleteVertexArrays(1, &vao);
}

/*
	funzione principale di disegno, viene richiamata dal main fintanto che non viene settata la volonta' di uscire (glfwSetWindowShouldClose)
	setta la dimensione dell'area di disegno (viewport) per seguire la dimensione della finestra nel caso in cui venga ridimensionata
	il colore con cui viene pulita la finestra puo' essere settato (dopo l'inizializzazione del context) con la funzione glClearColor(r,g,b,a), di default e' (0,0,0,0)
*/
void draw(GLFWwindow* window)
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glBindVertexArray(vao);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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

	while(!glfwWindowShouldClose(window))
	{
		draw(window); check(__LINE__);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	destroy_vao(); check(__LINE__);
	destroy_shader(); check(__LINE__);

	glfwDestroyWindow(window);

	glfwTerminate();
	return EXIT_SUCCESS;
}
