#pragma once

#include "hatching.h"
#include "mesh.h"
#include "rendering.h"
#include "shader.h"

#include <map>

namespace Copperplate {

	const int SEEDS_PER_OBJECT = 5000;
	const int MAX_SEEDS_PER_FACE = 10;
	
	class SceneObject {
	public:

		SceneObject(std::string meshFile, Shared<Shader> shader);

		void Draw();

		void SetShader(Shared<Shader> shader);

		void DrawSeedPoints();

	private:

		SceneObject();

		Unique<Mesh> m_Mesh;
		Shared<Shader> m_Shader;
		glm::mat4 m_Transform;
		std::vector<SeedPoint> m_SeedPoints;

		unsigned int m_SeedsVAO;
		unsigned int m_SeedsVertexBuffer;

	};

	class Scene {
	public:

		Scene(Shared<Window> window);

		void Draw();

		void MoveCamera(float x, float y, float z);
		void ViewportSizeChanged(int newWidth, int newHeight);

	private:

		void DrawObject(const Unique<SceneObject>& object, const std::string& shader);
		void DrawFlatColor(const Unique<SceneObject>& object, glm::vec3 color);
		void DrawContours(const Unique<SceneObject>& object);
		void DrawSeedPoints(const Unique<SceneObject>& object, glm::vec3 color);
		void DrawFramebufferContent(const std::string& framebuffer);

		Unique<Renderer> m_Renderer;
		Unique<Camera> m_Camera;
		std::map<std::string, Shared<Shader>> m_Shaders;
		std::vector<Unique<SceneObject>> m_SceneObjects;

	};
}