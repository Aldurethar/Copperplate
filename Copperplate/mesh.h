#pragma once

#include <string>
#include <iostream>
#include <glm/ext/matrix_float4x4.hpp>

namespace Copperplate {
	class Mesh {
	public:
		Mesh(float* verts, int numVerts, unsigned int* inds, int numInds);

		void Draw(unsigned int shaderId);

		void SetTransform(glm::mat4 ModelMatrix);

	private:
		float* m_Vertices;
		unsigned int* m_Indices;

		glm::mat4 m_ModelMatrix;

		unsigned int m_VertexArrayObject;
		unsigned int m_VertexBuffer;
		unsigned int m_ElementBuffer;
	};

	std::unique_ptr<Mesh> createTestMesh();

	 
}