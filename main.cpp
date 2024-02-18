#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types
#include "glm/gtx/string_cast.hpp" // to_string()

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "Skybox.hpp"

#include <iostream>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint fogDensityLoc;
GLint flashLoc;
GLint cutOffLoc;
GLint outerCutOffLoc;
GLint camFrontDirLoc;
GLint camPosLoc;
GLint greyLoc;

// camera
gps::Camera myCamera(
	glm::vec3(0.0f, 0.0f, 3.0f),
	glm::vec3(0.0f, 0.0f, -10.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));
bool cameraTour;
int step = 0;
int it = 0;
float tourSpeed;

GLfloat cameraSpeed = 0.1f;
GLfloat baseCameraSpeed = 5.0f;

// time 
double currentTime, lastTime, deltaTime;

// mouse
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
double lastX = 800.0f / 2.0;
double lastY = 600.0 / 2.0;
GLfloat lightAngle = 0.0f;

GLboolean pressedKeys[1024];

// models
gps::Model3D unmovable;
gps::Model3D asteroid1;
gps::Model3D landscape;
gps::Model3D earth;
gps::Model3D flake;
GLfloat angle;
float asteroidRotY = 45.0f;
float earthRotY = 90.0f;

// shaders
gps::Shader basicShader;
gps::Shader skyboxShader;
gps::Shader depthMapShader;

//shadows
GLuint shadowMapFBO;
GLuint depthMapTexture;
const unsigned int SHADOW_WIDTH = 8192;
const unsigned int SHADOW_HEIGHT = 8192;
bool showDepthMap;
glm::mat4 lightRotation;

// fullsecreen toggle
bool fullscreen;
float aspectRatio;

// field of view
float fov = 45.0f;

// fog density
float fogDensity = 0.01f;

// light 
float lightSpeed = 0.7f;

// spotlight (flash)
glm::vec3 camFrontDir;
glm::vec3 camPos;
float cutOff = 0.3f;
float outerCutOff = 0.1f;
bool flash = false;

bool grey = false;
bool snow = false;

// skybox
gps::SkyBox skyBox;
std::vector<const GLchar*> faces;

#define MAX_PARTICLES 3000
struct Particle {
	glm::vec3 position;

	float lifespan;
	bool alive;
	float fade;

	float velocity;
	float windVelocity;
};
Particle particles[MAX_PARTICLES];
float slowdown = 2.0f;
bool windN = false, windS = false, windE = false, windV = false;


GLenum glCheckError_(const char* file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
		case GL_INVALID_ENUM:
			error = "INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			error = "INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			error = "INVALID_OPERATION";
			break;
		case GL_OUT_OF_MEMORY:
			error = "OUT_OF_MEMORY";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			error = "INVALID_FRAMEBUFFER_OPERATION";
			break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)


void initStarfield() {
	faces.push_back("skybox/starfield/starfield_rt.tga"); //right
	faces.push_back("skybox/starfield/starfield_lf.tga"); //left
	faces.push_back("skybox/starfield/starfield_up.tga"); //top
	faces.push_back("skybox/starfield/starfield_dn.tga"); //bottom
	faces.push_back("skybox/starfield/starfield_bk.tga"); //back   
	faces.push_back("skybox/starfield/starfield_ft.tga"); //front

	skyBox.Load(faces);
}

void viewModes(GLFWwindow* window, int key, int action) {

	// fullscreen
	if (pressedKeys[GLFW_KEY_F11]) {
		fullscreen = !fullscreen;
		if (fullscreen) {
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		}
		else {
			// adjust these values for windowed mode
			glfwSetWindowMonitor(window, nullptr, 100, 100, 800, 600, GLFW_DONT_CARE);
		}
	}

	// view edges between vertices ( lines )
	if (pressedKeys[GLFW_KEY_6]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	// view vertices ( points )
	if (pressedKeys[GLFW_KEY_7]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	// default view
	if (pressedKeys[GLFW_KEY_8]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// scene tour
	if (pressedKeys[GLFW_KEY_1]) {
		step = 0;
		cameraTour = !cameraTour;
	}


}

void initFlakes(int i) {
	particles[i].position = glm::vec3((float)(rand() % 41 - 20), 21.0f, (float)(rand() % 41 - 20));

	particles[i].lifespan = 2.0f;
	particles[i].fade = float(rand() % 100) / 1000.0f + 0.003f;
	particles[i].alive = true;

	particles[i].velocity = 0.0f;
	particles[i].windVelocity = 0.0f;

}

float windSpeed = 2.2f;
void updateFlakes(float deltaTime) {
	for (int i = 0; i < MAX_PARTICLES; ++i) {
		if (particles[i].alive == true) {

			particles[i].position.y += particles[i].velocity / (slowdown * 1000);
			particles[i].velocity += -0.8;
			particles[i].lifespan -= particles[i].fade;

			if (particles[i].position.y < 15.0f) {
				if (windN) {
					particles[i].position.x += particles[i].windVelocity / (slowdown * 1000);
					particles[i].windVelocity += windSpeed;
				}
				if (windS) {
					particles[i].position.x -= particles[i].windVelocity / (slowdown * 1000);
					particles[i].windVelocity += windSpeed;
				}
				if (windV) {
					particles[i].position.z += particles[i].windVelocity / (slowdown * 1000);
					particles[i].windVelocity += windSpeed;
				}
				if (windE) {
					particles[i].position.z -= particles[i].windVelocity / (slowdown * 1000);
					particles[i].windVelocity += windSpeed;
				}

			}
			

			if (particles[i].position.y < -3.0f)
				particles[i].lifespan -= 1.0f;

			if (particles[i].lifespan < 0.0)
				initFlakes(i);

		}
	}
}

void moveLight() {

	if (pressedKeys[GLFW_KEY_KP_8]) {
		lightDir.y -= lightSpeed;
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));
	}

	if (pressedKeys[GLFW_KEY_KP_5]) {
		lightDir.y += lightSpeed;
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));
	}

	if (pressedKeys[GLFW_KEY_KP_4]) {
		lightDir.x -= lightSpeed;
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));
	}

	if (pressedKeys[GLFW_KEY_KP_6]) {
		lightDir.x += lightSpeed;
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));
	}

	if (pressedKeys[GLFW_KEY_KP_1]) {
		lightDir.z -= lightSpeed;
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));
	}

	if (pressedKeys[GLFW_KEY_KP_3]) {
		lightDir.z += lightSpeed;
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));
	}

	if (pressedKeys[GLFW_KEY_KP_7]) {
		lightDir = glm::vec3(-9.09f, 9.39f, -1.80f);
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));
	}

	if (pressedKeys[GLFW_KEY_KP_9]) {
		grey = !grey;
		glUniform1i(greyLoc, grey);

	}
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);

	WindowDimensions newWindowSize = WindowDimensions{ width, height };
	myWindow.setWindowDimensions(newWindowSize);

	aspectRatio = (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height;
	projection = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
	projectionLoc = glGetUniformLocation(basicShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glViewport(0, 0, width, height);

}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS) {
			pressedKeys[key] = true;
		}
		else if (action == GLFW_RELEASE) {
			pressedKeys[key] = false;
		}
	}

	// swap between view modes of scene
	viewModes(window, key, action);
	moveLight();

	// reset position of camera
	if (pressedKeys[GLFW_KEY_R]) {
		myCamera.reset();
		//update view matrix
		view = myCamera.getViewMatrix();
		basicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	}

	// fog
	if (pressedKeys[GLFW_KEY_B]) {
		fogDensity += 0.01f;
		glUniform1f(fogDensityLoc, fogDensity);
	}

	if (pressedKeys[GLFW_KEY_V]) {
		fogDensity -= 0.01f;
		glUniform1f(fogDensityLoc, fogDensity);
	}

	// flashlight
	if (pressedKeys[GLFW_KEY_F]) {
		flash = !flash;
		glUniform1i(flashLoc, flash);
	}

	// snow
	if (pressedKeys[GLFW_KEY_G]) {
		snow = !snow;

	}

	// wind
	if (pressedKeys[GLFW_KEY_UP]) {
		windN = !windN;
	}

	if (pressedKeys[GLFW_KEY_DOWN]) {
		windS = !windS;
	}
	if (pressedKeys[GLFW_KEY_RIGHT]) {
		windE = !windE;
	}
	if (pressedKeys[GLFW_KEY_LEFT]) {
		windV = !windV;
	}


}

void processDeltaSpeed() {
	currentTime = glfwGetTime();
	deltaTime = currentTime - lastTime;
	lastTime = currentTime;

	aspectRatio = (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height;
	cameraSpeed = baseCameraSpeed * aspectRatio * deltaTime;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	fov -= (float)yoffset;
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 90.0f)
		fov = 90.0f;

	aspectRatio = (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height;
	projection = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
	projectionLoc = glGetUniformLocation(basicShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	double xoffset = xpos - lastX;
	double yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	double sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	myCamera.rotate(pitch, yaw);
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));

}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		fov = 15.0f;
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
		fov = 45.0f;

	aspectRatio = (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height;
	projection = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
	projectionLoc = glGetUniformLocation(basicShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void sceneTour() {

	std::cout << it << std::endl;

	tourSpeed = cameraSpeed * 0.2f;
	// part 1
	if (step == 0) {
		myCamera.setPosition(glm::vec3(18.0f, 1.0f, 14.0f));
		myCamera.setFront(glm::vec3(-0.58f, -0.12f, -0.80));
		it++;
		if (it > 10) {
			it = 0;
			step++;
		}
	}
	if (step == 1) {
		myCamera.move(gps::MOVE_LEFT, tourSpeed);
		it++;
		if (it > 460 * 2) {
			it = 0;
			step++;
		}
	}

	// part 2
	if (step == 2) {
		myCamera.setPosition(glm::vec3(-9.274306f, -0.943014f, 1.866937f));
		myCamera.setFront(glm::vec3(0.327038f, -0.033155f, -0.944429f));
		it++;
		if (it > 10) {
			it = 0;
			step++;
		}
	}
	if (step == 3) {
		myCamera.move(gps::MOVE_RIGHT, tourSpeed);
		it++;
		if (it > 400 * 2) {
			it = 0;
			step++;
		}
	}

	// part 3
	if (step == 4) {
		myCamera.setPosition(glm::vec3(1.406184f, -0.910371f, -0.164452));
		myCamera.setFront(glm::vec3(-0.998873f, -0.027922f, 0.038373f));
		it++;
		if (it > 10) {
			it = 0;
			step++;
		}
	}
	if (step == 5) {
		myCamera.move(gps::MOVE_BACKWARD, tourSpeed);
		it++;
		if (it > 450 * 2) {
			it = 0;
			step = 0;
		}
	}

}

void processMovement() {

	if (cameraTour) {
		sceneTour();
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		basicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		basicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		basicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		basicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_SPACE]) {
		myCamera.move(gps::MOVE_UP, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		basicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}


	if (pressedKeys[GLFW_KEY_LEFT_CONTROL]) {
		myCamera.move(gps::MOVE_DOWN, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		basicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_Q]) {
		angle -= 1.0f;
		// update model matrix for teapot
		model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
		// update normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angle += 1.0f;
		// update model matrix for teapot
		model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
		// update normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_LEFT_SHIFT]) {
		baseCameraSpeed = 15.0f;
	}
	else {
		baseCameraSpeed = 5.0f;
	}

}

void initOpenGLWindow() {
	myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
	glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
	glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
	glfwSetScrollCallback(myWindow.getWindow(), scrollCallback);
	glfwSetMouseButtonCallback(myWindow.getWindow(), mouseButtonCallback);
	glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
	unmovable.LoadModel("models/unmovable/unmovable.obj", "models/unmovable/");

	asteroid1.LoadModel("models/asteroid/asteroid1.obj", "models/asteroid/");

	earth.LoadModel("models/earth/earth.obj", "models/earth/");

	landscape.LoadModel("models/landscape/landscape.obj", "models/landscape/");

	flake.LoadModel("models/flake/flakeu.obj", "models/flake/");

}

void initShaders() {
	basicShader.loadShader(
		"shaders/basic.vert",
		"shaders/basic.frag");
	depthMapShader.loadShader(
		"shaders/depthMap.vert",
		"shaders/depthMap.frag");
	skyboxShader.loadShader(
		"shaders/skyboxShader.vert",
		"shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
}

void initFBO() {

	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void initUniforms() {
	basicShader.useShaderProgram();

	// create model matrix for teapot
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(basicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(basicShader.shaderProgram, "view");
	// send view matrix to shader
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	// compute normal matrix for teapot
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(basicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
		(float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
		0.1f, 100.0f);
	projectionLoc = glGetUniformLocation(basicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(-9.09f, 9.39f, -1.80f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(basicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(basicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	// fog density
	fogDensityLoc = glGetUniformLocation(basicShader.shaderProgram, "fogDensity");

	//spotlight -  flash
	camPosLoc = glGetUniformLocation(basicShader.shaderProgram, "cameraPos");
	camFrontDirLoc = glGetUniformLocation(basicShader.shaderProgram, "cameraFront");

	// flash 
	flashLoc = glGetUniformLocation(basicShader.shaderProgram, "flash");
	glUniform1i(flashLoc, flash);

	// grey
	greyLoc = glGetUniformLocation(basicShader.shaderProgram, "grey");
	glUniform1f(greyLoc, grey);
}

glm::mat4 computeLightSpaceTrMatrix() {
	glm::mat4 lightView = glm::lookAt(glm::inverseTranspose(glm::mat3(lightRotation)) * lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const GLfloat near_plane = 0.5f, far_plane = 100.0f;
	glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

	return lightSpaceTrMatrix;
}

void renderObject(gps::Shader shader, bool depthPass) {
	// select active shader program
	shader.useShaderProgram();

	// -- landscape : moon surface
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	landscape.Draw(shader);

	// -- unmovable objects
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	unmovable.Draw(shader);

	// asteroid animated
	asteroidRotY += 20.0f * deltaTime;
	glm::mat4 trAst = glm::translate(glm::mat4(1.0f), glm::vec3(-4.036f, 6.9571f, -4.23668f));
	glm::mat4 rotAst = glm::rotate(trAst, glm::radians(asteroidRotY), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 modelAst = glm::translate(rotAst, glm::vec3(4.036f, -6.9571f, 4.23668f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelAst));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	asteroid1.Draw(shader);

	// earth animated
	earthRotY += 3.0f * deltaTime;
	glm::mat4 trErt = glm::translate(glm::mat4(1.0f), glm::vec3(-52.0, 0.0f, -3.0));
	glm::mat4 rotErt = glm::rotate(trErt, glm::radians(earthRotY), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 modelErt = glm::translate(rotErt, glm::vec3(52.0f, 0.0f, 3.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelErt));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	earth.Draw(shader);

	if (snow) {
		for (int i = 0; i < MAX_PARTICLES; ++i) {

			glm::mat4 modelFlake = glm::translate(glm::mat4(1.0f), particles[i].position);
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelFlake));
			glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
			flake.Draw(shader);
		}
	}

}

void renderScene() {

	// -- SHADOWS !!!!
	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	depthMapShader.useShaderProgram();

	// 1.
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	renderObject(depthMapShader, true);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 2.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

	basicShader.useShaderProgram();
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(basicShader.shaderProgram, "shadowMap"), 3);

	glUniformMatrix4fv(glGetUniformLocation(basicShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
	renderObject(basicShader, false);

	// if(flash)
	if (flash) {
		glm::vec3 camFlash = camPos + glm::vec3(0.0, 5.0f, 10.0f);
		lightDir = camFlash;
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));
	}

	skyBox.Draw(skyboxShader, view, projection);

	// to calculate flash
	basicShader.useShaderProgram();
	camFrontDir = myCamera.getFront();
	glUniform3fv(camFrontDirLoc, 1, glm::value_ptr(camFrontDir));
	camPos = myCamera.getPosition();
	glUniform3fv(camPosLoc, 1, glm::value_ptr(camPos));

	std::cout << it << std::endl;
	//std::cout << glm::to_string(particles[0].position) << std::endl;
	//std::cout << windN << windS << windE << windV << std::endl;

}

void cleanup() {
	myWindow.Delete();
	//cleanup code for your own data
}

int main(int argc, const char* argv[]) {


	try {
		initOpenGLWindow();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	initOpenGLState();
	initModels();
	initStarfield();
	initShaders();
	initUniforms();
	initFBO();
	setWindowCallbacks();

	for (size_t i = 0; i < MAX_PARTICLES; i++)
	{
		initFlakes(i);
	}

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
		processDeltaSpeed();
		processMovement();
		renderScene();
		updateFlakes(deltaTime);

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());
	}

	cleanup();

	return EXIT_SUCCESS;
}
