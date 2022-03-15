#pragma once

#include "application.h"
#include "statistics.h"

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
			Statistics::Get().newFrame();
			// Poll and handle Input
			glfwPollEvents();

			m_Scene->Update();

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
			DisplaySettings::RegenerateHatching = !DisplaySettings::RegenerateHatching;
		}
		else if (key == GLFW_KEY_KP_6) {
			DisplaySettings::RenderHatchingCollision = !DisplaySettings::RenderHatchingCollision;
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
			
		}
		else if (key == GLFW_KEY_PAGE_DOWN) {
			
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
		else if (key == GLFW_KEY_6) {
			DisplaySettings::FramebufferToDisplay = EFramebuffers::FB_Diffuse;
		}
		else if (key == GLFW_KEY_7) {
			DisplaySettings::FramebufferToDisplay = EFramebuffers::FB_ShadingGradient;
		}
		else if (key == GLFW_KEY_F1) {
			m_Scene->SetLayer1Direction(HD_LargestCurvature);
			//DisplaySettings::HatchingDirection = EHatchingDirections::HD_LargestCurvature;
		}
		else if (key == GLFW_KEY_F2) {
			m_Scene->SetLayer1Direction(HD_SmallestCurvature);
			//DisplaySettings::HatchingDirection = EHatchingDirections::HD_SmallestCurvature;
		}
		else if (key == GLFW_KEY_F3) {
			m_Scene->SetLayer1Direction(HD_Normal);
			//DisplaySettings::HatchingDirection = EHatchingDirections::HD_Normal;
		}
		else if (key == GLFW_KEY_F4) {
			m_Scene->SetLayer1Direction(HD_Tangent);
			//DisplaySettings::HatchingDirection = EHatchingDirections::HD_Tangent;
		}
		else if (key == GLFW_KEY_F5) {
			m_Scene->SetLayer1Direction(HD_ShadeGradient);
			//DisplaySettings::HatchingDirection = EHatchingDirections::HD_ShadeGradient;
		}
		else if (key == GLFW_KEY_F6) {
			m_Scene->SetLayer1Direction(HD_ShadeNormal);
			//DisplaySettings::HatchingDirection = EHatchingDirections::HD_ShadeNormal;
		}

		// General Window input
		else if (key == GLFW_KEY_F7) {
			m_Scene->ExampleAnimation();
		}
		else if (key == GLFW_KEY_F8) {
			Statistics::Get().printMeanValues();
		}
		else if (key == GLFW_KEY_F9) {
			Statistics::Get().printLastFrame();
		}
		else if (key == GLFW_KEY_F10) {
			if (!DisplaySettings::RecordVideo) {
				DisplaySettings::RecordVideo = true;
				DisplaySettings::RecordFrameCount = 0;
			}
			else {
				DisplaySettings::RecordVideo = false;
			}
		}
		else if (key == GLFW_KEY_F11) {
		DisplaySettings::RecordScreenShot = true;
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