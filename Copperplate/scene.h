#pragma once

#include "hatching.h"
#include "mesh.h"
#include "rendering.h"
#include "shader.h"

#include <map>

namespace Copperplate {

	const int SEEDS_PER_OBJECT = 16000;
	const float Z_MIN = 0.1f;
	const float Z_MAX = 50.0f;
	const int COMPUTE_GROUPSIZE = 128;
	const std::string SCREENSHOT_PATH = "screenshots/";
	const std::string VIDEO_PATH = "video/";
	
	class SceneObject {
	public:

		SceneObject(std::string meshFile, Shared<Shader> shader, int id, Shared<SceneObject> parent, Shared<Hatching> hatching);

		void Draw();

		void Update();

		void ExtractContours();
		void DrawContours();

		void SetShader(Shared<Shader> shader);

		void DrawSeedPoints();

		void TransformSeedPoints(Shared<ComputeShader> shader);
		
		int getId();
		std::vector<glm::vec2>& GetContourSegments();

		void Move(glm::vec3 translation);
		void SetPos(glm::vec3 position);

		void Rotate(glm::vec3 axis, float angle);
		void SetRot(glm::vec3 axis, float angle);

		glm::mat4 getTransform();

	private:

		SceneObject();

		Unique<Mesh> m_Mesh;
		Shared<Shader> m_Shader;
		Shared<Hatching> m_Hatching;
		Shared<SceneObject> m_Parent;
		int m_Id;
		glm::mat4 m_Transform;
		glm::mat4 m_PrevTransform;
		glm::mat4 m_LocalTransform;
		std::vector<SeedPoint> m_SeedPoints;
		std::vector<glm::vec2> m_ContourSegments;
		
		unsigned int m_SeedsVAO;
		unsigned int m_SeedsVertexBuffer;
		unsigned int m_SeedsSSBO;
		unsigned int m_ContoursFeedbackBuffer;
		unsigned int m_ContoursVAO;
		unsigned int m_ContoursVBO;

	};
	
	enum EShaders {
		SH_Flatcolor,
		SH_ExtractContours,
		SH_Contours,
		SH_DisplayTex,
		SH_DisplayTexAlpha,
		SH_Normals,
		SH_Depth,
		SH_Curvature,
		SH_TransformSeeds,
		SH_Screenpoints,
		SH_HatchingLines,
		SH_SphereNormals,
		SH_Movement,
		SH_Diffuse,
		SH_ShadingGradient,
		SH_Hatching,
	};

	class Scene {
	public:

		Scene(Shared<Window> window);

		void Update();
		void Draw();

		void MoveCamera(float x, float y, float z);
		void ViewportSizeChanged(int newWidth, int newHeight);

		//DEBUG
		void SetLayer1Direction(EHatchingDirections newDir);
		void CreateDensityHistogram();
		void ExampleAnimation();

	private:

		void CreateShaders();
		void UpdateUniforms();

		void DrawObject(const Shared<SceneObject>& object, EShaders shader);
		void DrawFlatColor(const Shared<SceneObject>& object, glm::vec3 color);
		void ExtractContours(const Shared<SceneObject>& object);
		void DrawContours(const Shared<SceneObject>& object,  glm::vec3 color);
		void DrawSeedPoints(const Shared<SceneObject>& object, glm::vec3 color, float pointSize);
		void TransformSeedPoints(const Shared<SceneObject>& object);
		void DrawFramebufferContent(EFramebuffers framebuffer);
		void DrawFramebufferAlpha(EFramebuffers framebuffer);
		void DrawFullScreen(EShaders shader, EFramebuffers sourceTex);
		void DrawScreenSeeds(glm::vec3 color, float pointSize);
		void DrawHatchingLines(EShaders shader, glm::vec3 color);
		void DrawHatchingCollision(glm::vec3 color, float pointSize);
		void DrawHatching(EShaders shader, glm::vec3 color);
		
		unsigned int m_DebugTexture;
		unsigned int m_FrameNumber;

		Unique<Renderer> m_Renderer;
		Shared<Hatching> m_Hatching;
		Unique<Camera> m_Camera;
		glm::vec3 m_LightDir;
		std::map<EShaders, Shared<Shader>> m_Shaders;
		std::map<EShaders, Shared<ComputeShader>> m_ComputeShaders;
		std::vector<Shared<SceneObject>> m_SceneObjects;

	};
}