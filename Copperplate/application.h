#pragma once

//#include "rendering.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader.h"
#include "mesh.h"

namespace Copperplate {

	class Window;
	class Camera;

	class Application {
	public:
		static void Init();
		static void Run();

		static void HandleKeyInput(int key);
		static void Resize(int width, int height);

		static bool ShouldClose;

	private:
		static std::unique_ptr<Window> m_Window;
		static std::unique_ptr<Camera> m_Camera;
		static std::unique_ptr<Shader> m_TestShader;
		static std::unique_ptr<Mesh> m_TestMesh;
	};
	
}

GLenum glCheckError_(const char* file, int line);
#define glCheckError() glCheckError_(__FILE__, __LINE__)
