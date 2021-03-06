#pragma once

#include "scene.h"
#include "core.h"
#include "utility.h"
#include "statistics.h"
#include <glm\gtx\transform.hpp>

namespace Copperplate {

	struct OutputSeed {
		glm::vec2 pos;		//8 Bytes, total 8
		float importance;	//4 Bytes, total 12
		float id;			//4 Bytes, total 16
		int keep;			//4 Bytes, total 20
		int padding1;		//4 bytes
		int padding2;		//4 bytes
		int padding3;		//4 bytes, total 32
	};

	//SCENEOBJECT IMPLEMENTATION
	SceneObject::SceneObject(std::string meshFile, Shared<Shader> shader, int id, Shared<SceneObject> parent, Shared<Hatching> hatching) {
		m_Mesh = MeshCreator::ImportMesh(meshFile);
		m_Shader = shader;
		m_Id = id;
		m_Hatching = hatching;
		m_Parent = parent;
		m_LocalTransform = glm::mat4(1.0f);
		if (m_Parent)
			m_Transform = m_Parent->getTransform();
		else
			m_Transform = glm::mat4(1.0f);
		m_PrevTransform = m_Transform;

		//Create Seed Points for Hatching Strokes
		m_SeedPoints = std::vector<SeedPoint>();
		m_SeedPoints.reserve(SEEDS_PER_OBJECT);
		m_Hatching->CreateSeedPoints(m_SeedPoints, *m_Mesh, m_Id, SEEDS_PER_OBJECT);

		m_ContourSegments = std::vector<glm::vec2>();
		m_ContourSegments.reserve(m_Mesh->GetFaces().size() * 3 * 2);

		std::vector<float> seedsData;
		seedsData.reserve(8 * m_SeedPoints.size());
		for (SeedPoint sp : m_SeedPoints) {
			seedsData.push_back(sp.m_Pos.x);
			seedsData.push_back(sp.m_Pos.y);
			seedsData.push_back(sp.m_Pos.z);
			seedsData.push_back(1.0f);	//padding
			seedsData.push_back(sp.m_Importance);
			seedsData.push_back(sp.m_Id);
			seedsData.push_back(1.0f);	//padding
			seedsData.push_back(1.0f);	//padding
		}

		//Setup OpenGL Buffers
		//Seed Points
		glGenVertexArrays(1, &m_SeedsVAO);
		glGenBuffers(1, &m_SeedsVertexBuffer);

		glBindVertexArray(m_SeedsVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_SeedsVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, seedsData.size() * sizeof(float), seedsData.data(), GL_STATIC_DRAW);

		int stride = 8 * sizeof(float);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(4 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(5 * sizeof(float)));
		glEnableVertexAttribArray(0);

		glCheckError();

		//SSBO for Compute Shader output
		int numOutputs = (((m_SeedPoints.size() - 1) / COMPUTE_GROUPSIZE) + 1) * COMPUTE_GROUPSIZE;
		
		glGenBuffers(1, &m_SeedsSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SeedsSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numOutputs * sizeof(struct OutputSeed), NULL, GL_DYNAMIC_READ);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		
		//Transform Feedback Buffer for Contour Segments
		m_Mesh->Bind();
		glGenBuffers(1, &m_ContoursFeedbackBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_ContoursFeedbackBuffer);
		glBufferData(GL_ARRAY_BUFFER, m_Mesh->GetFaces().size() * 3 * 2 * 2 * sizeof(float), nullptr, GL_DYNAMIC_READ);

		glCheckError();

		//Contour Vertex Data
		glGenVertexArrays(1, &m_ContoursVAO);
		glGenBuffers(1, &m_ContoursVBO);

		glBindVertexArray(m_ContoursVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_ContoursVBO);
		glBufferData(GL_ARRAY_BUFFER, m_Mesh->GetFaces().size() * 3 * 2 * 2 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid*)0);

		glCheckError();
	}

	void SceneObject::Draw() {
		m_Shader->SetMat4("model", m_Transform);
		m_Shader->SetMat4("modelInvTrans", glm::transpose(glm::inverse(m_Transform)));
		m_Shader->SetMat4("prevModel", m_PrevTransform);
		glCheckError();
		m_Shader->Use();
		m_Mesh->Draw();
	}

	void SceneObject::Update() {
		m_PrevTransform = m_Transform;
		if (m_Parent)
			m_Transform = m_LocalTransform * m_Parent->getTransform();
		else
			m_Transform = m_LocalTransform;
	}

	void SceneObject::ExtractContours() {
		unsigned int query;
		glGenQueries(1, &query);
		m_Shader->SetMat4("model", m_Transform);
		m_Shader->SetMat4("modelInvTrans", glm::transpose(glm::inverse(m_Transform)));
		glCheckError();
		m_Shader->Use();

		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_ContoursFeedbackBuffer);
		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);
		glBeginTransformFeedback(GL_LINES);
		glCheckError();
		m_Mesh->Draw();
		glEndTransformFeedback();
		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
		glFlush();
		glCheckError();

		unsigned int numLines;
		glGetQueryObjectuiv(query, GL_QUERY_RESULT, &numLines);

		glm::vec2* feedback{ new glm::vec2[numLines * 2]{} };

		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, numLines * 2 * 2 * sizeof(float), feedback);

		m_ContourSegments.clear();
		for (int i = 0; i < numLines; i++) {
			m_ContourSegments.push_back(feedback[2 * i]);
			m_ContourSegments.push_back(feedback[2 * i + 1]);
		}

		m_Hatching->AddContourCollision(m_ContourSegments);
		
		delete[] feedback;
	}

	void SceneObject::DrawContours() {
		m_Shader->Use();
		glBindVertexArray(m_ContoursVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_ContoursVBO);
		glBufferData(GL_ARRAY_BUFFER, m_ContourSegments.size() * 2 * sizeof(float), m_ContourSegments.data(), GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINES, 0, m_ContourSegments.size());
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
				
		for (int i = 0; i < numOutputs; i++) {
			OutputSeed oSeed = outputs[i];
			unsigned int id = (m_Id << 16) + (int)oSeed.id;
			m_Hatching->UpdateScreenSeed(id, oSeed.pos, oSeed.keep);
		}
		delete[] outputs;
	}
	
	int SceneObject::getId() {
		return m_Id;
	}
	
	std::vector<glm::vec2>& SceneObject::GetContourSegments() {
		return m_ContourSegments;
	}

	void SceneObject::Move(glm::vec3 translation) {
		m_LocalTransform[3] += glm::vec4(translation, 0.0f);
	}

	void SceneObject::SetPos(glm::vec3 position) {
		m_LocalTransform[3] = glm::vec4(position, 1.0f);
	}

	void SceneObject::Rotate(glm::vec3 axis, float angle) {
		m_LocalTransform = glm::rotate(m_LocalTransform, angle, axis);
	}

	void SceneObject::SetRot(glm::vec3 axis, float angle) {
		glm::vec4 translation = m_LocalTransform[3];
		m_LocalTransform = glm::rotate(angle, axis);
		m_LocalTransform[3] = translation;
	}
	
	glm::mat4 SceneObject::getTransform() {
		return m_Transform;
	}
		
	//SCENE IMPLEMENTATION
	Scene::Scene(Shared<Window> window) {
		m_Camera = CreateUnique<Camera>(window->GetWidth(), window->GetHeight());
		m_Renderer = CreateUnique<Renderer>(window);
		m_Hatching = CreateShared<Hatching>(window->GetWidth(), window->GetHeight());
		m_LightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, 0.0f));

		//Setup the shaders
		m_Shaders = std::map<EShaders, Shared<Shader>>();
		m_ComputeShaders = std::map<EShaders, Shared<ComputeShader>>();
		CreateShaders();
		
		//Create the Scene Objects
		int numObjects = 1;
		m_SceneObjects = std::vector<Shared<SceneObject>>();
		//Full Scene Version
		/*
		m_SceneObjects.push_back(CreateShared<SceneObject>("Plane.obj", m_Shaders[SH_Contours], numObjects, nullptr, m_Hatching));
		m_SceneObjects.back()->Move(glm::vec3(0.0f, -0.5f, -4.0f));
		numObjects++;
		m_SceneObjects.push_back(CreateShared<SceneObject>("BunnySmoothed.obj", m_Shaders[SH_Contours], numObjects, nullptr, m_Hatching));
		m_SceneObjects.back()->Move(glm::vec3(-1.0f, 0.0f, 0.0f));
		numObjects++;
		m_SceneObjects.push_back(CreateShared<SceneObject>("Teapot.obj", m_Shaders[SH_Contours], numObjects, nullptr, m_Hatching));
		m_SceneObjects.back()->Move(glm::vec3(1.0f, -0.5f, 0.3f));
		numObjects++;
		m_SceneObjects.push_back(CreateShared<SceneObject>("Blob.obj", m_Shaders[SH_Contours], numObjects, nullptr, m_Hatching));
		m_SceneObjects.back()->Move(glm::vec3(0.0f, 0.3f, -2.0f));
		numObjects++;*/
		//Single Objects
		//m_SceneObjects.push_back(CreateShared<SceneObject>("Blob.obj", m_Shaders[SH_Contours], numObjects, nullptr, m_Hatching));
		//m_SceneObjects.push_back(CreateShared<SceneObject>("Sphere.obj", m_Shaders[SH_Contours], numObjects, nullptr, m_Hatching));
		//m_SceneObjects.push_back(CreateShared<SceneObject>("Bunny.obj", m_Shaders[SH_Contours], numObjects, nullptr, m_Hatching));
		//numObjects++;
		//m_SceneObjects[0]->Move(glm::vec3(1.5f, 0.0f, 0.0f));
		//m_SceneObjects.push_back(CreateShared<SceneObject>("BunnySmoothed.obj", m_Shaders[SH_Contours], numObjects, nullptr, m_Hatching));
		//m_SceneObjects.push_back(CreateShared<SceneObject>("Teapot.obj", m_Shaders[SH_Contours], numObjects, nullptr, m_Hatching));
		//m_SceneObjects.push_back(CreateShared<SceneObject>("Plane.obj", m_Shaders[SH_Contours], numObjects, nullptr, m_Hatching));
		//m_SceneObjects[0]->Move(glm::vec3(0.0f, -0.5f, 0.0f));
		//m_SceneObjects.push_back(CreateShared<SceneObject>("LucyRotSub.obj", m_Shaders[SH_Contours], numObjects, nullptr, m_Hatching));
		//numObjects++;
		//m_SceneObjects.push_back(CreateShared<SceneObject>("SuzanneSubdiv.obj", m_Shaders[SH_Contours], numObjects, m_SceneObjects[0], m_Hatching));
		//m_SceneObjects[1]->Move(glm::vec3(0.0f, 1.0f, 0.0f));
		m_SceneObjects.push_back(CreateShared<SceneObject>("dragon.obj", m_Shaders[SH_Contours], numObjects, nullptr, m_Hatching));
		m_SceneObjects[0]->Move(glm::vec3(0.0f, 0.1f, 0.0f));
		numObjects++;
		

		//Load debug texture
		int width, height, channels;
		m_DebugTexture = loadTextureFile("Paper.png", width, height, channels);

	}

	void Scene::Update() {
		m_FrameNumber++;
		float amountMoved = sin((float)m_FrameNumber * 0.3f);
		//m_SceneObjects[0]->SetPos(glm::vec3(amountMoved, -0.5f, 0.0f));
		//m_SceneObjects[1]->SetRot(glm::vec3(0.0f, 0.0f, 1.0f), amountMoved);
	}

	void Scene::Draw() {
		// Update Scene Objects
		for (auto& object : m_SceneObjects) {
			object->Update();
		}

		//Update Shader Uniforms
		UpdateUniforms();
		m_Camera->Update();

		//Reset Hatching
		m_Hatching->ResetCollisions();

		//Fill Framebuffers
		//Normals
		m_Renderer->SwitchFrameBuffer(FB_Normals, true);
		glCheckError();
		for (auto& object : m_SceneObjects) {
			DrawObject(object, SH_Normals);
		}
		m_Hatching->GrabNormalData();
		//if(DisplaySettings::RenderCurrentDebug)
		//	DrawFullScreen(SH_SphereNormals, FB_Default); 

		//Depth
		m_Renderer->SwitchFrameBuffer(FB_Depth, true); 
		glCheckError();
		for (auto& object : m_SceneObjects) {
			DrawObject(object, SH_Depth);
		}

		//Movement
		m_Renderer->SwitchFrameBuffer(FB_Movement, true);
		glCheckError();
		for (auto& object : m_SceneObjects) {
			DrawObject(object, SH_Movement);
		}
		m_Hatching->GrabMovementData();

		//Curvature
		m_Renderer->SwitchFrameBuffer(FB_Curvature, true);
		glCheckError();
		DrawFullScreen(SH_Curvature, FB_Normals);
		m_Hatching->GrabCurvatureData();

		//Diffuse Shading
		m_Renderer->SwitchFrameBuffer(FB_Diffuse, true);
		glCheckError();
		for (auto& object : m_SceneObjects) {
			DrawObject(object, SH_Diffuse);
		}
		
		//Shading Gradient
		m_Renderer->SwitchFrameBuffer(FB_ShadingGradient, true);
		glCheckError();
		DrawFullScreen(SH_ShadingGradient, FB_Diffuse);
		m_Hatching->GrabGradientData();

		//DEBUG
		//m_Renderer->SwitchFrameBuffer(FB_Diffuse, true);
		//m_Shaders[SH_DisplayTex]->Use();
		//glDisable(GL_DEPTH_TEST);
		//m_Renderer->DrawTexFullscreen(m_DebugTexture);

		//Draw the Objects
		m_Renderer->SwitchFrameBuffer(FB_Default, true);
		glCheckError();
		m_Shaders[SH_DisplayTex]->Use();
		glDisable(GL_DEPTH_TEST);
		m_Renderer->DrawTexFullscreen(m_DebugTexture);
		for (auto& object : m_SceneObjects) {			
			//DrawFlatColor(object, glm::vec3(1.0f));
			ExtractContours(object);
			TransformSeedPoints(object);
			if (DisplaySettings::RenderContours) 
				DrawContours(object, glm::vec3(0.0f));
			if (DisplaySettings::RenderSeedPoints) 
				DrawSeedPoints(object, glm::vec3(0.89f, 0.37f, 0.27f), 4.0f);
		}

		m_Hatching->CreateHatchingLines();

		if (DisplaySettings::RenderScreenSpaceSeeds)
			DrawScreenSeeds(glm::vec3(0.13f, 0.67f, 0.27f), 4.0f);
		if (DisplaySettings::RenderHatching)
			//DrawHatchingLines(SH_HatchingLines, glm::vec3(0.16f, 0.37f, 0.74f));
			//glLineWidth(2.0f);
			//DrawHatching(SH_HatchingLines, glm::vec3(0.8f, 0.4f, 0.2f));
			DrawHatching(SH_Hatching, glm::vec3(0.0f));
		if (DisplaySettings::RenderHatchingCollision)
			DrawHatchingCollision(glm::vec3(0.9f, 0.3f, 0.7f), 3.0f);
		
		if (DisplaySettings::FramebufferToDisplay != EFramebuffers::FB_Default)
			DrawFramebufferContent(DisplaySettings::FramebufferToDisplay);

		if (DisplaySettings::RecordScreenShot) {
			DisplaySettings::RecordScreenShot = false;			
			std::string path = SCREENSHOT_PATH + currTimeToString() + ".png";
			m_Renderer->SaveCurrFramebufferContent(path);
			std::cout << "Saved Screenshot " << path << std::endl;
		}

		if (DisplaySettings::RecordVideo) {
			std::string path = VIDEO_PATH + std::to_string(DisplaySettings::RecordFrameCount) + ".png";
			m_Renderer->SaveCurrFramebufferContent(path);
			std::cout << "Saved Video Frame " << DisplaySettings::RecordFrameCount << std::endl;
			DisplaySettings::RecordFrameCount++;
		}

	}

	void Scene::MoveCamera(float x, float y, float z) {
		m_Camera->Move(x, y, z);
	}

	void Scene::ViewportSizeChanged(int newWidth, int newHeight) {
		m_Camera->SetViewportSize(newWidth, newHeight);
	}

	void Scene::SetLayer1Direction(EHatchingDirections newDir) {
		m_Hatching->SetLayer1Direction(newDir);
	}

	void Scene::CreateDensityHistogram() {
		m_Hatching->measureHatchingDensity(3000, 8.0f);
	}

	void Scene::ExampleAnimation() {
		static int frame = 0;
		if (frame >= 10 && frame < 40) {
			m_Camera->Move(-0.05f, 0.0f, 0.0f);
		}
		if (frame < 50) {
			Statistics::Get().printLastFrame();
			frame++;
		}
	}

	void Scene::CreateShaders() {
		Shared<Shader> flatColor = CreateShared<Shader>(ST_VertFrag, "shaders/flatcolor.vert", nullptr, "shaders/flatcolor.frag");
		m_Shaders[SH_Flatcolor] = flatColor;

		//Shared<Shader> contours = CreateShared<Shader>(ST_VertGeomFrag, "shaders/contours.vert", "shaders/contours.geom", "shaders/contours.frag", "screenPos");
		Shared<Shader> extractContours = CreateShared<Shader>(ST_VertGeom, "shaders/extractcontours.vert", "shaders/extractcontours.geom", nullptr, "screenPos");
		m_Shaders[SH_ExtractContours] = extractContours;

		Shared<Shader> contours = CreateShared<Shader>(ST_VertFrag, "shaders/contours.vert", nullptr, "shaders/contours.frag");
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

		Shared<Shader> movement = CreateShared<Shader>(ST_VertFrag, "shaders/movement.vert", nullptr, "shaders/movement.frag");
		m_Shaders[SH_Movement] = movement;

		Shared<Shader> diffuse = CreateShared<Shader>(ST_VertFrag, "shaders/diffuse.vert", nullptr, "shaders/diffuse.frag");
		m_Shaders[SH_Diffuse] = diffuse;

		Shared<Shader> shadingGrad = CreateShared<Shader>(ST_VertFrag, "shaders/shadingGradient.vert", nullptr, "shaders/shadingGradient.frag");
		m_Shaders[SH_ShadingGradient] = shadingGrad;

		Shared<Shader> hatching = CreateShared<Shader>(ST_VertGeomFrag, "shaders/hatching.vert", "shaders/hatching.geom", "shaders/hatching.frag");
		m_Shaders[SH_Hatching] = hatching;
	}

	void Scene::UpdateUniforms() {
		m_Shaders[SH_Flatcolor]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders[SH_Flatcolor]->SetMat4("projection", m_Camera->GetProjectionMatrix());

		m_Shaders[SH_ExtractContours]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders[SH_ExtractContours]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		m_Shaders[SH_ExtractContours]->SetVec3("viewDirection", m_Camera->GetForwardVector());

		m_Shaders[SH_Normals]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders[SH_Normals]->SetMat4("viewInvTrans", glm::transpose(glm::inverse(m_Camera->GetViewMatrix())));
		m_Shaders[SH_Normals]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		m_Shaders[SH_Normals]->SetFloat("zMin", Z_MIN);
		m_Shaders[SH_Normals]->SetFloat("zMax", Z_MAX);

		m_Shaders[SH_Depth]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders[SH_Depth]->SetMat4("projection", m_Camera->GetProjectionMatrix());

		m_Shaders[SH_Curvature]->SetFloat("sigma", 3.3f);

		m_Shaders[SH_Movement]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders[SH_Movement]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		m_Shaders[SH_Movement]->SetMat4("prevView", m_Camera->GetPrevViewMatrix());

		m_Shaders[SH_Diffuse]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders[SH_Diffuse]->SetMat4("viewInvTrans", glm::transpose(glm::inverse(m_Camera->GetViewMatrix())));
		m_Shaders[SH_Diffuse]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		m_Shaders[SH_Diffuse]->SetVec3("lightDirection", m_LightDir);

		m_Shaders[SH_ShadingGradient]->SetFloat("sigma", 2.0f);
		
		m_ComputeShaders[SH_TransformSeeds]->SetMat4("view", m_Camera->GetViewMatrix());
		m_ComputeShaders[SH_TransformSeeds]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		glCheckError();
	}

	void Scene::DrawObject(const Shared<SceneObject>& object, EShaders shader) {
		object->SetShader(m_Shaders[shader]);
		glEnable(GL_DEPTH_TEST);
		object->Draw();
	}

	void Scene::DrawFlatColor(const Shared<SceneObject>& object, glm::vec3 color) {
		m_Shaders[SH_Flatcolor]->SetVec3("color", color);
		object->SetShader(m_Shaders[SH_Flatcolor]);
		glEnable(GL_DEPTH_TEST);
		object->Draw();
	}

	void Scene::ExtractContours(const Shared<SceneObject>& object) {
		TIME_FUNCTION(T_RenderContour);
		object->SetShader(m_Shaders[SH_ExtractContours]);
		m_Renderer->UseFrameBufferTexture(FB_Depth);
		object->ExtractContours();
	}

	void Scene::DrawContours(const Shared<SceneObject>& object, glm::vec3 color) {
		TIME_FUNCTION(T_RenderContour);
		object->SetShader(m_Shaders[SH_Contours]);
		m_Shaders[SH_Contours]->SetVec3("color", color);
		glDisable(GL_DEPTH_TEST);
		glLineWidth(2.0f);
		object->DrawContours();
	}

	void Scene::DrawSeedPoints(const Shared<SceneObject>& object, glm::vec3 color, float pointSize) {
		m_Shaders[SH_Flatcolor]->SetVec3("color", color);
		object->SetShader(m_Shaders[SH_Flatcolor]);
		glEnable(GL_DEPTH_TEST);
		glPointSize(pointSize);
		object->DrawSeedPoints();
	}

	void Scene::TransformSeedPoints(const Shared<SceneObject>& object)
	{		
		m_ComputeShaders[SH_TransformSeeds]->Use();
		m_Renderer->UseFrameBufferTexture(FB_Depth); 
		object->TransformSeedPoints(m_ComputeShaders[SH_TransformSeeds]);
	}

	void Scene::DrawScreenSeeds(glm::vec3 color, float pointSize)	{
		m_Shaders[SH_Screenpoints]->SetVec3("color", color);
		m_Shaders[SH_Screenpoints]->Use();
		glPointSize(pointSize);
		m_Hatching->DrawScreenSeeds();
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
		m_Hatching->DrawHatchingLines(m_Shaders[shader]);
	}

	void Scene::DrawHatchingCollision(glm::vec3 color, float pointSize) {
		m_Shaders[SH_Screenpoints]->SetVec3("color", color);
		m_Shaders[SH_Screenpoints]->Use();
		glPointSize(pointSize);
		m_Hatching->DrawCollisionPoints();
	}

	void Scene::DrawHatching(EShaders shader, glm::vec3 color) {
		m_Shaders[shader]->SetVec3("color", color);
		m_Shaders[shader]->Use();
		m_Renderer->UseFrameBufferTexture(EFramebuffers::FB_Diffuse);
		glDisable(GL_DEPTH_TEST);
		m_Hatching->DrawHatchingLines(m_Shaders[shader]);
	}
}