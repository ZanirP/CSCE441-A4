#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Materials.h"
#include "Light.h"
#include "ScreenObject.h"

using namespace std;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
int TASK = 1;
bool OFFLINE = false;

shared_ptr<Camera> camera;
vector<shared_ptr<Program>> progs;
shared_ptr<Program> normalProg;
shared_ptr<Program> phongProg;
shared_ptr<Program> silhouetteProg;
shared_ptr<Program> celProg;
shared_ptr<Shape> shape;
shared_ptr<Shape> teapot;
shared_ptr<Shape> sphereShape;
shared_ptr<Shape> groundShape;
vector<Materials> materials;
vector<Light> lights;
std::vector<SceneObject> sceneObjects;

int materialIndex = 0;
int progIndex = 0;
int lightIndex = 0;

bool keyToggles[256] = {false}; // only for English keyboards!

// This function is called when a GLFW error occurs
static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera->moveForward(0.05f);
	}
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera->moveForward(-0.05f);
	}
	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		camera->moveRight(-0.05f);
	}
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		camera->moveRight(0.05f);
	}
	if(glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
		camera->zoom(-0.01f);
	}
	if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
		camera->zoom(0.01f);
	}
}

// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = (mods & GLFW_MOD_SHIFT) != 0;
		bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;
		bool alt   = (mods & GLFW_MOD_ALT) != 0;
		camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
	}
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved((float)xmouse, (float)ymouse);
	}
}

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
static void saveImage(const char *filepath, GLFWwindow *w)
{
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	int rc = stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
	if(rc) {
		cout << "Wrote to " << filepath << endl;
	} else {
		cout << "Couldn't write to " << filepath << endl;
	}
}



// This function is called once to initialize the scene and OpenGL
static void init()
{
    glfwSetTime(0.0);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

	float t = glfwGetTime();

    phongProg = make_shared<Program>();
    phongProg->setShaderNames(RESOURCE_DIR + "phong_vert.glsl",
                              RESOURCE_DIR + "phong_frag.glsl");
    phongProg->init();
    phongProg->addAttribute("aPos");
    phongProg->addAttribute("aNor");
	phongProg->addAttribute("aTex");
    phongProg->addUniform("MV");
    phongProg->addUniform("P");
    phongProg->addUniform("lightPos");
    phongProg->addUniform("lightColor");
    phongProg->addUniform("normalMatrix");
    phongProg->addUniform("ka");
    phongProg->addUniform("kd");
    phongProg->addUniform("ks");
    phongProg->addUniform("s");

	progs.push_back(phongProg);

    shape = make_shared<Shape>();
    shape->loadMesh(RESOURCE_DIR + "bunny.obj");
    shape->init();

    teapot = make_shared<Shape>();
    teapot->loadMesh(RESOURCE_DIR + "teapot.obj");
    teapot->init();

	sphereShape = make_shared<Shape>();
	sphereShape->loadMesh(RESOURCE_DIR + "sphere.obj");
	sphereShape->init();

	groundShape = make_shared<Shape>();
	groundShape->loadMesh(RESOURCE_DIR + "cube.obj");
	groundShape->init();

    camera = make_shared<Camera>();

    Materials mat;
    mat.ka = glm::vec3(0.2f);
    mat.kd = glm::vec3(0.8f);
    mat.ks = glm::vec3(0.5f);
    mat.s = 100.0f;
    materials.push_back(mat);

    Light sun;
    sun.position = glm::vec3(-2.0f, 3.0f, 4.0f);
    sun.color = glm::vec3(1.0f, 1.0f, 0.9f);
    lights.push_back(sun);

    srand((unsigned int)time(nullptr));

    float bunnyMinY = shape->getMinY();
    float teapotMinY = teapot->getMinY();

    int rows = 10;
    int cols = 10;
    float spacing = 1.2f;

    for (int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            SceneObject obj;

            if((i + j) % 2 == 0) {
                obj.mesh = shape;
                obj.minY = bunnyMinY;
            } else {
                obj.mesh = teapot;
                obj.minY = teapotMinY;
            }

            float rx = ((rand() / (float)RAND_MAX) - 0.5f) * 0.3f;
            float rz = ((rand() / (float)RAND_MAX) - 0.5f) * 0.3f;

            obj.pos = glm::vec3(
                (j - cols/2) * spacing + rx,
                0.0f,
                (i - rows/2) * spacing + rz
            );

            obj.rot = glm::vec3(0.0f,
                (rand() / (float)RAND_MAX) * 2.0f * M_PI,
                0.0f);

            obj.scl = glm::vec3(0.25f);
            obj.shear = glm::mat4(1.0f);
            obj.color = glm::vec3(
                rand() / (float)RAND_MAX,
                rand() / (float)RAND_MAX,
                rand() / (float)RAND_MAX
            );

            sceneObjects.push_back(obj);
        }
    }

    GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyToggles[(unsigned)'z']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
	
	double t = glfwGetTime();
	
	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);

	auto prog = phongProg;
	prog->bind();

	Materials &mat = materials[materialIndex];
	glm::vec3 lightCam = glm::vec3(MV->topMatrix() * glm::vec4(lights[0].position, 1.0f));
	glm::vec3 lightColor = lights[0].color;

	glUniform3fv(prog->getUniform("lightPos"), 1, glm::value_ptr(lightCam));
	glUniform3fv(prog->getUniform("lightColor"), 1, glm::value_ptr(lightColor));
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniform3fv(prog->getUniform("lightPos"), 1, glm::value_ptr(lightCam));
	glUniform3fv(prog->getUniform("lightColor"), 1, glm::value_ptr(lightColor));
	glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(mat.ka));
	glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(mat.ks));
	glUniform1f(prog->getUniform("s"), mat.s);

	MV->pushMatrix();
	MV->translate(lights[0].position);
	MV->scale(glm::vec3(0.2f));

	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV->topMatrix())));
	glUniformMatrix3fv(prog->getUniform("normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

	glm::vec3 sunAmbient = glm::vec3(1.0f, 0.9f, 0.3f);
	glm::vec3 zero = glm::vec3(0.0f);

	glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(sunAmbient));
	glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(zero));
	glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(zero));
	glUniform1f(prog->getUniform("s"), 1.0f);

	sphereShape->draw(prog);
	MV->popMatrix();

	MV->pushMatrix();
	MV->translate(glm::vec3(0.0f, 0.0f, 0.0f));
	MV->scale(glm::vec3(20.0f, 0.05f, 20.0f));
	MV->translate(glm::vec3(0.0f, -0.5f, 0.0f));

	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV->topMatrix())));
	glUniformMatrix3fv(prog->getUniform("normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

	glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(mat.ka));
	glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(mat.ks));
	glUniform1f(prog->getUniform("s"), mat.s);
	glm::vec3 groundColor(0.5f, 0.7f, 0.5f);
	glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(groundColor));

	groundShape->draw(prog);
	MV->popMatrix();

	glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(mat.ka));
	glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(mat.ks));
	glUniform1f(prog->getUniform("s"), mat.s);

	for(const auto &obj : sceneObjects) {
		MV->pushMatrix();

		float timeScale = 1.5f + 0.25f * sin((float)t + obj.pos.x + obj.pos.z);
		glm::vec3 animatedScale = obj.scl * timeScale;

		MV->translate(glm::vec3(obj.pos.x, obj.pos.y, obj.pos.z));
		MV->rotate(obj.rot.x, glm::vec3(1,0,0));
		MV->rotate(obj.rot.y, glm::vec3(0,1,0));
		MV->rotate(obj.rot.z, glm::vec3(0,0,1));
		MV->multMatrix(obj.shear);
		MV->scale(animatedScale);

		MV->translate(glm::vec3(0.0f, -obj.minY, 0.0f));

		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV->topMatrix())));
		glUniformMatrix3fv(prog->getUniform("normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

		glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(obj.color));

		obj.mesh->draw(prog);

		MV->popMatrix();
	}

	// HUD stuff here?
	auto PHUD = make_shared<MatrixStack>();
	auto MVHUD = make_shared<MatrixStack>();

	PHUD->pushMatrix();
	MVHUD->pushMatrix();
	float aspect = (float)width / (float)height;
	PHUD->multMatrix(glm::perspective(45.0f * (float)M_PI / 180.0f, aspect, 0.1f, 100.0f));

	MVHUD->translate(glm::vec3(0.0f, 0.0f, -2.0f));

	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(PHUD->topMatrix()));

	glUniform3fv(prog->getUniform("lightPos"), 1, glm::value_ptr(lightCam));
	glUniform3fv(prog->getUniform("lightColor"), 1, glm::value_ptr(lightColor));
	glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(mat.ka));
	glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(mat.ks));
	glUniform1f(prog->getUniform("s"), mat.s);

	MVHUD->pushMatrix();
	MVHUD->translate(glm::vec3(-0.85f, 0.5f, 0.0f));
	MVHUD->rotate((float)t, glm::vec3(0.0f, 1.0f, 0.0f));
	MVHUD->scale(glm::vec3(0.2f));
	MVHUD->translate(glm::vec3(0.0f, -shape->getMinY(), 0.0f));

	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MVHUD->topMatrix()));
	glm::mat3 normalMatrixHUD1 = glm::transpose(glm::inverse(glm::mat3(MVHUD->topMatrix())));
	glUniformMatrix3fv(prog->getUniform("normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrixHUD1));

	glm::vec3 hudColor1(0.6f, 0.6f, 0.6f);
	glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(hudColor1));

	shape->draw(prog);
	MVHUD->popMatrix();

	MVHUD->pushMatrix();
	MVHUD->translate(glm::vec3(0.9f, 0.5f, 0.0f));
	MVHUD->rotate((float)t, glm::vec3(0.0f, 1.0f, 0.0f));
	MVHUD->scale(glm::vec3(0.2f));
	MVHUD->translate(glm::vec3(0.0f, -teapot->getMinY(), 0.0f));

	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MVHUD->topMatrix()));
	glm::mat3 normalMatrixHUD2 = glm::transpose(glm::inverse(glm::mat3(MVHUD->topMatrix())));
	glUniformMatrix3fv(prog->getUniform("normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrixHUD2));

	glm::vec3 hudColor2(0.6f, 0.6f, 0.6f);
	glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(hudColor2));

	teapot->draw(prog);
	MVHUD->popMatrix();


	MVHUD->popMatrix();
	PHUD->popMatrix();


	prog->unbind();
	
	MV->popMatrix();
	P->popMatrix();
	
	GLSL::checkError(GET_FILE_LINE);
	
	if(OFFLINE) {
		string filename = "output" + std::to_string(TASK) + ".png";
		saveImage(filename.c_str(), window);
		GLSL::checkError(GET_FILE_LINE);
		glfwSetWindowShouldClose(window, true);
	}
}

int main(int argc, char **argv)
{
	if(argc < 3) {
		cout << "Usage: A3 RESOURCE_DIR TASK" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	TASK = atoi(argv[2]);
	
	// Optional argument
	if(argc >= 4) {
		OFFLINE = atoi(argv[3]) != 0;
	}

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "Zanir Pirani", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	GLSL::checkVersion();
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Set the window resize call back.
	glfwSetFramebufferSizeCallback(window, resize_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
