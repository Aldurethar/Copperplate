#pragma once

#include "scene.h"
#include "core.h"

namespace Copperplate {

	//SCENEOBJECT IMPLEMENTATION
	SceneObject::SceneObject(std::string meshFile, std::shared_ptr<Shader> shader) {
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
		glCheckError();
		m_Shader->Use();
		m_Mesh->Draw();
	}

	void SceneObject::SetShader(std::shared_ptr<Shader> shader)	{
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
	Scene::Scene(int viewportWidth, int viewportHeight) {
		m_Camera = std::make_unique<Camera>(viewportWidth, viewportHeight);

		//Setup the shaders
		std::shared_ptr<Shader> flatColor = std::make_shared<Shader>("shaders/flatcolor.vert", "shaders/flatcolor.frag");
		m_Shaders["FlatColor"] = flatColor;

		std::shared_ptr<Shader> contours = std::make_shared<Shader>("shaders/contours.vert", "shaders/contours.geom", "shaders/contours.frag");
		m_Shaders["Contours"] = contours;

		//Create the Scene Objects
		m_SceneObjects = std::vector<std::unique_ptr<SceneObject>>();
		m_SceneObjects.push_back(std::make_unique<SceneObject>("SuzanneSmooth.obj", m_Shaders["Contours"]));

	}

	void Scene::Draw() {
		//Update Shader Uniforms
		m_Shaders["FlatColor"]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders["FlatColor"]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		m_Shaders["FlatColor"]->SetVec3("color", glm::vec3(1.0f, 1.0f, 1.0f));
		m_Shaders["Contours"]->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shaders["Contours"]->SetMat4("projection", m_Camera->GetProjectionMatrix());
		m_Shaders["Contours"]->SetVec3("viewDirection", m_Camera->GetForwardVector());
		glCheckError();

		//Draw the Objects
		for (auto& object : m_SceneObjects) {
			object->SetShader(m_Shaders["FlatColor"]);
			object->Draw();
			object->SetShader(m_Shaders["Contours"]);
			glDepthFunc(GL_LEQUAL);
			glLineWidth(2.0f);
			object->Draw();
			object->SetShader(m_Shaders["FlatColor"]);
			m_Shaders["FlatColor"]->SetVec3("color", glm::vec3(0.89f, 0.37f, 0.27f));
			glPointSize(4.0f);
			object->DrawSeedPoints();
		}
	}

	void Scene::MoveCamera(float x, float y, float z)
	{
		m_Camera->Move(x, y, z);
	}

	void Scene::ViewportSizeChanged(int newWidth, int newHeight)
	{
		m_Camera->SetViewportSize(newWidth, newHeight);
	}

}