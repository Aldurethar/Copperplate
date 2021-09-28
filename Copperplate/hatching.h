#pragma once

#include "mesh.h"

namespace Copperplate {

	struct SeedPoint {
		glm::vec3 m_Pos;
		Face& m_Face;
	};
	
	void CreateSeedPoints(std::vector<SeedPoint>& outSeedPoints, Mesh& mesh, int totalPoints, int maxPointsPerFace);

}