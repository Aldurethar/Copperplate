#pragma once

#include <glad/glad.h>
#include <glm/ext/matrix_transform.hpp>
#include "mesh.h"
#include "application.h"
#include <glm\gtc\type_ptr.hpp>

namespace Copperplate {

	float testVertices[] = { 0.5f,  0.5f, 0.0f,
							 0.5f, -0.5f, 0.0f,
							-0.5f, -0.5f, 0.0f,
							-0.5f,  0.5f, 0.0f };

	unsigned int testIndices[] = { 0, 1, 3,
								1, 2, 3 };

	std::unique_ptr<Mesh> createTestMesh() {
		std::unique_ptr<Mesh> testMesh = std::make_unique<Mesh>(testVertices, 12, testIndices, 6);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		
		//Testing
		//model = glm::identity<glm::mat4>();

		testMesh->SetTransform(model);
		return testMesh;
	}

	Mesh::Mesh(float* verts, int numVerts, unsigned int* inds, int numInds) {
		m_Vertices = verts;
		m_Indices = inds;

		m_ModelMatrix = glm::mat4(1.0f);

		glGenVertexArrays(1, &m_VertexArrayObject);
		glGenBuffers(1, &m_VertexBuffer);
		glGenBuffers(1, &m_ElementBuffer);

		glBindVertexArray(m_VertexArrayObject);
		glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(float), verts, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ElementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, numInds * sizeof(float), inds, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glCheckError();
	}

	void Mesh::SetTransform(glm::mat4 ModelMatrix) {
		m_ModelMatrix = ModelMatrix;
	}

	void Mesh::Draw(unsigned int shaderId) {
		int modelLoc = glGetUniformLocation(shaderId, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m_ModelMatrix));
		glCheckError();

		glBindVertexArray(m_VertexArrayObject);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	
}