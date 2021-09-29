#pragma once

#include "core.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <glm/ext/matrix_float4x4.hpp>

#include <iostream>
#include <string>
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
		Mesh();

		void Draw();

		float GetTotalArea();

		std::vector<Face>& GetFaces();

		

	private:	
		
		void Initialize();

		std::vector<Vertex> m_Vertices;
		std::vector<HalfEdge> m_HalfEdges;
		std::vector<Face> m_Faces;

		std::vector<float> m_VertexData;
		std::vector<unsigned int> m_IndexData;

		unsigned int m_VertexArrayObject;
		unsigned int m_VertexBuffer;
		unsigned int m_ElementBuffer;		
	};

	class MeshCreator {
	public:
		static Unique<Mesh> CreateTestMesh();
		static Unique<Mesh> ImportMesh(const std::string& fileName);

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