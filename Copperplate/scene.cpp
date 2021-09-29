#pragma once

#include "scene.h"
#include "core.h"

namespace Copperplate {

	//SCENEOBJECT IMPLEMENTATION
	SceneObject::SceneObject(std::string meshFile, Shared<Shader> shader) {
		m_Mesh = MeshCreator::ImportMesh(meshFile);
		m_Shader = shader;
		m_Transform = glm::mat4(1.0f);

		//Create Seed Points for Hatching Strokes
		m_SeedPoints = std::vector<SeedPoint>();
		m_SeedPoints.reserve(SEEDS_PER_OBJECT);
		CreateSeedPoints(m_SeedPoints, *m_Mesh, SEEDS_PER_OBJECT, MAX_SEEDS_PER_FACE);

		std::vector<float> seedsData;
		seedsData.reserve(3 * m_SeedPoints.size());
		for (SeedPoint sp : m_SeedPoints) {
			seedsData.push_back(sp.m_Pos.x);
			seedsData.push_back(sp.m_Pos.y);
			seedsData.push_back(sp.m_Pos.z);
		}

		//Setup OpenGL Buffers
		glGenVertexArrays(1, &m_SeedsVAO);
		glGenBuffers(1, &m_SeedsVertexBuffer);

		glBindVertexArray(m_SeedsVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_SeedsVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, seedsData.size() * sizeof(float), seedsData.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);
		glEnableVertexAttribArray(0);

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
	
	//SCENE IMPLEMENTATION
	Scene::Scene(Shared<Window> window) {
		m_Camera = CreateUnique<Camera>(window->GetWidth(), window->GetHeight());
		m_Renderer = CreateUnique<Renderer>(window);

		//Setup the shaders
		Shared<Shader> flatColor = CreateShared<Shader>("shaders/flatcolor.vert", "shaders/flatcolor.frag");
		m_Shaders["FlatColor"] = flatColor;

		Shared<Shader> contours = CreateShared<Shader>("shaders/contours.vert", "shaders/contours.geom", "shaders/contours.frag");
		m_Shaders["Contours"] = contours;

		Shared<Shader> displayTex = CreateShared<Shader>("shaders/displaytex.vert", "shaders/displaytex.frag");
		m_Shaders["DisplayTex"] = displayTex;

		Shared<Shader> normals = CreateShared<Shader>("shaders/normals.vert", "shaders/normals.frag");
		m_Shaders["Normals"] = normals;

		//Create the Scene Objects
		m_SceneObjects = std::vector<Unique<SceneObject>>();
		m_SceneObjects.push_back(CreateUnique<SceneObject>("SuzanneSmooth.obj", m_Shaders["Contours"]));

	}

	void Scene::Draw() {
		//Update Shader Uniforms
		m_Shaders["FlatColor"]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders["FlatColor"]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		m_Shaders["Contours"]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders["Contours"]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		m_Shaders["Contours"]->SetVec3("viewDirection", m_Camera->GetForwardVector());
		m_Shaders["Normals"]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders["Normals"]->SetMat4("viewInvTrans", glm::transpose(glm::inverse(m_Camera->GetViewMatrix())));
		m_Shaders["Normals"]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		glCheckError();

		//Draw the Objects		
		m_Renderer->SwitchFrameBuffer("Default");
		for (auto& object : m_SceneObjects) {
			m_Renderer->SwitchFrameBuffer("Normal");
			DrawObject(object, "Normals");

			m_Renderer->SwitchFrameBuffer("Default");
			DrawFlatColor(object, glm::vec3(1.0f));
			DrawContours(object);
			DrawSeedPoints(object, glm::vec3(0.89f, 0.37f, 0.27f));
			
		}
		DrawFramebufferContent("Normal"); 
	}

	void Scene::MoveCamera(float x, float y, float z)
	{
		m_Camera->Move(x, y, z);
	}

	void Scene::ViewportSizeChanged(int newWidth, int newHeight)
	{
		m_Camera->SetViewportSize(newWidth, newHeight);
	}

	void Scene::DrawObject(const Unique<SceneObject>& object, const std::string& shader) {
		object->SetShader(m_Shaders[shader]);
		glEnable(GL_DEPTH_TEST);
		object->Draw();
	}

	void Scene::DrawFlatColor(const Unique<SceneObject>& object, glm::vec3 color) {
		object->SetShader(m_Shaders["FlatColor"]);
		m_Shaders["FlatColor"]->SetVec3("color", color);
		glEnable(GL_DEPTH_TEST);
		object->Draw();
	}

	void Scene::DrawContours(const Unique<SceneObject>& object) {
		object->SetShader(m_Shaders["Contours"]);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glLineWidth(2.0f);
		object->Draw();
	}

	void Scene::DrawSeedPoints(const Unique<SceneObject>& object, glm::vec3 color) {
		object->SetShader(m_Shaders["FlatColor"]);
		m_Shaders["FlatColor"]->SetVec3("color", color);
		glEnable(GL_DEPTH_TEST);
		glPointSize(4.0f);
		object->DrawSeedPoints();
	}

	void Scene::DrawFramebufferContent(const std::string& framebuffer)
	{
		m_Shaders["DisplayTex"]->Use();
		glDisable(GL_DEPTH_TEST);
		m_Renderer->DrawFramebufferContent(framebuffer);
	}

}