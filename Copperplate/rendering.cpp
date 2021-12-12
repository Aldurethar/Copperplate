#pragma once

#include "rendering.h"
#include "application.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm\gtx\string_cast.hpp>

#include <iostream>

namespace Copperplate {

	const glm::vec4 IMAGE_CLEARCOLOR = glm::vec4(0.89f, 0.87f, 0.53f, 1.0f);
	const glm::vec4 NORMAL_CLEARCOLOR = glm::vec4(0.0f);
	const glm::vec4 DEPTH_CLEARCOLOR = glm::vec4(100.0f, 1.0f, 1.0f, 0.0f);
	const glm::vec4 CURVATURE_CLEARCOLOR = glm::vec4(0.0f);
	const glm::vec4 MOVEMENT_CLEARCOLOR = glm::vec4(0.0f);

	// Display Settings
	bool DisplaySettings::RenderContours = true;
	bool DisplaySettings::RenderSeedPoints = false;
	bool DisplaySettings::RenderScreenSpaceSeeds = false;
	bool DisplaySettings::RenderHatching = true;
	bool DisplaySettings::OnlyUpdateHatchLines = false;
	bool DisplaySettings::RenderHatchingCollision = false;
	bool DisplaySettings::RenderCurrentDebug = true;
	int DisplaySettings::NumHatchingLines = -1;
	int DisplaySettings::NumPointsPerHatch = -1;
	float DisplaySettings::LineSeparationDistance = 8.0f;
	float DisplaySettings::LineTestDistance = 4.0f;
	EHatchingDirections DisplaySettings::HatchingDirection = EHatchingDirections::HD_LargestCurvature;
	EFramebuffers DisplaySettings::FramebufferToDisplay = EFramebuffers::FB_Default;


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
		
		std::cout << "Window created! \n";
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
		Update();
	}

	void Camera::Update() {
		m_PrevViewMatrix = m_ViewMatrix;
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

	glm::mat4 Camera::GetPrevViewMatrix() {
		return m_PrevViewMatrix;
	}

	glm::vec3 Camera::GetForwardVector() {
		return m_Forward;
	}

	//Screen Quad				 Pos				TexCoords
	float ScreenQuadVerts[] = { -1.0f, -1.0f, 0.0f,	0.0f, 0.0f,
								 1.0f, -1.0f, 0.0f,	1.0f, 0.0f,
								-1.0f,  1.0f, 0.0f,	0.0f, 1.0f,
								 1.0f,  1.0f, 0.0f,	1.0f, 1.0f };

	//Renderer Class
	Renderer::Renderer(Shared<Window> window)
	{
		m_Window = window;
		//Setup Screen Quad VAO
		glGenVertexArrays(1, &m_ScreenQuadVAO);
		unsigned int vbo;
		glGenBuffers(1, &vbo);

		glBindVertexArray(m_ScreenQuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, 4 * 5 * sizeof(float), ScreenQuadVerts, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)0);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(0);

		glCheckError();

		//Setup Framebuffers
		//Default Render to Screen
		unsigned int clearFlags = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		FrameBuffer default = { 0, 0, IMAGE_CLEARCOLOR, clearFlags };
		m_Framebuffers[FB_Default] = default;

		//Normal Framebuffer
		FrameBuffer normal;
		normal.m_ClearColor = NORMAL_CLEARCOLOR;
		normal.m_ClearFlags = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		glGenFramebuffers(1, &normal.m_FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, normal.m_FBO);
		
		glGenTextures(1, &normal.m_Texture);
		glBindTexture(GL_TEXTURE_2D, normal.m_Texture);

		// Needs a floating point internal format to avoid values being clamped to [0;1]
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Window->GetWidth(), m_Window->GetHeight(), 0, GL_RGBA, GL_SHORT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, normal.m_Texture, 0);

		unsigned int rbo;
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Window->GetWidth(), m_Window->GetHeight());
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glCheckError();
		glCheckFrameBufferError();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		m_Framebuffers[FB_Normals] = normal;

		//Depth Framebuffer
		FrameBuffer depth;
		depth.m_ClearColor = DEPTH_CLEARCOLOR;
		depth.m_ClearFlags = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

		glGenFramebuffers(1, &depth.m_FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, depth.m_FBO);

		glGenTextures(1, &depth.m_Texture);
		glBindTexture(GL_TEXTURE_2D, depth.m_Texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_Window->GetWidth(), m_Window->GetHeight(), 0, GL_RED, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depth.m_Texture, 0);
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Window->GetWidth(), m_Window->GetHeight());
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glCheckError();
		glCheckFrameBufferError();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		m_Framebuffers[FB_Depth] = depth;

		//Curvature Framebuffer
		FrameBuffer curvature;
		curvature.m_ClearColor = CURVATURE_CLEARCOLOR;
		curvature.m_ClearFlags = GL_COLOR_BUFFER_BIT;
		glGenFramebuffers(1, &curvature.m_FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, curvature.m_FBO);

		glGenTextures(1, &curvature.m_Texture);
		glBindTexture(GL_TEXTURE_2D, curvature.m_Texture);

		// Needs a floating point internal format to avoid values being clamped to [0;1] 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Window->GetWidth(), m_Window->GetHeight(), 0, GL_RGBA, GL_SHORT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curvature.m_Texture, 0);

		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Window->GetWidth(), m_Window->GetHeight());
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glCheckError();
		glCheckFrameBufferError();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		m_Framebuffers[FB_Curvature] = curvature;

		// Movement Framebuffer
		FrameBuffer movement;
		movement.m_ClearColor = MOVEMENT_CLEARCOLOR;
		movement.m_ClearFlags = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		glGenFramebuffers(1, &movement.m_FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, movement.m_FBO);

		glGenTextures(1, &movement.m_Texture);
		glBindTexture(GL_TEXTURE_2D, movement.m_Texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, m_Window->GetWidth(), m_Window->GetHeight(), 0, GL_RG, GL_SHORT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, movement.m_Texture, 0);

		glCheckError();

		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Window->GetWidth(), m_Window->GetHeight());
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glCheckError();
		glCheckFrameBufferError();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		m_Framebuffers[FB_Movement] = movement;

	}

	void Renderer::SwitchFrameBuffer(EFramebuffers framebuffer, bool clear)
	{
		FrameBuffer& fb = m_Framebuffers[framebuffer];
		glBindFramebuffer(GL_FRAMEBUFFER, fb.m_FBO);
		if (clear) {
			glClearColor(fb.m_ClearColor.x, fb.m_ClearColor.y, fb.m_ClearColor.z, fb.m_ClearColor.w);
			glClear(fb.m_ClearFlags);
		}
	}

	void Renderer::UseFrameBufferTexture(EFramebuffers framebuffer) {
		unsigned int tex = m_Framebuffers[framebuffer].m_Texture;
		glBindTexture(GL_TEXTURE_2D, tex);
	}

	void Renderer::DrawFramebufferContent(EFramebuffers framebuffer)
	{
		unsigned int tex = m_Framebuffers[framebuffer].m_Texture;
		glBindVertexArray(m_ScreenQuadVAO);
		glBindTexture(GL_TEXTURE_2D, tex);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			Application::HandleKeyInput(key);
		}
	}

	void GlfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
		//Currently not in Use
	}

	void GlfwMousePosCallback(GLFWwindow* window, double xPos, double yPos) {
		//Currently not in Use
	}

}