#pragma once

#include "core.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/matrix_float4x4.hpp>

#include <map>
#include <string>

namespace Copperplate {	

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

		void Update();
		void SetPosition(float azimuth, float height, float zoom);
		void Move(float addAzimuth, float addHeight, float addZoom);
		void SetViewportSize(int width, int height);

		glm::mat4 GetViewMatrix();		
		glm::mat4 GetProjectionMatrix();
		glm::mat4 GetPrevViewMatrix();
		glm::vec3 GetForwardVector();
		
	private:	
		float m_Azimuth;
		float m_Height;
		float m_Zoom;
		glm::vec3 m_Position;
		glm::vec3 m_Forward;
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_PrevViewMatrix;

		void CalculateViewMatrix();
	
	};

	struct FrameBuffer {
		unsigned int m_FBO;
		unsigned int m_Texture;
		glm::vec4 m_ClearColor;
		unsigned int m_ClearFlags;
	};

	enum EFramebuffers {
		FB_Default,
		FB_Normals,
		FB_Depth,
		FB_Curvature,
		FB_Movement,
		FB_Diffuse,
		FB_ShadingGradient,
	};

	//RENDERER CLASS
	class Renderer {
	public:

		Renderer(Shared<Window> window);

		void SwitchFrameBuffer(EFramebuffers framebuffer, bool clear);

		void UseFrameBufferTexture(EFramebuffers framebuffer);

		void DrawFramebufferContent(EFramebuffers framebuffer);

		void SaveCurrFramebufferContent(const std::string& path);
		
		void DrawTexFullscreen(unsigned int texture);

	private:

		Shared<Window> m_Window;

		std::map<EFramebuffers, FrameBuffer> m_Framebuffers;

		unsigned int m_ScreenQuadVAO;

	};

	enum EHatchingDirections {
		HD_LargestCurvature,
		HD_SmallestCurvature,
		HD_Normal,
		HD_Tangent,
		HD_ShadeGradient,
		HD_ShadeNormal,
	};

	//DISPLAY SETTINGS CLASS
	class DisplaySettings {
	public:
		static bool RenderSeedPoints;
		static bool RenderScreenSpaceSeeds;
		static bool RenderContours;
		static bool RenderHatching;
		static bool RegenerateHatching;
		static bool RenderHatchingCollision;
		static bool RenderCurrentDebug;
		static int NumHatchingLines;
		static int NumPointsPerHatch;
		static EHatchingDirections HatchingDirection;
		static EFramebuffers FramebufferToDisplay;
		static bool RecordScreenShot;
		static bool RecordVideo;
		static int RecordFrameCount;
	};

	// Callback Functions for the window
	void GlfwErrorCallback(int error, const char* description);

	void GlfwFramebufferSizeCallback(GLFWwindow* window, int width, int height);

	void GlfwCloseWindowCallback(GLFWwindow* window);

	void GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	void GlfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

	void GlfwMousePosCallback(GLFWwindow* window, double xPos, double yPos);
}