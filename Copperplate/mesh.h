#pragma once

#include <string>
#include <iostream>


class Mesh {
public:
	Mesh(float* verts, int numVerts, unsigned int* inds, int numInds);

	void Draw();

private:
	float* m_Vertices;
	unsigned int* m_Indices;

	unsigned int m_VertexArrayObject;
	unsigned int m_VertexBuffer;
	unsigned int m_ElementBuffer;
};

Mesh* createTestMesh();

GLenum glCheckError_(const char* file, int line);
#define glCheckError() glCheckError_(__FILE__, __LINE__) 