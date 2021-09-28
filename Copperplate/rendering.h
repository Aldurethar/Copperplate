#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/matrix_float4x4.hpp>

namespace Copperplate {

	//forward declarations

	// WINDOW CLASS
	class Window {
	public:
		Window();
		~Window();

		void SwapBuffers();

		int GetWidth();
		int GetHeight();
				
	private:
		GLFWwindow* m_Window;
	};

	// CAMERA CLASS
	class Camera {
	public:
	
		Camera(int width, int height);

		void SetPosition(float azimuth, float height, float zoom);
		void Move(float addAzimuth, float addHeight, float addZoom);
		void SetViewportSize(int width, int height);

		glm::mat4 GetViewMatrix();		
		glm::mat4 GetProjectionMatrix();
		glm::vec3 GetForwardVector();
		
	private:	
		float m_Azimuth;
		float m_Height;
		float m_Zoom;
		glm::vec3 m_Position;
		glm::vec3 m_Forward;
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;

		void CalculateViewMatrix();
	
	};

	// Callback Functions for the window
	void GlfwErrorCallback(int error, const char* description);

	void GlfwFramebufferSizeCallback(GLFWwindow* window, int width, int height);

	void GlfwCloseWindowCallback(GLFWwindow* window);

	void GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	void GlfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

	void GlfwMousePosCallback(GLFWwindow* window, double xPos, double yPos);
}