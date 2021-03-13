#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include "shader.h"
#include "mesh.h"


void errorCallback(int error, const char* description) {
	fprintf(stderr, "GLFW error %d: %s \n", error, description);
}

GLFWwindow* initialize() {
	glfwSetErrorCallback(errorCallback);

	int glfwInitRes = glfwInit();
	if (!glfwInitRes) {
		fprintf(stderr, "Unable to initialize GLFW! \n");
		return nullptr;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Copperplate", NULL, NULL);
	if (!window) {
		fprintf(stderr, "Window creation failed! \n");
		glfwTerminate();
	}

	glfwMakeContextCurrent(window);

	int gladInitRes = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	if (!gladInitRes) {
		fprintf(stderr, "Unable to initialize glad");
		glfwDestroyWindow(window);
		glfwTerminate();
		return nullptr;
	}
	glViewport(0, 0, 1280, 720);

	return window;
}

int main(int argc, char* argv[]) {
	printf("Copperplate: simple program is running!\n");

	GLFWwindow* window = initialize();
	if (!window) return 0;

	glClearColor(0.1f, 0.6f, 0.3f, 1.0f);
	
	Shader* testShader = new Shader("shaders/flatcolor.vert", "shaders/flatcolor.frag");
	glCheckError();

	Mesh* testMesh = createTestMesh();
	glCheckError();

	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);

		testShader->use();
		testMesh->Draw();
		glCheckError();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}