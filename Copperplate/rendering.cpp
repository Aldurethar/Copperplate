#pragma once

#include "rendering.h"
#include <iostream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>

namespace Copperplate {


	// Window Class
	Window::Window() {
		glfwSetErrorCallback(GlfwErrorCallback);

		// initialize GLFW
		if (!glfwInit()) {
			std::cerr << "GLFW Initialization Failed!";
		}

		// setup OpenGL context and window
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		m_Window = glfwCreateWindow(1280, 720, "Copperplate", NULL, NULL);
		if (!m_Window) {
			std::cerr << "Window Creation Failed!";
			glfwTerminate();
		}

		glfwMakeContextCurrent(m_Window);
		int gladInitRes = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		if (!gladInitRes) {
			std::cerr << "Unable to initialize glad!";
			glfwDestroyWindow(m_Window);
			glfwTerminate();
		}
		glViewport(0, 0, 1280, 720);

		// hookup callback functions
		glfwSetFramebufferSizeCallback(m_Window, GlfwFramebufferSizeCallback);
		glfwSetWindowCloseCallback(m_Window, GlfwCloseWindowCallback);
		glfwSetKeyCallback(m_Window, GlfwKeyCallback);
		glfwSetMouseButtonCallback(m_Window, GlfwMouseButtonCallback);
		glfwSetCursorPosCallback(m_Window, GlfwMousePosCallback);
		
		std::cout << "Window created!";
	}

	Window::~Window() {
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void Window::SwapBuffers() {
		glfwSwapBuffers(m_Window);
	}

	float Window::GetHeight() {
		int width;
		int height;
		glfwGetFramebufferSize(m_Window, &width, &height);
		return (float)height;
	}

	float Window::GetWidth() {
		int width;
		int height;
		glfwGetFramebufferSize(m_Window, &width, &height);
		return (float)width;
	}
	

	// Camera Class
	Camera::Camera(float width, float height) {
		m_Azimuth = 0.0f;
		m_Height = 0.0f;
		m_Zoom = 3.0f;

		CalculateViewMatrix();
		SetViewportSize(width, height);
	}

	void Camera::CalculateViewMatrix() {
		glm::mat4 View = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0f, 0.0f, -m_Zoom));
		View = glm::rotate(View, m_Azimuth, glm::vec3(0.0f, 1.0f, 0.0f));
		View = glm::rotate(View, m_Height, glm::vec3(-1.0f, 0.0f, 0.0f));
		m_ViewMatrix = View;

		//Testing
		//m_ViewMatrix = glm::identity<glm::mat4>();
	}

	void Camera::SetViewportSize(float width, float height) {
		m_ProjectionMatrix = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);
		//Testing
		//m_ProjectionMatrix = glm::identity<glm::mat4>();
	}

	void Camera::SetPosition(float azimuth, float height, float zoom) {
		m_Azimuth = azimuth;
		m_Height = height;
		m_Zoom = zoom;
		CalculateViewMatrix();
	}

	void Camera::Move(float addAzimuth, float addHeight, float addZoom) {
		m_Azimuth += addAzimuth;
		m_Height += addHeight;
		m_Zoom += addZoom;
		CalculateViewMatrix();
	}

	glm::mat4 Camera::GetViewMatrix() {
		return m_ViewMatrix;
	}

	glm::mat4 Camera::GetProjectionMatrix() {
		return m_ProjectionMatrix;
	}

	// GLFW Callback Functions
	void GlfwErrorCallback(int error, const char* description) {
		std::cerr << "GLFW Error:" << error << description;
	}

	void GlfwFramebufferSizeCallback(GLFWwindow* window, int width, int height) {
		Application::Resize(width, height);
	}

	void GlfwCloseWindowCallback(GLFWwindow* window) {
		Application::ShouldClose = true;
	}

	void GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		Application::HandleKeyInput(key);
	}

	void GlfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
		//TODO: Fill this
	}

	void GlfwMousePosCallback(GLFWwindow* window, double xPos, double yPos) {
		//TODO: Fill this
	}

}