#pragma once

#include "scene.h"
#include "core.h"

namespace Copperplate {

	struct OutputSeed {
		glm::vec2 pos;
		float importance;
		int keep;
	};

	//SCENEOBJECT IMPLEMENTATION
	SceneObject::SceneObject(std::string meshFile, Shared<Shader> shader) {
		m_Mesh = MeshCreator::ImportMesh(meshFile);
		m_Shader = shader;
		m_Transform = glm::mat4(1.0f);

		//Create Seed Points for Hatching Strokes
		m_SeedPoints = std::vector<SeedPoint>();
		m_SeedPoints.reserve(SEEDS_PER_OBJECT);
		CreateSeedPoints(m_SeedPoints, *m_Mesh, SEEDS_PER_OBJECT, MAX_SEEDS_PER_FACE);

		m_ScreenSpaceSeeds = std::vector<ScreenSpaceSeed>();
		m_ScreenSpaceSeeds.reserve(m_SeedPoints.size());

		std::vector<float> seedsData;
		seedsData.reserve(4 * m_SeedPoints.size());
		for (SeedPoint sp : m_SeedPoints) {
			seedsData.push_back(sp.m_Pos.x);
			seedsData.push_back(sp.m_Pos.y);
			seedsData.push_back(sp.m_Pos.z);
			seedsData.push_back(sp.m_Importance);
		}

		//Setup OpenGL Buffers
		glGenVertexArrays(1, &m_SeedsVAO);
		glGenBuffers(1, &m_SeedsVertexBuffer);

		glBindVertexArray(m_SeedsVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_SeedsVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, seedsData.size() * sizeof(float), seedsData.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(3 * sizeof(float)));
		glEnableVertexAttribArray(0);

		glCheckError();

		//SSBO for Compute Shader output
		int numOutputs = (((m_SeedPoints.size() - 1) / COMPUTE_GROUPSIZE) + 1) * COMPUTE_GROUPSIZE;
		
		glGenBuffers(1, &m_SeedsSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SeedsSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numOutputs * sizeof(struct OutputSeed), NULL, GL_STREAM_READ);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		//Screen Space Seed Points
		glGenVertexArrays(1, &m_ScreenSeedsVAO);
		glGenBuffers(1, &m_ScreenSeedsVBO);

		glBindVertexArray(m_ScreenSeedsVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_ScreenSeedsVBO);
		glBufferData(GL_ARRAY_BUFFER, m_SeedPoints.size() * 3 * sizeof(float), NULL, GL_STREAM_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);

		glCheckError();
	}

	void SceneObject::Draw() {
		m_Shader->SetMat4("model", m_Transform);
		m_Shader->SetMat4("modelInvTrans", glm::transpose(glm::inverse(m_Transform)));
		glCheckError();
		m_Shader->Use();
		m_Mesh->Draw();
	}

	void SceneObject::SetShader(Shared<Shader> shader)	{
		m_Shader = shader;
	}

	void SceneObject::DrawSeedPoints() {
		m_Shader->SetMat4("model", m_Transform);
		glCheckError();
		m_Shader->Use();
		glBindVertexArray(m_SeedsVAO);
		glDrawArrays(GL_POINTS, 0, m_SeedPoints.size());
	}

	void SceneObject::TransformSeedPoints(Shared<ComputeShader> shader) {		
		shader->SetMat4("model", m_Transform);
		shader->SetFloat("numSeeds", (float)m_SeedPoints.size());
		shader->UpdateUniforms();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_SeedsVertexBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_SeedsSSBO);

		int numGroups = ((m_SeedPoints.size() - 1) / COMPUTE_GROUPSIZE) + 1;
		shader->Dispatch(numGroups, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		int numOutputs = m_SeedPoints.size();
		OutputSeed* outputs{ new OutputSeed[numOutputs]{} };
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SeedsSSBO);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, numOutputs * sizeof(struct OutputSeed), outputs);
		
		m_ScreenSpaceSeeds.clear();
		for (int i = 0; i < numOutputs; i++) {
			if (outputs[i].keep) {
				ScreenSpaceSeed seed = { outputs[i].pos, outputs[i].importance };
				m_ScreenSpaceSeeds.push_back(seed);
			}
		}
		delete[] outputs;
		//std::cout << m_ScreenSpaceSeeds.size() << "Screen Space Seeds \n";
	}

	void SceneObject::DrawScreenSeeds()	{
		m_Shader->Use();
		glBindVertexArray(m_ScreenSeedsVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_ScreenSeedsVBO);
		glBufferData(GL_ARRAY_BUFFER, m_SeedPoints.size() * 3 * sizeof(float), m_ScreenSpaceSeeds.data(), GL_STREAM_DRAW);
		glDrawArrays(GL_POINTS, 0, m_ScreenSpaceSeeds.size());
	}

	std::vector<ScreenSpaceSeed>& SceneObject::GetScreenSeeds()	{
		return m_ScreenSpaceSeeds;
	}

	void SceneObject::Move(glm::vec3 translation)
	{
		m_Transform[3] += glm::vec4(translation, 0.0f);
	}
		
	//SCENE IMPLEMENTATION
	Scene::Scene(Shared<Window> window) {
		m_Camera = CreateUnique<Camera>(window->GetWidth(), window->GetHeight());
		m_Renderer = CreateUnique<Renderer>(window);
		m_Hatching = CreateUnique<Hatching>(window->GetWidth(), window->GetHeight());

		//Setup the shaders
		m_Shaders = std::map<EShaders, Shared<Shader>>();
		m_ComputeShaders = std::map<EShaders, Shared<ComputeShader>>();
		CreateShaders();
		
		//Create the Scene Objects
		m_SceneObjects = std::vector<Unique<SceneObject>>();
		//m_SceneObjects.push_back(CreateUnique<SceneObject>("SuzanneSmooth.obj", m_Shaders[SH_Contours]));
		//m_SceneObjects[0]->Move(glm::vec3(3.0f, 0.0f, 0.0f));
		m_SceneObjects.push_back(CreateUnique<SceneObject>("SuzanneSubdiv.obj", m_Shaders[SH_Contours]));

	}

	void Scene::Draw() {
		//Update Shader Uniforms
		UpdateUniforms();

		//Reset Hatching
		m_Hatching->ResetSeeds();
		m_Hatching->ResetHatching();

		//Fill Framebuffers
		//Normals
		m_Renderer->SwitchFrameBuffer(FB_Normals, true);
		glCheckError();
		for (auto& object : m_SceneObjects) {
			DrawObject(object, SH_Normals);
		}
		//if(DisplaySettings::RenderCurrentDebug)
		//	DrawFullScreen(SH_SphereNormals, FB_Default); 

		//Depth
		m_Renderer->SwitchFrameBuffer(FB_Depth, true); 
		glCheckError();
		for (auto& object : m_SceneObjects) {
			DrawObject(object, SH_Depth);
		}

		//Curvature
		m_Renderer->SwitchFrameBuffer(FB_Curvature, true);
		glCheckError();
		DrawFullScreen(SH_Curvature, FB_Normals);

		//Draw the Objects
		m_Renderer->SwitchFrameBuffer(FB_Default, true);
		glCheckError();
		for (auto& object : m_SceneObjects) {			
			DrawFlatColor(object, glm::vec3(1.0f));
			TransformSeedPoints(object);
			m_Hatching->AddSeeds(object->GetScreenSeeds());
			if (DisplaySettings::RenderContours) 
				DrawContours(object);
			if (DisplaySettings::RenderSeedPoints) 
				DrawSeedPoints(object, glm::vec3(0.89f, 0.37f, 0.27f), 8.0f);
			if (DisplaySettings::RenderScreenSpaceSeeds) 
				DrawScreenSeeds(object, glm::vec3(0.13f, 0.67f, 0.27f), 4.0f); 
		}
		m_Hatching->CreateHatchingLines();
		if (DisplaySettings::RenderHatching) 
			DrawHatchingLines(SH_HatchingLines, glm::vec3(0.16f, 0.37f, 0.74f));
		
		if (DisplaySettings::FramebufferToDisplay != EFramebuffers::FB_Default)
			DrawFramebufferContent(DisplaySettings::FramebufferToDisplay);
	}

	void Scene::MoveCamera(float x, float y, float z) {
		m_Camera->Move(x, y, z);
	}

	void Scene::ViewportSizeChanged(int newWidth, int newHeight) {
		m_Camera->SetViewportSize(newWidth, newHeight);
	}

	void Scene::CreateShaders() {
		Shared<Shader> flatColor = CreateShared<Shader>(ST_VertFrag, "shaders/flatcolor.vert", nullptr, "shaders/flatcolor.frag");
		m_Shaders[SH_Flatcolor] = flatColor;

		Shared<Shader> contours = CreateShared<Shader>(ST_VertGeomFrag, "shaders/contours.vert", "shaders/contours.geom", "shaders/contours.frag");
		m_Shaders[SH_Contours] = contours;

		Shared<Shader> displayTex = CreateShared<Shader>(ST_VertFrag, "shaders/displaytex.vert", nullptr, "shaders/displaytex.frag");
		m_Shaders[SH_DisplayTex] = displayTex;

		Shared<Shader> displayTexAlpha = CreateShared<Shader>(ST_VertFrag, "shaders/displaytexalpha.vert", nullptr, "shaders/displaytexalpha.frag");
		m_Shaders[SH_DisplayTexAlpha] = displayTexAlpha;

		Shared<Shader> normals = CreateShared<Shader>(ST_VertFrag, "shaders/normals.vert", nullptr, "shaders/normals.frag");
		m_Shaders[SH_Normals] = normals;

		Shared<Shader> depth = CreateShared<Shader>(ST_VertFrag, "shaders/depth.vert", nullptr, "shaders/depth.frag");
		m_Shaders[SH_Depth] = depth;

		Shared<Shader> curvature = CreateShared<Shader>(ST_VertFrag, "shaders/curvature.vert", nullptr, "shaders/curvature.frag");
		m_Shaders[SH_Curvature] = curvature;

		Shared<ComputeShader> transformSeeds = CreateShared<ComputeShader>("shaders/transformseeds.comp");
		m_ComputeShaders[SH_TransformSeeds] = transformSeeds;

		Shared<Shader> screenPoints = CreateShared<Shader>(ST_VertFrag, "shaders/screenpoints.vert", nullptr, "shaders/screenpoints.frag");
		m_Shaders[SH_Screenpoints] = screenPoints;

		Shared<Shader> hatchingLines = CreateShared<Shader>(ST_VertFrag, "shaders/hatchinglines.vert", nullptr, "shaders/hatchinglines.frag");
		m_Shaders[SH_HatchingLines] = hatchingLines;

		Shared<Shader> sphereNormals = CreateShared<Shader>(ST_VertFrag, "shaders/spherenormals.vert", nullptr, "shaders/spherenormals.frag");
		m_Shaders[SH_SphereNormals] = sphereNormals;
	}

	void Scene::UpdateUniforms() {
		m_Shaders[SH_Flatcolor]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders[SH_Flatcolor]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		m_Shaders[SH_Contours]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders[SH_Contours]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		m_Shaders[SH_Contours]->SetVec3("viewDirection", m_Camera->GetForwardVector());
		m_Shaders[SH_Normals]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders[SH_Normals]->SetMat4("viewInvTrans", glm::transpose(glm::inverse(m_Camera->GetViewMatrix())));
		m_Shaders[SH_Normals]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		m_Shaders[SH_Normals]->SetFloat("zMin", Z_MIN);
		m_Shaders[SH_Normals]->SetFloat("zMax", Z_MAX);
		m_Shaders[SH_Depth]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders[SH_Depth]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		m_Shaders[SH_Curvature]->SetFloat("sigma", 2.6f);
		m_ComputeShaders[SH_TransformSeeds]->SetMat4("view", m_Camera->GetViewMatrix());
		m_ComputeShaders[SH_TransformSeeds]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		glCheckError();
	}

	void Scene::DrawObject(const Unique<SceneObject>& object, EShaders shader) {
		object->SetShader(m_Shaders[shader]);
		glEnable(GL_DEPTH_TEST);
		object->Draw();
	}

	void Scene::DrawFlatColor(const Unique<SceneObject>& object, glm::vec3 color) {
		object->SetShader(m_Shaders[SH_Flatcolor]);
		m_Shaders[SH_Flatcolor]->SetVec3("color", color);
		glEnable(GL_DEPTH_TEST);
		object->Draw();
	}

	void Scene::DrawContours(const Unique<SceneObject>& object) {
		object->SetShader(m_Shaders[SH_Contours]);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glLineWidth(2.0f);
		object->Draw();
	}

	void Scene::DrawSeedPoints(const Unique<SceneObject>& object, glm::vec3 color, float pointSize) {
		object->SetShader(m_Shaders[SH_Flatcolor]);
		m_Shaders[SH_Flatcolor]->SetVec3("color", color);
		glEnable(GL_DEPTH_TEST);
		glPointSize(pointSize);
		object->DrawSeedPoints();
	}

	void Scene::TransformSeedPoints(const Unique<SceneObject>& object)
	{		
		m_ComputeShaders[SH_TransformSeeds]->Use();
		m_Renderer->UseFrameBufferTexture(FB_Depth); 
		object->TransformSeedPoints(m_ComputeShaders[SH_TransformSeeds]);
	}

	void Scene::DrawScreenSeeds(const Unique<SceneObject>& object, glm::vec3 color, float pointSize)	{
		object->SetShader(m_Shaders[SH_Screenpoints]);
		m_Shaders[SH_Screenpoints]->SetVec3("color", color);
		glPointSize(pointSize);
		object->DrawScreenSeeds();
	}

	void Scene::DrawFramebufferContent(EFramebuffers framebuffer) {
		m_Shaders[SH_DisplayTex]->Use();
		glDisable(GL_DEPTH_TEST);
		m_Renderer->DrawFramebufferContent(framebuffer);
	}

	void Scene::DrawFramebufferAlpha(EFramebuffers framebuffer) {
		m_Shaders[SH_DisplayTexAlpha]->Use();
		glDisable(GL_DEPTH_TEST);
		m_Renderer->DrawFramebufferContent(framebuffer);
	}

	void Scene::DrawFullScreen(EShaders shader, EFramebuffers sourceTex) {
		m_Shaders[shader]->Use();
		glDisable(GL_DEPTH_TEST);
		m_Renderer->DrawFramebufferContent(sourceTex);
	}

	void Scene::DrawHatchingLines(EShaders shader, glm::vec3 color) {
		m_Shaders[shader]->SetVec3("color", color);
		m_Shaders[shader]->Use();
		glDisable(GL_DEPTH_TEST);
		glLineWidth(2.0f);
		m_Hatching->DrawHatchingLines();
	}

}