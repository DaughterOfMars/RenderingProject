#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <iostream>
#include "Map.hpp"
#include "Pipeline.hpp"
#include "Texture.hpp"
#include "MeshObject.hpp"
#include "Camera.hpp"
#include "TilesetReader.hpp"

static const float WALK_SPEED = 0.1f, SPRINT_SPEED = 1.0f, FLY_SPEED = 1.0f, CAMERA_SPEED = 0.002f;

class Application {
	const int INIT_WIDTH = 800, INIT_HEIGHT = 600;
	int width, height;
	GLFWwindow* window;
	Map map;
	Pipeline* pipeline;
	Camera* camera;

	bool keyboardState[GLFW_KEY_LAST] = { false };
	float speedModifier = 1.0f;
	double cursorPos[2];

	bool walkMode = false;

public:
	void run() {
		width = INIT_WIDTH;
		height = INIT_HEIGHT;
		map.initMap("./textures/pieces2.png");
		map.saveMap("./textures/output.png");
		
		initWindow();
		initInput();
		initPipeline();
		mainLoop();
	}

	void mainLoop() {
		float speed_modifier = 1.0f;
		while (!glfwWindowShouldClose(window)) {
			auto startTime = std::chrono::high_resolution_clock::now();

			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			glViewport(0, 0, width, height);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			pipeline->use();

			if (pipeline->map.count("lightPos") && pipeline->map.count("camPos")) {
				glUniform3fv(pipeline->map["lightPos"][1], 1, &glm::vec3(10.0f, 10.0f, 10.0f)[0]);
				glUniform3fv(pipeline->map["camPos"][1], 1, &camera->pos[0]);
			}

			auto vp = camera->projection*camera->view;
			glUniformMatrix4fv(pipeline->map["vp"][1], 1, GL_FALSE, &vp[0][0]);
			pipeline->draw(map.map);

			glfwSwapBuffers(window);
			glfwPollEvents();

			auto endTime = std::chrono::high_resolution_clock::now();
			float delta = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() / 1000.0f;
			speedModifier = 1.0f / delta / 60.0f;

			// Handle camera stuff
			// We should move this somewhere better maybe
			float speed;
			if (walkMode) {
				if (keyboardState[GLFW_KEY_LEFT_SHIFT])
					speed = SPRINT_SPEED;
				else
					speed = WALK_SPEED;
			}
			else speed = FLY_SPEED;
			if (keyboardState[GLFW_KEY_W]) {
				if (walkMode) camera->translate(Direction::WalkForward, speed*speedModifier);
				else camera->translate(Direction::Forward, speed*speedModifier);
			}
			if (keyboardState[GLFW_KEY_S]) {
				if (walkMode) camera->translate(Direction::WalkBackward, speed*speedModifier);
				else camera->translate(Direction::Backward, speed*speedModifier);
			}
			if (keyboardState[GLFW_KEY_A]) {
				camera->translate(Direction::Left, speed*speedModifier);
			}
			if (keyboardState[GLFW_KEY_D]) {
				camera->translate(Direction::Right, speed*speedModifier);
			}

			camera->update();

			//glClearColor(camera->pos.x/1000.0f, camera->pos.y / 1000.0f, camera->pos.z / 1000.0f, 1.0f);
			//std::cout << "(" << camera->pos.x << ", " << camera->pos.y << ", " << camera->pos.z << ") (" << camera->dir.x << ", " << camera->dir.y << ", " << camera->dir.z << ")" << std::endl;
		}

		// Cleanup

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void initWindow() {

		glfwSetErrorCallback(onWindowError);

		if(!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");
		//glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		window = glfwCreateWindow(INIT_WIDTH, INIT_HEIGHT, "Rendering Lunch and Learn", nullptr, nullptr);

		glfwMakeContextCurrent(window);

		if (!window) {
			glfwTerminate();
			throw std::runtime_error("Failed to create GLFW window");
		}

		glfwSetWindowUserPointer(window, this);
		glfwSetWindowSizeCallback(window, onWindowResized);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			throw std::runtime_error("Failed initialize OpenGL extensions");
		}
		glfwSwapInterval(1);

		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_CULL_FACE);

		camera = new Camera(width, height, glm::vec3(-5.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	void initPipeline() {
		Shader VertShader{ "./shaders/basic_shader.vert", GL_VERTEX_SHADER };
		Shader FragShader{ "./shaders/basic_shader.frag", GL_FRAGMENT_SHADER };

		pipeline = new Pipeline{ &VertShader, &FragShader };
		pipeline->use();

		readOBJ("./models/tileset.obj", "./models", "./models/metadata.json", pipeline->map["pos"][1], pipeline->map["normal"][1], pipeline->map["texCoord"][1], pipeline->map["texSampler"][1], pipeline->tiles);
	}

	void initInput() {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		if (glfwRawMouseMotionSupported())
			glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

		glfwGetCursorPos(window, &cursorPos[0], &cursorPos[1]);

		glfwSetKeyCallback(window, handleKeyInput);
		glfwSetCursorPosCallback(window, handleCursorInput);
		glfwSetMouseButtonCallback(window, handleMouseInput);
	}

	static void onWindowError(int error, const char* description)
	{
		std::cout << "Error: " << description << std::endl;
	}

	static void onWindowResized(GLFWwindow * window, int width, int height) {
		if (width == 0 || height == 0) return;

		Application * app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

		app->width = width;
		app->height = height;

		glViewport(0, 0, width, height);

		app->camera->resize(width, height);

		// Do anything needed when window resizes
	}

	static void handleCursorInput(GLFWwindow* window, double xpos, double ypos) {
		Application * app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

		app->camera->rotate(Axis::V, CAMERA_SPEED*app->speedModifier*(app->cursorPos[0] - xpos));
		float angle = app->camera->uAxisAngle();
		float yDelta = app->cursorPos[1] - ypos;
		if((angle > 0.1f && yDelta > 0) || (glm::pi<float>() - angle > 0.1f && yDelta < 0)) // This works okay unless you move the camera really fast, but whatevs
			app->camera->rotate(Axis::U, CAMERA_SPEED*app->speedModifier*(yDelta));

		app->cursorPos[0] = xpos;
		app->cursorPos[1] = ypos;

		app->camera->update();
	}

	static void handleKeyInput(GLFWwindow* window, int key, int scancode, int action, int mods) {
		Application * app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

		if (action == GLFW_PRESS) {
			app->keyboardState[key] = true;
			if (app->keyboardState[GLFW_KEY_ESCAPE] && glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				glfwSetCursorPosCallback(window, nullptr);
				glfwSetKeyCallback(window, nullptr);
			}
			else if (app->keyboardState[GLFW_KEY_G]) {
				app->walkMode = !app->walkMode;
				if (app->walkMode) app->camera->pos = glm::vec3(0.0f, 3.0f, 0.0f);
			}
		}
		else if (action == GLFW_RELEASE) {
			app->keyboardState[key] = false;
		}
	}

	static void handleMouseInput(GLFWwindow* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwSetCursorPosCallback(window, handleCursorInput);
			glfwSetKeyCallback(window, handleKeyInput);
		}
	}
};

int main() {
	Application app;
	try {
		app.run();
	}
	catch (const std::runtime_error & e) {
		std::cout << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}