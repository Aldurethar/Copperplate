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
		static Shared<Window> m_Window;
		static Unique<Scene> m_Scene;
	};
	
}