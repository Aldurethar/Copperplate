#pragma once

#include "mesh.h"

#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <map>

namespace Copperplate {

	float testVertices[] = { 0.5f,  0.5f,  0.5f,
							 0.5f, -0.5f,  0.5f,
							-0.5f, -0.5f,  0.5f,
							-0.5f,  0.5f,  0.5f,
							 0.5f,  0.5f, -0.5f,
							 0.5f, -0.5f, -0.5f,
							-0.5f, -0.5f, -0.5f,
							-0.5f,  0.5f, -0.5f };

	unsigned int testIndices[] = {	0, 1, 3,
									1, 2, 3,
									0, 1, 4,
									1, 5, 4,
									0, 3, 7,
									0, 7, 4,
									2, 1, 6,
									1, 5, 6,
									2, 6, 3,
									6, 7, 3,
									6, 5, 7,
									5, 4, 7	};

	//MESHCREATOR IMPLEMENTATION
	Unique<Mesh> MeshCreator::CreateTestMesh() {		
		/*
		std::unique_ptr<Mesh> testMesh = std::make_unique<Mesh>(testVertices, 24, testIndices, 36);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		
		//Testing
		//model = glm::identity<glm::mat4>();

		testMesh->SetTransform(model);
		return testMesh; */
		return nullptr;
	}

	Unique<Mesh> MeshCreator::ImportMesh(const std::string& fileName) {
		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(fileName, 
			aiProcess_Triangulate			|
			aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType			|
			aiProcess_GenSmoothNormals);

		if (!scene) {
			std::cout << "Asset Import Failed:" << importer.GetErrorString() << "\n";
			return nullptr;
		}

		Unique<Mesh> mesh = CreateUnique<Mesh>();

		aiMesh* importedMesh = scene->mMeshes[0];
		std::cout << "Importing Mesh with " << importedMesh->mNumVertices << " Verts and " << importedMesh->mNumFaces << " Faces \n";

		// Construct Halfedge data structure
		// Vertices
		int numVerts = importedMesh->mNumVertices;
		mesh->m_Vertices.reserve(numVerts);
		for (int i = 0; i < numVerts; i++) {
			mesh->m_Vertices.emplace_back();
			glm::vec3 pos = glm::vec3(importedMesh->mVertices[i].x, importedMesh->mVertices[i].y, importedMesh->mVertices[i].z);
			glm::vec3 norm = glm::vec3(importedMesh->mNormals[i].x, importedMesh->mNormals[i].y, importedMesh->mNormals[i].z);
			mesh->m_Vertices[i].position = pos;
			mesh->m_Vertices[i].normal = norm;
			mesh->m_Vertices[i].index = i;
		}
		// Faces and Halfedges
		int numFaces = importedMesh->mNumFaces;
		mesh->m_HalfEdges.reserve(numFaces * 3);
		mesh->m_Faces.reserve(numFaces);

		std::map<std::pair<int, int>, HalfEdge*> edgeMap; //map ensures halfedge uniqueness for connecting twins later

		for (int i = 0; i < numFaces; i++) {
			Face* face = &mesh->m_Faces.emplace_back();

			int indices[3];
			Vertex* vertices[3];
			HalfEdge* halfEdges[3];

			// create the halfedges
			for (int j = 0; j < 3; j++) {
				indices[j] = importedMesh->mFaces[i].mIndices[j];
				vertices[j] = &mesh->m_Vertices[indices[j]];
				halfEdges[j] = &mesh->m_HalfEdges.emplace_back();
			}
			face->outer = halfEdges[0];

			// connect the halfedges
			for (int j = 0; j < 3; j++) {
				int next = (j + 1) % 3;
				halfEdges[j]->face = face;
				halfEdges[j]->origin = vertices[j];
				if (!vertices[j]->edge) vertices[j]->edge = halfEdges[j];
				halfEdges[j]->next = halfEdges[next];
				
				// connection to twin
				int in1 = indices[j];
				int in2 = indices[next];
				edgeMap[std::pair(in1, in2)] = halfEdges[j];
				if (edgeMap.find(std::pair(in2, in1)) != edgeMap.end()) {
					edgeMap[std::pair(in1, in2)]->twin = edgeMap[std::pair(in2, in1)];
					edgeMap[std::pair(in2, in1)]->twin = edgeMap[std::pair(in1, in2)];
				}
			}			
		}
		
		mesh->Initialize();

		return mesh;
	}

	//MESH IMPLEMENTATION
	Mesh::Mesh() {
		m_Vertices = std::vector<Vertex>();
		m_Faces = std::vector<Face>();
		m_HalfEdges = std::vector<HalfEdge>();

		m_VertexData = std::vector<float>();
		m_IndexData = std::vector<unsigned int>();
	}

	void Mesh::Initialize() {
		const int floatsPerVert = 6;
		const int indsPerFace = 6;
		int numVerts = m_Vertices.size();
		int numFaces = m_Faces.size();
		
		// setup raw data for vertex and index buffers
		m_VertexData.reserve(numVerts * floatsPerVert);
		for (int i = 0; i < numVerts; i++) {
			m_VertexData.push_back(m_Vertices[i].position.x);
			m_VertexData.push_back(m_Vertices[i].position.y);
			m_VertexData.push_back(m_Vertices[i].position.z);
			m_VertexData.push_back(m_Vertices[i].normal.x);
			m_VertexData.push_back(m_Vertices[i].normal.y);
			m_VertexData.push_back(m_Vertices[i].normal.z);
		}
		m_IndexData.reserve(numFaces * indsPerFace);
		for (int i = 0; i < numFaces; i++) {
			// this currently sets any missing neighbor triangles to be the backside of the current triangle
			// i think this is the most elegant solution, but might be subject to change
			HalfEdge* he1 = m_Faces[i].outer;
			HalfEdge* he2 = he1->next;
			HalfEdge* he3 = he2->next;
			m_IndexData.push_back(he1->origin->index);
			if (he1->twin) m_IndexData.push_back(he1->twin->next->next->origin->index);
			else m_IndexData.push_back(he1->next->next->origin->index);

			m_IndexData.push_back(he2->origin->index);
			if (he2->twin) m_IndexData.push_back(he2->twin->next->next->origin->index);
			else m_IndexData.push_back(he2->next->next->origin->index);

			m_IndexData.push_back(he3->origin->index);
			if (he3->twin) m_IndexData.push_back(he3->twin->next->next->origin->index);
			else m_IndexData.push_back(he3->next->next->origin->index);
		}

		// setup opengl buffers
		glGenVertexArrays(1, &m_VertexArrayObject);
		glGenBuffers(1, &m_VertexBuffer);
		glGenBuffers(1, &m_ElementBuffer);

		glBindVertexArray(m_VertexArrayObject);
		glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, numVerts * floatsPerVert * sizeof(float), m_VertexData.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ElementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, numFaces * indsPerFace * sizeof(unsigned int), m_IndexData.data(), GL_STATIC_DRAW);

		// configure vertex attributes
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, floatsPerVert * sizeof(float), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, floatsPerVert * sizeof(float), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(0);

		glCheckError();
	}
	
	void Mesh::Draw() {
		glBindVertexArray(m_VertexArrayObject);
		glDrawElements(GL_TRIANGLES_ADJACENCY, m_IndexData.size(), GL_UNSIGNED_INT, 0);
	}

	float Mesh::GetTotalArea() {
		float area = 0;
		for (Face face : m_Faces) {
			glm::vec3 a = face.outer->origin->position;
			glm::vec3 b = face.outer->next->origin->position;
			glm::vec3 c = face.outer->next->next->origin->position;

			glm::vec3 ab = b - a;
			glm::vec3 ac = c - a;
			area += glm::cross(ab, ac).length() * 0.5f;
		}
		return area;
	}

	std::vector<Face>& Mesh::GetFaces(){
		return m_Faces;
	}
}