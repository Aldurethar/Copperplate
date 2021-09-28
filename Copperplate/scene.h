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

		SceneObject(std::string meshFile, std::shared_ptr<Shader> shader);

		void Draw();

		void SetShader(std::shared_ptr<Shader> shader);

		void DrawSeedPoints();

	private:

		SceneObject();

		std::unique_ptr<Mesh> m_Mesh;
		std::shared_ptr<Shader> m_Shader;
		glm::mat4 m_Transform;
		std::vector<SeedPoint> m_SeedPoints;

		unsigned int m_SeedsVAO;
		unsigned int m_SeedsVertexBuffer;

	};

	class Scene {
	public:

		Scene(int viewportWidth, int viewportHeight);

		void Draw();

		void MoveCamera(float x, float y, float z);
		void ViewportSizeChanged(int newWidth, int newHeight);

	private:

		std::unique_ptr<Camera> m_Camera;
		std::map<std::string, std::shared_ptr<Shader>> m_Shaders;
		std::vector<std::unique_ptr<SceneObject>> m_SceneObjects;

	};
}