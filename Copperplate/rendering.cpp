#pragma once

#include "rendering.h"
#include "application.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm\gtx\string_cast.hpp>

#include <iostream>

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

	int Window::GetHeight() {
		int width;
		int height;
		glfwGetFramebufferSize(m_Window, &width, &height);
		return height;
	}

	int Window::GetWidth() {
		int width;
		int height;
		glfwGetFramebufferSize(m_Window, &width, &height);
		return width;
	}
	

	// Camera Class
	Camera::Camera(int width, int height) {
		m_Azimuth = 3.4f;
		m_Height = -0.2f;
		m_Zoom = 3.0f;

		CalculateViewMatrix();
		SetViewportSize(width, height);
	}

	void Camera::CalculateViewMatrix() {
		if (m_Height > 1.57f) m_Height = 1.57f;
		if (m_Height < -1.57f) m_Height = -1.57f;

		float posX = sin(m_Azimuth) * cos(m_Height) * m_Zoom;
		float posY = sin(m_Height) * m_Zoom;
		float posZ = cos(m_Azimuth) * cos(m_Height) * m_Zoom;
		m_Position = glm::vec3(posX, posY, posZ);

		m_Forward = glm::normalize(glm::vec3(0.0f) - m_Position);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 right = glm::normalize(glm::cross(m_Forward, up));
		up = glm::normalize(glm::cross(right, m_Forward));

		glm::mat4 View = glm::mat4();
		View[0] = glm::vec4(right.x, up.x, m_Forward.x, 0.0f);
		View[1] = glm::vec4(right.y, up.y, m_Forward.y, 0.0f);
		View[2] = glm::vec4(right.z, up.z, m_Forward.z, 0.0f);
		View[3] = glm::vec4(glm::dot(right, m_Position), glm::dot(up, m_Position), glm::dot(m_Forward, m_Position), 1.0f);
		
		m_ViewMatrix = View;

		//Testing
		//m_ViewMatrix = glm::identity<glm::mat4>();
	}

	void Camera::SetViewportSize(int width, int height) {
		m_ProjectionMatrix = glm::perspective(glm::radians(45.0f), (float) width / (float) height, 0.1f, 100.0f);
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

	glm::vec3 Camera::GetForwardVector() {
		return m_Forward;
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