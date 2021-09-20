#pragma once

#include "application.h"
#include "rendering.h"
#include <glm\gtx\string_cast.hpp>

namespace Copperplate {

	// Definitions so the Compiler knows where to put these Variables
	bool Application::ShouldClose = false;
	std::unique_ptr<Window> Application::m_Window;
	std::unique_ptr<Camera> Application::m_Camera;
	std::unique_ptr<Shader> Application::m_TestShader;
	std::unique_ptr<Mesh> Application::m_TestMesh;

	void Application::Init() {
		m_Window = std::make_unique<Window>();
		m_Camera = std::make_unique<Camera>(m_Window->GetWidth(), m_Window->GetHeight());

		glClearColor(0.2f, 0.8f, 0.3f, 1.0f);

		//m_TestShader = std::make_unique<Shader>("shaders/flatcolor.vert", "shaders/flatcolor.frag");
		m_TestShader = std::make_unique<Shader>("shaders/flatcolor.vert", "shaders/lines.geom", "shaders/flatcolor.frag");
		glCheckError();

		m_TestMesh = MeshCreator::CreateTestMesh();
		glCheckError();

		//TESTING
		m_TestMesh = MeshCreator::ImportMesh("SuzanneSmooth.obj"); 

		ShouldClose = false;
	}

	void Application::Run() {
		while (!ShouldClose) {
			// Poll and handle Input
			glfwPollEvents();

			//TEMPORARY: move Camera
			//m_Camera->Move(0.02f, 0.0f, 0.0f);

			//Render
			glClear(GL_COLOR_BUFFER_BIT);
			m_TestShader->Use();
			m_TestShader->SetMat4("view", m_Camera->GetViewMatrix());
			m_TestShader->SetMat4("projection", m_Camera->GetProjectionMatrix());

			//Testing
			glm::vec3 viewDir = m_Camera->GetForwardVector(); 
			m_TestShader->SetVec3("viewDirection", viewDir);

			glCheckError();  
			m_TestMesh->Draw(m_TestShader->GetId());
			glCheckError();

			m_Window->SwapBuffers();
		}
	}

	void Application::HandleKeyInput(int key) {
		if (key == GLFW_KEY_E) {
			m_Camera->Move(0.0f, 0.0f, -0.1f);
		}
		else if (key == GLFW_KEY_Q) {
			m_Camera->Move(0.0f, 0.0f, 0.1f);
		}
		else if (key == GLFW_KEY_W) {
			m_Camera->Move(0.0f, 0.03f, 0.0f);
		}
		else if (key == GLFW_KEY_S) {
			m_Camera->Move(0.0f, -0.03f, 0.0f);
		}
		else if (key == GLFW_KEY_A) {
			m_Camera->Move(0.03f, 0.0f, 0.0f);
		}
		else if (key == GLFW_KEY_D) {
			m_Camera->Move(-0.03f, 0.0f, 0.0f);
		}
		else if (key == GLFW_KEY_ESCAPE) {
			ShouldClose = true;
		}
	}

	void Application::Resize(int width, int height) {
		glViewport(0, 0, width, height);
		m_Camera->SetViewportSize((float)width, (float)height);
	}
		
}

GLenum glCheckError_(const char* file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}