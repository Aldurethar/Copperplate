#pragma once

#include "hatching.h"

#include <random>

namespace Copperplate {

	float getFaceArea(Face& face) {
		glm::vec3 a = face.outer->origin->position;
		glm::vec3 b = face.outer->next->origin->position;
		glm::vec3 c = face.outer->next->next->origin->position;

		glm::vec3 ab = b - a;
		glm::vec3 ac = c - a;
		return glm::cross(ab, ac).length() * 0.5f;
	}

	void CreateSeedPoints(std::vector<SeedPoint>& outSeedPoints, Mesh& mesh, int totalPoints, int maxPointsPerFace) {
		//Setup Random
		std::random_device rd;
		std::mt19937 engine(rd());
		std::uniform_real_distribution<> dist(0.0, 1.0);

		float totalArea = mesh.GetTotalArea();
		float pointsFrac = (float)totalPoints / (float)maxPointsPerFace;
		std::vector<Face>& faces = mesh.GetFaces();

		for (Face& face : faces) {
			float faceArea = getFaceArea(face);
			float pointChance = (faceArea / totalArea) * pointsFrac;
			for (int i = 0; i < maxPointsPerFace; i++) {
				float r = dist(engine);
				if (r < pointChance) {	
					glm::vec3 v1 = face.outer->origin->position;
					glm::vec3 v2 = face.outer->next->origin->position;
					glm::vec3 v3 = face.outer->next->next->origin->position;
					float a = dist(engine);
					float b = dist(engine);
					float c = dist(engine);
					float sum = a + b + c;
					a /= sum;
					b /= sum;
					c /= sum;
					glm::vec3 pos = (a * v1) + (b * v2) + (c * v3);
					SeedPoint sp = { pos, face };
					outSeedPoints.push_back(sp);
				}
			}
		}
	}

	
}