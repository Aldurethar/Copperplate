#pragma once

#include <glad/glad.h>
#include "mesh.h"

float testVertices[] = { 0.5f,  0.5f, 0.0f,
						 0.5f, -0.5f, 0.0f,
						-0.5f, -0.5f, 0.0f,
						-0.5f,  0.5f, 0.0f };

unsigned int testIndices[] = { 0, 1, 3,
							1, 2, 3 };

Mesh* createTestMesh() {
	Mesh* testMesh = new Mesh(testVertices, 12, testIndices, 6);

	return testMesh;
}

Mesh::Mesh(float* verts, int numVerts, unsigned int* inds, int numInds) {
	m_Vertices = verts;
	m_Indices = inds;	

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

void Mesh::Draw() {
	glBindVertexArray(m_VertexArrayObject);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

GLenum glCheckError_(const char* file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}