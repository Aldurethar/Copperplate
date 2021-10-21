#pragma once

#include "hatching.h"
#include "mesh.h"
#include "rendering.h"
#include "shader.h"

#include <map>

namespace Copperplate {

	const int SEEDS_PER_OBJECT = 2000;// 5000;
	const int MAX_SEEDS_PER_FACE = 20;
	const float Z_MIN = 0.1f;
	const float Z_MAX = 50.0f;
	const int COMPUTE_GROUPSIZE = 128;
	
	class SceneObject {
	public:

		SceneObject(std::string meshFile, Shared<Shader> shader);

		void Draw();

		void SetShader(Shared<Shader> shader);

		void DrawSeedPoints();

		void TransformSeedPoints(Shared<ComputeShader> shader);

		void DrawScreenSeeds();

		std::vector<ScreenSpaceSeed>& GetScreenSeeds();

		void Move(glm::vec3 translation);

	private:

		SceneObject();

		Unique<Mesh> m_Mesh;
		Shared<Shader> m_Shader;
		glm::mat4 m_Transform;
		std::vector<SeedPoint> m_SeedPoints;
		std::vector<ScreenSpaceSeed> m_ScreenSpaceSeeds;

		unsigned int m_SeedsVAO;
		unsigned int m_SeedsVertexBuffer;
		unsigned int m_SeedsSSBO;
		unsigned int m_ScreenSeedsVAO;
		unsigned int m_ScreenSeedsVBO;

	};
	
	enum EShaders {
		SH_Flatcolor,
		SH_Contours,
		SH_DisplayTex,
		SH_DisplayTexAlpha,
		SH_Normals,
		SH_Depth,
		SH_Curvature,
		SH_TransformSeeds,
		SH_Screenpoints,
		SH_HatchingLines,
	};

	class Scene {
	public:

		Scene(Shared<Window> window);

		void Draw();

		void MoveCamera(float x, float y, float z);
		void ViewportSizeChanged(int newWidth, int newHeight);

	private:

		void CreateShaders();
		void UpdateUniforms();

		void DrawObject(const Unique<SceneObject>& object, EShaders shader);
		void DrawFlatColor(const Unique<SceneObject>& object, glm::vec3 color);
		void DrawContours(const Unique<SceneObject>& object);
		void DrawSeedPoints(const Unique<SceneObject>& object, glm::vec3 color, float pointSize);
		void TransformSeedPoints(const Unique<SceneObject>& object);
		void DrawScreenSeeds(const Unique<SceneObject>& object, glm::vec3 color, float pointSize);
		void DrawFramebufferContent(EFramebuffers framebuffer);
		void DrawFramebufferAlpha(EFramebuffers framebuffer);
		void DrawFullScreen(EShaders shader, EFramebuffers sourceTex);
		void DrawHatchingLines(EShaders shader, glm::vec3 color);

		Unique<Renderer> m_Renderer;
		Unique<Hatching> m_Hatching;
		Unique<Camera> m_Camera;
		std::map<EShaders, Shared<Shader>> m_Shaders;
		std::map<EShaders, Shared<ComputeShader>> m_ComputeShaders;
		std::vector<Unique<SceneObject>> m_SceneObjects;

	};
}