#pragma once

#include "application.h"

#include <glm\gtx\string_cast.hpp>

namespace Copperplate {

	// Definitions so the Compiler knows where to put these Variables
	bool Application::ShouldClose = false;
	Shared<Window> Application::m_Window;
	Unique<Scene> Application::m_Scene;

	void Application::Init() {
		m_Window = CreateShared<Window>();
		m_Scene = CreateUnique<Scene>(m_Window);

		glClearColor(0.89f, 0.87f, 0.53f, 1.0f);
		glEnable(GL_DEPTH_TEST);
		
		ShouldClose = false;
	}

	void Application::Run() {
		while (!ShouldClose) {
			// Poll and handle Input
			glfwPollEvents();

			//Render
			glEnable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			m_Scene->Draw();
			
			m_Window->SwapBuffers();
		}
	}

	void Application::HandleKeyInput(int key) {
		if (key == GLFW_KEY_E) {
			m_Scene->MoveCamera(0.0f, 0.0f, -0.1f);
		}
		else if (key == GLFW_KEY_Q) {
			m_Scene->MoveCamera(0.0f, 0.0f, 0.1f);
		}
		else if (key == GLFW_KEY_W) {
			m_Scene->MoveCamera(0.0f, 0.03f, 0.0f);
		}
		else if (key == GLFW_KEY_S) {
			m_Scene->MoveCamera(0.0f, -0.03f, 0.0f);
		}
		else if (key == GLFW_KEY_A) {
			m_Scene->MoveCamera(0.03f, 0.0f, 0.0f);
		}
		else if (key == GLFW_KEY_D) {
			m_Scene->MoveCamera(-0.03f, 0.0f, 0.0f);
		}
		else if (key == GLFW_KEY_ESCAPE) {
			ShouldClose = true;
		}
	}

	void Application::Resize(int width, int height) {
		glViewport(0, 0, width, height);
		m_Scene->ViewportSizeChanged(width, height);
	}
		
}