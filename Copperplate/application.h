#pragma once

#include "scene.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>


namespace Copperplate {

	class Window;
	class Scene;

	class Application {
	public:
		static void Init();
		static void Run();

		static void HandleKeyInput(int key);
		static void Resize(int width, int height);

		static bool ShouldClose;

	private:
		static std::unique_ptr<Window> m_Window;
		static std::unique_ptr<Scene> m_Scene;
		/*static std::unique_ptr<Camera> m_Camera;
		static std::unique_ptr<Shader> m_TestShader;
		static std::unique_ptr<Mesh> m_TestMesh;*/
	};
	
}