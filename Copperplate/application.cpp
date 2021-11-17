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
		// Camera Movement
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

		// Debug Display Settings
		else if (key == GLFW_KEY_KP_1) {
			DisplaySettings::RenderContours = !DisplaySettings::RenderContours;
		}
		else if (key == GLFW_KEY_KP_2) {
			DisplaySettings::RenderSeedPoints = !DisplaySettings::RenderSeedPoints;
		}
		else if (key == GLFW_KEY_KP_3) {
			DisplaySettings::RenderScreenSpaceSeeds = !DisplaySettings::RenderScreenSpaceSeeds;
		}
		else if (key == GLFW_KEY_KP_4) {
			DisplaySettings::RenderHatching = !DisplaySettings::RenderHatching;
		}
		else if (key == GLFW_KEY_KP_5) {
			DisplaySettings::OnlyUpdateHatchLines = !DisplaySettings::OnlyUpdateHatchLines;
		}
		else if (key == GLFW_KEY_KP_9) {
			DisplaySettings::RenderCurrentDebug = !DisplaySettings::RenderCurrentDebug;
		}
		else if (key == GLFW_KEY_KP_ADD) {
			DisplaySettings::NumHatchingLines++;
			std::cout << "Drawing " << DisplaySettings::NumHatchingLines << "lines now \n";
		}
		else if (key == GLFW_KEY_KP_SUBTRACT) {
			DisplaySettings::NumHatchingLines--;
			std::cout << "Drawing " << DisplaySettings::NumHatchingLines << "lines now \n";
		}
		else if (key == GLFW_KEY_KP_MULTIPLY) {
			DisplaySettings::NumPointsPerHatch++;
		}
		else if (key == GLFW_KEY_KP_DIVIDE) {
			DisplaySettings::NumPointsPerHatch--;
		}
		else if (key == GLFW_KEY_PAGE_UP) {
			DisplaySettings::LineSeparationDistance *= 1.2f;
			std::cout << "Line Separation Distance is now " << DisplaySettings::LineSeparationDistance << std::endl;
		}
		else if (key == GLFW_KEY_PAGE_DOWN) {
			DisplaySettings::LineSeparationDistance /= 1.2f;
			std::cout << "Line Separation Distance is now " << DisplaySettings::LineSeparationDistance << std::endl;
		}
		else if (key == GLFW_KEY_HOME) {
			DisplaySettings::LineTestDistance *= 1.2f;
			std::cout << "Line Test Distance is now " << DisplaySettings::LineTestDistance << std::endl;
		}
		else if (key == GLFW_KEY_END) {
			DisplaySettings::LineTestDistance /= 1.2f;
			std::cout << "Line Test Distance is now " << DisplaySettings::LineTestDistance << std::endl;
		}
		else if (key == GLFW_KEY_1) {
			DisplaySettings::FramebufferToDisplay = EFramebuffers::FB_Default;
		}
		else if (key == GLFW_KEY_2) {
			DisplaySettings::FramebufferToDisplay = EFramebuffers::FB_Normals;
		}
		else if (key == GLFW_KEY_3) {
			DisplaySettings::FramebufferToDisplay = EFramebuffers::FB_Depth;
		}
		else if (key == GLFW_KEY_4) {
			DisplaySettings::FramebufferToDisplay = EFramebuffers::FB_Curvature;
		}
		else if (key == GLFW_KEY_5) {
			DisplaySettings::FramebufferToDisplay = EFramebuffers::FB_Movement;
		}
		else if (key == GLFW_KEY_F1) {
			DisplaySettings::HatchingDirection = EHatchingDirections::HD_LargestCurvature;
		}
		else if (key == GLFW_KEY_F2) {
			DisplaySettings::HatchingDirection = EHatchingDirections::HD_SmallestCurvature;
		}
		else if (key == GLFW_KEY_F3) {
			DisplaySettings::HatchingDirection = EHatchingDirections::HD_Normal;
		}
		else if (key == GLFW_KEY_F4) {
			DisplaySettings::HatchingDirection = EHatchingDirections::HD_Tangent;
		}

		// General Window input
		else if (key == GLFW_KEY_ESCAPE) {
			ShouldClose = true;
		}
	}

	void Application::Resize(int width, int height) {
		glViewport(0, 0, width, height);
		m_Scene->ViewportSizeChanged(width, height);
	}
		
}