#pragma once

#include <string>
#include <iostream>
#include <glm/ext/matrix_float4x4.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <vector>

namespace Copperplate {
	class Vertex;
	class Face;
	class HalfEdge;

	class MeshCreator;

	class Mesh {
		friend class MeshCreator;
	public:	
		//Mesh(float* verts, int numVerts, unsigned int* inds, int numInds);

		void Draw(unsigned int shaderId);

		void SetTransform(glm::mat4 ModelMatrix);

	private:	
		Mesh();
		void Initialize();

		std::vector<Vertex> m_Vertices;
		std::vector<HalfEdge> m_HalfEdges;
		std::vector<Face> m_Faces;

		std::vector<float> m_VertexData;
		std::vector<unsigned int> m_IndexData;

		glm::mat4 m_ModelMatrix;

		unsigned int m_VertexArrayObject;
		unsigned int m_VertexBuffer;
		unsigned int m_ElementBuffer;		
	};

	class MeshCreator {
	public:
		static std::unique_ptr<Mesh> CreateTestMesh();
		static std::unique_ptr<Mesh> ImportMesh(const std::string& fileName);

	private:

	};

	// Half Edge data structure
	class HalfEdge {
	public:
		Vertex* origin;
		HalfEdge* twin;
		HalfEdge* next;
		Face* face;
	};

	class Vertex {
	public:
		glm::vec3 position;
		glm::vec3 normal;
		unsigned int index;
		HalfEdge* edge;
	};

	class Face {
	public:
		HalfEdge* outer;
	};
	 
}